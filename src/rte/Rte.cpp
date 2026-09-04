// Rte.cpp
// RTEレイヤの実装

#include "Rte.h"
#include "../platform/Platform.h"
#include "../platform/PlatformConfig.h"
#include "../platform/PlatformBluetooth.h"
#include "../platform/PlatformController.h"
#include "../platform/PlatformHttp.h"
#include <Arduino.h>

namespace {

    // ■システム用
    // シャットダウン要求フラグ
    bool s_shutdownRequested = false;

    DriveInput s_driveInput = {};

    // ■OUTPUT
    struct DriveCommand {
        int8_t motorRightOutput;
        int8_t motorLeftOutput;
    };

    DriveCommand s_driveCommand = {};

    // ---------------------------------------------------------
    // 走行指令をモーター出力へ変換
    // ---------------------------------------------------------
    void updateMotorOutput()
    {
        int8_t motorRightOutput = s_driveCommand.motorRightOutput;
        int8_t motorLeftOutput  = s_driveCommand.motorLeftOutput;

        // モーター1（右）
        if (motorRightOutput >= 0) {
            Platform::setPwmDuty(PwmOutput::MOTOR1_IN1, static_cast<uint8_t>(motorRightOutput));
            Platform::setPwmDuty(PwmOutput::MOTOR1_IN2, 0);
        }
        else {
            Platform::setPwmDuty(PwmOutput::MOTOR1_IN1, 0);
            Platform::setPwmDuty(PwmOutput::MOTOR1_IN2, static_cast<uint8_t>(-motorRightOutput));
        }

        // モーター2（左）
        if (motorLeftOutput >= 0) {
            Platform::setPwmDuty(PwmOutput::MOTOR2_IN1, static_cast<uint8_t>(motorLeftOutput));
            Platform::setPwmDuty(PwmOutput::MOTOR2_IN2, 0);
        }
        else {
            Platform::setPwmDuty(PwmOutput::MOTOR2_IN1, 0);
            Platform::setPwmDuty(PwmOutput::MOTOR2_IN2, static_cast<uint8_t>(-motorLeftOutput));
        }
    }
}

namespace Rte {

    //■プロトタイプ宣言
    //デバッグ用
    void debugButtonState();

    // 走行入力更新
    void updateDriveInput();


    //■システム用
    // ---------------------------------------------------------
    // RTE初期化処理
    // ---------------------------------------------------------
    void init()
    {
        s_shutdownRequested = false;

        s_driveCommand.motorRightOutput = 0;
        s_driveCommand.motorLeftOutput = 0;
    }

    // ---------------------------------------------------------
    // Controller入力更新処理
    // ---------------------------------------------------------
    void updateInput()
    {
        if (PlatformController::isControllerConnected()) {
            updateDriveInput();
        }
        else {
            s_driveInput = {};
        }

        debugButtonState();
    }

    // ---------------------------------------------------------
    // モーター出力更新処理
    // ---------------------------------------------------------
    void updateOutput()
    {
        // RTEでの出力処理をここに追加
        updateMotorOutput();
    }

    // ---------------------------------------------------------
    // シャットダウン要求状態を取得
    // ---------------------------------------------------------
    bool isShutdownRequested()
    {
        return s_shutdownRequested;
    }

    //■INPUT用
    // ---------------------------------------------------------
    // コントローラ接続状態を取得
    // ---------------------------------------------------------
    bool isControllerConnected()
    {
        
        return PlatformController::isControllerConnected() || PlatformHttp::isControllerConnected();
    }

    // ---------------------------------------------------------
    // ボタン状態を取得
    // ---------------------------------------------------------
    ButtonState getButtonState(uint32_t buttonMask)
    {
        PlatformControllerData input = PlatformController::getControllerInput();

        if (input.buttonsPressed & buttonMask) {
            return ButtonState::PRESSED;
        }

        if (input.buttonsReleased & buttonMask) {
            return ButtonState::RELEASED;
        }

        if (input.buttons & buttonMask) {
            return ButtonState::HOLD;
        }

        return ButtonState::NONE;
    }

    // ---------------------------------------------------------
    // Miscボタン状態を取得
    // ---------------------------------------------------------
    ButtonState getMiscButtonState(uint8_t buttonMask)
    {
        PlatformControllerData input = PlatformController::getControllerInput();

        if (input.miscButtonsPressed & buttonMask) {
            return ButtonState::PRESSED;
        }

        if (input.miscButtonsReleased & buttonMask) {
            return ButtonState::RELEASED;
        }

        if (input.miscButtons & buttonMask) {
            return ButtonState::HOLD;
        }

        return ButtonState::NONE;
    }

