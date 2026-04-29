#pragma once
#include "../include/DataTypes.h"
#include "../include/Config.h"

class SoCEstimator {
public:
    // Guesses initial SoC based on Boot Voltage
    static void init(BmsRecord& record);
    
    // Integrates Amps over Time (Coulomb Counting)
    static void update(BmsRecord& record);

private:
    static uint64_t lastUpdateTime;
};