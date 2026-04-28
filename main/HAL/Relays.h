#pragma once

#include "../../include/DataTypes.h"
#include <Arduino.h>

class Relays {
public:
    static void init();
    static void update(const BmsRecord& record);
};