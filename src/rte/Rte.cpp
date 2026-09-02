// Rte.cpp
// RTEレイヤの実装

#include "Rte.h"
#include "../platform/Platform.h"
#include "../platform/PlatformConfig.h"
#include "../platform/PlatformBluetooth.h"
#include "../platform/PlatformHttp.h"
#include <Arduino.h>

namespace {

    //■システム用
    // シャットダウン要求フラグ
    bool s_shutdownRequested = false;

    //■INPUT
    // APPのボタン名とPlatformのボタン値を対応付ける
    struct ButtonMap {
        AppButton appButton;
        uint32_t platformValue;
    };
    struct MiscButtonMap {
        AppMiscButton appButton;
        uint32_t platformValue;
    };
    struct DpadMap {
        AppDpad appDpad;
        uint8_t platformValue;
    };
    
    // ボタン変換マップ
    constexpr ButtonMap g_buttonMap[] = {
        { AppButton::A,        0x0002 },
        { AppButton::B,        0x0001 },
        { AppButton::X,        0x0008 },
        { AppButton::Y,        0x0004 },
        { AppButton::R1,       0x0020 },
        { AppButton::L1,       0x0010 },
        { AppButton::R2,       0x0080 },
        { AppButton::L2,       0x0040 },
        { AppButton::R3,       0x0200 },
        { AppButton::L3,       0x0100 },
    };

    constexpr MiscButtonMap g_miscButtonMap[] = {
        { AppMiscButton::HOME,   0x01 },
        { AppMiscButton::MINUS,  0x02 },
        { AppMiscButton::PLUS,   0x04 },
        { AppMiscButton::PICT,   0x08 }
    };

    constexpr DpadMap g_dpadMap[] = {
        { AppDpad::UP,    0x01 },
        { AppDpad::DOWN,  0x02 },
        { AppDpad::LEFT,  0x08 },
        { AppDpad::RIGHT, 0x04 }
    };

    PlatformControllerInput s_controllerInput = {};

    // ---------------------------------------------------------
    // AppButtonから対応するPlatformのマスク値を取得
    // ---------------------------------------------------------
    uint32_t getPlatformButtonValue(AppButton button)
    {
        return g_buttonMap[static_cast<uint8_t>(button)].platformValue;
    }

    // ---------------------------------------------------------
    // AppMiscButtonから対応するPlatformのマスク値を取得
    // ---------------------------------------------------------
    uint32_t getPlatformMiscButtonValue(AppMiscButton button)
    {
        return g_miscButtonMap[static_cast<uint8_t>(button)].platformValue;
    }
    
    // ---------------------------------------------------------
    // AppDpadから対応するPlatformのマスク値を取得
    // ---------------------------------------------------------
    uint8_t getPlatformDpadValue(AppDpad dpad)
    {
        return g_dpadMap[static_cast<uint8_t>(dpad)].platformValue;
    }

    DriveInput s_driveInput = {};

    //■OUTPUT
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
        // Controller Input
        if (PlatformBluetooth::isControllerConnected()) {

            // Bluetoothを優先
            s_controllerInput = PlatformBluetooth::getControllerInput();

        }
        else if (PlatformHttp::isControllerConnected()) {

            // Bluetooth未接続ならHTTP
            s_controllerInput = PlatformHttp::getControllerInput();

        }
        else {

            // どちらも未接続なら安全側へ
            s_controllerInput = {};
        }

        debugButtonState();

        updateDriveInput();
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
        
