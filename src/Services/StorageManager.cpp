#include "StorageManager.h"
#include <cmath>

#ifdef ARDUINO
#include <Arduino.h>
#include <Preferences.h>
static Preferences preferences;
#endif

uint32_t StorageManager::lastSaveTime = 0;
uint32_t StorageManager::lastSavedFaults = FAULT_NONE;
float StorageManager::lastSavedSoC = 0.0f;

#ifndef ARDUINO
uint32_t StorageManager::mockFlashFaults = FAULT_NONE;
float StorageManager::mockFlashSoC = -1.0f;
#endif

void StorageManager::init(BmsRecord &record)
{
#ifdef ARDUINO
    if (Config::ENABLE_NVS_STORAGE)
    {
        preferences.begin("bms_data", false);
        record.faultFlags = preferences.getUChar("faults", FAULT_NONE);
        record.stateOfCharge = preferences.getFloat("soc", -1.0f);
    }
    else
    {
        Serial.println("[STORAGE] NVS Writes DISABLED via Config.h");
    }
#else
    record.faultFlags = mockFlashFaults;
    record.stateOfCharge = mockFlashSoC;
#endif

    lastSavedFaults = record.faultFlags;
    lastSavedSoC = record.stateOfCharge;
    lastSaveTime = 0;
}

void StorageManager::update(const BmsRecord &record, uint32_t currentMillis)
{
    // 1. INSTANT SAVE ON FAULTS
    if (record.faultFlags != lastSavedFaults)
    {
        lastSavedFaults = record.faultFlags;

#ifdef ARDUINO
        if (Config::ENABLE_NVS_STORAGE)
        {
            preferences.putUChar("faults", lastSavedFaults);
        }
        Serial.printf("[STORAGE] Fault State updated: 0x%02X\n", lastSavedFaults);
#else
        mockFlashFaults = lastSavedFaults;
#endif
    }

    // 2. PERIODIC SAVE FOR SOC (Wear Leveling)
    if (currentMillis - lastSaveTime >= Config::SAVE_INTERVAL_MS)
    {
        // Only burn a write cycle if SoC changed by > 1%
        if (std::abs(record.stateOfCharge - lastSavedSoC) >= 1.0f)
        {
            lastSavedSoC = record.stateOfCharge;

#ifdef ARDUINO
            if (Config::ENABLE_NVS_STORAGE)
            {
                preferences.putFloat("soc", lastSavedSoC);
            }
            Serial.printf("[STORAGE] SoC saved: %.0f%%\n", lastSavedSoC);
#else
            mockFlashSoC = lastSavedSoC;
#endif
            // ONLY reset the timer after a successful save!
            lastSaveTime = currentMillis;
        }
    }
}
void StorageManager::clearFaults(BmsRecord &record)
{
    record.faultFlags = FAULT_NONE;
    lastSavedFaults = FAULT_NONE;

#ifdef ARDUINO
    if (Config::ENABLE_NVS_STORAGE)
    {
        preferences.putUChar("faults", FAULT_NONE);
    }
    Serial.println("[STORAGE] Faults cleared.");
#else
    mockFlashFaults = FAULT_NONE;
#endif
}