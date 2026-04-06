#pragma once

#include "DataTypes.h"
#include "Config.h"

class Protection {
public:
    static void runDiagnostics(BmsRecord& record, uint32_t currentMillis);
};