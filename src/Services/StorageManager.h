#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "../../include/DataTypes.h"

class StorageManager {
private:
    static Preferences preferences;
    
    static uint32_t lastSaveTime;
    static uint8_t lastSavedFaults;
    static float lastSavedCapacity;
    
    // Save capacity every 5 minutes (300,000 ms) to prevent flash wear-out
    static const uint32_t SAVE_INTERVAL_MS = 300000; 

public:
    static void init(BmsRecord& record);
    static void update(const BmsRecord& record);
    static void clearFaults();
};

#endif