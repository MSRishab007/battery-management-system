#include "StorageManager.h"

Preferences StorageManager::preferences;
uint32_t StorageManager::lastSaveTime = 0;
uint8_t StorageManager::lastSavedFaults = FAULT_NONE;
float StorageManager::lastSavedCapacity = 0.0;

void StorageManager::init(BmsRecord& record) {
    // Open the NVS namespace "bms_data". The 'false' means Read/Write mode.
    preferences.begin("bms_data", false);
    
    // 1. Load Faults
    // If the key "faults" doesn't exist (first boot ever), default to FAULT_NONE (0x00)
    record.faultFlags = preferences.getUChar("faults", FAULT_NONE);
    lastSavedFaults = record.faultFlags;
    
    if (record.faultFlags != FAULT_NONE) {
        Serial.printf("[STORAGE] WARNING: Loaded active fault from memory: 0x%02X\n", record.faultFlags);
    }
    
    // 2. Load Capacity
    // Default to -1.0 so the CoulombCounter knows if it needs to guess the SoC from voltage
    record.capacityRemaining_mAh = preferences.getFloat("cap_mah", -1.0);
    lastSavedCapacity = record.capacityRemaining_mAh;
    
    lastSaveTime = millis();
    Serial.println("[STORAGE] NVS Data Loaded.");
}

void StorageManager::update(const BmsRecord& record) {
    // 1. INSTANT SAVE ON FAULTS (Safety Critical)
    // If a new fault appears, save it to flash memory immediately.
    if (record.faultFlags != lastSavedFaults) {
        preferences.putUChar("faults", record.faultFlags);
        lastSavedFaults = record.faultFlags;
        Serial.printf("[STORAGE] Fault State Saved to NVS: 0x%02X\n", lastSavedFaults);
    }
    
    // 2. PERIODIC SAVE FOR CAPACITY (Wear Leveling)
    if (millis() - lastSaveTime > SAVE_INTERVAL_MS) {
        // Only burn a write cycle if the capacity actually changed by more than 1 mAh
        if (abs(record.capacityRemaining_mAh - lastSavedCapacity) > 1.0) {
            preferences.putFloat("cap_mah", record.capacityRemaining_mAh);
            lastSavedCapacity = record.capacityRemaining_mAh;
            Serial.printf("[STORAGE] Capacity saved: %.0f mAh\n", lastSavedCapacity);
        }
        lastSaveTime = millis();
    }
}

void StorageManager::clearFaults() {
    preferences.putUChar("faults", FAULT_NONE);
    lastSavedFaults = FAULT_NONE;
    Serial.println("[STORAGE] Faults cleared from NVS memory.");
}