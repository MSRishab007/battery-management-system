#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include "DataTypes.h"
#include "Config.h"

class StateManager {
private:
    static uint32_t idleStartTime;
    
    
    static constexpr float CURRENT_NOISE_FLOOR = 0.05; // Treat anything < 50mA as 0A
    static constexpr float TAIL_CURRENT = 0.10;        // 100mA cutoff for Full Charge

public:
    
    static void init(BmsRecord& record);
    static void evaluateState(BmsRecord& record);
};

#endif