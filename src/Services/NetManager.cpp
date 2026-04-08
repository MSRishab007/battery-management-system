#include "NetManager.h"
#include "../../include/secrets.h"

#ifdef ARDUINO
#include <WiFi.h>
#include <PubSubClient.h>

static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

static uint32_t lastWifiAttempt = 0;
static uint32_t lastMqttAttempt = 0;
#endif

void NetManager::init() {
#ifdef ARDUINO
    Serial.printf("[NET] Initializing WiFi: %s\n", WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    // Time Sync (IST = UTC + 5:30 -> 19800 seconds)
    configTime(19800, 0, "pool.ntp.org");
    
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
#endif
}

void NetManager::loop(uint32_t currentMillis) {
#ifdef ARDUINO
    // 1. Keep WiFi alive asynchronously (Try every 5 seconds)
    if (WiFi.status() != WL_CONNECTED) {
        if (currentMillis - lastWifiAttempt > 5000) {
            lastWifiAttempt = currentMillis;
            Serial.println("[NET] Attempting WiFi Reconnect...");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASS);
        }
        return; // Don't try MQTT if WiFi is down
    }
    
    // 2. Keep MQTT alive asynchronously (Try every 5 seconds)
    if (!mqttClient.connected()) {
        if (currentMillis - lastMqttAttempt > 5000) {
            lastMqttAttempt = currentMillis;
            Serial.println("[NET] Attempting MQTT Reconnect...");
            if (mqttClient.connect("ESP32-BMS-Master")) {
                Serial.println("[NET] MQTT Connected!");
            }
        }
    } else {
        // Process incoming MQTT messages (Commands from Python)
        mqttClient.loop();
    }
#endif
}

void NetManager::publishTelemetry(const BmsRecord& record) {
#ifdef ARDUINO
    if (!mqttClient.connected()) return; // Don't crash if offline

    // Build the expanded JSON string for the 10-state architecture
    String payload = "{";
    payload += "\"time\":\"" + getTimeString() + "\",";
    payload += "\"state\":" + String(record.currentState) + ",";
    payload += "\"faults\":" + String(record.faultFlags) + ",";
    
    // Granular Sensor Data
    payload += "\"v1\":" + String(record.cellVoltages[0].value, 2) + ",";
    payload += "\"v2\":" + String(record.cellVoltages[1].value, 2) + ",";
    payload += "\"v3\":" + String(record.cellVoltages[2].value, 2) + ",";
    payload += "\"v4\":" + String(record.cellVoltages[3].value, 2) + ",";
    payload += "\"current\":" + String(record.current.value, 2) + ",";
    payload += "\"t1\":" + String(record.temperatures[0].value, 1) + ",";
    payload += "\"t2\":" + String(record.temperatures[1].value, 1) + ",";
    
    // SoC and Pack Math
    payload += "\"soc\":" + String(record.stateOfCharge, 1) + ",";
    payload += "\"vMax\":" + String(record.vMax, 2) + ",";
    payload += "\"vMin\":" + String(record.vMin, 2) + ",";
    
    // Hardware Flags
    payload += "\"charger\":" + String(record.chargerConnected) + ",";
    payload += "\"relay_c\":" + String(record.cmdChargeRelay) + ",";
    payload += "\"relay_d\":" + String(record.cmdDischargeRelay) + ",";
    payload += "\"relay_r\":" + String(record.cmdRecoveryRelay) + ",";
    
    // Balancing Array
    payload += "\"bal\":[" + String(record.balanceEnables[0]) + "," + 
                             String(record.balanceEnables[1]) + "," + 
                             String(record.balanceEnables[2]) + "," + 
                             String(record.balanceEnables[3]) + "]";
    payload += "}";

    mqttClient.publish(MQTT_TOPIC_PUB, payload.c_str());
#endif
}

String NetManager::getTimeString() {
#ifdef ARDUINO
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        return "No Sync";
    }
    char timeStringBuff[20];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
    return String(timeStringBuff);
#else
    return "00:00:00";
#endif
}

bool NetManager::isWiFiConnected() {
#ifdef ARDUINO
    return WiFi.status() == WL_CONNECTED;
#else
    return false;
#endif
}

bool NetManager::isMqttConnected() {
#ifdef ARDUINO
    return mqttClient.connected();
#else
    return false;
#endif
}