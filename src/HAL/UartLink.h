#ifndef UARTLINK_H
#define UARTLINK_H

#include <Arduino.h>
#include "../../include/DataTypes.h"

class UartLink {
public:
    static void processIncoming(BmsRecord& record);
};

#endif