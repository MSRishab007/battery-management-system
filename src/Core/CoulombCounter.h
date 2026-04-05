#ifndef COULOMB_COUNTER_H
#define COULOMB_COUNTER_H

#include "../../include/DataTypes.h"
#include "../../include/Config.h"

class CoulombCounter {
private:
    static uint32_t lastUpdateTimeMs;
    static constexpr float CURRENT_NOISE_FLOOR = 0.05; 

public:
    static void init(BmsRecord& record);
    
    static void update(BmsRecord& record);
};

#endif