#include <Arduino.h>
#include <bluepad32.h>

#include "PlatformBluetooth.h"

namespace {

    PlatformControllerInput s_controllerInput = {};

    // 前回の入力状態
    uint32_t s_previousButtons = 0;
    uint8_t s_previousDpad = 0;
    uint8_t s_previousMiscButtons = 0;

    // 接続中のコントローラ
    ControllerPtr s_controller = nullptr;
    
    // ---------------------------------------------------------
    // スティック入力値を-100～+100へ正規化
    // ---------------------------------------------------------
    // Bluepad32から取得した入力値を、アプリケーションで使用する
    // -100～+100の範囲へ変換する。
    // -5～+5はデッドゾーンとして0にする。
    // ---------------------------------------------------------
    int8_t normalizeStickValue(int32_t value)
    {
        int32_t normalized = (value * 100) / 512;

        if (normalized > 100) {
            normalized = 100;
        }
        else if (normalized < -100) {
            normalized = -100;
        }

        if (normalized >= -5 && normalized <= 5) {
            normalized = 0;
        }

        return static_cast<int8_t>(normalized);
    }

    // ---------------------------------------------------------
    // コントローラ接続時処理
    // ---------------------------------------------------------
    void onConnectedController(ControllerPtr ctl)
    {
        if (s_controller == nullptr) {

            Serial.print("[BT] CALLBACK: Controller connected, index=");
            Serial.println(ctl->index());

            s_controller = ctl;

            // 接続されたデバイスの種類を確認
            Serial.printf("[BT] Is Gamepad: %d\n", ctl->isGamepad());
            Serial.printf("[BT] Is Mouse: %d\n", ctl->isMouse());
            Serial.printf("[BT] Is Keyboard: %d\n", ctl->isKeyboard());
        }
    }


    // ---------------------------------------------------------
    // コントローラ切断時の処理
    // ---------------------------------------------------------
    void onDisconnectedController(ControllerPtr ctl)
    {
        if (s_controller == ctl) {

            Serial.println("[BT] CALLBACK: Controller disconnected");

            s_controller = nullptr;
        }
    }
    // ---------------------------------------------------------
    // コントローラ入力を更新
    // ---------------------------------------------------------
    void updateControllerInput()
    {
        // コントローラ未接続
        if (s_controller == nullptr ||
            !s_controller->isConnected()) {

            // 未接続時は入力をクリア
            s_controllerInput = {};
            s_previousButtons = 0;
            s_previousDpad = 0;
            s_previousMiscButtons = 0;

            return;
        }

        // スティック入力
        {
            s_controllerInput.leftStickX = normalizeStickValue(s_controller->axisX());
            s_controllerInput.leftStickY = normalizeStickValue(s_controller->axisY());
            s_controllerInput.rightStickX = normalizeStickValue(s_controller->axisRX());
            s_controllerInput.rightStickY = normalizeStickValue(s_controller->axisRY());
        }

        // 通常ボタン入力
        {
            uint32_t currentButtons = s_controller->buttons();

            s_controllerInput.buttons = currentButtons;                              // 現在押されている
            s_controllerInput.buttonsPressed = currentButtons & ~s_previousButtons;  // 今回押された
            s_controllerInput.buttonsReleased = s_previousButtons & ~currentButtons; // 今回離された
            s_previousButtons = currentButtons;                                      // 次回比較用
        }

        // D-Pad入力
        {
            uint8_t currentDpad = s_controller->dpad();

            s_controllerInput.dpad = currentDpad;                           // 現在押されている
            s_controllerInput.dpadPressed = currentDpad & ~s_previousDpad;  // 今回押された
            s_controllerInput.dpadReleased = s_previousDpad & ~currentDpad; // 今回離された
            s_previousDpad = currentDpad;                                   // 次回比較用
        }

        // Miscボタン入力
        {
            uint8_t currentMiscButtons = s_controller->miscButtons();

            s_controllerInput.miscButtons = currentMiscButtons;                                  // 現在押されている
            s_controllerInput.miscButtonsPressed = currentMiscButtons & ~s_previousMiscButtons;  // 今回押された
            s_controllerInput.miscButtonsReleased = s_previousMiscButtons & ~currentMiscButtons; // 今回離された
            s_previousMiscButtons = currentMiscButtons;                                          // 次回比較用
        }
    }

}

namespace PlatformBluetooth {
    // ---------------------------------------------------------
    // Bluetooth初期化処理
    // ---------------------------------------------------------
    void init()
    {
        Serial.println("[INIT] Initializing Bluepad32 Bluetooth Stack...");

        BP32.setup(&onConnectedController, &onDisconnectedController);
    }

    // ---------------------------------------------------------
    // Bluetooth入力更新処理
    // ---------------------------------------------------------
    void update()
    {
        // Bluepad32から最新のコントローラ情報を取得
        BP32.update();

        // コントローラ入力を更新
        updateControllerInput();
    }

    // ---------------------------------------------------------
    // コントローラ接続状態を取得
    // ---------------------------------------------------------
    bool isControllerConnected()
    {
        return s_controller != nullptr && s_controller->isConnected();
    }

    // ---------------------------------------------------------
    // コントローラ入力を取得
    // ---------------------------------------------------------
    PlatformControllerInput getControllerInput()
    {
        return s_controllerInput;
    }
}