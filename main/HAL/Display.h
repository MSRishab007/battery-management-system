#pragma once
#include "../include/DataTypes.h"
#include "../include/Config.h"

class Display {
public:
    // Spawns the background FreeRTOS task for the OLED
    static void startTask(BmsRecord* record);

private:
    // The actual FreeRTOS task loop
    static void displayTask(void* pvParameters);
    
    // Internal hardware setup
    static void initI2C();
    static void initPMU();
    
    // UI Drawing
    static void drawUI(BmsRecord* record, uint32_t currentMillis);
    
    // Helper to replace Arduino String
    static const char* getStateString(SystemState state);
};