        return PlatformBluetooth::isControllerConnected() || PlatformHttp::isControllerConnected();
    }

    // ---------------------------------------------------------
    // ボタン状態を取得
    // ---------------------------------------------------------
    ButtonState getButtonState(AppButton button)
    {
        uint32_t mask = getPlatformButtonValue(button);

        // 今回押された
        if ((s_controllerInput.buttonsPressed & mask) != 0) {
            return ButtonState::PRESSED;
        }

        // 今回離された
        if ((s_controllerInput.buttonsReleased & mask) != 0) {
            return ButtonState::RELEASED;
        }

        // 押されている
        if ((s_controllerInput.buttons & mask) != 0) {
            return ButtonState::HOLD;
        }

        // 押されていない
        return ButtonState::NONE;
    }

    // ---------------------------------------------------------
    // Miscボタン状態を取得
    // ---------------------------------------------------------
    ButtonState getMiscButtonState(AppMiscButton button)
    {
        uint32_t mask = getPlatformMiscButtonValue(button);

        // 今回押された
        if ((s_controllerInput.miscButtonsPressed & mask) != 0) {
            return ButtonState::PRESSED;
        }

        // 今回離された
        if ((s_controllerInput.miscButtonsReleased & mask) != 0) {
            return ButtonState::RELEASED;
        }

        // 押されている
        if ((s_controllerInput.miscButtons & mask) != 0) {
            return ButtonState::HOLD;
        }

        // 押されていない
        return ButtonState::NONE;
    }

    // ---------------------------------------------------------
    // D-Pad状態を取得
    // ---------------------------------------------------------
    ButtonState getDpadState(AppDpad dpad)
    {
        uint8_t mask = getPlatformDpadValue(dpad);

        // 今回押された
        if ((s_controllerInput.dpadPressed & mask) != 0) {
            return ButtonState::PRESSED;
        }

        // 今回離された
        if ((s_controllerInput.dpadReleased & mask) != 0) {
            return ButtonState::RELEASED;
        }

        // 押されている
        if ((s_controllerInput.dpad & mask) != 0) {
            return ButtonState::HOLD;
        }

        // 押されていない
        return ButtonState::NONE;
    }

    // ---------------------------------------------------------
    // スティック状態を取得
    // ---------------------------------------------------------
    StickValue getStickValue(AppStick stick)
    {
        if (stick == AppStick::LEFT) {
            return {
                s_controllerInput.leftStickX,
                s_controllerInput.leftStickY
            };
        }

        if (stick == AppStick::RIGHT) {
            return {
                s_controllerInput.rightStickX,
                s_controllerInput.rightStickY
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
        if (getButtonState(AppButton::A) == ButtonState::HOLD ||
            getButtonState(AppButton::A) == ButtonState::PRESSED) {
            s_driveInput.output = 100;
        }

        // 出力：Bボタン押下中はブレーキ
        if (getButtonState(AppButton::B) == ButtonState::HOLD ||
            getButtonState(AppButton::B) == ButtonState::PRESSED) {
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
        if (getButtonState(AppButton::A) != ButtonState::NONE) {
            Serial.printf("[BUTTON] A: 0x%02X\n",
                        static_cast<uint8_t>(getButtonState(AppButton::A)));
        }

        if (getButtonState(AppButton::B) != ButtonState::NONE) {
            Serial.printf("[BUTTON] B: 0x%02X\n",
                        static_cast<uint8_t>(getButtonState(AppButton::B)));
        }

        if (getButtonState(AppButton::X) != ButtonState::NONE) {
            Serial.printf("[BUTTON] X: 0x%02X\n",
                        static_cast<uint8_t>(getButtonState(AppButton::X)));
        }

        if (getButtonState(AppButton::Y) != ButtonState::NONE) {
            Serial.printf("[BUTTON] Y: 0x%02X\n",
                        static_cast<uint8_t>(getButtonState(AppButton::Y)));
        }

        if (getButtonState(AppButton::R1) != ButtonState::NONE) {
            Serial.printf("[BUTTON] R1: 0x%02X\n",
                        static_cast<uint8_t>(getButtonState(AppButton::R1)));
        }

        if (getButtonState(AppButton::L1) != ButtonState::NONE) {
            Serial.printf("[BUTTON] L1: 0x%02X\n",
                        static_cast<uint8_t>(getButtonState(AppButton::L1)));
        }

        if (getButtonState(AppButton::R2) != ButtonState::NONE) {
            Serial.printf("[BUTTON] R2: 0x%02X\n",
                        static_cast<uint8_t>(getButtonState(AppButton::R2)));
        }

        if (getButtonState(AppButton::L2) != ButtonState::NONE) {
            Serial.printf("[BUTTON] L2: 0x%02X\n",
                        static_cast<uint8_t>(getButtonState(AppButton::L2)));
        }

        if (getButtonState(AppButton::R3) != ButtonState::NONE) {
            Serial.printf("[BUTTON] R3: 0x%02X\n",
                        static_cast<uint8_t>(getButtonState(AppButton::R3)));
        }

        if (getButtonState(AppButton::L3) != ButtonState::NONE) {
            Serial.printf("[BUTTON] L3: 0x%02X\n",
                        static_cast<uint8_t>(getButtonState(AppButton::L3)));
        }

        // MISCボタン
        if (getMiscButtonState(AppMiscButton::HOME) != ButtonState::NONE) {
            Serial.printf("[MISC] HOME: 0x%02X\n",
                        static_cast<uint8_t>(getMiscButtonState(AppMiscButton::HOME)));
        }

        if (getMiscButtonState(AppMiscButton::MINUS) != ButtonState::NONE) {
            Serial.printf("[MISC] MINUS: 0x%02X\n",
                        static_cast<uint8_t>(getMiscButtonState(AppMiscButton::MINUS)));
        }

        if (getMiscButtonState(AppMiscButton::PLUS) != ButtonState::NONE) {
            Serial.printf("[MISC] PLUS: 0x%02X\n",
                        static_cast<uint8_t>(getMiscButtonState(AppMiscButton::PLUS)));
        }

        if (getMiscButtonState(AppMiscButton::PICT) != ButtonState::NONE) {
            Serial.printf("[MISC] PICT: 0x%02X\n",
                        static_cast<uint8_t>(getMiscButtonState(AppMiscButton::PICT)));
        }

        // D-Pad
        if (getDpadState(AppDpad::UP) != ButtonState::NONE) {
            Serial.printf("[DPAD] UP: 0x%02X\n",
                        static_cast<uint8_t>(getDpadState(AppDpad::UP)));
        }

        if (getDpadState(AppDpad::DOWN) != ButtonState::NONE) {
            Serial.printf("[DPAD] DOWN: 0x%02X\n",
                        static_cast<uint8_t>(getDpadState(AppDpad::DOWN)));
        }

        if (getDpadState(AppDpad::LEFT) != ButtonState::NONE) {
            Serial.printf("[DPAD] LEFT: 0x%02X\n",
                        static_cast<uint8_t>(getDpadState(AppDpad::LEFT)));
        }

        if (getDpadState(AppDpad::RIGHT) != ButtonState::NONE) {
            Serial.printf("[DPAD] RIGHT: 0x%02X\n",
                        static_cast<uint8_t>(getDpadState(AppDpad::RIGHT)));
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