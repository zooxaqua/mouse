#include "PlatformHttp.h"
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>

namespace {

    WebServer s_server(80);
    WebSocketsServer s_webSocket(81);
    PlatformControllerInput s_controllerInput = {};

    // ---------------------------------------------------------
    // Httpルートページを送信
    // ---------------------------------------------------------
    void handleRoot()
    {
        File file = LittleFS.open("/index.html", "r");

        if (!file) {
            Serial.println("[HTTP] Failed to open /index.html");

            s_server.send(404, "text/plain", "File not found");

            return;
        }

        s_server.streamFile(file, "text/html");
        file.close();
    }

    // ---------------------------------------------------------
    // HTTPステータスを送信
    // ---------------------------------------------------------
    void handleStatus()
    {
        s_server.send(200, "text/plain", "OK");
    }
    
    // ---------------------------------------------------------
    // WebSocket
    // ---------------------------------------------------------
    unsigned long s_lastWebSocketReceiveMs = 0;
    bool s_webSocketConnected = false;
    bool s_webSocketTimeout = false;

    constexpr unsigned long WEBSOCKET_TIMEOUT_MS = 1000;

    // ---------------------------------------------------------
    // WebSocket受信データをコントローラ入力へ変換
    // ---------------------------------------------------------
    // Webページから受信したボタン・D-Pad・スティック入力を
    // PlatformControllerInputへ変換する。
    // ---------------------------------------------------------
    void processWebSocketInput( uint8_t clientNum, uint8_t* payload, size_t length)
    {
        String message;

        for (size_t i = 0; i < length; i++) {
            message += (char)payload[i];
        }
        
        if (message == "heartbeat") {
            s_webSocket.sendTXT(clientNum, "heartbeat_ack");
            return;
        }
        // ---------------------------------------------------------
        // A
        // ---------------------------------------------------------
        if (message == "button:A:pressed") {
            s_controllerInput.buttons |= PlatformControllerButton::A;
            s_controllerInput.buttonsPressed |= PlatformControllerButton::A;

            return;
        }

        if (message == "button:A:released") {
            s_controllerInput.buttons &= ~PlatformControllerButton::A;
            s_controllerInput.buttonsReleased |= PlatformControllerButton::A;

            return;
        }

        // ---------------------------------------------------------
        // B
        // ---------------------------------------------------------
        if (message == "button:B:pressed") {

            s_controllerInput.buttons |= PlatformControllerButton::B;
            s_controllerInput.buttonsPressed |= PlatformControllerButton::B;

            return;
        }

        if (message == "button:B:released") {

            s_controllerInput.buttons &= ~PlatformControllerButton::B;
            s_controllerInput.buttonsReleased |= PlatformControllerButton::B;

            return;
        }

        // ---------------------------------------------------------
        // X
        // ---------------------------------------------------------
        if (message == "button:X:pressed") {

            s_controllerInput.buttons |= PlatformControllerButton::X;
            s_controllerInput.buttonsPressed |= PlatformControllerButton::X;

            return;
        }

        if (message == "button:X:released") {

            s_controllerInput.buttons &= ~PlatformControllerButton::X;
            s_controllerInput.buttonsReleased |= PlatformControllerButton::X;

            return;
        }

        // ---------------------------------------------------------
        // Y
        // ---------------------------------------------------------
        if (message == "button:Y:pressed") {

            s_controllerInput.buttons |= PlatformControllerButton::Y;
            s_controllerInput.buttonsPressed |= PlatformControllerButton::Y;

            return;
        }

        if (message == "button:Y:released") {

            s_controllerInput.buttons &= ~PlatformControllerButton::Y;
            s_controllerInput.buttonsReleased |= PlatformControllerButton::Y;

            return;
        }
        // ---------------------------------------------------------
        // D-Pad UP
        // ---------------------------------------------------------
        if (message == "button:UP:pressed") {

            s_controllerInput.dpad |= PlatformControllerDpad::UP;
            s_controllerInput.dpadPressed |= PlatformControllerDpad::UP;

            return;
        }

        if (message == "button:UP:released") {

            s_controllerInput.dpad &= ~PlatformControllerDpad::UP;
            s_controllerInput.dpadReleased |= PlatformControllerDpad::UP;

            return;
        }

        // ---------------------------------------------------------
        // D-Pad DOWN
        // ---------------------------------------------------------
        if (message == "button:DOWN:pressed") {

            s_controllerInput.dpad |= PlatformControllerDpad::DOWN;
            s_controllerInput.dpadPressed |= PlatformControllerDpad::DOWN;

            return;
        }

        if (message == "button:DOWN:released") {

            s_controllerInput.dpad &= ~PlatformControllerDpad::DOWN;
            s_controllerInput.dpadReleased |= PlatformControllerDpad::DOWN;

            return;
        }

        // ---------------------------------------------------------
        // D-Pad LEFT
        // ---------------------------------------------------------

        if (message == "button:LEFT:pressed") {

            s_controllerInput.dpad |= PlatformControllerDpad::LEFT;
            s_controllerInput.dpadPressed |= PlatformControllerDpad::LEFT;

            return;
        }

        if (message == "button:LEFT:released") {

            s_controllerInput.dpad &= ~PlatformControllerDpad::LEFT;
            s_controllerInput.dpadReleased |= PlatformControllerDpad::LEFT;

            return;
        }

        // ---------------------------------------------------------
        // D-Pad RIGHT
        // ---------------------------------------------------------
        if (message == "button:RIGHT:pressed") {

            s_controllerInput.dpad |= PlatformControllerDpad::RIGHT;
            s_controllerInput.dpadPressed |= PlatformControllerDpad::RIGHT;

            return;
        }

        if (message == "button:RIGHT:released") {

            s_controllerInput.dpad &= ~PlatformControllerDpad::RIGHT;
            s_controllerInput.dpadReleased |= PlatformControllerDpad::RIGHT;

            return;
        }
        
        // ---------------------------------------------------------
        // Left Stick
        // ---------------------------------------------------------
        if (message.startsWith("stick:left:")) {

            int firstColon = message.indexOf(':', 11);

            if (firstColon >= 0) {

                int x = message.substring(11, firstColon).toInt();
                int y = message.substring(firstColon + 1).toInt();

                s_controllerInput.leftStickX = constrain(x, -100, 100);
                s_controllerInput.leftStickY = constrain(y, -100, 100);
            }

            return;
        }


        // ---------------------------------------------------------
        // Right Stick
        // ---------------------------------------------------------
        if (message.startsWith("stick:right:")) {

            int firstColon = message.indexOf(':', 12);

            if (firstColon >= 0) {

                int x = message.substring(12, firstColon).toInt();
                int y = message.substring(firstColon + 1).toInt();

                s_controllerInput.rightStickX = constrain(x, -100, 100);
                s_controllerInput.rightStickY = constrain(y, -100, 100);
            }

            return;
        }
    }

