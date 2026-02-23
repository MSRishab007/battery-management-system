#ifndef NETMANAGER_H
#define NETMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include "../../include/DataTypes.h"
#include "../../include/Secrets.h" 

class NetManager {
private:
    static WiFiClient espClient;
    static PubSubClient mqttClient;
    static void connectWiFi();
    static void connectMQTT();

public:
    static void init();
    static void loop();
    static void publishTelemetry(BmsRecord& record);
    
    // Moved to PUBLIC so main.cpp and Display.cpp can see them
    static String getTimeString();
    static bool isWiFiConnected() { return WiFi.status() == WL_CONNECTED; }
    static bool isMqttConnected() { return mqttClient.connected(); }
};

#endif