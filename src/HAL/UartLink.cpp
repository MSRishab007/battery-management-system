#include "UartLink.h"
#include <stdlib.h>
#include <string.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

char UartLink::rxBuffer[UartLink::MAX_BUFFER_SIZE];
int UartLink::rxIndex = 0;

void UartLink::init() {
    memset(rxBuffer, 0, MAX_BUFFER_SIZE);
    rxIndex = 0;
}


void UartLink::readSerial(BmsRecord& record, uint32_t currentMillis) {
#ifdef ARDUINO
    while (Serial.available() > 0) {
        char c = Serial.read();

        if (c == '\n') {
            rxBuffer[rxIndex] = '\0'; 
            processFrame(rxBuffer, record, currentMillis);
            
            rxIndex = 0;
            memset(rxBuffer, 0, MAX_BUFFER_SIZE);
        } 
        else if (rxIndex < MAX_BUFFER_SIZE - 1) {
            if (c != '\r') rxBuffer[rxIndex++] = c;
        } else {
            rxIndex = 0; 
            memset(rxBuffer, 0, MAX_BUFFER_SIZE);
        }
    }
#endif
}


bool UartLink::processFrame(char* frame, BmsRecord& record, uint32_t currentMillis) {
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

void UartLink::parsePayload(char* payload, BmsRecord& record, uint32_t currentMillis) {
    char* pair = strtok(payload, ",");
    
    while (pair != nullptr) {
        char* equals = strchr(pair, '=');
        
        if (equals != nullptr) {
            *equals = '\0'; 
            const char* key = pair;
            const char* valueStr = equals + 1;
            float value = atof(valueStr); 

            if (strcmp(key, "v1") == 0) {
                record.cellVoltages[0].value = value;
                record.cellVoltages[0].lastUpdate = currentMillis;
            } else if (strcmp(key, "v2") == 0) {
                record.cellVoltages[1].value = value;
                record.cellVoltages[1].lastUpdate = currentMillis;
            } else if (strcmp(key, "v3") == 0) {
                record.cellVoltages[2].value = value;
                record.cellVoltages[2].lastUpdate = currentMillis;
            } else if (strcmp(key, "v4") == 0) {
                record.cellVoltages[3].value = value;
                record.cellVoltages[3].lastUpdate = currentMillis;
            } else if (strcmp(key, "c") == 0) {
                record.current.value = value;
                record.current.lastUpdate = currentMillis;
            } else if (strcmp(key, "t1") == 0) {
                record.temperatures[0].value = value;
                record.temperatures[0].lastUpdate = currentMillis;
            } else if (strcmp(key, "t2") == 0) {
                record.temperatures[1].value = value;
                record.temperatures[1].lastUpdate = currentMillis;
            } else if (strcmp(key, "CHG") == 0) {
                record.chargerConnected = (atoi(valueStr) > 0);
            }
        }
        pair = strtok(NULL, ",");
    }
}