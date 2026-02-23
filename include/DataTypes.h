#ifndef DATATYPES_H
#define DATATYPES_H

#include <stdint.h>

// 1. SYSTEM STATES
enum BmsState {
    STATE_BOOT,
    STATE_IDLE,
    STATE_PRE_CHARGE,
    STATE_CHARGING,
    STATE_DISCHARGING,
    STATE_FAULT_SOFT,
    STATE_FAULT_HARD
};

// 2. FAULT CODES 
enum FaultFlags {
    FAULT_NONE          = 0x00,
    FAULT_CELL_OVP      = 0x01, // Over-voltage
    FAULT_CELL_UVP      = 0x02, // Under-voltage
    FAULT_OVER_CURRENT  = 0x04,
    FAULT_OVER_TEMP     = 0x08,
    FAULT_TEMP_DELTA    = 0x10, // Charge Temp vs Discharge Temp mismatch
    FAULT_STALE_DATA    = 0x20  // Python/UART stopped sending
};

// 3. THE SENSOR OBJECT 
struct Sensor {
    float value;
    uint32_t lastUpdate;
};

// 4. THE MASTER RECORD
struct BmsRecord {
    // Inputs
    Sensor cellVolts[4];
    Sensor currentA;
    Sensor tempCharge;
    Sensor tempDischarge;
    bool chargerPhysicallyConnected;
    
    // Outputs 
    BmsState currentState;
    uint8_t faultFlags;
    float stateOfCharge; // Percentage 0-100%
    
    // Actuators
    bool cmdChargeRelay;
    bool cmdDischargeRelay;
};

#endif