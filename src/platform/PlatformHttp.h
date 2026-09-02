#ifndef PLATFORMHTTP_H
#define PLATFORMHTTP_H

#include "PlatformControllerInput.h"

namespace PlatformHttp {

    void init();                                    // HTTP / WebSocket初期化処理
    void update();                                  // HTTP / WebSocket更新処理
    void updateInfo();                              // デバッグ情報送信処理
    
    bool isControllerConnected();                   // Webコントローラ接続状態を取得
    
    PlatformControllerInput getControllerInput();   // Webコントローラ入力を取得
    void sendInfo(const char* message);             // デバッグ情報を送信バッファへ追加
    
}

#endif // PLATFORMHTTP_H