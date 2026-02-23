#include "Relays.h"

void Relays::init() {
    pinMode(PIN_RELAY_CHARGE, OUTPUT);
    pinMode(PIN_RELAY_DISCHARGE, OUTPUT);
    
    // Default to safe state (OPEN)
    setCharge(false);
    setDischarge(false);
}

void Relays::setCharge(bool state) {
    // If state is true, write HIGH. Otherwise LOW.
    digitalWrite(PIN_RELAY_CHARGE, state ? HIGH : LOW);
}

void Relays::setDischarge(bool state) {
    digitalWrite(PIN_RELAY_DISCHARGE, state ? HIGH : LOW);
}