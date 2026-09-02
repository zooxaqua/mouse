/**
 * @file main.cpp
 * @brief エントリーポイント
 * @details システムRuntimeへの接続のみを担う。アプリケーションやハード制御のコードは直接書かない。
 */

#include <Arduino.h>
#include "system/System.h"

/**
 * @brief MCU起動時の初回呼び出し
 */
void setup() {
    System::init();
}

/**
 * @brief MCUメインループ呼び出し
 */
void loop() {
    System::run();
}