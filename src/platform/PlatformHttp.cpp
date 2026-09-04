// PlatformHttp.cpp
// HTTP通信を使用したコントローラ入力処理

#include "PlatformHttp.h"
#include "PlatformController.h"
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>

namespace {

    WebServer s_server(80);
    WebSocketsServer s_webSocket(81);

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
    // PlatformControllerData
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
            PlatformController::updateHttpButtonA(true);

            return;
        }

        if (message == "button:A:released") {
            PlatformController::updateHttpButtonA(false);

            return;
        }

        // ---------------------------------------------------------
        // B
        // ---------------------------------------------------------
        if (message == "button:B:pressed") {
            PlatformController::updateHttpButtonB(true);

            return;
        }

        if (message == "button:B:released") {
            PlatformController::updateHttpButtonB(false);

            return;
        }

        // ---------------------------------------------------------
        // X
        // ---------------------------------------------------------
        if (message == "button:X:pressed") {
            PlatformController::updateHttpButtonX(true);

            return;
        }

        if (message == "button:X:released") {
            PlatformController::updateHttpButtonX(false);

            return;
        }

        // ---------------------------------------------------------
        // Y
        // ---------------------------------------------------------
        if (message == "button:Y:pressed") {
            PlatformController::updateHttpButtonY(true);

            return;
        }

        if (message == "button:Y:released") {
            PlatformController::updateHttpButtonY(false);

            return;
        }
        // ---------------------------------------------------------
        // D-Pad UP
        // ---------------------------------------------------------
        if (message == "button:UP:pressed") {
            PlatformController::updateHttpDpadUp(true);

            return;
        }

        if (message == "button:UP:released") {
            PlatformController::updateHttpDpadUp(false);

            return;
        }

        // ---------------------------------------------------------
        // D-Pad DOWN
        // ---------------------------------------------------------
        if (message == "button:DOWN:pressed") {
            PlatformController::updateHttpDpadDown(true);

            return;
        }

        if (message == "button:DOWN:released") {
            PlatformController::updateHttpDpadDown(false);

            return;
        }

        // ---------------------------------------------------------
        // D-Pad LEFT
        // ---------------------------------------------------------
        if (message == "button:LEFT:pressed") {
            PlatformController::updateHttpDpadLeft(true);

            return;
        }

        if (message == "button:LEFT:released") {
            PlatformController::updateHttpDpadLeft(false);

            return;
        }

        // ---------------------------------------------------------
        // D-Pad RIGHT
        // ---------------------------------------------------------
        if (message == "button:RIGHT:pressed") {
            PlatformController::updateHttpDpadRight(true);

            return;
        }

        if (message == "button:RIGHT:released") {
            PlatformController::updateHttpDpadRight(false);

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

                PlatformController::updateHttpLeftStick(x, y);
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

                PlatformController::updateHttpRightStick(x, y);
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
            PlatformController::clearHttpControllerInput();
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