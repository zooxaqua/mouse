// PlatformWifi.h
// Wi-Fi接続処理

#ifndef PLATFORMWIFI_H
#define PLATFORMWIFI_H

namespace PlatformWifi {

    void init();        //初期化処理
    void update();      //更新処理

    bool isConnected(); //接続確認API

}

#endif // PLATFORMWIFI_H