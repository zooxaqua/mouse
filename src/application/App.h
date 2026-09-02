// App.h
// Application レイヤ インターフェース定義

#ifndef APP_H
#define APP_H

namespace App {

    void init();       // アプリケーション初期化
    void run();        //アプリケーション周期処理
    bool shutdown();   //アプリケーション停止処理

} // namespace App

#endif // APP_H