// PlatformConfig.cpp
// Platformレイヤのハードウェア設定
#include "PlatformConfig.h"

// =============================================================
// PlatformConfig設定ルール
// =============================================================
// ・使用する機能のみ設定する。
// ・未使用の機能はコメントアウトまたは未記述とする。
// ・未記述の機能はPinModeType::UNUSEDとして初期化される。
// ・設定の記述順序は任意。
// =============================================================
const PlatformConfig g_platformConfig = []() {
    PlatformConfig cfg;

    // =========================================================================
    //  現在有効なピン設定（M5Stamp-Pico Mate）
    // =========================================================================

    // --- オンボード周辺機能 ---
    cfg.statusLed     = { 27, PinModeType::NEOPIXEL_OUT,       0, 0,    0 }; // G27: オンボード SK6812 RGB LED
    cfg.userButton    = { 39, PinModeType::DIGITAL_IN_PULLUP,  0, 0,    0 }; // G39: オンボード ボタン (Active LOW)

    // --- シリアル通信バス ---

    // --- モーター制御系 ---
    cfg.motor1In1 = { 26, PinModeType::PWM_OUT, 0, 5000, 8 }; // G26 → DRV8835 AIN1
    cfg.motor1In2 = { 18, PinModeType::PWM_OUT, 1, 5000, 8 }; // G18 → DRV8835 AIN2
    cfg.motor2In1 = { 21, PinModeType::PWM_OUT, 2, 5000, 8 }; // G21 → DRV8835 BIN1
    cfg.motor2In2 = { 22, PinModeType::PWM_OUT, 3, 5000, 8 }; // G22 → DRV8835 BIN2

    // --- アナログ入力系 (ADC) ---

    // --- パルスカウンタ系 (PCNT) ---
    cfg.encoder1A     = { 34, PinModeType::PULSE_COUNTER,     0, 0,    0 }; // G34: [IN-ONLY] エンコーダ1 A相
    cfg.encoder2A     = { 35, PinModeType::PULSE_COUNTER,     0, 0,    0 }; // G35: [IN-ONLY] エンコーダ2 A相

    // --- アナログ出力系 (DAC) ---
    cfg.dacOut1       = { 25, PinModeType::ANALOG_OUT,         0, 0,    0 }; // G25: [DAC1] 8bit DAC出力 (0〜3.3V)


    // =========================================================================
    // 未使用・予備ピン設定 
    // 必要になった機能を有効化する場合に使用する。
    // ピンの競合がないことを確認してから有効化する。
    // =========================================================================
    //cfg.currentSense1 = { 36, PinModeType::ANALOG_IN,          0, 0,    0 }; // G36: [ADC/IN-ONLY] モーター1 電流検出
    //cfg.currentSense2 = { 39, PinModeType::ANALOG_IN,          0, 0,    0 }; // G39: [ADC/IN-ONLY] モーター2 電流検出
    //cfg.motor2Pwm     = { 19, PinModeType::PWM_OUT,            1, 5000, 8 }; // G19: [CS] モーター2 PWM (CH1)
    //cfg.i2cBus        = { 21, 25, 400000 };                               // G21[SDA] / G25[SCL]: I2Cバス (400kHz)
    // cfg.uartComm      = { 1,  3,  115200 };                               // G1[Tx0] / G3[Rx0]: リプロ・デバッグ専用ポートのため開放
    // cfg.dacOut2       = { 26, PinModeType::ANALOG_OUT,         0, 0,    0 }; // G26: [DAC2] 8bit DAC出力
    // cfg.encoder1B     = { 32, PinModeType::PULSE_COUNTER,     0, 0,    0 }; // 予備: エンコーダ1 B相
    // cfg.encoder2B     = { 33, PinModeType::PULSE_COUNTER,     0, 0,    0 }; // 予備: エンコーダ2 B相
    // cfg.analogInAux1  = { 32, PinModeType::ANALOG_IN,          0, 0,    0 }; // G32: [ADC] 予備アナログ入力1
    // cfg.analogInAux2  = { 33, PinModeType::ANALOG_IN,          0, 0,    0 }; // G33: [ADC] 予備アナログ入力2
    // cfg.gpioInAux1    = { 0,  PinModeType::DIGITAL_IN_PULLUP,  0, 0,    0 }; // G0: 予備デジタル入力
    // cfg.gpioOutAux1   = { 2,  PinModeType::DIGITAL_OUT,        0, 0,    0 }; // G2: 予備デジタル出力

    return cfg;
}();