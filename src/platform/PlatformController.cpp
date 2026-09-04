//PlatformController.cpp
// コントローラ入力情報の定義

#include "PlatformController.h"
#include "PlatformBluetooth.h"
#include "PlatformHttp.h"

#include <Arduino.h>

// =========================================================
// Controller Mapping
// =========================================================
// Switch
const PlatformControllerButtonMap g_switchButtonMap =
{
    .A  = 0x0002,
    .B  = 0x0001,
    .X  = 0x0008,
    .Y  = 0x0004,
    .R1 = 0x0020,
    .L1 = 0x0010,
    .R2 = 0x0080,
    .L2 = 0x0040,
    .R3 = 0x0200,
    .L3 = 0x0100
};

const PlatformControllerDpadMap g_switchDpadMap =
{
    .UP    = 0x01,
    .DOWN  = 0x02,
    .LEFT  = 0x08,
    .RIGHT = 0x04
};

const PlatformControllerMiscButtonMap g_switchMiscButtonMap =
{
    .HOME  = 0x01,
    .MINUS = 0x02,
    .PLUS  = 0x04,
    .PICT  = 0x08
};

// =========================================================
// Current Controller Map
// =========================================================
const PlatformControllerButtonMap* g_platformControllerButtonMap = &g_switchButtonMap;
const PlatformControllerDpadMap* g_platformControllerDpadMap = &g_switchDpadMap;
const PlatformControllerMiscButtonMap* g_platformControllerMiscButtonMap = &g_switchMiscButtonMap;

namespace PlatformController
{
    // =========================================================
    // Controller Input State
    // =========================================================
    PlatformControllerData s_controllerInput = {};
    PlatformControllerData s_bluetoothControllerInput = {};
    PlatformControllerData s_httpControllerInput = {};

    uint32_t s_previousButtons = 0;
    uint8_t s_previousDpad = 0;
    uint8_t s_previousMiscButtons = 0;

    uint32_t s_previousHttpButtons = 0;
    uint8_t s_previousHttpDpad = 0;

    // ---------------------------------------------------------
    // コントローラ入力を更新する
    // ---------------------------------------------------------
    void updateControllerInput()
    {
        if (PlatformBluetooth::isControllerConnected()) {
            s_controllerInput = s_bluetoothControllerInput;
        }
        else if (PlatformHttp::isControllerConnected()) {

            s_httpControllerInput.buttonsPressed = s_httpControllerInput.buttons & ~s_previousHttpButtons;
            s_httpControllerInput.buttonsReleased = s_previousHttpButtons & ~s_httpControllerInput.buttons;
            s_previousHttpButtons = s_httpControllerInput.buttons;
            
            s_httpControllerInput.dpadPressed = s_httpControllerInput.dpad & ~s_previousHttpDpad;
            s_httpControllerInput.dpadReleased = s_previousHttpDpad & ~s_httpControllerInput.dpad;
            s_previousHttpDpad = s_httpControllerInput.dpad;
            s_controllerInput = s_httpControllerInput;

        }
        else {
            clearControllerInput();
        }
    }

    // ---------------------------------------------------------
    // スティック入力値を-100～+100へ正規化
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
    // コントローラの種類を設定する
    // ---------------------------------------------------------
    void setControllerType(PlatformControllerType type)
    {
        switch (type)
        {
            case PlatformControllerType::SWITCH:
                g_platformControllerButtonMap = &g_switchButtonMap;
                g_platformControllerDpadMap = &g_switchDpadMap;
                g_platformControllerMiscButtonMap = &g_switchMiscButtonMap;
                break;

            case PlatformControllerType::PS3:
                // TODO: PS3 MAP
                break;
        }
    }

    // ---------------------------------------------------------
    // コントローラ入力を更新する
    // ---------------------------------------------------------
    void updateBlueToothControllerInput(int32_t leftStickX, int32_t leftStickY, int32_t rightStickX, int32_t rightStickY, uint32_t buttons, uint8_t dpad, uint8_t miscButtons)
    {
        // スティック入力
        s_bluetoothControllerInput.leftStickX = normalizeStickValue(leftStickX);
        s_bluetoothControllerInput.leftStickY = normalizeStickValue(leftStickY);
        s_bluetoothControllerInput.rightStickX = normalizeStickValue(rightStickX);
        s_bluetoothControllerInput.rightStickY = normalizeStickValue(rightStickY);

        // 通常ボタン入力
        {
            s_bluetoothControllerInput.buttons = buttons;
            s_bluetoothControllerInput.buttonsPressed = buttons & ~s_previousButtons;
            s_bluetoothControllerInput.buttonsReleased = s_previousButtons & ~buttons;

            s_previousButtons = buttons;
        }

        // D-Pad入力
        {
            s_bluetoothControllerInput.dpad = dpad;
            s_bluetoothControllerInput.dpadPressed = dpad & ~s_previousDpad;
            s_bluetoothControllerInput.dpadReleased = s_previousDpad & ~dpad;

            s_previousDpad = dpad;
        }

        // Miscボタン入力
        {
            s_bluetoothControllerInput.miscButtons = miscButtons;
            s_bluetoothControllerInput.miscButtonsPressed = miscButtons & ~s_previousMiscButtons;
            s_bluetoothControllerInput.miscButtonsReleased = s_previousMiscButtons & ~miscButtons;

            s_previousMiscButtons = miscButtons;
        }
    }

