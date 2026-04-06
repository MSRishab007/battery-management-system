#pragma once
#include "../../include/DataTypes.h"

class Relays {
public:
    static void init();
    
    static void update(const BmsRecord& record);


#ifndef ARDUINO
    static bool mockChargePin;
    static bool mockDischargePin;
    static bool mockRecoveryPin;
    static bool mockBalancePins[NUM_CELLS];
#endif
};