#pragma once
#include <Arduino.h>
#include "../../include/DataTypes.h"

class Display
{
public:
    static void init();

    static void update(const BmsRecord &record, const String &timeStr,
                       bool isWiFiConnected, bool isMqttConnected,
                       uint32_t currentMillis);
};