    // ---------------------------------------------------------
    // D-Pad状態を取得
    // ---------------------------------------------------------
    ButtonState getDpadState(uint8_t dpadMask)
    {
        PlatformControllerData input = PlatformController::getControllerInput();

        if (input.dpadPressed & dpadMask) {
            return ButtonState::PRESSED;
        }

        if (input.dpadReleased & dpadMask) {
            return ButtonState::RELEASED;
        }

        if (input.dpad & dpadMask) {
            return ButtonState::HOLD;
        }

        return ButtonState::NONE;
    }

    // ---------------------------------------------------------
    // スティック状態を取得
    // ---------------------------------------------------------
    StickValue getStickValue(AppStick stick)
    {
        PlatformControllerData input = PlatformController::getControllerInput();

        if (stick == AppStick::LEFT) {
            return {
                input.leftStickX,
                input.leftStickY
            };
        }

        if (stick == AppStick::RIGHT) {
            return {
                input.rightStickX,
                input.rightStickY
            };
        }

        return { 0, 0 };
    }

    //OUTPUT用
    // ---------------------------------------------------------
    // 走行指令を設定
    // ---------------------------------------------------------
    void setDriveOutput(int8_t motorRightOutput, int8_t motorLeftOutput)
    {
        motorRightOutput = constrain(motorRightOutput, -100, 100);
        motorLeftOutput = constrain(motorLeftOutput, -100, 100);

        s_driveCommand.motorRightOutput = motorRightOutput;
        s_driveCommand.motorLeftOutput = motorLeftOutput;
    }

    // ---------------------------------------------------------
    // ステータスLEDを緑に設定
    // ---------------------------------------------------------
    void setLedGreen()
    {
        Platform::setStatusLedColor({0, 255, 0});
    }

    // ---------------------------------------------------------
    // ステータスLEDを赤に設定
    // ---------------------------------------------------------
    void setLedRed()
    {
        Platform::setStatusLedColor({255, 0, 0});
    }

    // ---------------------------------------------------------
    // ステータスLEDを青に設定
    // ---------------------------------------------------------
    void setLedBlue()
    {
        Platform::setStatusLedColor({0, 0, 255});
    }

    // ---------------------------------------------------------
    // ステータスLEDを消灯
    // ---------------------------------------------------------
    void setLedOff()
    {
        Platform::setStatusLedColor({0, 0, 0});
    }

    // ---------------------------------------------------------
    // RTE停止処理
    // ---------------------------------------------------------
    bool shutdown()
    {
        return true;
    }

    // ---------------------------------------------------------
    // Controller入力から走行入力を更新
    // ---------------------------------------------------------
    void updateDriveInput()
    {
        s_driveInput.brake = false;
        
        // スティック入力を取得
        StickValue stickL = getStickValue(AppStick::LEFT);
        StickValue stickR = getStickValue(AppStick::RIGHT);

        // 出力：右スティックY
        s_driveInput.output = static_cast<int8_t>(-stickR.y);

        // 操舵量：左スティックX
        s_driveInput.steeringRatio = static_cast<int16_t>(stickL.x);

        // 出力：Aボタン押下中は出力100
        if (getButtonState(g_switchButtonMap.A) == ButtonState::HOLD ||
            getButtonState(g_switchButtonMap.A) == ButtonState::PRESSED) {
            s_driveInput.output = 100;
        }

        // 出力：Bボタン押下中はブレーキ
        if (getButtonState(g_switchButtonMap.B) == ButtonState::HOLD ||
            getButtonState(g_switchButtonMap.B) == ButtonState::PRESSED) {
            s_driveInput.brake = true;
        }
        // デバッグ出力
        /*
        Serial.printf(
            "[DRIVE INPUT] Output:%4d steeringRatio:%4d\n",
            s_driveInput.output,
            s_driveInput.steeringRatio
        );
        */
    }

    // ---------------------------------------------------------
    // 走行入力を取得
    // ---------------------------------------------------------
    DriveInput getDriveInput()
    {
        return s_driveInput;
    }

