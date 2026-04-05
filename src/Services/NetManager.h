#ifndef NETMANAGER_H
#define NETMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "../../include/DataTypes.h"
#include "../../include/Secrets.h" // Holds WIFI_SSID, WIFI_PASS, MQTT_SERVER

class NetManager {
private:
    static WiFiClient espClient;
    static PubSubClient mqttClient;
    
    static uint32_t lastWifiAttempt;
    static uint32_t lastMqttAttempt;

    static void connectWiFi();
    static void connectMQTT();

public:
    static void init();
    static void loop();
    static void publishTelemetry(const BmsRecord& record);
    static String getTimeString();
    
    static inline bool isWiFiConnected() { return WiFi.status() == WL_CONNECTED; }
    static inline bool isMqttConnected() { return mqttClient.connected(); }
};

#endif