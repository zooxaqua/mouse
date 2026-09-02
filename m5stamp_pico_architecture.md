**# M5Stamp-Pico 車両制御ソフトウェア アーキテクチャ（現行版）**

\> 本書は、現在の M5Stamp-Pico

\> 車両制御ソフトウェアの最新実装を基準として、アーキテクチャ、レイヤ責務、データフロー、周期実行、モーター制御、Controller

\> Input、通信、Debug

\> Console、Config、実装状況を整理した現行版ドキュメントである。

\>

\> 過去の設計経緯や旧案は扱わず、現在の実装・設計方針を記載する。

\------------------------------------------------------------------------

**# 1. プロジェクト概要**

**## 1.1 目的**

M5Stamp-Pico Mate を使用し、Bluetooth コントローラまたは Web Controller

からの入力に応じて左右独立2輪の DC モーターを制御する。

主な機能：

\-   Bluetooth コントローラ入力

\-   HTTP/WebSocket による Web Controller 入力

\-   Bluetooth 優先の Controller Input 切替

\-   左右2輪の差動駆動

\-   DRV8835 によるモーター制御

\-   加速・減速・ブレーキ制御

\-   操舵量からの旋回半径計算

\-   速度に応じた旋回半径補正

\-   RGB LED による接続状態表示

\-   Browser への Debug 情報送信

\-   System によるライフサイクル管理

\-   System Config による周期処理管理

**## 1.2 ハードウェア**

\-   MCU：ESP32

\-   ボード：M5Stamp-Pico Mate

\-   Framework：Arduino

\-   開発環境：VS Code + PlatformIO

\-   Bluetooth：Bluepad32

\-   Controller：Nintendo Switch Pro Controller

\-   Motor Driver：DRV8835

\-   車両形式：左右独立2輪駆動（Differential Drive）

\------------------------------------------------------------------------

# 1.3 Flash パーティション構成

M5Stamp-Pico Mate の ESP32 内蔵 4MB Flash は、OTA（Over-The-Air）更新に対応するため、
Application 領域を APP0 / APP1 の2領域に分割する。

また、設定値を保持する NVS、OTA の起動情報を保持する OTA Data、
Web Controller のファイルを保存する LittleFS を配置する。

## 1.3.1 Flash メモリマップ

```text
0x000000
    │
    │ 予約領域
    │
0x009000 ┌─────────────┐
         │ NVS 20 KiB  │
0x00E000 ├─────────────┤
         │ OTA Data    │
         │  8 KiB      │
0x010000 ├─────────────┤
         │             │
         │    APP0     │
         │   1.75 MiB  │
         │             │
0x1D0000 ├─────────────┤
         │             │
         │    APP1     │
         │   1.75 MiB  │
         │             │
0x390000 ├─────────────┤
         │             │
         │  LittleFS   │
         │   448 KiB   │
         │             │
0x400000 └─────────────┘
```

4MB Flash 全体のアドレス範囲は `0x000000 ～ 0x3FFFFF` である。

## 1.3.2 パーティション定義

現在の `partitions.csv` は以下の構成とする。

```csv
# Name,Type,SubType,Offset,Size,Flags
nvs,data,nvs,0x9000,0x5000,
otadata,data,ota,0xE000,0x2000,
app0,app,ota_0,0x10000,0x1C0000,
app1,app,ota_1,0x1D0000,0x1C0000,
littlefs,data,spiffs,0x390000,0x70000,
```

| Name | Type | SubType | Offset | Size | 容量 |
|---|---|---|---|---|---:|
| `nvs` | data | nvs | `0x9000` | `0x5000` | 20 KiB |
| `otadata` | data | ota | `0xE000` | `0x2000` | 8 KiB |
| `app0` | app | ota_0 | `0x10000` | `0x1C0000` | 1.75 MiB |
| `app1` | app | ota_1 | `0x1D0000` | `0x1C0000` | 1.75 MiB |
| `littlefs` | data | spiffs | `0x390000` | `0x70000` | 448 KiB |

### 各領域の役割

**NVS（20 KiB）**

電源を切っても保持する設定値などを保存する。

将来的には、以下のような永続設定を保存することを想定する。

- Wi-Fi SSID
- Wi-Fi Password
- Hostname
- その他の永続設定

**OTA Data（8 KiB）**

OTA に使用する起動情報を保持する領域。

ESP32 が APP0 / APP1 のどちらを次回起動するかを管理するために使用する。

**APP0 / APP1（各1.75 MiB）**

Firmware / Application を格納する領域。

OTA 更新では、現在実行中の Application とは別の領域へ新しい Firmware を書き込み、
書き込み完了後に次回起動する Application 領域を切り替える。

APP0 / APP1 は同一サイズとし、交互に OTA の書き込み先として使用する。

**LittleFS（448 KiB）**

Application とは別に、Web Controller の HTML / CSS / JavaScript などを保存するファイルシステム領域。

Firmware と Web UI のファイルを Flash 上で分離することで、それぞれを独立して管理できる構成とする。

## 1.3.3 OTA 更新の考え方

OTA は、現在実行している Firmware を直接上書きするのではなく、
空いているもう一方の Application 領域へ新しい Firmware を書き込む構成とする。

