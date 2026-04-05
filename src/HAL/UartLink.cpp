#include "UartLink.h"
#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

// Initialize static variables
char UartLink::rxBuffer[UartLink::MAX_BUFFER_SIZE];
int UartLink::rxIndex = 0;

void UartLink::init() {
    memset(rxBuffer, 0, MAX_BUFFER_SIZE);
    rxIndex = 0;
}

void UartLink::processIncoming(BmsRecord& record) {
    // Read all available bytes from the serial buffer
    while (Serial.available() > 0) {
        char c = Serial.read();

        // End of frame detected
        if (c == '\n') {
            rxBuffer[rxIndex] = '\0'; // Null-terminate the string

            // Find the checksum delimiter '*'
            char* asterisk = strchr(rxBuffer, '*');
            
            if (asterisk != nullptr) {
                // Split the string into Payload and Checksum
                *asterisk = '\0'; // Replace '*' with null terminator
                const char* payload = rxBuffer;
                const char* providedChecksumStr = asterisk + 1;

                // Calculate XOR checksum of the payload
                uint8_t calculatedChecksum = calculateChecksum(payload);
                
                // Convert provided checksum from Hex string to integer
                uint8_t providedChecksum = (uint8_t)strtol(providedChecksumStr, NULL, 16);

                // Verify integrity
                if (calculatedChecksum == providedChecksum) {
                    // Checksum matches! Safe to parse data.
                    parsePayload((char*)payload, record);
                } else {
                    Serial.printf("UART Checksum Error! Calc: %02X, Got: %02X\n", calculatedChecksum, providedChecksum);
                }
            }
            
            // Reset buffer for the next frame
            rxIndex = 0;
            memset(rxBuffer, 0, MAX_BUFFER_SIZE);
        } 
        // Prevent buffer overflow
        else if (rxIndex < MAX_BUFFER_SIZE - 1) {
            // Ignore carriage returns, only keep valid characters
            if (c != '\r') {
                rxBuffer[rxIndex++] = c;
            }
        } else {
            // Buffer overflowed (garbage data on line), reset it
            rxIndex = 0;
            memset(rxBuffer, 0, MAX_BUFFER_SIZE);
        }
    }
}

// Standard XOR Checksum (NMEA style)
uint8_t UartLink::calculateChecksum(const char* payload) {
    uint8_t checksum = 0;
    for (int i = 0; payload[i] != '\0'; i++) {
        checksum ^= payload[i];
    }
    return checksum;
}

void UartLink::parsePayload(char* payload, BmsRecord& record) {
    uint32_t currentMillis = millis();

    // Tokenize the payload by commas
    // Example: "v1=3.5,v2=3.6,c=-2.1,CHG=1"
    char* pair = strtok(payload, ",");
    
    while (pair != nullptr) {
        // Find the '=' in the key-value pair
        char* equals = strchr(pair, '=');
        
        if (equals != nullptr) {
            *equals = '\0'; // Split key and value
            const char* key = pair;
            const char* valueStr = equals + 1;
            float value = atof(valueStr); // Convert string to float

            if (strcmp(key, "v1") == 0) {
                record.cellVolts[0].value = value;
                record.cellVolts[0].lastUpdate = currentMillis;
            } 
            else if (strcmp(key, "v2") == 0) {
                record.cellVolts[1].value = value;
                record.cellVolts[1].lastUpdate = currentMillis;
            } 
            else if (strcmp(key, "v3") == 0) {
                record.cellVolts[2].value = value;
                record.cellVolts[2].lastUpdate = currentMillis;
            } 
            else if (strcmp(key, "v4") == 0) {
                record.cellVolts[3].value = value;
                record.cellVolts[3].lastUpdate = currentMillis;
            } 
            else if (strcmp(key, "c") == 0) {
                record.currentA.value = value;
                record.currentA.lastUpdate = currentMillis;
            } 
            else if (strcmp(key, "t1") == 0) {
                record.tempCharge.value = value;
                record.tempCharge.lastUpdate = currentMillis;
            } 
            else if (strcmp(key, "t2") == 0) {
                record.tempDischarge.value = value;
                record.tempDischarge.lastUpdate = currentMillis;
            } 
            else if (strcmp(key, "CHG") == 0) {
                record.chargerPhysicallyConnected = (atoi(valueStr) > 0);
                // 
            }
        }

        pair = strtok(NULL, ",");
    }
}