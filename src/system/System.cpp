// System.cpp
// Systemレイヤの実装

#include "System.h"
#include "SystemConfig.h"
#include "../platform/Platform.h"
#include "../platform/PlatformConfig.h"
#include "../rte/Rte.h"
#include "../application/App.h"
#include <Arduino.h>

// System状態の初期値
SystemState System::m_state = SystemState::BOOT;

namespace {
    // System周期管理の基準時刻
    uint32_t s_lastTickMs = 0;

    // 各周期の実行状態
    struct CycleRuntime {
        uint32_t lastRunMs;
    };

    CycleRuntime s_cycleRuntime[
        sizeof(g_systemCycleConfig) /
        sizeof(g_systemCycleConfig[0])
    ];

    // ---------------------------------------------------------
    // 周期処理の実行時間計測を開始
    // ---------------------------------------------------------
    uint32_t startCycleMeasure()
    {
        return micros();
    }

    // ---------------------------------------------------------
    // 周期処理の実行時間と負荷率を計測
    // ---------------------------------------------------------
    void measureCycleLoad(uint32_t startUs, uint32_t periodMs)
    {
        uint32_t endUs = micros();
        uint32_t execUs = endUs - startUs;

        float execMs = execUs / 1000.0f;
        float load =
            (execMs / periodMs) * 100.0f;
/*
        Serial.printf(
            "[SYSTEM] cycle=%lums, exec=%.3fms, load=%.1f%%\n",
            (unsigned long)periodMs,
            execMs,
            load
        );
*/
    }
    
    // コントローラの前回接続状態
    bool s_previousControllerConnected = false;
}

// ---------------------------------------------------------
// System初期化処理
// ---------------------------------------------------------
void System::init() {

    // シリアル通信の開始（速度は 115200 に合わせる）
    Serial.begin(115200);
    delay(1000); // 安定待ち

    m_state = SystemState::INIT;
    
    // 周期管理情報を初期化
    for (size_t i = 0; i < g_systemCycleCount; i++) {
        s_cycleRuntime[i].lastRunMs = 0;
    }

    // 1. Platformの初期化
    Platform::init(g_platformConfig);

    // 2. RTEレイヤの初期化
    Rte::init();

    // 3. APPレイヤの初期化
    App::init();

    // 起動状態をREADYへ移行
    Rte::setLedRed();              //起動時LEDを赤く点灯
    m_state = SystemState::READY;  //システム状態をREADYに遷移

    // 周期処理の基準時刻を設定
    s_lastTickMs = millis();       //msec単位での基準時間取得
}

// ---------------------------------------------------------
// System周期処理
// ---------------------------------------------------------
void System::run() {

    // READY状態の場合はRUN状態へ遷移
    // RUN状態以外の場合は停止処理へ移行
    if (m_state == SystemState::READY) {

        // 初めて run() が呼ばれた瞬間を検知して RUN 状態へ遷移
        m_state = SystemState::RUN;

        // タイマの基準を初期化
        s_lastTickMs = millis();

        // 各周期の実行基準も現在時刻に合わせる
        for (size_t i = 0; i < g_systemCycleCount; i++) {
            s_cycleRuntime[i].lastRunMs = s_lastTickMs;
        }
    }

    if (m_state != SystemState::RUN) {
        shutdown();
        return;
    }

    uint32_t currentMs = millis();

    // 1ms単位で周期処理の実行判定を行う
    if (currentMs - s_lastTickMs >= 1) {
        s_lastTickMs = currentMs;

        // 登録されている全周期を確認
        for (size_t i = 0; i < g_systemCycleCount; i++) {

            const CycleConfig& cycle = g_systemCycleConfig[i];
            CycleRuntime& runtime = s_cycleRuntime[i];

            // この周期の実行タイミングか確認
            if (currentMs - runtime.lastRunMs >= cycle.periodMs) {

                // 次回判定の基準時刻を更新
                runtime.lastRunMs = currentMs;

                // 周期処理時間の計測開始
                uint32_t startUs = startCycleMeasure();

                // Configに登録された関数を順番に実行
                for (size_t j = 0; j < cycle.functions.count; j++) {
                    cycle.functions.functions[j]();
                }

                // 周期処理時間の計測終了
                measureCycleLoad(startUs, cycle.periodMs);

                // シャットダウントリガの確認
                if (Rte::isShutdownRequested()) {
                    shutdown();
                    return;
                }
            }
        }
    }
}

// ---------------------------------------------------------
// System停止処理
// ---------------------------------------------------------
void System::shutdown() {
    if (m_state == SystemState::SHUTDOWN) {
        return;
    }

    // 1. APP層の停止と完了確認
    if (!App::shutdown()) {
        m_state = SystemState::ERROR;
        return;
    }

    // 2. RTE層の停止と完了確認
    if (!Rte::shutdown()) {
        m_state = SystemState::ERROR;
        return;
    }

    // 3. Platform層（ハードウェア）の停止と完了確認
    if (!Platform::shutdown()) {
        m_state = SystemState::ERROR;
        return;
    }

    // 全レイヤの停止完了を確認したら SHUTDOWN 状態へ遷移
    m_state = SystemState::SHUTDOWN;
}

// ---------------------------------------------------------
// コントローラ接続状態に応じてステータスLEDを更新
// ---------------------------------------------------------
void System::led() {
    bool controllerConnected = Rte::isControllerConnected();

    // 接続状態が変化した場合のみLEDを更新
    if (controllerConnected != s_previousControllerConnected) {

        if (controllerConnected) {
            Rte::setLedGreen();   // コントローラ接続 → 緑
        } else {
            Rte::setLedRed();     // コントローラ未接続 → 赤
        }
        // 現在の状態を保存
        s_previousControllerConnected = controllerConnected;
    }
}

// ---------------------------------------------------------
// 周期処理用の空関数
// ---------------------------------------------------------
void System::stub() {

}