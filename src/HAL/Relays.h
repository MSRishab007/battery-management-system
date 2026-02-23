#ifndef RELAYS_H
#define RELAYS_H

#include <Arduino.h>
#include "../include/Config.h"

class Relays {
public:
    static void init();
    static void setCharge(bool state);
    static void setDischarge(bool state);
};

#endif