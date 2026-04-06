#include "Protection.h"

void Protection::runDiagnostics(BmsRecord& record, uint32_t currentMillis) {
    record.faultFlags = FAULT_NONE;

    // Zero-Trust Stale Data Check
    bool isStale = false;
    for(int i = 0; i < NUM_CELLS; i++) {
        if ((currentMillis - record.cellVoltages[i].lastUpdate) > Config::TIMEOUT_STALE_DATA) {
            isStale = true;
        }
    }
    for(int i = 0; i < NUM_TEMPS; i++) {
        if ((currentMillis - record.temperatures[i].lastUpdate) > Config::TIMEOUT_STALE_DATA) {
            isStale = true;
        }
    }
    
    if (isStale) {
        record.faultFlags |= FAULT_STALE_DATA;
    }

    // Voltage Checks & Update vMax/vMin
    record.vMax = 0.0f;
    record.vMin = 5.0f; 
    
    for(int i = 0; i < NUM_CELLS; i++) {
        float v = record.cellVoltages[i].value;
        if (v > record.vMax) record.vMax = v;
        if (v < record.vMin) record.vMin = v;
        if (v > Config::VOLTAGE_MAX_CELL) record.faultFlags |= FAULT_OVER_VOLTAGE;
        if (v < Config::VOLTAGE_MIN_CELL) record.faultFlags |= FAULT_UNDER_VOLTAGE;
    }

    // Temperature Checks & Delta Validation
    float tMax = -100.0f;
    float tMin = 100.0f;
    
    for(int i = 0; i < NUM_TEMPS; i++) {
        float t = record.temperatures[i].value;
        
        if (t > tMax) tMax = t;
        if (t < tMin) tMin = t;

        if (t > Config::TEMP_MAX_SAFE) record.faultFlags |= FAULT_OVER_TEMP;
    }

    // Compare the highest and lowest thermistors for a localized fire risk
    if ((tMax - tMin) > Config::MAX_TEMP_DELTA) {
        record.faultFlags |= FAULT_TEMP_DELTA;
    }
}