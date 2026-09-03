// PlatformWifi.cpp
// Wi-Fi接続処理

#include "PlatformWifi.h"
#include "../../secrets/secrets.h"

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>

namespace {

    // ---------------------------------------------------------
    // Wi-Fi設定
    // ---------------------------------------------------------


    // ---------------------------------------------------------
    // Wi-Fi内部状態
    // ---------------------------------------------------------
    String s_wifiHostname = Secrets::WIFI_HOSTNAME;
    String s_wifiSsid = "";
    String s_wifiPassword = "";

    bool s_wifiPreviousConnected = false;


    // ---------------------------------------------------------
    // Wi-Fi接続開始
    // ---------------------------------------------------------
    void wifi_setup(const char* ssid, const char* password)
    {
        // 設定値に変更がなければ何もしない
        if (s_wifiSsid == ssid &&
            s_wifiPassword == password) {
            return;
        }

        Serial.println("[WIFI] Settings changed.");

        // 現在の接続を切断
        WiFi.disconnect();

        // 現在の設定を保存
        s_wifiSsid = ssid;
        s_wifiPassword = password;

        // STAモードで接続開始
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);

        Serial.print("[WIFI] Connection started. SSID: ");
        Serial.println(ssid);
    }


    // ---------------------------------------------------------
    // Wi-Fi接続状態確認
    // ---------------------------------------------------------
    void wifi_checkConnected()
    {
        bool connected = (WiFi.status() == WL_CONNECTED);

        // 未接続 → 接続済みになった瞬間
        if (connected && !s_wifiPreviousConnected) {

            // mDNS開始
            if (MDNS.begin(s_wifiHostname.c_str())) {

                Serial.print("[WIFI] mDNS started");

                Serial.print("[WIFI] access URL: http://");
                Serial.print(s_wifiHostname);
                Serial.println(".local");
            }
            else {
                Serial.println("[WIFI] mDNS start failed");
            }
        }

        // 現在状態を保存
        s_wifiPreviousConnected = connected;
    }

}

namespace PlatformWifi {

    // ---------------------------------------------------------
    // Wi-Fi初期化処理
    // ---------------------------------------------------------
    void init()
    {
        Serial.println("[WIFI] Initializing...");

        // 現在はSTAのみ
        wifi_setup(Secrets::WIFI_SSID, Secrets::WIFI_PASSWORD);
    }

    // ---------------------------------------------------------
    // Wi-Fi更新処理
    // ---------------------------------------------------------
    void update()
    {
        wifi_checkConnected();
    }

    // ---------------------------------------------------------
    // Wi-Fi接続状態を取得
    // ---------------------------------------------------------
    bool isConnected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

}