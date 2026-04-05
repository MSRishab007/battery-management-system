#include "CoulombCounter.h"
#include <Arduino.h>

uint32_t CoulombCounter::lastUpdateTimeMs = 0;

void CoulombCounter::init(BmsRecord& record) {
    lastUpdateTimeMs = millis();
    if (record.capacityRemaining_mAh >= 0.0) {
        record.stateOfCharge = (record.capacityRemaining_mAh / Config::PACK_CAPACITY_MAH) * 100.0;
        return; 
    }

    float packMaxV = Config::OVP_LIMIT * 4.0;
    float packMinV = Config::UVP_LIMIT * 4.0;
    
    if (record.packVoltage <= packMinV) {
        record.stateOfCharge = 0.0;
    } else if (record.packVoltage >= packMaxV) {
        record.stateOfCharge = 100.0;
    } else {
        record.stateOfCharge = ((record.packVoltage - packMinV) / (packMaxV - packMinV)) * 100.0;
    }

    record.capacityRemaining_mAh = (record.stateOfCharge / 100.0) * Config::PACK_CAPACITY_MAH;
}

void CoulombCounter::update(BmsRecord& record) {
    uint32_t currentTimeMs = millis();
    uint32_t deltaMs = currentTimeMs - lastUpdateTimeMs;
 
    if (deltaMs > 5000) {
        lastUpdateTimeMs = currentTimeMs;
        return;
    }

    float current = record.currentA.value;

    if (abs(current) < CURRENT_NOISE_FLOOR) {
        current = 0.0;
    }

    float delta_mAh = (current * deltaMs) / 3600.0;

    record.capacityRemaining_mAh += delta_mAh;

    if (record.capacityRemaining_mAh > Config::PACK_CAPACITY_MAH) {
        record.capacityRemaining_mAh = Config::PACK_CAPACITY_MAH;
    } else if (record.capacityRemaining_mAh < 0.0) {
        record.capacityRemaining_mAh = 0.0;
    }

    record.stateOfCharge = (record.capacityRemaining_mAh / Config::PACK_CAPACITY_MAH) * 100.0;

    lastUpdateTimeMs = currentTimeMs;
}