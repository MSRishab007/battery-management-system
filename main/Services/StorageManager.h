#pragma once

#include "../../include/DataTypes.h"
#include "../../include/Config.h"
#include <stdint.h>

class StorageManager {
public:
    static void init(BmsRecord& record);
    static void update(const BmsRecord& record, uint32_t currentMillis);
    static void clearFaults(BmsRecord& record);

private:
    static uint32_t lastSaveTime;
    static uint32_t lastSavedFaults;
    static float lastSavedSoC;


#ifndef ARDUINO
public:
    static uint32_t mockFlashFaults;
    static float mockFlashSoC;
#endif
};