    // ---------------------------------------------------------
    // WebSocketイベント処理
    // ---------------------------------------------------------
    // 接続・切断・テキスト受信などのWebSocketイベントを処理する。
    // ---------------------------------------------------------
    void onWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload, size_t length)
    {
        switch (type) {

            case WStype_CONNECTED:
            {
                Serial.print("[WS] Client connected: ");
                Serial.println(clientNum);

                s_webSocketConnected = true;
                s_webSocketTimeout = false;
                s_lastWebSocketReceiveMs = millis();
                
                break;
            }


            case WStype_DISCONNECTED:
            {
                Serial.print("[WS] Client disconnected: ");
                Serial.println(clientNum);

                s_webSocketConnected = false;
                s_webSocketTimeout = true;

                s_controllerInput = {};

                break;
            }


            case WStype_TEXT:
            {
                // ① WebSocket受信時刻を更新
                s_lastWebSocketReceiveMs = millis();
                // ② 接続中・Timeoutしていない状態にする
                s_webSocketConnected = true;
                s_webSocketTimeout = false;

                // ③ Controller入力処理へ渡す
                processWebSocketInput(clientNum, payload, length);

                break;
            }

            default:
                break;
        }
    }

    constexpr size_t INFO_BUFFER_SIZE = 32;

    String s_infoBuffer[INFO_BUFFER_SIZE];
    size_t s_infoCount = 0;

}

