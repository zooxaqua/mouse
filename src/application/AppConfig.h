// AppConfig.h
// APPレイヤの適合値

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

namespace AppConfig {

    // 2次元MAP定義（int8）
    struct MapPoint_int8 {
        int8_t x;
        int8_t y;
    };

    // 2次元MAP定義（int16）
    struct MapPoint_int16 {
        int16_t x;
        int16_t y;
    };

    // 車体トレッド
    constexpr uint16_t g_track = 160; // [mm]

    // ---------------------------------------------------------
    // MAP設定ルール
    // ---------------------------------------------------------
    // X値は昇順で定義する。
    // X値は重複させない。
    // Xの範囲外は端点のY値を使用する。
    // ---------------------------------------------------------

    // モータ出力特性MAP（加速）
    constexpr MapPoint_int8 g_accelerationMap[] = {
        // X: 目標モータ出力[%], Y: 加速カウンタ[%]
        {   0,  2 },
        {  20,  4 },
        {  40,  6 },
        {  60,  8 },
        {  80, 10 },
        { 100, 20 }
    };

    // モータ出力特性MAP（減速）
    constexpr MapPoint_int8 g_decelerationMap[] = {
        // X: 目標モータ出力[%], Y: 減速カウンタ[%]
        {   0, 15 },
        {  20, 10 },
        {  40,  8 },
        {  60,  5 },
        {  80,  3 },
        { 100,  1 }
    };

    // モータ出力特性MAP（ブレーキ）
    constexpr MapPoint_int8 g_brakeMap[] = {
        // X: 現在モータ出力[%], Y: ブレーキ減速度[%]
        {   0,  50 },
        {  20,  40 },
        {  40,  30 },
        {  60,  25 },
        {  80,  20 },
        { 100,  20 }
    };

    // 旋回半径MAP
    constexpr MapPoint_int16 g_turningRadiusMap[] = {
        // X: 操舵量[%], Y: 旋回半径[mm]
        {   0, 10000 },
        {  20,  5000 },
        {  40,  2500 },
        {  60,  1500 },
        {  80,  1000 },
        { 100,   700 }
    };

    // 旋回半径補正MAP（モータ出力）
    constexpr MapPoint_int8 g_radiusCorrectionMap[] = {
        // X: モータ出力[%], Y: 旋回半径補正率[%]
        {   0,   0 },
        {  20,   0 },
        {  40,   5 },
        {  60,  10 },
        {  80,  25 },
        { 100,  40 }
    };
}

#endif // APP_CONFIG_H