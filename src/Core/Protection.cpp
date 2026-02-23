#include "Protection.h"

uint8_t Protection::runDiagnostics(BmsRecord& record, uint32_t currentTimeMs) {
    uint8_t currentFaults = FAULT_NONE;

    // 1. STALE DATA CHECK (Watchdog)
    // If any cell hasn't reported in 3 seconds, trigger a fault
    for(int i = 0; i < 4; i++) {
        if ((currentTimeMs - record.cellVolts[i].lastUpdate) > TIMEOUT_SENSOR_MS) {
            currentFaults |= FAULT_STALE_DATA;
        }
    }

    // 2. VOLTAGE LIMITS (OVP / UVP)
    for(int i = 0; i < 4; i++) {
        if (record.cellVolts[i].value >= LIMIT_OVP) currentFaults |= FAULT_CELL_OVP;
        if (record.cellVolts[i].value <= LIMIT_UVP) currentFaults |= FAULT_CELL_UVP;
    }

    // 3. TEMPERATURE DELTA LOGIC
    float tempDiff = fabs(record.tempCharge.value - record.tempDischarge.value);
    if (tempDiff >= LIMIT_TEMP_DELTA) {
        currentFaults |= FAULT_TEMP_DELTA;
    }
    
    // 4. ABSOLUTE TEMPERATURE
    if (record.tempCharge.value >= LIMIT_TEMP_MAX || record.tempDischarge.value >= LIMIT_TEMP_MAX) {
        currentFaults |= FAULT_OVER_TEMP;
    }

    record.faultFlags = currentFaults;
    return currentFaults;
}

BmsState Protection::evaluateState(BmsRecord& record) {
    // HARD FAULT OVERRIDE: If any severe fault exists, lock down immediately
    if (record.faultFlags > 0) {
        record.cmdChargeRelay = false;
        record.cmdDischargeRelay = false;
        return STATE_FAULT_HARD;
    }

    // NORMAL OPERATION LOGIC
    if (record.chargerPhysicallyConnected) {
        record.cmdChargeRelay = true;
        record.cmdDischargeRelay = true;
        return STATE_CHARGING;
    } else {
        record.cmdChargeRelay = false;
        record.cmdDischargeRelay = true; 
        return STATE_DISCHARGING;
    }
}