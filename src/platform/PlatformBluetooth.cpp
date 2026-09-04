// PlatformBluetooth.cpp
// Bluepad32を使用したBluetoothコントローラ入力処理

#include <Arduino.h>
#include <bluepad32.h>

#include "PlatformBluetooth.h"

namespace {
    // 接続中のコントローラ
    ControllerPtr s_controller = nullptr;

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

            // 現在はSwitchコントローラとしてMAPを設定（仮）
            PlatformController::setControllerType(PlatformControllerType::SWITCH);

            // コントローラ接続状態を通知
            PlatformController::setControllerConnected(true);
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

            // コントローラ接続状態を通知
            PlatformController::setControllerConnected(false);
        }
    }
    // ---------------------------------------------------------
    // コントローラ入力を更新
    // ---------------------------------------------------------
    void updateControllerInput()
    {
        // コントローラ未接続
        if (s_controller == nullptr || !s_controller->isConnected()) {

            s_controller = nullptr;

            return;
        }

        //引数はs_controllerだけでよくない？
        PlatformController::updateBlueToothControllerInput(
            s_controller->axisX(),
            s_controller->axisY(),
            s_controller->axisRX(),
            s_controller->axisRY(),
            s_controller->buttons(),
            s_controller->dpad(),
            s_controller->miscButtons()
        );
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
}