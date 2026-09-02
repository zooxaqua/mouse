# M5Stamp-Pico Robot Controller

M5Stamp-Pico（ESP32）を中心とした小型ロボット／車両制御システムの開発プロジェクトです。

BluetoothコントローラまたはWi-Fi経由のブラウザから走行指示を受け取り、操舵量とモータ出力から左右2個のDCモータを制御します。

現在は走行制御の基本機能を実装済みで、ソフトウェア構成の整理・安定化と、将来的な拡張を進めています。

---

## Features

- Bluetoothコントローラによる操作
- Wi-Fi経由のWebブラウザ操作
- WebSocketによるリアルタイム操作入力
- Bluetooth / Wi-Fi入力の抽象化
- 左右2個のDCモータ制御
- DRV8835によるモータ駆動
- 加速・減速・ブレーキ特性MAP
- 操舵量に応じた左右モータ出力制御
- 旋回半径MAPおよび速度による補正
- RGB LEDによる状態表示
- mDNSによるホスト名アクセス
- HTTP / WebSocket通信
- LittleFSを利用したWeb関連データの保存
- OTA更新を想定したデュアルAPPパーティション
- Platform / RTE / Application / Systemのレイヤ構成

---

## Hardware

### Main Controller

- M5Stamp-Pico Mate
- ESP32
- Flash: 4 MB
- RAM: 320 KB

### Motor Driver

- DRV8835
- DCモータ × 2

### Communication

- Bluetooth
- Wi-Fi
- HTTP
- WebSocket

### On-board Device

- SK6812 RGB LED
- User Button

---

## Software Architecture

本プロジェクトでは、ハードウェア依存部分とアプリケーションロジックを分離するため、以下のレイヤ構成を採用しています。

```text
┌─────────────────────────────┐
│           System            │
│  Lifecycle / Cycle Manager  │
└──────────────┬──────────────┘
               │
┌──────────────▼──────────────┐
│       Application           │
│  Driving Logic / MAP        │
└──────────────┬──────────────┘
               │
┌──────────────▼──────────────┐
│            RTE              │
│ Input / Output Conversion   │
└──────────────┬──────────────┘
               │
┌──────────────▼──────────────┐
│          Platform           │
│ Hardware / Communication    │
└─────────────────────────────┘
```

### System

システム全体のライフサイクルと周期処理を管理します。

主な状態：

```text
BOOT
  ↓
INIT
  ↓
READY
  ↓
RUN
  ↓
SHUTDOWN / ERROR
```

また、複数の周期処理を設定ファイルから管理します。

### Application

走行に関するアプリケーションロジックを担当します。

主な処理：

- モータ出力計算
- 加速特性
- 減速特性
- ブレーキ特性
- 操舵
- 旋回半径計算
- 旋回半径補正
- 左右モータ出力計算

Applicationは最終的な左右モータ出力を `-100 ～ +100` の範囲で生成します。

```text
+100 : 正方向最大出力
   0 : 停止
-100 : 逆方向最大出力
```

### RTE

ApplicationとPlatformの間を接続します。

左右モータ出力について、

```text
App
  │
  │ Right / Left : -100 ～ +100
  ▼
RTE
  │
  ├── 正方向 → IN1 PWM
  └── 逆方向 → IN2 PWM
  │
  ▼
Platform
```

のように、アプリケーション上の値をハードウェア出力へ変換します。

### Platform

ESP32および接続デバイスに依存する処理を担当します。

- PWM
- GPIO
- ADC
- DAC
- NeoPixel
- Bluetooth
- Wi-Fi
- HTTP
- WebSocket
- mDNS

などを抽象化します。

---

## Motor Control

DRV8835のIN/IN制御方式を使用しています。

左右モータそれぞれについて2本のPWMを使用します。

```text
Motor 1
  IN1 / IN2

Motor 2
  IN1 / IN2
```

RTEではモータ出力の符号に応じてPWMを振り分けます。

