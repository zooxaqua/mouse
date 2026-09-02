// Platform.h
// Platformレイヤの公開インターフェース

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include "PlatformConfig.h"

// 上位レイヤから指定するPWM
enum class PwmOutput : uint8_t {
    MOTOR1_IN1,
    MOTOR1_IN2,
    MOTOR2_IN1,
    MOTOR2_IN2
};

// 上位レイヤから指定するGPIO出力
enum class LogicalGpioOut {
    MOTOR1_DIR, // モーター1回転方向
    MOTOR2_DIR  // モーター2回転方向
};

// 上位レイヤから指定するGPIO入力
enum class LogicalGpioIn {
    USER_BUTTON // ユーザボタン
};

// 上位レイヤから指定するADC入力
enum class LogicalAnalogIn {
    M1_CURRENT, // モーター1電流
    M2_CURRENT  // モーター2電流
};

// 上位レイヤから指定するDAC出力
enum class LogicalDacOut {
    DAC_CH1     // DAC出力チャネル1
};

// 上位レイヤから指定するパルスカウンタ
enum class LogicalPulseCounter {
    ENCODER_1,  // モーター1エンコーダ
    ENCODER_2   // モーター2エンコーダ
};

// RGB LEDの色情報
struct RgbColor {
    uint8_t r;  //赤色輝度 (0〜255)
    uint8_t g;  //緑色輝度 (0〜255)
    uint8_t b;  //青色輝度 (0〜255)
};

class Platform {
public:

    static void init(const PlatformConfig& config);                        // Platformとハードウェアを初期化
    static void updateInput();                                             // ハードウェアから入力を取得
    static void updateOutput();                                            // 出力バッファの内容をハードウェアへ反映
    static void updateInfo();                                              // 情報送信用
    static bool shutdown();                                                // ハードウェアを安全状態へ移行
    static void setPwmDuty(PwmOutput output, uint8_t dutyPercent);         // PWMデューティ比を設定
    static void setGpioOutput(LogicalGpioOut target, bool state);          // GPIO出力状態を設定
    static void setDacVoltage(LogicalDacOut target, uint8_t rawValue);     // DAC出力値を設定
    static void setStatusLedColor(RgbColor color);                         // RGB LEDの色を設定
    static bool getGpioInput(LogicalGpioIn target);                        // GPIO入力状態を取得
    static uint16_t getAnalogInput(LogicalAnalogIn target);                // ADC入力値を取得
    static int32_t getPulseCount(LogicalPulseCounter target);              // パルスカウンタ値を取得

private:

    // Platform内部で保持する入力状態
    struct InputBuffer {
        bool buttonPressed;    //ボタン押下フラグ
        uint16_t m1CurrentRaw; //モーター1 電流検出ADC直値
        uint16_t m2CurrentRaw; //モーター2 電流検出ADC直値
        int32_t enc1Count;     //エンコーダ1 累積パルス数
        int32_t enc2Count;     //エンコーダ2 累積パルス数
    };

    // Platform内部で保持する出力状態
    struct OutputBuffer {
        uint8_t m1In1Duty;   // モーター1 AIN1 Duty (%)
        uint8_t m1In2Duty;   // モーター1 AIN2 Duty (%)
        uint8_t m2In1Duty;   // モーター2 BIN1 Duty (%)
        uint8_t m2In2Duty;   // モーター2 BIN2 Duty (%)
        uint8_t dacValue;    //DAC出力レジスタ値
        RgbColor ledColor;   //SK6812 RGBカラー値
    };

    static OutputBuffer m_outBuf;                                                   //内部出力バッファ実体
    static InputBuffer  m_inBuf;                                                    //内部入力バッファ実体
    static PlatformConfig m_config;                                                 //適用中のプラットフォーム設定
    static void drv_pwmInit(const PinConfig& cfg);                                  // PWMを初期化
    static void drv_pwmWrite(const PinConfig& cfg, uint8_t dutyPercent);            // PWMを出力
    static void drv_gpioInit(const PinConfig& cfg);                                 // GPIOを初期化
    static void drv_gpioWrite(const PinConfig& cfg, bool state);                    // GPIOへ出力
    static bool drv_gpioRead(const PinConfig& cfg);                                 // GPIOから入力
    static uint16_t drv_adcRead(const PinConfig& cfg);                              // ADCから入力
    static void drv_dacWrite(const PinConfig& cfg, uint8_t value);                  // DACへ出力
    static void drv_neopixelWrite(uint8_t pin, uint8_t r, uint8_t g, uint8_t b);    // RGB LEDへ出力
};

#endif // PLATFORM_H