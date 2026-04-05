#ifndef PROTECTION_H
#define PROTECTION_H

#include "../../include/DataTypes.h"
#include "../../include/Config.h"

class Protection {
public:
    // Only does one thing: mathematically validates limits and sets faultFlags.
    static void runDiagnostics(BmsRecord& record);
};

#endif