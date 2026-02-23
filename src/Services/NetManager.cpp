#include "NetManager.h"

WiFiClient NetManager::espClient;
PubSubClient NetManager::mqttClient(NetManager::espClient);

void NetManager::init() {
    connectWiFi();
    
    // Time Sync (IST = UTC + 5:30)
    configTime(19800, 0, "pool.ntp.org");
    
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

void NetManager::connectWiFi() {
    Serial.printf("[NET] Connecting to WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    // We don't want a blocking while() loop that freezes the BMS if WiFi dies.
    // So we just wait 3 seconds. If it fails, we catch it in the loop() later.
    uint32_t startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 3000) {
        delay(100);
    }
    if(WiFi.status() == WL_CONNECTED) {
        Serial.printf("[NET] WiFi Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    }
}

void NetManager::connectMQTT() {
    if (mqttClient.connect("ESP32-BMS-Master")) {
        Serial.println("[NET] MQTT Connected!");
    }
}

String NetManager::getTimeString() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        return "No Time Sync";
    }
    char timeStringBuff[20];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
    return String(timeStringBuff);
}

void NetManager::loop() {
    // Keep WiFi alive
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
        return; // Don't try MQTT if WiFi is down
    }
    
    // Keep MQTT alive
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
}

void NetManager::publishTelemetry(BmsRecord& record) {
    if (!mqttClient.connected()) return; // Don't crash if offline

    // Build a JSON string from the Brain's data
    String payload = "{";
    payload += "\"time\":\"" + getTimeString() + "\",";
    payload += "\"state\":" + String(record.currentState) + ",";
    payload += "\"faults\":" + String(record.faultFlags) + ",";
    payload += "\"v1\":" + String(record.cellVolts[0].value, 2) + ",";
    payload += "\"current\":" + String(record.currentA.value, 1) + ",";
    payload += "\"temp_chg\":" + String(record.tempCharge.value, 1) + ",";
    payload += "\"relay_c\":" + String(record.cmdChargeRelay) + ",";
    payload += "\"relay_d\":" + String(record.cmdDischargeRelay);
    payload += "}";

    mqttClient.publish(MQTT_TOPIC_PUB, payload.c_str());
}