// Rte.h
// RTEレイヤの公開インターフェース

#ifndef RTE_H
#define RTE_H

#include <stdint.h>

enum class AppStick : uint8_t {
    LEFT,
    RIGHT
};

// APPへ返すボタン状態
enum class ButtonState : uint8_t {
    NONE     = 0x00,  // 押されていない
    PRESSED  = 0x01,  // 今回押された
    HOLD     = 0x02,  // 押されている
    RELEASED = 0x03   // 今回離された
};

// APPへ返すスティック状態
struct StickValue {
    int8_t x;       //-100(左) ~ 100(右)
    int8_t y;       //-100(上) ~ 100(下)
};

// APPへ返す走行入力
struct DriveInput {
    int8_t  output;
    int16_t steeringRatio;
    bool brake;
};

namespace Rte {

    void init();                                            // RTEの初期化
    void updateInput();                                     // 入力処理
    void updateOutput();                                    // 出力処理
    bool shutdown();                                        // RTEの停止処理

    bool isShutdownRequested();                             // シャットダウン要求状態を取得
    bool isControllerConnected();                           // コントローラの接続状態を取得
    void setDriveOutput(int8_t motorRightOutput, int8_t motorLeftOutput); // 走行指令を設定
    void setLedGreen();                                     // LEDを緑にする
    void setLedRed();                                       // LEDを赤にする
    void setLedBlue();                                      // LEDを青にする
    void setLedOff();                                       // LEDを消灯する

    ButtonState getButtonState(uint32_t buttonMask);        // ボタン状態を取得
    ButtonState getMiscButtonState(uint8_t buttonMask);     // Miscボタン状態を取得
    ButtonState getDpadState(uint8_t dpadMask);             // D-Pad状態を取得
    StickValue getStickValue(AppStick stick);               // スティック状態を取得
    DriveInput getDriveInput();                             // 走行入力を取得
    
    void sendInfo(const char* message);                     // デバッグ情報を送信バッファへ追加
} // namespace Rte

#endif // RTE_H