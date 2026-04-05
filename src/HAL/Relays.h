#ifndef RELAYS_H
#define RELAYS_H

#include <Arduino.h>

class Relays {
private:
    static bool currentChargeState;
    static bool currentDischargeState;

public:
    static void init();
    static void setCharge(bool state);
    static void setDischarge(bool state);
};

#endif