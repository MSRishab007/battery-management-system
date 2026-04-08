#pragma once
#include "DataTypes.h"
#include "Config.h"
#include <stdint.h>

class CoulombCounter {
public:
    static void init(BmsRecord& record);
    
    static void update(BmsRecord& record, uint32_t currentMillis);

private:
    static uint32_t lastUpdateMillis;
    static float estimateSocFromVoltage(float vMin);
};