    // ---------------------------------------------------------
    // コントローラ入力をクリアする
    // ---------------------------------------------------------
    void clearControllerInput()
    {
        s_controllerInput = {};
        s_bluetoothControllerInput = {};
        s_httpControllerInput = {};

        s_previousButtons = 0;
        s_previousDpad = 0;
        s_previousMiscButtons = 0;

        s_previousHttpButtons = 0;
        s_previousHttpDpad = 0;
    }
    // ---------------------------------------------------------
    // HTTPコントローラ入力をクリアする
    // ---------------------------------------------------------
    void clearHttpControllerInput()
    {
        s_httpControllerInput = {};
        s_previousHttpButtons = 0;
        s_previousHttpDpad = 0;
    }

    // ---------------------------------------------------------
    // Bluetoothコントローラ入力をクリアする
    // ---------------------------------------------------------
    void clearBluetoothControllerInput()
    {
        s_bluetoothControllerInput = {};
    }

    // ---------------------------------------------------------
    // コントローラ入力を取得する
    // ---------------------------------------------------------
    PlatformControllerData getControllerInput()
    {
        return s_controllerInput;
    }
    
    // ---------------------------------------------------------
    // コントローラ接続状態を設定する
    // ---------------------------------------------------------
    void setControllerConnected(bool connected)
    {
        if (!connected) {
            clearControllerInput();
        }
    }

    // ---------------------------------------------------------
    // コントローラ接続状態を取得する
    // ---------------------------------------------------------
    bool isControllerConnected()
    {
        if (PlatformBluetooth::isControllerConnected()) {
            return true;
        }

        if (PlatformHttp::isControllerConnected()) {
            return true;
        }

        return false;
    }
        
    // ---------------------------------------------------------
    // HTTP Aボタン入力を更新する
    // ---------------------------------------------------------
    void updateHttpButtonA(bool pressed)
    {
        if (pressed) {
            s_httpControllerInput.buttons |= g_platformControllerButtonMap->A;
        }
        else {
            s_httpControllerInput.buttons &= ~g_platformControllerButtonMap->A;
        }
    }

    // ---------------------------------------------------------
    // HTTP Bボタン入力を更新する
    // ---------------------------------------------------------
    void updateHttpButtonB(bool pressed)
    {
        if (pressed) {
            s_httpControllerInput.buttons |= g_platformControllerButtonMap->B;
        }
        else {
            s_httpControllerInput.buttons &= ~g_platformControllerButtonMap->B;
        }
    }

    // ---------------------------------------------------------
    // HTTP Xボタン入力を更新する
    // ---------------------------------------------------------
    void updateHttpButtonX(bool pressed)
    {
        if (pressed) {
            s_httpControllerInput.buttons |= g_platformControllerButtonMap->X;
        }
        else {
            s_httpControllerInput.buttons &= ~g_platformControllerButtonMap->X;
        }
    }

    // ---------------------------------------------------------
    // HTTP Yボタン入力を更新する
    // ---------------------------------------------------------
    void updateHttpButtonY(bool pressed)
    {
        if (pressed) {
            s_httpControllerInput.buttons |= g_platformControllerButtonMap->Y;
        }
        else {
            s_httpControllerInput.buttons &= ~g_platformControllerButtonMap->Y;
        }
    }

    // ---------------------------------------------------------
    // HTTP D-Pad UP入力を更新する
    // ---------------------------------------------------------
    void updateHttpDpadUp(bool pressed)
    {
        if (pressed) {
            s_httpControllerInput.dpad |= g_platformControllerDpadMap->UP;
        }
        else {
            s_httpControllerInput.dpad &= ~g_platformControllerDpadMap->UP;
        }
    }

    // ---------------------------------------------------------
    // HTTP D-Pad DOWN入力を更新する
    // ---------------------------------------------------------
    void updateHttpDpadDown(bool pressed)
    {
        if (pressed) {
            s_httpControllerInput.dpad |= g_platformControllerDpadMap->DOWN;
        }
        else {
            s_httpControllerInput.dpad &= ~g_platformControllerDpadMap->DOWN;
        }
    }

    // ---------------------------------------------------------
    // HTTP D-Pad LEFT入力を更新する
    // ---------------------------------------------------------
    void updateHttpDpadLeft(bool pressed)
    {
        if (pressed) {
            s_httpControllerInput.dpad |= g_platformControllerDpadMap->LEFT;
        }
        else {
            s_httpControllerInput.dpad &= ~g_platformControllerDpadMap->LEFT;
        }
    }

    // ---------------------------------------------------------
    // HTTP D-Pad RIGHT入力を更新する
    // ---------------------------------------------------------
    void updateHttpDpadRight(bool pressed)
    {
        if (pressed) {
            s_httpControllerInput.dpad |= g_platformControllerDpadMap->RIGHT;
        }
        else {
            s_httpControllerInput.dpad &= ~g_platformControllerDpadMap->RIGHT;
        }
    }

    // ---------------------------------------------------------
    // HTTP Left Stick入力を更新する
    // ---------------------------------------------------------
    void updateHttpLeftStick(int8_t x, int8_t y)
    {
        s_httpControllerInput.leftStickX = x;
        s_httpControllerInput.leftStickY = y;
    }

    // ---------------------------------------------------------
    // HTTP Right Stick入力を更新する
    // ---------------------------------------------------------
    void updateHttpRightStick(int8_t x, int8_t y)
    {
        s_httpControllerInput.rightStickX = x;
        s_httpControllerInput.rightStickY = y;
    }

}