#include "Protection.h"
#include <Arduino.h>
#include <math.h>

void Protection::runDiagnostics(BmsRecord& record) {
    uint8_t currentFaults = FAULT_NONE;
    uint32_t currentTimeMs = millis();

    for(int i = 0; i < 4; i++) {
        if ((currentTimeMs - record.cellVolts[i].lastUpdate) > Config::TIME_STALE_DATA) {
            currentFaults |= FAULT_STALE_DATA;
        }
    }

    for(int i = 0; i < 4; i++) {
        if (record.cellVolts[i].value >= Config::OVP_LIMIT) currentFaults |= FAULT_CELL_OVP;
        if (record.cellVolts[i].value <= Config::UVP_LIMIT) currentFaults |= FAULT_CELL_UVP;
    }

    float tempDiff = fabs(record.tempCharge.value - record.tempDischarge.value);
    if (tempDiff >= Config::MAX_DELTA_T) {
        currentFaults |= FAULT_TEMP_DELTA;
    }
    
    if (record.tempCharge.value >= Config::OTP_LIMIT || record.tempDischarge.value >= Config::OTP_LIMIT) {
        currentFaults |= FAULT_OVER_TEMP;
    }

    record.faultFlags = currentFaults;
}