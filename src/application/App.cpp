// App.cpp
// Appレイヤの実装

#include "App.h"
#include "AppConfig.h"
#include "../rte/Rte.h"
#include <Arduino.h>

namespace {
    
    int8_t s_currentMotorOutput = 0;
    
    // App内部で計算した左右モータ出力
    struct DriveOutput {
        int8_t motorRightOutput;
        int8_t motorLeftOutput;
    };

    DriveOutput s_driveOutput = {};
}

namespace App {

    //プロトタイプ宣言
    int8_t calculateMotorOutput(const DriveInput& driveInput);
    uint16_t calculateTurningRadius(const DriveInput& driveInput, int8_t motorOutput);
    DriveOutput calculateDriveOutput( const DriveInput& driveInput, int8_t baseMotorOutput, uint16_t turningRadius);
    template <size_t N> int8_t interpolateMap( const AppConfig::MapPoint_int8 (&map)[N], int8_t x);
    template <size_t N> uint16_t interpolateMap(const AppConfig::MapPoint_int16 (&map)[N], int16_t x);

    // ---------------------------------------------------------
    // App初期化処理
    // ---------------------------------------------------------
    void init() {
        //アプリ初期化
    }

    void run() {
        // RTEから走行入力を取得
        DriveInput driveInput = Rte::getDriveInput();

        // ベースモータ出力を計算
        int8_t baseMotorOutput = calculateMotorOutput(driveInput);

        // 旋回半径を計算
        uint16_t turningRadius = calculateTurningRadius(driveInput, baseMotorOutput);

        // 左右モータ出力を計算
        s_driveOutput = calculateDriveOutput(driveInput, baseMotorOutput, turningRadius);

        // 計算した左右モータ出力をRTEへ渡す
        Rte::setDriveOutput(s_driveOutput.motorRightOutput, s_driveOutput.motorLeftOutput);
    }

    bool shutdown() {

        // App層の停止完了を返却
        return true;
    }

	// ---------------------------------------------------------
	// モータ出力を目標値へ徐々に追従させる
	// ---------------------------------------------------------
	int8_t calculateMotorOutput(const DriveInput& driveInput)
    {
        // 目標モータ出力
        int8_t targetOutput = driveInput.output;

        // ---------------------------------------------------------
        // 目標出力の大きさを取得
        // ---------------------------------------------------------
        int8_t targetMagnitude = static_cast<int8_t>(abs(targetOutput));

        // ---------------------------------------------------------
        // 現在のモータ出力を取得
        // ---------------------------------------------------------
        int8_t currentOutput = s_currentMotorOutput;


        // ---------------------------------------------------------
        // ブレーキ処理
        // ---------------------------------------------------------
        if (driveInput.brake) {

            // ブレーキ特性MAPから今回の減速度を取得
            int8_t brakeStep = interpolateMap(AppConfig::g_brakeMap, static_cast<int8_t>(abs(currentOutput)));

            // 正方向から減速
            if (currentOutput > 0) {

                currentOutput -= brakeStep;

                // 0を下回らないようにする
                currentOutput = max(currentOutput, static_cast<int8_t>(0));
            }

            // 負方向から減速
            else if (currentOutput < 0) {

                currentOutput += brakeStep;

                // 0を上回らないようにする
                currentOutput = min(currentOutput, static_cast<int8_t>(0));
            }
        }
        // ---------------------------------------------------------
        // モータ出力を入力に合わせて加減速する処理
        // ---------------------------------------------------------
        else if (abs(currentOutput) < abs(targetOutput)) {

            // 加速特性MAPから今回の加速度を取得
            int8_t accelerationStep = interpolateMap(AppConfig::g_accelerationMap, targetMagnitude);
            
            // 正方向へ出力を増加
            if (targetOutput > 0) {

                currentOutput += accelerationStep;

                // 目標値を上回らないようにする
                currentOutput = min(currentOutput, targetOutput);
            }
            // 負方向へ出力を増加
            else if (targetOutput < 0) {

                currentOutput -= accelerationStep;

                // 目標値を下回らないようにする
                currentOutput = max(currentOutput, targetOutput);
            }
        }

        // ---------------------------------------------------------
        // 減速処理
        // ---------------------------------------------------------
        else if (abs(currentOutput) > abs(targetOutput)) {

            // 減速特性MAPから今回の減速度を取得
            int8_t decelerationStep = interpolateMap(AppConfig::g_decelerationMap, targetMagnitude);

            // 正方向から減速
            if (currentOutput > 0) {

                currentOutput -= decelerationStep;

                // 目標値を下回らないようにする
                currentOutput = max(currentOutput, targetOutput);
            }

            // 負方向から減速
            else if (currentOutput < 0) {

                currentOutput += decelerationStep;

                // 目標値を上回らないようにする
                currentOutput = min(currentOutput, targetOutput);
            }
        }

	    // ---------------------------------------------------------
	    // 現在のモータ出力を保存
	    // ---------------------------------------------------------
        s_currentMotorOutput = currentOutput;

	    // ---------------------------------------------------------
	    // 車体基準のモータ出力を返却
	    // ---------------------------------------------------------
        return currentOutput;
    }
    

