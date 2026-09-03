// Platform.cpp
// Platformレイヤの実装

#include "Platform.h"
#include "PlatformBluetooth.h"
#include "PlatformWifi.h"
#include "PlatformHttp.h"
#include "PlatformControllerInput.h"
#include <Arduino.h>

// 静的メンバ変数
Platform::OutputBuffer Platform::m_outBuf = { 0, 0, 0, 0, 0, {0, 0, 0} };
Platform::InputBuffer  Platform::m_inBuf  = { false, 0, 0, 0, 0 };
PlatformConfig         Platform::m_config;

/*
namespace {

}
*/

// Platformの初期化
void Platform::init(const PlatformConfig& config)
{
    Serial.println("[INIT] Platform initializing...");

    m_config = config;

    // Bluetoothを初期化
    PlatformBluetooth::init();

    //Wifiを初期化
    PlatformWifi::init();

    //HTTPを初期化
    PlatformHttp::init();

    // PWMを初期化
    drv_pwmInit(m_config.motor1In1);
    drv_pwmInit(m_config.motor1In2);

    drv_pwmInit(m_config.motor2In1);
    drv_pwmInit(m_config.motor2In2);

    // GPIOを初期化
    drv_gpioInit(m_config.userButton);

    // DACを初期化
    if (m_config.dacOut1.mode == PinModeType::ANALOG_OUT) {
        dacWrite(m_config.dacOut1.gpioPin, 0);
    }

    // RGB LEDを初期化
    if (m_config.statusLed.mode == PinModeType::NEOPIXEL_OUT) {
        drv_neopixelWrite(m_config.statusLed.gpioPin, 0, 0, 0); // 初期消灯
    }

    Serial.println("[INIT] Platform Full-Feature Initialization COMPLETE");
}


// ---------------------------------------------------------
// Platform入力処理
// ---------------------------------------------------------
void Platform::updateInput()
{
    // 端子処理
    //なし

    // Bluetooth入力処理
    PlatformBluetooth::update();

    // Wifi処理
    PlatformWifi::update();

    // HTTP / WebSocket入力処理
    PlatformHttp::update();

    // コントローラ入力を更新
    PlatformControllerInput::updateControllerInput();
}

// ---------------------------------------------------------
// Platform出力処理
// ---------------------------------------------------------
void Platform::updateOutput() {
    // ■端子処理
    // モーター1（AIN1 / AIN2）
    drv_pwmWrite(m_config.motor1In1, m_outBuf.m1In1Duty);
    drv_pwmWrite(m_config.motor1In2, m_outBuf.m1In2Duty);

    // モーター2（BIN1 / BIN2）
    drv_pwmWrite(m_config.motor2In1, m_outBuf.m2In1Duty);
    drv_pwmWrite(m_config.motor2In2, m_outBuf.m2In2Duty);

    // SK6812 RGB LEDの物理更新
    drv_neopixelWrite(m_config.statusLed.gpioPin, m_outBuf.ledColor.r, m_outBuf.ledColor.g, m_outBuf.ledColor.b);
}

// ---------------------------------------------------------
// デバッグ情報を更新
// ---------------------------------------------------------
void Platform::updateInfo()
{
    PlatformHttp::updateInfo();
}

// ---------------------------------------------------------
// Platform安全停止処理
// ---------------------------------------------------------
bool Platform::shutdown() {
    Serial.println("[SHUTDOWN] Platform shutting down safely...");

    // 1. 内部出力バッファをすべて安全状態（0/OFF/消灯）にクリア
    m_outBuf.m1In1Duty = 0;
    m_outBuf.m1In2Duty = 0;
    m_outBuf.m2In1Duty = 0;
    m_outBuf.m2In2Duty = 0;
    m_outBuf.dacValue  = 0;
    m_outBuf.ledColor  = {0, 0, 0};

    // 2. クリアしたバッファを物理ハードウェアへ即時出力
    updateOutput();

    Serial.println("[SHUTDOWN] Platform Safe-State Established");

    return true;
}

// =============================================================
// 上位レイヤ用API
// =============================================================

// PWM出力値を設定
void Platform::setPwmDuty(PwmOutput output, uint8_t dutyPercent)
{
    switch (output) {

        case PwmOutput::MOTOR1_IN1:
            m_outBuf.m1In1Duty = dutyPercent;
            break;

        case PwmOutput::MOTOR1_IN2:
            m_outBuf.m1In2Duty = dutyPercent;
            break;

        case PwmOutput::MOTOR2_IN1:
            m_outBuf.m2In1Duty = dutyPercent;
            break;

        case PwmOutput::MOTOR2_IN2:
            m_outBuf.m2In2Duty = dutyPercent;
            break;
    }
}
// ---------------------------------------------------------
// GPIO出力値を設定
// ---------------------------------------------------------
void Platform::setGpioOutput(LogicalGpioOut target, bool state) {
    
}
// ---------------------------------------------------------
// DAC出力値を設定
// ---------------------------------------------------------
void Platform::setDacVoltage(LogicalDacOut target, uint8_t rawValue) {
    if (target == LogicalDacOut::DAC_CH1) m_outBuf.dacValue = rawValue;
}

