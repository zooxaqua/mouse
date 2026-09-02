#pragma once

#include <stdint.h>

// コントローラ入力情報
struct PlatformControllerInput {

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

// コントローラボタン定義
namespace PlatformControllerButton {

    constexpr uint32_t B = 0x0001;
    constexpr uint32_t A = 0x0002;
    constexpr uint32_t Y = 0x0004;
    constexpr uint32_t X = 0x0008;

    constexpr uint32_t L1 = 0x0010;
    constexpr uint32_t R1 = 0x0020;

    constexpr uint32_t L2 = 0x0040;
    constexpr uint32_t R2 = 0x0080;

    constexpr uint32_t L3 = 0x0100;
    constexpr uint32_t R3 = 0x0200;
}

// D-Pad定義
namespace PlatformControllerDpad {

    constexpr uint8_t UP    = 0x01;
    constexpr uint8_t DOWN  = 0x02;
    constexpr uint8_t RIGHT = 0x04;
    constexpr uint8_t LEFT  = 0x08;
}