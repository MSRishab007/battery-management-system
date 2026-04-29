#pragma once
#include "../include/DataTypes.h"
#include "driver/gpio.h"

class Relays {
public:
    static void init();
    static void update(BmsRecord& record);
};