// PlatformHttp.h
// HTTP通信を使用したコントローラ入力処理

#ifndef PLATFORMHTTP_H
#define PLATFORMHTTP_H

namespace PlatformHttp {

    void init();                                    // HTTP / WebSocket初期化処理
    void update();                                  // HTTP / WebSocket更新処理
    void updateInfo();                              // デバッグ情報送信処理
    
    bool isControllerConnected();                   // Webコントローラ接続状態を取得
    
    void sendInfo(const char* message);             // デバッグ情報を送信バッファへ追加
    
}

#endif // PLATFORMHTTP_H