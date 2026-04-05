#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "../../include/DataTypes.h"

class Display {
public:
    static void init();
    static void update(const BmsRecord& record, String timeStr, bool isWiFiConnected, bool isMqttConnected);
};

#endif