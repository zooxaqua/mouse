// PlatformConfig.h
// Platformレイヤ 全物理ピン機能統合設定ヘッダー

#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

#include <stdint.h>

// ESP32 全物理ピン機能モード定義
enum class PinModeType {
    UNUSED,              // 未使用ピン (初期化処理をスキップ)
    
    // --- デジタル入出力 ---
    DIGITAL_IN,          // デジタル入力 (Hi-Zハイインピーダンス)
    DIGITAL_IN_PULLUP,   // 内部プルアップ抵抗付きデジタル入力
    DIGITAL_IN_PULLDOWN, // 内部プルダウン抵抗付きデジタル入力
    DIGITAL_OUT,         // デジタル出力 (プッシュプル)
    
    // --- アナログ機能 ---
    ANALOG_IN,           // ADC アナログ入力 (12bit: 0〜4095)
    ANALOG_OUT,          // DAC アナログ電圧出力 (8bit: 0〜255)
    
    // --- タイマー・パルス・モータ機能 ---
    PWM_OUT,             // LEDC PWM信号出力
    PULSE_COUNTER,       // PCNT パルスカウンタ入力 (エンコーダ等)
    
    // --- シリアル制御機能 ---
    NEOPIXEL_OUT         // 1-Wireシリアルデータ出力 (SK6812 / WS2812B RGB LED)
};

// 単一ピンのピン配置および動作パラメータ設定構造体
struct PinConfig {
    uint8_t gpioPin;       // 割り当てるGPIOピン番号 (未使用時は 255)
    PinModeType mode;      // ピン動作モード
    uint8_t pwmChannel;    // PWM使用時のタイマーチャネル (0〜15)
    uint32_t pwmFreq;      // PWM周波数 (Hz)
    uint8_t pwmResBits;    // PWM解像度ビット数 (Bits)

    // デフォルトコンストラクタ（未設定時は自動的にUNUSED扱い）
    constexpr PinConfig() 
        : gpioPin(255), mode(PinModeType::UNUSED), pwmChannel(0), pwmFreq(0), pwmResBits(0) {}

    // パラメータ指定コンストラクタ
    constexpr PinConfig(uint8_t pin, PinModeType m, uint8_t ch = 0, uint32_t freq = 0, uint8_t res = 0)
        : gpioPin(pin), mode(m), pwmChannel(ch), pwmFreq(freq), pwmResBits(res) {}
};

// I2C 通信バス設定構造体
struct I2cConfig {
    uint8_t sdaPin;        // SDA(データ) GPIOピン
    uint8_t sclPin;        // SCL(クロック) GPIOピン
    uint32_t clockHz;      // クロック周波数 (Hz)

    constexpr I2cConfig(uint8_t sda = 255, uint8_t scl = 255, uint32_t hz = 100000)
        : sdaPin(sda), sclPin(scl), clockHz(hz) {}
};

// UART シリアル通信バス設定構造体
struct UartConfig {
    uint8_t txPin;         // TX(送信) GPIOピン
    uint8_t rxPin;         // RX(受信) GPIOピン
    uint32_t baudRate;     // ボーレート (bps)

    constexpr UartConfig(uint8_t tx = 255, uint8_t rx = 255, uint32_t baud = 115200)
        : txPin(tx), rxPin(rx), baudRate(baud) {}
};

// Platformレイヤ全体のハードウェア統合設定
struct PlatformConfig {
    // === モーター制御系 (DRV8835 IN/IN) ===
    PinConfig motor1In1;     // モーター1 AIN1
    PinConfig motor1In2;     // モーター1 AIN2
    PinConfig motor2In1;     // モーター2 BIN1
    PinConfig motor2In2;     // モーター2 BIN2

    // === アナログ入力系 (ADC) ===
    PinConfig currentSense1; // モーター1 電流検出ADCピン
    PinConfig currentSense2; // モーター2 電流検出ADCピン
    PinConfig analogInAux1;  // 予備 アナログ入力ピン1
    PinConfig analogInAux2;  // 予備 アナログ入力ピン2

    // === パルスカウンタ系 (PCNT / エンコーダ) ===
    PinConfig encoder1A;     // エンコーダ1 A相パルス入力
    PinConfig encoder1B;     // エンコーダ1 B相パルス入力
    PinConfig encoder2A;     // エンコーダ2 A相パルス入力
    PinConfig encoder2B;     // エンコーダ2 B相パルス入力

    // === アナログ出力系 (DAC) ===
    PinConfig dacOut1;       // アナログ出力(DAC1 / GPIO25)
    PinConfig dacOut2;       // アナログ出力(DAC2 / GPIO26)

    // === デジタル汎用入出力 (GPIO) ===
    PinConfig userButton;    // ユーザ操作用入力ボタン
    PinConfig gpioInAux1;    // 予備 デジタル入力ピン
    PinConfig gpioOutAux1;   // 予備 デジタル出力ピン

    // === オンボード周辺機能 ===
    PinConfig statusLed;     // オンボード SK6812 Smart RGB LED

    // === シリアル通信バス系 ===
    I2cConfig i2cBus;        // I2C 通信バス
    UartConfig uartComm;     // UART シリアル通信バス

    // デフォルトコンストラクタ（全項目初期化）
    PlatformConfig() = default;
};

// 全体で共通参照する Platform設定実体への外部参照宣言
extern const PlatformConfig g_platformConfig;

#endif // PLATFORM_CONFIG_H