```text
出力 >= 0
    IN1 = 出力
    IN2 = 0

出力 < 0
    IN1 = 0
    IN2 = -出力
```

Application側では加速・減速・ブレーキ・操舵を考慮した最終的な左右出力を計算します。

---

## Input

### Bluetooth Controller

Bluepad32を使用しています。

コントローラ入力はPlatform層で取得し、`PlatformControllerInput` に統一します。

入力には以下があります。

- Left / Right Stick
- Buttons
- D-Pad
- Misc Buttons
- Pressed
- Hold
- Released

スティック値は `-100 ～ +100` に正規化します。

### Wi-Fi Controller

スマートフォン等のブラウザからWi-Fi経由で操作できます。

```text
Browser
   │
   │ WebSocket
   ▼
ESP32
   │
PlatformHttp
   ▼
RTE
   ▼
Application
```

WebSocket通信が切断された場合は、入力状態をクリアして安全側へ移行する設計です。

---

## Network

ESP32はWi-Fi STAモードで家庭内ネットワークへ接続します。

mDNSを使用して、IPアドレスではなくホスト名でアクセスできるようにしています。

現在のホスト名：

```text
m5robot-00.local
```

将来的にはWi-Fi設定をソースコードから分離し、別領域へ保存する予定です。

---

## Flash Partition

4 MB Flashを使用しています。

OTAによるファームウェア更新を試すため、APP0 / APP1のデュアルパーティション構成を採用しています。

```text
0x000000
    │
    │ 予約領域
    │
0x009000 ┌─────────────┐
         │ NVS 20 KiB  │
0x00E000 ├─────────────┤
         │ OTA  8 KiB  │
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
         │   LittleFS  │
         │   448 KiB   │
         │             │
0x400000 └─────────────┘
```

Partition CSV：

```csv
# Name,Type,SubType,Offset,Size,Flags
nvs,data,nvs,0x9000,0x5000,
otadata,data,ota,0xE000,0x2000,
app0,app,ota_0,0x10000,0x1C0000,
app1,app,ota_1,0x1D0000,0x1C0000,
littlefs,data,spiffs,0x390000,0x70000,
```

LittleFSはWeb関連データなどの保存領域として使用します。

OTA領域は今後のOTA実装・学習用です。

---

## Development Environment

### IDE

Visual Studio Code + PlatformIO

### Framework

Arduino for ESP32

### Platform

Espressif32

### Main Libraries

- Bluepad32
- WebSockets
- WiFi
- WebServer
- ESPmDNS
- LittleFS

---

## Project Status

現在は以下の基本機能が動作しています。

- Bluetoothによる操作
- Wi-Fiによるブラウザ操作
- 操舵入力
- モータ出力制御
- 左右モータPWM制御
- 走行特性MAP
- WebSocket通信
- mDNS

現在は、コード全体のレビュー・コメント・関数ヘッダ整備・レイヤ間の責務整理を進めています。

---

## Planned

今後の主な予定：

1. 走行計算の端点・符号テスト
2. レイヤ間・ファイル間の連携確認
3. RAM使用量の最適化
4. WebSocket / ブラウザ側の切断状態表示
5. Wi-Fi SSID / Passwordの別領域保存
6. Hostname等の設定管理整理
7. OTA機能の実装
8. 将来的なカメラ映像配信
9. スマートフォンGUIの拡張

---

## Design Philosophy

このプロジェクトでは、将来的な機能追加やハードウェア変更に対応しやすい構造を重視しています。

特に、

```text
Hardware
   ↓
Platform
   ↓
RTE
   ↓
Application
```

という依存方向を意識し、ApplicationがESP32固有の処理を直接扱わない構成を目指しています。

また、走行特性をMAPとして分離することで、制御ロジックと車両特性を分離し、将来的に異なる走行特性や制御モデルへ交換できる構造を目指しています。

---

## License

未定.
