#ifndef PROTECTION_H
#define PROTECTION_H

#include "../include/DataTypes.h"
#include "../include/Config.h"
#include <math.h>

class Protection {
public:
    static uint8_t runDiagnostics(BmsRecord& record, uint32_t currentTimeMs);
    
    static BmsState evaluateState(BmsRecord& record);
};

#endif