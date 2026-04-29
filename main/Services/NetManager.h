#pragma once
#include "../include/DataTypes.h"
#include "../include/Config.h"
#include "../include/Secrets.h" // Holds your SSID, PASS, and MQTT URIs
#include "esp_event.h"

class NetManager {
public:
    // Starts the WiFi/MQTT drivers and spawns the telemetry publish task
    static void startTask(BmsRecord* record);

    // Getters for the OLED Display UI
    static bool isWiFiConnected();
    static bool isMqttConnected();
    static void getTimeString(char* buffer, size_t maxLen);

private:
    static BmsRecord* bmsData; // Local pointer to the master record
    static bool wifiConnected;
    static bool mqttConnected;
    
    // The ESP-IDF MQTT Client Handle
    static void* mqttClient; 

    // Internal Setup Functions
    static void initWiFi();
    static void initSNTP();
    static void initMQTT();

    // The Telemetry Task
    static void publishTask(void* pvParameters);

    // Event Handlers 
    static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);
};