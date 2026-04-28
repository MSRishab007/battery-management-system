#include "UartLink.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "UART_LINK";

char UartLink::rxBuffer[UartLink::MAX_BUFFER_SIZE];
int UartLink::rxIndex = 0;

void UartLink::startTask(BmsRecord* record) {
    memset(rxBuffer, 0, MAX_BUFFER_SIZE);
    rxIndex = 0;

    // Spawn the listener task on Core 0 (leaving Core 1 for the safety math)
    xTaskCreatePinnedToCore(
        uartTask, 
        "Uart_Listener", 
        4096, 
        (void*)record, // Pass the master record pointer into the task
        3,             // Priority (Medium)
        NULL, 
        0              // Pin to Core 0
    );
    ESP_LOGI(TAG, "UART Listener Task Started on Core 0");
}

void UartLink::uartTask(void* pvParameters) {
    BmsRecord* record = (BmsRecord*)pvParameters;
    uint8_t data[MAX_BUFFER_SIZE];

    while (true) {
        // Read available bytes from the native USB hardware buffer
        int len = usb_serial_jtag_read_bytes(data, MAX_BUFFER_SIZE, pdMS_TO_TICKS(10));

        for (int i = 0; i < len; i++) {
            char c = (char)data[i];
            if (c == '\n') {
                rxBuffer[rxIndex] = '\0';
                uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
                processFrame(rxBuffer, record, now);
                rxIndex = 0;
            } else if (rxIndex < MAX_BUFFER_SIZE - 1) {
                if (c != '\r') rxBuffer[rxIndex++] = c;
            } else {
                rxIndex = 0; // Overflow safety
            }
        }
        // Small delay to let other tasks run
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
bool UartLink::processFrame(char* frame, BmsRecord* record, uint32_t currentMillis) {
    char* asterisk = strchr(frame, '*');
            
    if (asterisk != nullptr) {
        *asterisk = '\0'; 
        const char* payload = frame;
        const char* providedChecksumStr = asterisk + 1;

        uint8_t calculatedChecksum = calculateChecksum(payload);
        uint8_t providedChecksum = (uint8_t)strtol(providedChecksumStr, NULL, 16);

        if (calculatedChecksum == providedChecksum) {
            parsePayload((char*)payload, record, currentMillis);
            return true;
        } else {
            ESP_LOGW(TAG, "Checksum failed! Calc: %02X, Got: %02X", calculatedChecksum, providedChecksum);
        }
    }
    return false;
}

uint8_t UartLink::calculateChecksum(const char* payload) {
    uint8_t checksum = 0;
    for (int i = 0; payload[i] != '\0'; i++) {
        checksum ^= payload[i];
    }
    return checksum;
}

void UartLink::parsePayload(char* payload, BmsRecord* record, uint32_t currentMillis) {
    // ==========================================================
    // CRITICAL: We are writing to the Master Record!
    // We MUST take the lock so the Core Task doesn't read half-written data.
    // ==========================================================
    if (xSemaphoreTake(record->lock, pdMS_TO_TICKS(20)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to get Mutex lock! Dropping incoming UART data.");
        return; 
    }

    char* pair = strtok(payload, ",");
    
    while (pair != nullptr) {
        char* equals = strchr(pair, '=');
        
        if (equals != nullptr) {
            *equals = '\0'; 
            const char* key = pair;
            const char* valueStr = equals + 1;
            float value = atof(valueStr); 

            if (strcmp(key, "v1") == 0) {
                record->cellVoltages[0].value = value;
                record->cellVoltages[0].lastUpdate = currentMillis;
            } else if (strcmp(key, "v2") == 0) {
                record->cellVoltages[1].value = value;
                record->cellVoltages[1].lastUpdate = currentMillis;
            } else if (strcmp(key, "v3") == 0) {
                record->cellVoltages[2].value = value;
                record->cellVoltages[2].lastUpdate = currentMillis;
            } else if (strcmp(key, "v4") == 0) {
                record->cellVoltages[3].value = value;
                record->cellVoltages[3].lastUpdate = currentMillis;
            } else if (strcmp(key, "c") == 0) {
                record->current.value = value;
                record->current.lastUpdate = currentMillis;
            } else if (strcmp(key, "t1") == 0) {
                record->temperatures[0].value = value;
                record->temperatures[0].lastUpdate = currentMillis;
            } else if (strcmp(key, "t2") == 0) {
                record->temperatures[1].value = value;
                record->temperatures[1].lastUpdate = currentMillis;
            } else if (strcmp(key, "CHG") == 0) {
                record->chargerConnected = (atoi(valueStr) > 0);
            }
        }
        pair = strtok(NULL, ",");
    }

    // RELEASE THE LOCK
    xSemaphoreGive(record->lock);
}