    // ---------------------------------------------------------
    // 旋回半径を計算
    // ---------------------------------------------------------
    uint16_t calculateTurningRadius(const DriveInput& driveInput, int8_t motorOutput)
    {
        // ---------------------------------------------------------
        // 操舵量の大きさを取得
        // steeringRatio : -100～+100
        // ---------------------------------------------------------
        int8_t steeringMagnitude = static_cast<int8_t>(abs(driveInput.steeringRatio));

        // ---------------------------------------------------------
        // 操舵量が0の場合は直進
        // ---------------------------------------------------------
        if (steeringMagnitude == 0) { return 0; }

        // ---------------------------------------------------------
        // 旋回半径MAPから基本旋回半径を取得
        // ---------------------------------------------------------
        uint16_t baseTurningRadius = interpolateMap(AppConfig::g_turningRadiusMap, steeringMagnitude);

        // ---------------------------------------------------------
        // モータ出力の大きさから旋回半径補正率を取得
        // ---------------------------------------------------------
        int8_t motorMagnitude = static_cast<int8_t>(abs(motorOutput));
        int8_t correctionRatio = interpolateMap(AppConfig::g_radiusCorrectionMap, motorMagnitude);

        // ---------------------------------------------------------
        // 速度に応じて旋回半径を補正
        // ---------------------------------------------------------
        uint16_t turningRadius = static_cast<uint16_t>((static_cast<uint32_t>(baseTurningRadius) * (100 + correctionRatio)) / 100);
        
        return turningRadius;
    }

    // ---------------------------------------------------------
    // 左右モータ出力を計算
    // ---------------------------------------------------------
    DriveOutput calculateDriveOutput(const DriveInput& driveInput, int8_t baseMotorOutput, uint16_t turningRadius)
    {
        DriveOutput driveOutput = {};

        // ---------------------------------------------------------
        // 直進の場合は左右にベース出力を設定
        // ---------------------------------------------------------
        if (turningRadius == 0) {
            driveOutput.motorRightOutput = baseMotorOutput;
            driveOutput.motorLeftOutput  = baseMotorOutput;

            return driveOutput;
        }

        // ---------------------------------------------------------
        // 左右輪の旋回半径を計算
        // ---------------------------------------------------------
        uint32_t halfTrack = AppConfig::g_track / 2;
        uint32_t innerRadius;
        uint32_t outerRadius;

        if (turningRadius <= halfTrack) {
            innerRadius = 0;
            outerRadius = AppConfig::g_track;
        }
        else {
            innerRadius = turningRadius - halfTrack;
            outerRadius = turningRadius + halfTrack;
        }
        // ---------------------------------------------------------
        // 内側モータの出力比を計算
        // ---------------------------------------------------------
        int8_t motorRatio = static_cast<int8_t>((innerRadius * 100) / outerRadius);

        // ---------------------------------------------------------
        // 内側モータの出力を計算
        // ---------------------------------------------------------
        int8_t innerMotorOutput = static_cast<int8_t>((static_cast<int16_t>(baseMotorOutput) * motorRatio) / 100);

        // ---------------------------------------------------------
        // 操舵方向に応じて左右モータへ出力
        // steeringRatio > 0 : 左旋回
        // steeringRatio < 0 : 右旋回
        // ---------------------------------------------------------
        if (driveInput.steeringRatio > 0) {
            driveOutput.motorRightOutput = baseMotorOutput;
            driveOutput.motorLeftOutput  = innerMotorOutput;
        }
        else {
            driveOutput.motorRightOutput = innerMotorOutput;
            driveOutput.motorLeftOutput  = baseMotorOutput;
        }

        return driveOutput;
    }

    // ---------------------------------------------------------
    // 2次元MAP線形補間（int8）
    // ---------------------------------------------------------
    template <size_t N> int8_t interpolateMap( const AppConfig::MapPoint_int8 (&map)[N], int8_t x)
    {
        // MAPが空の場合
        if (N == 0) { return 0; }

        // XがMAPの先頭より小さい場合
        if (x <= map[0].x) { return map[0].y; }

        // MAP内の補間区間を検索
        for (size_t i = 0; i < N - 1; ++i) {

            const AppConfig::MapPoint_int8& lower = map[i];
            const AppConfig::MapPoint_int8& upper = map[i + 1];

            if (x <= upper.x) {

                // X方向の位置
                int16_t dx = upper.x - lower.x;
                int16_t position = x - lower.x;

                // Yを線形補間
                int16_t y = lower.y + ((upper.y - lower.y) * position) / dx;

                return static_cast<int8_t>(y);
            }
        }

        // XがMAPの最後より大きい場合
        return map[N - 1].y;
    }

    // ---------------------------------------------------------
    // 2次元MAP線形補間（int16）
    // ---------------------------------------------------------
    template <size_t N> uint16_t interpolateMap(const AppConfig::MapPoint_int16 (&map)[N], int16_t x)
    {
        // MAPが空の場合
        if (N == 0) { return 0; }

        // XがMAPの先頭より小さい場合
        if (x <= map[0].x) { return static_cast<uint16_t>(map[0].y); }

        // MAP内の補間区間を検索
        for (size_t i = 0; i < N - 1; ++i) {

            const AppConfig::MapPoint_int16& lower = map[i];
            const AppConfig::MapPoint_int16& upper = map[i + 1];

            if (x <= upper.x) {

                // X方向の位置
                int32_t dx = upper.x - lower.x;
                int32_t position = x - lower.x;

                // Yを線形補間
                int32_t y =
                    lower.y +
                    ((upper.y - lower.y) * position) / dx;

                return static_cast<uint16_t>(y);
            }
        }

        // XがMAPの最後より大きい場合
        return static_cast<uint16_t>(map[N - 1].y);
    }
} // namespace App