// ---------------------------------------------------------
// RGB LEDの色を設定
// ---------------------------------------------------------
void Platform::setStatusLedColor(RgbColor color) {
    m_outBuf.ledColor = color;
}

// ---------------------------------------------------------
// GPIO入力値を取得
// ---------------------------------------------------------
bool Platform::getGpioInput(LogicalGpioIn target) {
    if (target == LogicalGpioIn::USER_BUTTON) return m_inBuf.buttonPressed;
    return false;
}

// ---------------------------------------------------------
// ADC入力値を取得
// ---------------------------------------------------------
uint16_t Platform::getAnalogInput(LogicalAnalogIn target) {
    if (target == LogicalAnalogIn::M1_CURRENT) return m_inBuf.m1CurrentRaw;
    if (target == LogicalAnalogIn::M2_CURRENT) return m_inBuf.m2CurrentRaw;
    return 0;
}

// ---------------------------------------------------------
// パルスカウンタ値を取得
// ---------------------------------------------------------
int32_t Platform::getPulseCount(LogicalPulseCounter target) {
    if (target == LogicalPulseCounter::ENCODER_1) return m_inBuf.enc1Count;
    if (target == LogicalPulseCounter::ENCODER_2) return m_inBuf.enc2Count;
    return 0;
}

// =============================================================
// 内部ドライバー実装 (マイコン固有処理の吸収)
// =============================================================

// ---------------------------------------------------------
// PWM出力を初期化
// ---------------------------------------------------------
void Platform::drv_pwmInit(const PinConfig& cfg) {
    if (cfg.mode != PinModeType::PWM_OUT) return;
    ledcSetup(cfg.pwmChannel, cfg.pwmFreq, cfg.pwmResBits);
    ledcAttachPin(cfg.gpioPin, cfg.pwmChannel);
    ledcWrite(cfg.pwmChannel, 0);
}
// ---------------------------------------------------------
// PWM出力を物理デバイスへ書き込む
// ---------------------------------------------------------
void Platform::drv_pwmWrite(const PinConfig& cfg, uint8_t dutyPercent) {
    if (cfg.mode != PinModeType::PWM_OUT) return;
    uint32_t rawDuty = (dutyPercent * 255) / 100;
    ledcWrite(cfg.pwmChannel, rawDuty);
}

// ---------------------------------------------------------
// GPIOを初期化
// ---------------------------------------------------------
void Platform::drv_gpioInit(const PinConfig& cfg) {
    if (cfg.mode == PinModeType::DIGITAL_OUT) {
        pinMode(cfg.gpioPin, OUTPUT);
        digitalWrite(cfg.gpioPin, LOW);
    } else if (cfg.mode == PinModeType::DIGITAL_IN) {
        pinMode(cfg.gpioPin, INPUT);
    } else if (cfg.mode == PinModeType::DIGITAL_IN_PULLUP) {
        pinMode(cfg.gpioPin, INPUT_PULLUP);
    } else if (cfg.mode == PinModeType::DIGITAL_IN_PULLDOWN) {
        pinMode(cfg.gpioPin, INPUT_PULLDOWN);
    }
}

// ---------------------------------------------------------
// GPIOへ物理レベルを出力
// ---------------------------------------------------------
void Platform::drv_gpioWrite(const PinConfig& cfg, bool state) {
    if (cfg.mode != PinModeType::DIGITAL_OUT) return;
    digitalWrite(cfg.gpioPin, state ? HIGH : LOW);
}

// ---------------------------------------------------------
// GPIOから物理レベルを読み出す
// ---------------------------------------------------------
bool Platform::drv_gpioRead(const PinConfig& cfg) {
    if (cfg.mode == PinModeType::UNUSED) return false;
    bool raw = digitalRead(cfg.gpioPin);
    // プルアップ設定の場合は入力論理を反転（ボタン押下＝LOW検出＝trueに変換）
    return (cfg.mode == PinModeType::DIGITAL_IN_PULLUP) ? !raw : raw;
}

// ---------------------------------------------------------
// ADC入力値を読み出す
// ---------------------------------------------------------
uint16_t Platform::drv_adcRead(const PinConfig& cfg) {
    if (cfg.mode != PinModeType::ANALOG_IN) return 0;
    return analogRead(cfg.gpioPin); // 12bit解像度 (0〜4095)
}

// ---------------------------------------------------------
// DAC出力値を書き込む
// ---------------------------------------------------------
void Platform::drv_dacWrite(const PinConfig& cfg, uint8_t value) {
    if (cfg.mode != PinModeType::ANALOG_OUT) return;
    dacWrite(cfg.gpioPin, value); // 8bit解像度 (0〜255)
}

// ---------------------------------------------------------
// NeoPixelを物理出力
// ---------------------------------------------------------
// SK6812 / WS2812B Smart RGB LEDを1-Wire制御する。（0~255)
// ---------------------------------------------------------
void Platform::drv_neopixelWrite(uint8_t pin, uint8_t r, uint8_t g, uint8_t b) {
    neopixelWrite(pin, r, g, b);
}

