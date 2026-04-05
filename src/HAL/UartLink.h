#ifndef UARTLINK_H
#define UARTLINK_H

#include "../../include/DataTypes.h"
#include "../../include/Config.h"

class UartLink {
private:
    static const int MAX_BUFFER_SIZE = 128;
    static char rxBuffer[MAX_BUFFER_SIZE];
    static int rxIndex;

    static uint8_t calculateChecksum(const char* payload);
    static void parsePayload(char* payload, BmsRecord& record);

public:
    static void init();
    static void processIncoming(BmsRecord& record);
};

#endif