```text
現在起動中
APP0
  │
  │ OTA書き込み
  ↓
APP1
  │
  │ 書き込み完了
  ↓
OTA Data 更新
  │
  │ 次回起動先を APP1 に変更
  ↓
再起動
  │
  ↓
APP1 起動
```

次回 OTA 更新では、APP1 から APP0 へ同様に更新する。

この構成により、現在動作している Firmware を残したまま、
別の Application 領域へ新しい Firmware を書き込める。

OTA は今回のアーキテクチャで採用する構成の一つであり、
今後の実装・学習対象とする。


------------------------------------------------------------------------

**# 2. ソフトウェア全体構成**

\`\`\` text

+--------------------------------------------------+

\|                    System                        |

\|  Startup / LifeCycle / Scheduler / State        |

+--------------------------+-----------------------+

                           |

                           v

+--------------------------------------------------+

\|                  Application                     |

\|  車両走行ロジック / 走行特性 / 適合値            |

+--------------------------+-----------------------+

                           |

                           v

+--------------------------------------------------+

\|                     RTE                          |

\|  論理IF / Controller抽象化 / HW制御変換          |

+--------------------------+-----------------------+

                           |

                           v

+--------------------------------------------------+

\|                   Platform                       |

\| GPIO / PWM / ADC / DAC / Bluetooth / Wi-Fi / HTTP|

+--------------------------+-----------------------+

                           |

                           v

                    MCU / Hardware

\`\`\`

**## 2.1 レイヤの基本思想**

\`\`\` text

System

    \= システム全体を動かす

Application

    \= 車両としてどう動くかを決める

RTE

    \= 論理要求と Platform の制御方式を変換する

Platform

    \= MCU・通信・周辺機能を抽象化する

\`\`\`

原則：

\-   Application は GPIO、PWM Channel、LEDC、DRV8835 を直接扱わない

\-   Application は Bluetooth / HTTP / WebSocket を知らない

\-   RTE は Application と Platform の IF を接続する

\-   RTE は DRV8835 の IN/IN 制御方式を扱う

\-   Platform は車両概念を持たない

\-   各レイヤ固有の適合値は Config に分離する

\------------------------------------------------------------------------

**# 3. ディレクトリ構成**

\`\`\` text

M5StampControl/

|

+-- platformio.ini

|

+-- src/

    |

    +-- main.cpp

    |

    +-- system/

    \|   +-- System.cpp

    \|   +-- System.h

    \|   +-- SystemConfig.h

    |

    +-- application/

    \|   +-- App.cpp

    \|   +-- App.h

    \|   +-- AppConfig.h

    |

    +-- rte/

    \|   +-- Rte.cpp

    \|   +-- Rte.h

    |

    +-- platform/

        +-- Platform.cpp

        +-- Platform.h

        +-- PlatformConfig.cpp

        +-- PlatformConfig.h

        +-- PlatformBluetooth.cpp

        +-- PlatformBluetooth.h

        +-- PlatformWifi.cpp

        +-- PlatformWifi.h

        +-- PlatformHttp.cpp

        +-- PlatformHttp.h

\`\`\`

Platform は機能単位で内部モジュールを分割する。

\`\`\` text

Platform

 ├─ PlatformBluetooth

 ├─ PlatformWifi

 └─ PlatformHttp

\`\`\`

\------------------------------------------------------------------------

**# 4. System Layer**

**## 4.1 責務**

\-   Startup

\-   初期化順序

\-   System State 管理

\-   Scheduler

\-   周期処理

\-   Shutdown

\-   Error 遷移

\-   LED 状態処理

Application の走行判断は System では行わない。

**## 4.2 SystemState**

\`\`\` cpp

enum class SystemState {

    BOOT,

    INIT,

    READY,

    RUN,

    SHUTDOWN,

    ERROR

};

\`\`\`

**## 4.3 Startup**

\`\`\` text

System::init()

    ↓

Serial.begin()

    ↓

state = INIT

    ↓

CycleRuntime 初期化

    ↓

Platform::init(g\_platformConfig)

    ↓

Rte::init()

    ↓

App::init()

    ↓

Rte::setLedRed()

    ↓

state = READY

\`\`\`

\------------------------------------------------------------------------

**# 5. System Scheduler**

**## 5.1 周期構成**

周期処理は System が一元管理する。

\`\`\` cpp

using CycleFunction = void (\*)();

struct CycleFunctions {

    const CycleFunction\* functions;

    size\_t count;

};

struct CycleConfig {

    uint32\_t periodMs;

    CycleFunctions functions;

};

\`\`\`

**## 5.2 現在の周期**

**### 100ms Cycle**

\`\`\` text

Platform::updateInput

        ↓

Rte::updateInput

        ↓

App::run

        ↓

Rte::updateOutput

        ↓

Platform::updateOutput

\`\`\`

**### 1000ms Cycle**

\`\`\` text

System::led

        ↓

PlatformHttp::update

\`\`\`

Debug 情報送信は走行周期とは分離し、1000ms 周期で処理する。

**## 5.3 Configによる実行順序**

\`\`\` cpp

constexpr CycleFunction g\_cycleA\_Functions[] = {

    Platform::updateInput,

    Rte::updateInput,

    App::run,

    Rte::updateOutput,

    Platform::updateOutput

};

\`\`\`

Scheduler 本体を変更せず、Config で周期と実行関数を変更できる。

\------------------------------------------------------------------------

**# 6. Application Layer**

Application は車両としての走行制御を担当する。

主な責務：

\-   Controller 入力の車両動作への解釈

\-   ベースモータ出力

\-   加速・減速・ブレーキ特性

\-   旋回半径

\-   速度による旋回半径補正

\-   左右モータ出力決定

Application はハードウェア詳細を知らない。

\------------------------------------------------------------------------

**# 7. Application の入力**

\`\`\` cpp

struct DriveInput {

    int8\_t  output;

    int16\_t steeringRatio;

    bool    brake;

};

\`\`\`

  項目              意味               範囲

  \----------------- ------------------ --------------

  \`output\`          車体基準出力要求   -100～+100

  \`steeringRatio\`   操舵操作量         -100～+100

  \`brake\`           ブレーキ要求       true / false

現在の入力：

\`\`\` text

右スティックY → output

左スティックX → steeringRatio

A             → output = +100

B             → brake = true

\`\`\`

\------------------------------------------------------------------------

**# 8. Application 走行処理**

\`\`\` text

Rte::getDriveInput()

        ↓

calculateMotorOutput()

        ↓

baseMotorOutput

        ↓

calculateTurningRadius()

        ↓

turningRadius

        ↓

calculateDriveOutput()

        ↓

DriveOutput

        ↓

Rte::setDriveCommand()

\`\`\`

\------------------------------------------------------------------------

**# 9. Motor Output 制御**

**## 9.1 加速**

前周期の出力から目標出力へ徐々に追従する。

\`\`\` text

currentOutput

      ↓

acceleration MAP

      ↓

加速ステップ

      ↓

targetOutput

\`\`\`

**## 9.2 減速**

目標値へ向けて減速 MAP を使用する。

**## 9.3 ブレーキ**

\`DriveInput.brake == true\` の場合は通常の減速 MAP ではなく専用

\`g\_brakeMap\` を使用する。

ブレーキ特性と DRV8835 の電気的 BRAKE 動作は別概念として扱う。

\------------------------------------------------------------------------

**# 10. Application Config**

車体トレッド：

\`\`\` cpp

constexpr uint16\_t g\_track = 160;

\`\`\`

加速 MAP：

\`\`\` text

0%   -> 2

20%  -> 4

40%  -> 6

60%  -> 8

80%  -> 10

100% -> 20

\`\`\`

減速 MAP：

\`\`\` text

0%   -> 15

20%  -> 10

40%  -> 8

60%  -> 5

80% -> 3

100% -> 1

\`\`\`

ブレーキ MAP：

\`\`\` text

0%   -> 50

20%  -> 40

40%  -> 30

60%  -> 25

80%  -> 20

100% -> 20

\`\`\`

\------------------------------------------------------------------------

**# 11. MAP補間**

MAP は線形補間を使用する。

\`\`\` cpp

template \<size\_t N>

int8\_t interpolateMap(

    const AppConfig::MapPoint\_int8 (&map)[N],

    int8\_t x);

\`\`\`

\`\`\` cpp

template \<size\_t N>

uint16\_t interpolateMap(

    const AppConfig::MapPoint\_int16 (&map)[N],

    int16\_t x);

\`\`\`

テンプレートにより配列要素数を自動取得する。

\------------------------------------------------------------------------

**# 12. 旋回半径制御**

\`\`\` text

steeringRatio

      ↓

turningRadiusMap

      ↓

baseTurningRadius

      ↓

motorOutput

      ↓

radiusCorrectionMap

      ↓

final turningRadius

\`\`\`

turningRadius MAP：

\`\`\` text

0%        10000mm

20%        5000mm

40%        2500mm

60%        1500mm

80%        1000mm

100%        700mm

\`\`\`

操舵量0%は \`turningRadius == 0\` として直進扱いする。

速度による補正：

\`\`\` text

motor output   correction

0%              0%

20%             0%

40%             5%

60%            10%

80%            25%

100%           40%

\`\`\`

\`\`\` cpp

finalRadius = baseRadius \* (100 + correctionRatio) / 100;

\`\`\`

\------------------------------------------------------------------------

**# 13. calculateDriveOutput()**

差動二輪では、

\`\`\` text

outerRadius = R + T/2

innerRadius = R - T/2

\`\`\`

内外輪比：

\`\`\` text

inner / outer

\= (R - T/2) / (R + T/2)

\`\`\`

直進：

\`\`\` text

Right = baseMotorOutput

Left  = baseMotorOutput

\`\`\`

旋回：

\`\`\` text

steeringRatio > 0

    Right = outer

    Left  = inner

steeringRatio < 0

    Right = inner

    Left  = outer

\`\`\`

\`turningRadius < track / 2\`

の領域では内側半径が負になるため、内側モーターを逆回転させず、最小半径では内側出力を0%とする方針。

\------------------------------------------------------------------------

**# 14. DriveOutput**

\`\`\` cpp

struct DriveOutput {

    int8\_t motorRightOutput;

    int8\_t motorLeftOutput;

};

\`\`\`

Application の計算結果として使用し、RTE の Drive Command IF へ渡す。

\------------------------------------------------------------------------

**# 15. RTE Layer**

RTE は Application と Platform の間の変換を担当する。

主な役割：

\-   Platform Controller Input の取得

\-   Controller 情報の論理化

\-   \`DriveInput\` 生成

\-   Drive Command の受け取り

\-   DRV8835 IN/IN 変換

\-   LED IF

\-   Controller Input Source 選択

\-   Debug 情報送信 API

\-   Shutdown Request

\------------------------------------------------------------------------

**# 16. Controller Input**

Platform から RTE へは \`PlatformControllerInput\` を渡す。

\`\`\` text

Left Stick X/Y

Right Stick X/Y

Buttons

Buttons Pressed

Buttons Released

D-Pad

D-Pad Pressed

D-Pad Released

Misc Buttons

Misc Buttons Pressed

Misc Buttons Released

\`\`\`

Bluetooth と HTTP/WebSocket の入力は、どちらも同じ

\`PlatformControllerInput\` に変換する。

\------------------------------------------------------------------------

**# 17. RTE 論理入力IF**

\`\`\` cpp

ButtonState getButtonState(AppButton button);

ButtonState getMiscButtonState(AppMiscButton button);

ButtonState getDpadState(AppDpad dpad);

StickValue getStickValue(AppStick stick);

DriveInput getDriveInput();

\`\`\`

\`\`\` cpp

enum class ButtonState : uint8\_t {

    NONE,

    PRESSED,

    HOLD,

    RELEASED

};

\`\`\`

Application は Controller の物理ビット値を直接扱わない。

\------------------------------------------------------------------------

**# 18. RTE Controller Input Source**

入力ソースの優先順位：

\`\`\` text

Bluetooth > HTTP > NONE

\`\`\`

\`\`\` cpp

if (PlatformBluetooth::isControllerConnected()) {

    s\_controllerInput =

        PlatformBluetooth::getControllerInput();

}

else if (PlatformHttp::isControllerConnected()) {

    s\_controllerInput =

        PlatformHttp::getControllerInput();

}

else {

    s\_controllerInput = {};

}

\`\`\`

この選択は RTE の責務である。

\------------------------------------------------------------------------

**# 19. RTE Output / VehicleCommand**

\`\`\` cpp

void setDriveCommand(

    VehicleCommand command,

    int8\_t motorRightOutput,

    int8\_t motorLeftOutput);

\`\`\`

\`\`\` cpp

enum class VehicleCommand : uint8\_t {

    STOP,

    FORWARD,

    REVERSE,

    BRAKE

};

\`\`\`

左右出力は -100～+100 に制限して保持する。

\------------------------------------------------------------------------

**# 20. DRV8835 IN/IN変換**

\`\`\` text

Motor 1

    AIN1

    AIN2

Motor 2

    BIN1

    BIN2

\`\`\`

正方向：

\`\`\` text

IN1 = Duty

IN2 = 0

\`\`\`

逆方向：

\`\`\` text

IN1 = 0

IN2 = Duty

\`\`\`

STOP：

\`\`\` text

IN1 = 0

IN2 = 0

\`\`\`

電気的 BRAKE は専用仕様を確認した上で実装する。

\------------------------------------------------------------------------

**# 21. Platform Layer**

Platform は ESP32 / Arduino / Bluepad32 / Wi-Fi / HTTP / WebSocket 等の

MCU・通信方式固有機能を吸収する。

対象：

\-   GPIO

\-   PWM / LEDC

\-   ADC

\-   DAC

\-   PCNT

\-   NeoPixel / SK6812

\-   Bluetooth / Bluepad32

\-   Wi-Fi

\-   mDNS

\-   HTTP Server

\-   WebSocket Server

\-   Controller 接続状態

\-   Debug 情報送信

\------------------------------------------------------------------------

**# 22. Platform 内部モジュール**

\`\`\` text

                         Platform

                            │

             ┌──────────────┼──────────────┐

             │              │              │

             ↓              ↓              ↓

    PlatformBluetooth  PlatformWifi  PlatformHttp

             │              │              │

             ↓              ↓              ↓

         Bluepad32        Wi-Fi       HTTP/WebSocket

\`\`\`

\`Platform.cpp\` は Platform 内部モジュールを統括する。

\------------------------------------------------------------------------

**# 23. PlatformBluetooth**

\`PlatformBluetooth\` は Bluepad32 固有処理を担当する。

\`\`\` text

Bluepad32

    ↓

PlatformBluetooth

    ↓

PlatformControllerInput

    ↓

RTE

\`\`\`

担当：

\-   \`BP32.update()\`

\-   接続 / 切断

\-   Controller 入力取得

\-   Stick 正規化

\-   Button / D-Pad / Misc Button

\-   Pressed / Released 生成

\------------------------------------------------------------------------

**# 24. PlatformWifi**

Wi-Fi 接続と mDNS を担当する。

現在は STA 接続を中心とする。

将来的な STA / AP 両対応を想定する。

\`\`\` text

PlatformWifi

 ├── STA

 │    └── Existing Wi-Fi network

 │

 └── AP

      └── M5Stamp-Pico Access Point

\`\`\`

SSID / Password は PlatformConfig に持たせない。

現在は \`const\` による仮設定を使用する。

mDNS は PlatformWifi の責務。

\------------------------------------------------------------------------

**# 25. PlatformHttp**

PlatformHttp は以下を担当する。

\-   LittleFS

\-   HTTP Server

\-   WebSocket Server

\-   Web Controller Input

\-   WebSocket 接続状態

\-   WebSocket Timeout

\-   Debug 情報送信

\-   Browser との情報通信

Controller Input：

\`\`\` text

Browser

    ↓

WebSocket

    ↓

PlatformHttp

    ↓

PlatformControllerInput

    ↓

RTE

\`\`\`

Debug 情報：

\`\`\` text

RTE

    ↓

PlatformHttp

    ↓

WebSocket

    ↓

Browser

\`\`\`

\------------------------------------------------------------------------

**# 26. HTTP Controller Input**

現在実装する入力：

\`\`\` text

A / B / X / Y

D-Pad

    UP / DOWN / LEFT / RIGHT

Left Stick

Right Stick

\`\`\`

現時点では以下を実装しない：

\`\`\` text

L1 / R1

L2 / R2

L3 / R3

HOME / MINUS / PLUS / PICT

\`\`\`

必要なボタンのみを実装対象とする。

\------------------------------------------------------------------------

**# 27. HTTP Button / D-Pad 状態モデル**

現在状態とイベント状態を分離する。

\`\`\` text

pressed

    ↓

current state = ON

pressed event = ON

次の周期

    ↓

current state = ON

pressed event = OFF

released

    ↓

current state = OFF

released event = ON

\`\`\`

RTE からは、

\`\`\` text

PRESSED → HOLD → NONE

\`\`\`

として扱う。

\`buttons\` / \`dpad\` は現在状態を保持する。

\`buttonsPressed\` / \`buttonsReleased\` / \`dpadPressed\` / \`dpadReleased\`

はイベントとして1周期だけ有効になる。

\------------------------------------------------------------------------

**# 28. HTTP Stick 状態モデル**

WebSocket で最後に受信した値を保持する。

\`\`\` text

stick\:left:80:-20

        ↓

leftStick = (80, -20)

        ↓

次のメッセージまで保持

\`\`\`

ブラウザが \`0:0\` を送信するとニュートラルへ戻る。

\------------------------------------------------------------------------

**# 29. WebSocket Timeout / Heartbeat**

Heartbeat は接続維持確認用であり、Controller Input ではない。

通常ログには表示せず、Controller Input 処理にも渡さない。

一定時間受信しなかった場合：

\`\`\` text

WebSocket Timeout

        ↓

connected = false

        ↓

Controller Input = {}

\`\`\`

切断時も安全側へ戻す。

\------------------------------------------------------------------------

**# 30. Platform PWM Interface**

\`\`\` cpp

enum class PwmOutput : uint8\_t {

    MOTOR1\_IN1,

    MOTOR1\_IN2,

    MOTOR2\_IN1,

    MOTOR2\_IN2

};

\`\`\`

上位レイヤは GPIO 番号を直接指定しない。

\`\`\` cpp

Platform::setPwmDuty(

    PwmOutput::MOTOR1\_IN1,

    dutyPercent);

\`\`\`

PlatformConfig により実 GPIO / PWM Channel へ変換する。

\------------------------------------------------------------------------

**# 31. Platform Output Buffer**

\`\`\` cpp

struct OutputBuffer {

    uint8\_t m1In1Duty;

    uint8\_t m1In2Duty;

    uint8\_t m2In1Duty;

    uint8\_t m2In2Duty;

    uint8\_t dacValue;

    RgbColor ledColor;

};

\`\`\`

\`Platform::updateOutput()\` で物理ハードウェアへ反映する。

\------------------------------------------------------------------------

**# 32. Platform Input Buffer**

Platform はハードウェア入力を内部バッファへ保持する。

\`\`\` cpp

struct InputBuffer {

    bool buttonPressed;

    uint16\_t m1CurrentRaw;

    uint16\_t m2CurrentRaw;

    int32\_t enc1Count;

    int32\_t enc2Count;

};

\`\`\`

Current Sense / Encoder は準備段階。

\------------------------------------------------------------------------

**# 33. PlatformConfig**

PlatformConfig は Platform が所有する物理割り当て設定。

主な項目：

\`\`\` text

motor1In1

motor1In2

motor2In1

motor2In2

currentSense1

currentSense2

encoder1A

encoder1B

encoder2A

encoder2B

dacOut1

dacOut2

userButton

gpioInAux1

gpioOutAux1

statusLed

i2cBus

uartComm

\`\`\`

\------------------------------------------------------------------------

**# 34. 現在の物理ピン割り当て**

\`\`\` text

M5Stamp-Pico Mate     DRV8835

G26  ---------------> AIN1

G18  ---------------> AIN2

G21  ---------------> BIN1

G22  ---------------> BIN2

\`\`\`

\`\`\` text

MOTOR1\_IN1 : GPIO26 / CH0 / 5kHz / 8bit

MOTOR1\_IN2 : GPIO18 / CH1 / 5kHz / 8bit

MOTOR2\_IN1 : GPIO21 / CH2 / 5kHz / 8bit

MOTOR2\_IN2 : GPIO22 / CH3 / 5kHz / 8bit

\`\`\`

オンボード：

\`\`\` text

G27 : SK6812 RGB LED

G39 : User Button / Active LOW

\`\`\`

その他：

\`\`\` text

G34 : Encoder1 A

G35 : Encoder2 A

G25 : DAC1

\`\`\`

G21 はモーター出力に使用しているため I2C SDA として同時使用しない。

\------------------------------------------------------------------------

**# 35. PinConfig**

\`\`\` cpp

struct PinConfig {

    uint8\_t gpioPin;

    PinModeType mode;

    uint8\_t pwmChannel;

    uint32\_t pwmFreq;

    uint8\_t pwmResBits;

};

\`\`\`

\`\`\` cpp

enum class PinModeType {

    UNUSED,

    DIGITAL\_IN,

    DIGITAL\_IN\_PULLUP,

    DIGITAL\_IN\_PULLDOWN,

    DIGITAL\_OUT,

    ANALOG\_IN,

    ANALOG\_OUT,

    PWM\_OUT,

    PULSE\_COUNTER,

    NEOPIXEL\_OUT

};

\`\`\`

\------------------------------------------------------------------------

**# 36. Config 所有権**

\`\`\` text

System       → SystemConfig

Application  → AppConfig

RTE          → RteConfig（必要に応じて将来分離）

Platform     → PlatformConfig

\`\`\`

原則：

1\.  Config は原則 \`const\`

2\.  コード本体から分離

3\.  Config の所有者だけが直接参照

4\.  他レイヤの Config を直接参照しない

5\.  レイヤ間の情報交換は Interface を使用

6\.  Config を共有グローバルデータ置き場にしない

\------------------------------------------------------------------------

**# 37. LED制御**

System は Controller 接続状態を監視する。

\`\`\` text

connected

    ↓

Green

disconnected

    ↓

Red

\`\`\`

LED の物理制御は Platform が担当する。

\------------------------------------------------------------------------

**# 38. Shutdown**

\`\`\` text

System detects shutdown request

        ↓

App::shutdown()

        ↓

Rte::shutdown()

        ↓

Platform::shutdown()

        ↓

SystemState = SHUTDOWN

\`\`\`

安全状態：

\`\`\` text

M1 IN1 = 0

M1 IN2 = 0

M2 IN1 = 0

M2 IN2 = 0

\`\`\`

LED / DAC も安全状態へ移行する。

\------------------------------------------------------------------------

**# 39. Input Data Flow**

Bluetooth：

\`\`\` text

Bluetooth Controller

        ↓

Bluepad32

        ↓

PlatformBluetooth

        ↓

PlatformControllerInput

        ↓

RTE

        ↓

DriveInput / ButtonState / StickValue

        ↓

Application

\`\`\`

HTTP/WebSocket：

\`\`\` text

Browser / iPhone

        ↓

WebSocket

        ↓

PlatformHttp

        ↓

PlatformControllerInput

        ↓

RTE

        ↓

DriveInput / ButtonState / StickValue

        ↓

Application

\`\`\`

Application は通信方式を知らない。

\------------------------------------------------------------------------

**# 40. Output Data Flow**

\`\`\` text

Application

    ↓

VehicleCommand + Left/Right Output

    ↓

RTE

    ↓

DRV8835 IN/IN

    ↓

PwmOutput

    ↓

Platform

    ↓

GPIO / PWM

    ↓

DRV8835

    ↓

DC Motors

\`\`\`

Application → Platform の直接呼び出しは行わない。

\------------------------------------------------------------------------

**# 41. Runtime Data Flow**

\`\`\` text

100ms Cycle

Platform::updateInput

        ↓

Rte::updateInput

        ↓

App::run

        ↓

Rte::updateOutput

        ↓

Platform::updateOutput

\`\`\`

\`\`\` text

1000ms Cycle

System::led

        ↓

PlatformHttp::update

\`\`\`

\------------------------------------------------------------------------

**# 42. Debug Console**

**## 42.1 位置づけ**

DebugLogger は Platform に属する。

ただし機能としては「デバッグ専用」ではなく、

\> Platform から外部へ情報を送信するための情報送信用機能

として扱う。

Controller Input の受信機能に対する情報送信機能として構成する。

**## 42.2 API**

上位レイヤは単純な API を呼ぶ。

\`\`\` cpp

Rte::sendInfo("[APP] Debug Console test");

\`\`\`

Application / System は WebSocket を直接扱わない。

**## 42.3 送信経路**

\`\`\` text

Application / System

        ↓

Rte::sendInfo()

        ↓

Platform

        ↓

PlatformHttp

        ↓

WebSocket

        ↓

Browser Debug Console

\`\`\`

**## 42.4 Buffer**

M5側 Info Buffer は現在32件とする。

\`\`\` text

最大32件

\`\`\`

満杯の場合、それ以上の情報はバッファへ追加しない。

送信済み情報はバッファから削除するため、同じ情報を次回周期に再送しない。

同じ内容を \`sendInfo()\` が再度呼べば、新しい情報として送信対象になる。

**## 42.5 送信周期**

PlatformHttp は情報を蓄積し、1000ms 周期の \`Platform::updateInfo()\`

でまとめて送信する。

走行用100ms周期とは分離する。

\------------------------------------------------------------------------

**# 43. Browser Debug Console**

Browser 側には Debug Console 表示領域を持つ。

\`\`\` html

\<section class="debug-console">

    \<h2>Debug Console\</h2>

    \<div

        id="debugConsole"

        class="debug-console-log">

    \</div>

\</section>

\`\`\`

JavaScript の WebSocket 受信処理から Console UI へ表示する。

\`\`\` text

WebSocket

    ↓

JavaScript

    ↓

Debug Console DOM

\`\`\`

Browser 側は最新約50件のみ保持する。

\`\`\` text

新しいログ

    ↓

追加

    ↓

50件超過

    ↓

古いログを削除

\`\`\`

長時間接続してもログが無制限に増えない。

\------------------------------------------------------------------------

**# 44. Debug Console の基本方針**

\-   上位レイヤは \`Rte::sendInfo()\` だけを呼ぶ

\-   WebSocket の詳細を上位レイヤへ公開しない

\-   PlatformHttp が情報を蓄積する

\-   1000ms 周期でまとめて送信する

\-   送信済み情報はバッファから削除する

\-   M5側は最大32件

\-   Browser側は最新50件

\-   Heartbeat は通常ログへ表示しない

\-   Controller Input の受信ログと Debug 情報を分離する

\-   同一状態を無制限に周期出力しない

\------------------------------------------------------------------------

**# 45. メモリ使用方針**

ESP32：

```text
RAM   : 320KB
Flash : 4MB
```

Flash は OTA 用の APP0 / APP1、OTA Data、NVS、
Web Controller 用の LittleFS に分割して使用する。

RAM は Wi-Fi / Bluetooth / WebSocket / FreeRTOS / String
等も使用するため、今後の機能追加では RAM 使用量を確認する。

Debug Console の M5側 Info Buffer は現時点で32件のままとする。


**# 46. 将来拡張**

**## 46.1 Controller Pairing**

NVS 等への Controller 接続履歴保存を将来検討する。

**## 46.2 Wi-Fi設定**

将来的に Browser / Bluetooth 等から SSID / Password

を設定できる構成を検討する。

**## 46.3 AP Mode**

PlatformWifi に AP モードを追加できる構造を維持する。

**## 46.4 Camera / 大容量データ**

将来カメラ等の大容量データを扱う可能性がある。

ただし現時点では対象外とし、Debug / Controller

通信とは別のデータ経路として設計する。

**## 46.5 Current Sense / Encoder**

Current Sense と Encoder は準備段階。

\------------------------------------------------------------------------

**# 47. 現在実装済みの主な機能**

  機能                                   状態

  \-------------------------------------- --------------------

  System lifecycle                       実装済み

  Scheduler                              実装済み

  100ms / 1000ms cycle                   実装済み

  Cycle function configuration           実装済み

  Bluetooth / Bluepad32                  実装済み

  HTTP / WebSocket Controller            実装済み

  Bluetooth優先入力切替                  実装済み

  Controller connection detection        実装済み

  Stick normalization                    実装済み

  Button A/B/X/Y                         実装済み

  D-Pad                                  実装済み

  HTTP button state hold                 実装済み

  HTTP D-Pad state hold                  実装済み

  HTTP stick latest-value hold           実装済み

  WebSocket heartbeat suppression        実装済み

  WebSocket timeout / safe input clear   実装済み

  RTE ButtonState                        実装済み

  RTE DriveInput                         実装済み

  Acceleration MAP                       実装済み

  Deceleration MAP                       実装済み

  Brake MAP                              実装済み

  Turning radius calculation             実装済み

  Radius speed correction                実装済み

  Differential drive calculation         実装状態を継続確認

  DRV8835 IN/IN conversion               実装済み

  Platform buffered PWM                  実装済み

  Platform physical PWM output           実装済み

  Shutdown safe output                   実装済み

  RTE \`sendInfo()\`                       実装済み

  PlatformHttp Info Buffer               実装済み

  1000ms Debug 情報送信                  実装済み

  Browser Debug Console                  実装済み

  Browser 最新50件保持                   実装済み

  残り Controller ボタン                 保留

  DRV8835 electrical BRAKE               未実装

  Current sensing                        予約 / 未使用

  Encoder                                準備段階

  Persistent controller pairing          TODO

\------------------------------------------------------------------------

**# 48. 現在残っている主な実装課題**

1\.  \`calculateDriveOutput()\` の最小旋回半径・安全範囲を確定

2\.  \`DriveOutput → Rte::setDriveCommand()\` の最終確認

3\.  Debug Console / Serial による演算確認

4\.  モーター未接続で PWM 出力確認

5\.  低 Duty で実モーター接続

6\.  前進 / 後退確認

7\.  直進 / 左右旋回確認

8\.  加速 / 減速 MAP 適合

9\.  ブレーキ適合

10\. Controller 切断時の安全動作確認

11\. Shutdown 確認

\------------------------------------------------------------------------

**# 49. レイヤ責務まとめ**

  項目                         System   Application   RTE   Platform

  \-------------------------- -------- ------------- ----- ----------

  Startup                           ◎                     

  Lifecycle                         ◎                     

  Scheduler                         ◎                     

  Controller取得                                        ○          ◎

  Input Source選択                                      ◎ 

  Controller論理変換                                    ◎ 

  DriveInput                                      ○     ◎ 

  車両走行判断                                    ◎       

  加速・減速・ブレーキ特性                        ◎       

  旋回半径                                        ◎       

  左右モーター出力決定                            ◎     ○ 

  VehicleCommand                                  ○     ○ 

  DRV8835 IN/IN変換                                     ◎ 

  PwmOutput                                             ○          ◎

  PWM / GPIO                                                       ◎

  Bluetooth物理処理                                                ◎

  Wi-Fi / HTTP / WebSocket                                         ◎

  LED状態判断                       ◎                   ○ 

  LED物理出力                                                      ◎

  Debug情報送信API                                ○     ◎ 

  Debug情報蓄積 / 送信                                             ◎

  Shutdown                          ◎             ○     ○          ◎

  ハードウェア安全停止                                             ◎

\------------------------------------------------------------------------

**# 50. 現行アーキテクチャの中心原則**

\`\`\` text

System

    \= システムを動かす

Application

    \= 車両としてどう動きたいかを決める

RTE

    \= 車両の論理要求とハードウェア制御方式を変換する

Platform

    \= MCU・通信・物理I/Oを実際に扱う

Config

    \= 各レイヤ固有の適合値を保持する

\`\`\`

Controller Input：

\`\`\` text

Bluetooth ─→ PlatformBluetooth ──┐

                                 ↓

                          PlatformControllerInput

                                 ↑

                                 │

Browser ─→ WebSocket ─→ PlatformHttp

                                 ↓

                                 RTE

                                 ↓

                            Application

\`\`\`

Debug 情報：

\`\`\` text

Application / System

        ↓

Rte::sendInfo()

        ↓

PlatformHttp

        ↓

WebSocket

        ↓

Browser Debug Console

\`\`\`

モーター：

\`\`\` text

Controller

    ↓

Platform

    ↓

RTE

    ↓

DriveInput

    ↓

Application

    ↓

VehicleCommand + Left/Right Output

    ↓

RTE

    ↓

DRV8835 IN/IN

    ↓

PwmOutput

    ↓

Platform

    ↓

GPIO / PWM

    ↓

DRV8835

    ↓

DC Motors

\`\`\`

この境界を維持することを、本プロジェクトのアーキテクチャ上の基本方針とする。

\------------------------------------------------------------------------

**# 51. 現在の実装優先順位**

Controller Input と Debug Console の基本機能は実装済み。

\`\`\` text

1\. calculateDriveOutput() の安全範囲確定

        ↓

2\. DriveOutput → Rte::setDriveCommand() の最終確認

        ↓

3\. Debug Console / Serial で一連の演算確認

        ↓

4\. モーター未接続でPWM出力確認

        ↓

5\. 低Dutyで実モーター接続

        ↓

6\. 前進 / 後退確認

        ↓

7\. 直進 / 左右旋回確認

        ↓

8\. 加速 / 減速 MAP 適合

        ↓

9\. ブレーキ適合

        ↓

10\. 安全停止・Controller切断時動作確認

\`\`\`

残りの Controller ボタンは現時点では実装しない。

\------------------------------------------------------------------------

**# 52. 最終まとめ**

本システムは System / Application / RTE / Platform

の4レイヤを明確に分離する。

\`\`\` text

System

  ↓

Runtime / Lifecycle / Scheduler

Application

  ↓

Vehicle behavior / Drive control / Tuning

RTE

  ↓

Logical interface / Controller abstraction /

Hardware control protocol conversion /

Information transmission API

Platform

  ↓

MCU / GPIO / PWM / ADC / DAC /

Bluetooth / Wi-Fi / HTTP / WebSocket

\`\`\`

重要なのは、

\-   Application と Platform を直接接続しない

\-   Controller の通信方式を Platform 内で吸収する

\-   RTE が入力ソースを選択する

\-   RTE が Application と Platform の IF を接続する

\-   Debug 情報も RTE API を経由する

\-   PlatformHttp が WebSocket の送受信を担当する

\-   100ms の走行処理と1000msの情報送信を分離する

\-   M5側 Debug Buffer は32件、Browser側は最新50件とする

\-   車両固有の適合値を Config に分離する

ことである。

現在の Debug Console は Controller Input

の受信機能と対になる情報送信機能として完成しており、上位レイヤからは

\`Rte::sendInfo()\` だけで利用できる。