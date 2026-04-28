#pragma once

#include "../include/DataTypes.h"
#include "../include/Config.h"

class Protection {
public:

    static uint32_t runDiagnostics(BmsRecord& record);
};