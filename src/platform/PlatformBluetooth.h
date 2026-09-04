// PlatformBluetooth.h
// Bluepad32を使用したBluetoothコントローラ入力処理

#ifndef PLATFORMBLUETOOTH_H
#define PLATFORMBLUETOOTH_H

#include "PlatformController.h"

namespace PlatformBluetooth {

    void init();                                    // Bluetooth初期化処理
    void update();                                  // Bluetooth入力更新処理

    bool isControllerConnected();                   // コントローラ接続状態を取得

}

#endif // PLATFORMBLUETOOTH_H