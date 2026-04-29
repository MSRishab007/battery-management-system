#include "NetManager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "mqtt_client.h"
#include <string.h>
#include <time.h>
#include "esp_event.h"

static const char* TAG = "NETWORK";

// Initialize Static Variables
BmsRecord* NetManager::bmsData = nullptr;
bool NetManager::wifiConnected = false;
bool NetManager::mqttConnected = false;
void* NetManager::mqttClient = nullptr;

void NetManager::startTask(BmsRecord* record) {
    bmsData = record;

    // 1. Initialize the core networking stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 2. Start Drivers
    initWiFi();
    initSNTP();

    // 3. Spawn the Telemetry Publisher Task
    xTaskCreatePinnedToCore(publishTask, "MQTT_Pub", 4096, NULL, 2, NULL, 0);
    ESP_LOGI(TAG, "Network Manager Initialized");
}

// =================================================================
// WIFI EVENT HANDLER (Background Connection Management)
// =================================================================
void NetManager::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)  {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi Started, connecting to %s...", Secrets::WIFI_SSID);
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifiConnected = false;
        mqttConnected = false;
        esp_wifi_connect(); // Auto-reconnect
        ESP_LOGW(TAG, "WiFi Disconnected. Retrying...");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi Connected! IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        wifiConnected = true;
        
        // Now that we have internet, start the MQTT Client
        if (mqttClient == nullptr) initMQTT();
    }
}

// =================================================================
// MQTT EVENT HANDLER (Command Processing)
// =================================================================
void NetManager::mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Broker Connected!");
            mqttConnected = true;
            // Subscribe to the Command Topic
            esp_mqtt_client_subscribe(client, Secrets::TOPIC_SUB_CMD, 1);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Broker Disconnected.");
            mqttConnected = false;
            break;
            
        case MQTT_EVENT_DATA:
            // Safely print the exact topic and payload using %.*s (because they aren't null-terminated!)
            ESP_LOGI(TAG, "=== Incoming MQTT Message ===");
            ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Payload: %.*s", event->data_len, event->data);
            
            // 1. Ensure it's exactly the topic we want
            if (event->topic_len == strlen(Secrets::TOPIC_SUB_CMD) &&
                strncmp(event->topic, Secrets::TOPIC_SUB_CMD, event->topic_len) == 0) {
                
                // 2. Check if the payload starts with "RESET" (ignoring trailing newlines/spaces)
                if (event->data_len >= 5 && strncmp(event->data, "RESET", 5) == 0) {
                    ESP_LOGW(TAG, ">>> SERVER COMMAND: RESET FAULTS <<<");
                    
                    if (xSemaphoreTake(bmsData->lock, pdMS_TO_TICKS(100)) == pdTRUE) {
                        bmsData->faultFlags = FAULT_NONE;
                        bmsData->currentState = STATE_BOOT; 
                        xSemaphoreGive(bmsData->lock);
                    }
                } else {
                    ESP_LOGW(TAG, "Unrecognized command received on Sub Topic.");
                }
            }
            break;
            
        default:
            break;
    }
}

// =================================================================
// HARDWARE INITIALIZATION
// =================================================================
void NetManager::initWiFi() {
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, (esp_event_handler_t)&wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, (esp_event_handler_t)&wifi_event_handler, NULL, &instance_got_ip));
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, Secrets::WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, Secrets::WIFI_PASS);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void NetManager::initSNTP() {
    ESP_LOGI(TAG, "Initializing SNTP Time Sync...");
    setenv("TZ", "IST-5:30", 1); // Set Timezone to Indian Standard Time
    tzset();
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

void NetManager::initMQTT() {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = Secrets::MQTT_BROKER_URI;
    
    // Optional: Add username/password if your broker uses them
    // mqtt_cfg.credentials.username = Secrets::MQTT_USER;
    // mqtt_cfg.credentials.authentication.password = Secrets::MQTT_PASS;

    mqttClient = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event((esp_mqtt_client_handle_t)mqttClient, MQTT_EVENT_ANY, (esp_event_handler_t)mqtt_event_handler, NULL);
    esp_mqtt_client_start((esp_mqtt_client_handle_t)mqttClient);
}

// =================================================================
// TELEMETRY PUBLISHER TASK
// =================================================================
void NetManager::publishTask(void* pvParameters) {
    char payload[512]; // Buffer for the JSON string
    char timeStr[20];

    while (true) {
        if (mqttConnected) {
            // Snapshot the data
            if (xSemaphoreTake(bmsData->lock, pdMS_TO_TICKS(50)) == pdTRUE) {
                
                getTimeString(timeStr, sizeof(timeStr));
                
                // Build the massive JSON string efficiently
                snprintf(payload, sizeof(payload), 
                    "{"
                    "\"time\":\"%s\","
                    "\"state\":%d,"
                    "\"faults\":%lu,"
                    "\"v1\":%.2f,\"v2\":%.2f,\"v3\":%.2f,\"v4\":%.2f,"
                    "\"current\":%.2f,"
                    "\"t1\":%.1f,\"t2\":%.1f,"
                    "\"soc\":%.1f,"
                    "\"vMax\":%.2f,\"vMin\":%.2f,"
                    "\"charger\":%d,"
                    "\"relay_c\":%d,\"relay_d\":%d,\"relay_r\":%d,"
                    "\"bal\":[%d,%d,%d,%d]"
                    "}",
                    timeStr, bmsData->currentState, bmsData->faultFlags,
                    bmsData->cellVoltages[0].value, bmsData->cellVoltages[1].value, 
                    bmsData->cellVoltages[2].value, bmsData->cellVoltages[3].value,
                    bmsData->current.value,
                    bmsData->temperatures[0].value, bmsData->temperatures[1].value,
                    bmsData->stateOfCharge, bmsData->vMax, bmsData->vMin,
                    bmsData->chargerConnected,
                    bmsData->cmdChargeRelay, bmsData->cmdDischargeRelay, bmsData->cmdRecoveryRelay,
                    bmsData->balanceEnables[0], bmsData->balanceEnables[1], 
                    bmsData->balanceEnables[2], bmsData->balanceEnables[3]
                );
                
                xSemaphoreGive(bmsData->lock);

                // Publish to broker (QoS 0)
                esp_mqtt_client_publish((esp_mqtt_client_handle_t)mqttClient, Secrets::TOPIC_PUB_STATUS, payload, 0, 0, 0);
            }
        }
        
        // Publish every 2 seconds
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// =================================================================
// HELPER GETTERS FOR DISPLAY UI
// =================================================================
bool NetManager::isWiFiConnected() { return wifiConnected; }
bool NetManager::isMqttConnected() { return mqttConnected; }

void NetManager::getTimeString(char* buffer, size_t maxLen) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2020 - 1900)) { // Not synced yet
        snprintf(buffer, maxLen, "No Sync");
    } else {
        strftime(buffer, maxLen, "%H:%M:%S", &timeinfo);
    }
}