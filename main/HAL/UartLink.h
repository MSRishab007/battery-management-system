#pragma once
#include "../include/DataTypes.h"
#include "../include/Config.h"
#include "driver/usb_serial_jtag.h"
class UartLink {
public:
    // Spawns the background FreeRTOS task to listen to the Python script
    static void startTask(BmsRecord* record);

private:
    static const int MAX_BUFFER_SIZE = 256;
    static char rxBuffer[MAX_BUFFER_SIZE];
    static int rxIndex;

    // The actual FreeRTOS task loop
    static void uartTask(void* pvParameters);
    
    // Processing functions
    static bool processFrame(char* frame, BmsRecord* record, uint32_t currentMillis);
    static uint8_t calculateChecksum(const char* payload);
    static void parsePayload(char* payload, BmsRecord* record, uint32_t currentMillis);
};