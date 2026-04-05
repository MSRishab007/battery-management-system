#include "Relays.h"
#include "../../include/Config.h" 

bool Relays::currentChargeState = false;
bool Relays::currentDischargeState = false;

void Relays::init() {
    pinMode(Config::PIN_RELAY_CHARGE, OUTPUT);
    pinMode(Config::PIN_RELAY_DISCHARGE, OUTPUT);
    
    digitalWrite(Config::PIN_RELAY_CHARGE, LOW);
    digitalWrite(Config::PIN_RELAY_DISCHARGE, LOW);
    
    currentChargeState = false;
    currentDischargeState = false;
}

void Relays::setCharge(bool state) {
    if (currentChargeState != state) {
        digitalWrite(Config::PIN_RELAY_CHARGE, state ? HIGH : LOW);
        currentChargeState = state; 
    }
}

void Relays::setDischarge(bool state) {
    if (currentDischargeState != state) {
        digitalWrite(Config::PIN_RELAY_DISCHARGE, state ? HIGH : LOW);
        currentDischargeState = state;
    }
}