    // ---------------------------------------------------------
    // Controller入力状態をシリアル出力
    // ---------------------------------------------------------
    void debugButtonState()
    {
        // 左スティック
        {
            StickValue stick = getStickValue(AppStick::LEFT);
            if (stick.x != 0 || stick.y != 0) {
                Serial.printf("[STICK] LEFT  X:%d Y:%d\n", stick.x, stick.y);
            }
        }

        // 右スティック
        {
            StickValue stick = getStickValue(AppStick::RIGHT);
            if (stick.x != 0 || stick.y != 0) {
                Serial.printf("[STICK] RIGHT X:%d Y:%d\n", stick.x, stick.y );
            }
        }

        // 通常ボタン
    if (getButtonState(g_switchButtonMap.A) != ButtonState::NONE) {
        Serial.printf("[BUTTON] A: 0x%02X\n", static_cast<uint8_t>(getButtonState(g_switchButtonMap.A)));
    }

    if (getButtonState(g_switchButtonMap.B) != ButtonState::NONE) {
        Serial.printf("[BUTTON] B: 0x%02X\n", static_cast<uint8_t>(getButtonState(g_switchButtonMap.B)));
    }

    if (getButtonState(g_switchButtonMap.X) != ButtonState::NONE) {
        Serial.printf("[BUTTON] X: 0x%02X\n", static_cast<uint8_t>(getButtonState(g_switchButtonMap.X)));
    }

    if (getButtonState(g_switchButtonMap.Y) != ButtonState::NONE) {
        Serial.printf("[BUTTON] Y: 0x%02X\n", static_cast<uint8_t>(getButtonState(g_switchButtonMap.Y)));
    }

    if (getButtonState(g_switchButtonMap.R1) != ButtonState::NONE) {
        Serial.printf("[BUTTON] R1: 0x%02X\n", static_cast<uint8_t>(getButtonState(g_switchButtonMap.R1)));
    }

    if (getButtonState(g_switchButtonMap.L1) != ButtonState::NONE) {
        Serial.printf("[BUTTON] L1: 0x%02X\n", static_cast<uint8_t>(getButtonState(g_switchButtonMap.L1)));
    }

    if (getButtonState(g_switchButtonMap.R2) != ButtonState::NONE) {
        Serial.printf("[BUTTON] R2: 0x%02X\n", static_cast<uint8_t>(getButtonState(g_switchButtonMap.R2)));
    }

    if (getButtonState(g_switchButtonMap.L2) != ButtonState::NONE) {
        Serial.printf("[BUTTON] L2: 0x%02X\n", static_cast<uint8_t>(getButtonState(g_switchButtonMap.L2)));
    }

    if (getButtonState(g_switchButtonMap.R3) != ButtonState::NONE) {
        Serial.printf("[BUTTON] R3: 0x%02X\n", static_cast<uint8_t>(getButtonState(g_switchButtonMap.R3)));
    }

    if (getButtonState(g_switchButtonMap.L3) != ButtonState::NONE) {
        Serial.printf("[BUTTON] L3: 0x%02X\n", static_cast<uint8_t>(getButtonState(g_switchButtonMap.L3)));
    }
        // MISCボタン
        if (getMiscButtonState(g_switchMiscButtonMap.HOME) != ButtonState::NONE) {
            Serial.printf("[MISC] HOME: 0x%02X\n",
                        static_cast<uint8_t>(getMiscButtonState(g_switchMiscButtonMap.HOME)));
        }

        if (getMiscButtonState(g_switchMiscButtonMap.MINUS) != ButtonState::NONE) {
            Serial.printf("[MISC] MINUS: 0x%02X\n",
                        static_cast<uint8_t>(getMiscButtonState(g_switchMiscButtonMap.MINUS)));
        }

        if (getMiscButtonState(g_switchMiscButtonMap.PLUS) != ButtonState::NONE) {
            Serial.printf("[MISC] PLUS: 0x%02X\n",
                        static_cast<uint8_t>(getMiscButtonState(g_switchMiscButtonMap.PLUS)));
        }

        if (getMiscButtonState(g_switchMiscButtonMap.PICT) != ButtonState::NONE) {
            Serial.printf("[MISC] PICT: 0x%02X\n",
                        static_cast<uint8_t>(getMiscButtonState(g_switchMiscButtonMap.PICT)));
        }

        // D-Pad
        if (getDpadState(g_switchDpadMap.UP) != ButtonState::NONE) {
            Serial.printf("[DPAD] UP: 0x%02X\n",
                        static_cast<uint8_t>(getDpadState(g_switchDpadMap.UP)));
        }

        if (getDpadState(g_switchDpadMap.DOWN) != ButtonState::NONE) {
            Serial.printf("[DPAD] DOWN: 0x%02X\n",
                        static_cast<uint8_t>(getDpadState(g_switchDpadMap.DOWN)));
        }

        if (getDpadState(g_switchDpadMap.LEFT) != ButtonState::NONE) {
            Serial.printf("[DPAD] LEFT: 0x%02X\n",
                        static_cast<uint8_t>(getDpadState(g_switchDpadMap.LEFT)));
        }

        if (getDpadState(g_switchDpadMap.RIGHT) != ButtonState::NONE) {
            Serial.printf("[DPAD] RIGHT: 0x%02X\n",
                        static_cast<uint8_t>(getDpadState(g_switchDpadMap.RIGHT)));
        }
    }

    // ---------------------------------------------------------
    // デバッグ情報を送信バッファへ追加
    // ---------------------------------------------------------
    void sendInfo(const char* message)
    {
        PlatformHttp::sendInfo(message);
    }
    
} // namespace Rte