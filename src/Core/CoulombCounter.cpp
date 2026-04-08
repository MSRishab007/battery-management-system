#include "CoulombCounter.h"

uint32_t CoulombCounter::lastUpdateMillis = 0;

void CoulombCounter::init(BmsRecord& record) {
    if (record.stateOfCharge >= 0.0f) {
        record.capacityRemaining_mAh = (record.stateOfCharge / 100.0f) * Config::PACK_CAPACITY_MAH;
    } else {
        record.capacityRemaining_mAh = -1.0f;
    }
    
    lastUpdateMillis = 0;
}

void CoulombCounter::update(BmsRecord& record, uint32_t currentMillis) {
    if (lastUpdateMillis == 0) {
        lastUpdateMillis = currentMillis;
        return; 
    }

    uint32_t deltaMs = currentMillis - lastUpdateMillis;
    lastUpdateMillis = currentMillis;

    if (record.capacityRemaining_mAh < 0.0f) {
        if (record.vMin > 1.0f) {
            record.stateOfCharge = estimateSocFromVoltage(record.vMin);
            record.capacityRemaining_mAh = (record.stateOfCharge / 100.0f) * Config::PACK_CAPACITY_MAH;
        } else {
            return; 
        }
    }

    float delta_mAh = (record.current.value * deltaMs) / 3600.0f;
    record.capacityRemaining_mAh += delta_mAh;
    record.stateOfCharge = (record.capacityRemaining_mAh / Config::PACK_CAPACITY_MAH) * 100.0f;

    if (record.currentState == STATE_FULL_IDLE) {
        record.capacityRemaining_mAh = Config::PACK_CAPACITY_MAH;
        record.stateOfCharge = 100.0f;
    }
    
    if (record.currentState == STATE_DEEP_SLEEP) {
        record.capacityRemaining_mAh = 0.0f;
        record.stateOfCharge = 0.0f;
    }

    if (record.stateOfCharge > 100.0f) record.stateOfCharge = 100.0f;
    if (record.stateOfCharge < 0.0f) record.stateOfCharge = 0.0f;
}

float CoulombCounter::estimateSocFromVoltage(float vMin) {
    if (vMin >= Config::VOLTAGE_FULL) return 100.0f;
    if (vMin <= Config::VOLTAGE_DEEP_SLEEP) return 0.0f;
    
    float soc = ((vMin - Config::VOLTAGE_RECOVERY_LIMIT) / 
                 (Config::VOLTAGE_FULL - Config::VOLTAGE_RECOVERY_LIMIT)) * 100.0f;
                 
    if (soc < 0.0f) soc = 0.0f;
    return soc;
}