namespace PlatformHttp {
    // ---------------------------------------------------------
    // HTTP / WebSocket初期化処理
    // ---------------------------------------------------------
    void init()
    {
        Serial.println("[HTTP] Initializing...");

            // LittleFS
            if (!LittleFS.begin()) {
                Serial.println("[HTTP] LittleFS Mount Failed");

                return;
            }

            Serial.println("[HTTP] LittleFS Mounted");

            // HTTP
            s_server.on("/", handleRoot);
            s_server.on("/status", handleStatus);
            s_server.serveStatic("/css",       LittleFS, "/css/");
            s_server.serveStatic( "/js", LittleFS, "/js/");
            s_server.serveStatic( "/framework", LittleFS, "/framework/");

            s_server.begin();

            Serial.println("[HTTP] Server started");

            // WebSocket
            s_webSocket.begin();
            s_webSocket.onEvent(onWebSocketEvent);

            Serial.println("[WS] Server started");
        }

    // ---------------------------------------------------------
    // HTTP / WebSocket更新処理
    // ---------------------------------------------------------
    // HTTPリクエスト、WebSocket通信、接続Timeoutを処理する。
    // ---------------------------------------------------------
    void update()
    {
        // 前回周期の押下・解放イベントをクリア
        s_controllerInput.buttonsPressed = 0;
        s_controllerInput.buttonsReleased = 0;
        s_controllerInput.dpadPressed = 0;
        s_controllerInput.dpadReleased = 0;
        s_controllerInput.miscButtonsPressed = 0;
        s_controllerInput.miscButtonsReleased = 0;
        
        // HTTP
        s_server.handleClient();

        // WebSocket
        s_webSocket.loop();

        // WebSocket Timeout
        if (s_webSocketConnected && !s_webSocketTimeout && millis() - s_lastWebSocketReceiveMs >= WEBSOCKET_TIMEOUT_MS) {

            s_webSocketTimeout = true;
            s_webSocketConnected = false;
            Serial.println("[WS] Timeout");
        }
        // Timeout時は全入力を安全側へ
        if (s_webSocketTimeout) {
            s_controllerInput = {};
        }
    }

    // ---------------------------------------------------------
    // デバッグ情報送信処理
    // ---------------------------------------------------------
    // バッファに蓄積された情報をWebSocketでWebページへ送信する。
    // ---------------------------------------------------------
    void updateInfo()
    {
        if (!s_webSocketConnected || s_infoCount == 0) {
            return;
        }

        String message;

        for (size_t i = 0; i < s_infoCount; i++) {

            message += s_infoBuffer[i];

            if (i < s_infoCount - 1) {
                message += "\n";
            }
        }

        s_webSocket.broadcastTXT(message);

        s_infoCount = 0;
    }

    // ---------------------------------------------------------
    // Webコントローラ接続状態を取得
    // ---------------------------------------------------------
    bool isControllerConnected()
    {
        return s_webSocketConnected && !s_webSocketTimeout;
    }

    // ---------------------------------------------------------
    // Webコントローラ入力を取得
    // ---------------------------------------------------------
    PlatformControllerInput getControllerInput()
    {
        return s_controllerInput;
    }

    // ---------------------------------------------------------
    // デバッグ情報を送信バッファへ追加
    // ---------------------------------------------------------
    // 情報はバッファへ蓄積し、updateInfo()でまとめて送信する。
    // ---------------------------------------------------------
    void sendInfo(const char* message)
    {
        if (message == nullptr) {
            return;
        }

        if (s_infoCount < INFO_BUFFER_SIZE) {
            s_infoBuffer[s_infoCount++] = message;
        }
    }
}