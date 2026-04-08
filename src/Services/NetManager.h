#pragma once

#include "../../include/DataTypes.h"
#include <Arduino.h>

class NetManager {
public:
    static void init();
    
    static void loop(uint32_t currentMillis);
    static void publishTelemetry(const BmsRecord& record);
    
    static String getTimeString();
    
    // Helpers for the Display module
    static bool isWiFiConnected();
    static bool isMqttConnected();
};