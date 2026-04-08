#include "Relays.h"

#ifdef ARDUINO
#include <Arduino.h>

constexpr int PIN_RELAY_CHARGE    = 15;
constexpr int PIN_RELAY_DISCHARGE = 2;
constexpr int PIN_RELAY_RECOVERY  = 4;
constexpr int PIN_BALANCE_CELL_1  = 16;
constexpr int PIN_BALANCE_CELL_2  = 17;
constexpr int PIN_BALANCE_CELL_3  = 18;
constexpr int PIN_BALANCE_CELL_4  = 19; 

const int BALANCE_PINS[NUM_CELLS] = {
    PIN_BALANCE_CELL_1, PIN_BALANCE_CELL_2, 
    PIN_BALANCE_CELL_3, PIN_BALANCE_CELL_4
};
#endif


#ifndef ARDUINO
bool Relays::mockChargePin = false;
bool Relays::mockDischargePin = false;
bool Relays::mockRecoveryPin = false;
bool Relays::mockBalancePins[NUM_CELLS] = {false};
#endif

void Relays::init() {
#ifdef ARDUINO
    Serial.println("   -> [RELAYS] HARDWARE AIR-GAPPED. Pins protected.");
    
    // =========================================================
    // PHYSICAL PINS DISABLED TO PREVENT USB SHORT-CIRCUIT
    // =========================================================
    /*
    pinMode(PIN_RELAY_CHARGE, OUTPUT);
    pinMode(PIN_RELAY_DISCHARGE, OUTPUT);
    pinMode(PIN_RELAY_RECOVERY, OUTPUT);
    for(int i = 0; i < NUM_CELLS; i++) {
        pinMode(BALANCE_PINS[i], OUTPUT);
    }
    digitalWrite(PIN_RELAY_CHARGE, LOW);
    digitalWrite(PIN_RELAY_DISCHARGE, LOW);
    digitalWrite(PIN_RELAY_RECOVERY, LOW);
    for(int i = 0; i < NUM_CELLS; i++) {
        digitalWrite(BALANCE_PINS[i], LOW);
    }
    */
#else
    mockChargePin = false;
    mockDischargePin = false;
    mockRecoveryPin = false;
    for(int i = 0; i < NUM_CELLS; i++) mockBalancePins[i] = false;
#endif
}

void Relays::update(const BmsRecord& record) {
#ifdef ARDUINO
    // HARDWARE COMMANDS COMMENTED OUT. 
    // We only read the variables into the void to keep the compiler happy.
    
    /*
    digitalWrite(PIN_RELAY_CHARGE, record.cmdChargeRelay ? HIGH : LOW);
    digitalWrite(PIN_RELAY_DISCHARGE, record.cmdDischargeRelay ? HIGH : LOW);
    digitalWrite(PIN_RELAY_RECOVERY, record.cmdRecoveryRelay ? HIGH : LOW);
    
    for(int i = 0; i < NUM_CELLS; i++) {
        digitalWrite(BALANCE_PINS[i], record.balanceEnables[i] ? HIGH : LOW);
    }
    */
#else
    mockChargePin = record.cmdChargeRelay;
    mockDischargePin = record.cmdDischargeRelay;
    mockRecoveryPin = record.cmdRecoveryRelay;
    for(int i = 0; i < NUM_CELLS; i++) {
        mockBalancePins[i] = record.balanceEnables[i];
    }
#endif
}