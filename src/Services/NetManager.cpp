#include "NetManager.h"

WiFiClient NetManager::espClient;
PubSubClient NetManager::mqttClient(NetManager::espClient);

uint32_t NetManager::lastWifiAttempt = 0;
uint32_t NetManager::lastMqttAttempt = 0;

void NetManager::init() {
    Serial.printf("[NET] Initializing WiFi: %s\n", WIFI_SSID);
    
    // Start the connection process (Non-blocking)
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    // Time Sync (IST = UTC + 5:30 -> 19800 seconds)
    configTime(19800, 0, "pool.ntp.org");
    
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

void NetManager::connectWiFi() {
    Serial.println("[NET] Attempting WiFi Reconnect...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void NetManager::connectMQTT() {
    Serial.println("[NET] Attempting MQTT Reconnect...");
    if (mqttClient.connect("ESP32-BMS-Master")) {
        Serial.println("[NET] MQTT Connected!");
    }
}

String NetManager::getTimeString() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        return "No Sync";
    }
    char timeStringBuff[20];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
    return String(timeStringBuff);
}

void NetManager::loop() {
    uint32_t currentMillis = millis();

    // 1. Keep WiFi alive asynchronously (Try every 5 seconds)
    if (WiFi.status() != WL_CONNECTED) {
        if (currentMillis - lastWifiAttempt > 5000) {
            lastWifiAttempt = currentMillis;
            connectWiFi();
        }
        return; // Don't try MQTT if WiFi is down
    }
    
    // 2. Keep MQTT alive asynchronously (Try every 5 seconds)
    if (!mqttClient.connected()) {
        if (currentMillis - lastMqttAttempt > 5000) {
            lastMqttAttempt = currentMillis;
            connectMQTT();
        }
    } else {
        // If connected, process incoming MQTT messages and keep-alives
        mqttClient.loop();
    }
}

void NetManager::publishTelemetry(const BmsRecord& record) {
    if (!mqttClient.connected()) return; // Don't crash if offline

    // Build the expanded JSON string from the Brain's data
    String payload = "{";
    payload += "\"time\":\"" + getTimeString() + "\",";
    payload += "\"state\":" + String(record.currentState) + ",";
    payload += "\"faults\":" + String(record.faultFlags) + ",";
    
    // Sensor Data
    payload += "\"v1\":" + String(record.cellVolts[0].value, 2) + ",";
    payload += "\"current\":" + String(record.currentA.value, 2) + ",";
    payload += "\"temp_chg\":" + String(record.tempCharge.value, 1) + ",";
    
    // Coulomb Counter Data (New!)
    payload += "\"soc\":" + String(record.stateOfCharge, 1) + ",";
    payload += "\"cap_mah\":" + String(record.capacityRemaining_mAh, 0) + ",";
    
    // Hardware Flags
    payload += "\"charger\":" + String(record.chargerPhysicallyConnected) + ",";
    payload += "\"relay_c\":" + String(record.cmdChargeRelay) + ",";
    payload += "\"relay_d\":" + String(record.cmdDischargeRelay);
    payload += "}";

    mqttClient.publish(MQTT_TOPIC_PUB, payload.c_str());
}