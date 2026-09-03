//PlatformControllerInput.cpp
// コントローラ入力情報の定義

#pragma once

#include <stdint.h>

enum class PlatformControllerType
{
    SWITCH,
    PS3
};

struct PlatformControllerButtonMap
{
    uint32_t A;
    uint32_t B;
    uint32_t X;
    uint32_t Y;
    uint32_t R1;
    uint32_t L1;
    uint32_t R2;
    uint32_t L2;
    uint32_t R3;
    uint32_t L3;
};

struct PlatformControllerDpadMap
{
    uint8_t UP;
    uint8_t DOWN;
    uint8_t LEFT;
    uint8_t RIGHT;
};

struct PlatformControllerMiscButtonMap
{
    uint8_t HOME;
    uint8_t MINUS;
    uint8_t PLUS;
    uint8_t PICT;
};

// コントローラ入力情報
struct PlatformControllerData {

    // スティック値（-100 ～ +100）
    int8_t leftStickX;
    int8_t leftStickY;
    int8_t rightStickX;
    int8_t rightStickY;

    // ボタン状態
    uint32_t buttons;
    uint32_t buttonsPressed;
    uint32_t buttonsReleased;

    // D-Pad状態
    uint8_t dpad;
    uint8_t dpadPressed;
    uint8_t dpadReleased;

    // その他のボタン状態
    uint8_t miscButtons;
    uint8_t miscButtonsPressed;
    uint8_t miscButtonsReleased;
};

// Controller Mapping
extern const PlatformControllerButtonMap g_switchButtonMap;
extern const PlatformControllerDpadMap g_switchDpadMap;
extern const PlatformControllerMiscButtonMap g_switchMiscButtonMap;

// Current Controller Map
extern const PlatformControllerButtonMap* g_platformControllerButtonMap;
extern const PlatformControllerDpadMap* g_platformControllerDpadMap;
extern const PlatformControllerMiscButtonMap* g_platformControllerMiscButtonMap;

namespace PlatformControllerInput
{
    // コントローラ種類を設定する
    void setControllerType(PlatformControllerType type);

    // コントローラ入力を更新する
    void updateBlueToothControllerInput(
        int32_t leftStickX,
        int32_t leftStickY,
        int32_t rightStickX,
        int32_t rightStickY,
        uint32_t buttons,
        uint8_t dpad,
        uint8_t miscButtons
    );

    // コントローラ入力をクリアする
    void clearControllerInput();

    // HTTPコントローラ入力をクリアする
    void clearHttpControllerInput();

    // Bluetoothコントローラ入力をクリアする
    void clearBluetoothControllerInput();

    // コントローラ接続状態を設定する
    void setControllerConnected(bool connected);

    // コントローラ接続状態を取得する
    bool isControllerConnected();

    // 現在のコントローラ入力を取得する
    PlatformControllerData getControllerInput();

    // コントローラ入力を更新する
    void updateControllerInput();

    // HTTPからAボタン入力を通知する
    void updateHttpButtonA(bool pressed);
    // HTTPからBボタン入力を通知する
    void updateHttpButtonB(bool pressed);
    // HTTPからXボタン入力を通知する
    void updateHttpButtonX(bool pressed);
    // HTTPからYボタン入力を通知する
    void updateHttpButtonY(bool pressed);

    // HTTPからD-Pad入力を通知する
    void updateHttpDpadUp(bool pressed);
    void updateHttpDpadDown(bool pressed);
    void updateHttpDpadLeft(bool pressed);
    void updateHttpDpadRight(bool pressed);

    // HTTPから左スティック入力を通知する
    void updateHttpLeftStick(int8_t x, int8_t y);
    // HTTPから右スティック入力を通知する
    void updateHttpRightStick(int8_t x, int8_t y);

}
