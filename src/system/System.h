// System.h
// Systemレイヤの公開インターフェース

#ifndef SYSTEM_H
#define SYSTEM_H

// システム全体のライフサイクル状態
enum class SystemState {
    BOOT,       // 電源投入・ブート状態
    INIT,       // 各レイヤの初期化中
    READY,      // 全レイヤの初期化完了
    RUN,        // 周期処理を実行中
    SHUTDOWN,   // システム停止・安全停止
    ERROR       // システム異常
};

class System {
    public:
        
        static void init();                               // System初期化処理
        static void run();                                // System周期処理
        static void shutdown();                           // System停止処理
        static SystemState getState() { return m_state; } // 現在のシステム状態を取得
        static void led();                                // コントローラ接続状態に応じてステータスLEDを更新
        static void stub();                               //動作確認用の空処理

    private:
        static SystemState m_state;                       // 現在のシステム状態
};


#endif // SYSTEM_H