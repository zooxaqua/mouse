// SystemConfig.h
// Systemレイヤの設定値定義

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <stdint.h>
#include <stddef.h>

// Systemは例外的に全レイヤのヘッダを参照
#include "../platform/Platform.h"
#include "../rte/Rte.h"
#include "../application/App.h"

// 周期処理関数は引数なし・戻り値なしで統一
using CycleFunction = void (*)();

// 1つの周期で実行する関数群
struct CycleFunctions {
    const CycleFunction* functions;
    size_t count;
};

// 配列から周期処理関数群を生成
template <size_t N> constexpr CycleFunctions makeCycleFunctions(const CycleFunction (&functions)[N])
{
    return {functions, N};
}

// 1つの周期と、その周期で実行する関数群
struct CycleConfig {
    uint32_t periodMs;
    CycleFunctions functions;
};

// A周期で実行する関数群
// 配列の上から順番に実行される
constexpr CycleFunction g_cycleA_Functions[] = {
    Platform::updateInput,
    Rte::updateInput,
    App::run,
    Rte::updateOutput,
    Platform::updateOutput
};

// B周期で実行する関数群
// 配列の上から順番に実行される
constexpr CycleFunction g_cycleB_Functions[] = {
    System::led,
    Platform::updateInfo
};

// システム周期設定
// 周期と実行する関数・順序をここで定義する
constexpr CycleConfig g_systemCycleConfig[] = {

    // cycleA周期
    {
        .periodMs = 100,
        .functions = makeCycleFunctions(g_cycleA_Functions)
    },

    // cycleB周期
    {
        .periodMs = 1000,
        .functions = makeCycleFunctions(g_cycleB_Functions)
    }
};

// システム周期数を自動取得
constexpr size_t g_systemCycleCount =
    sizeof(g_systemCycleConfig) /
    sizeof(g_systemCycleConfig[0]);

#endif // SYSTEM_CONFIG_H