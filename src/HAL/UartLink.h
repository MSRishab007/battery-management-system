#pragma once
#include <stdint.h>
#include "DataTypes.h"

class UartLink {
public:
    static constexpr int MAX_BUFFER_SIZE = 128;
    
    static void init();
    
    static void readSerial(BmsRecord& record, uint32_t currentMillis);
    
    static bool processFrame(char* frame, BmsRecord& record, uint32_t currentMillis);
    
    static uint8_t calculateChecksum(const char* payload);

private:
    static char rxBuffer[MAX_BUFFER_SIZE];
    static int rxIndex;
    static void parsePayload(char* payload, BmsRecord& record, uint32_t currentMillis);
};