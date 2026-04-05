#ifndef DATATYPES_H
#define DATATYPES_H

#include <stdint.h>

// States 
enum BmsState {
    STATE_BOOT = 0,
    STATE_MAINTENANCE,
    STATE_IDLE_STANDBY,
    STATE_IDLE_LIGHT,
    STATE_DEEP_SLEEP,
    STATE_PRE_CHARGE,
    STATE_DISCHARGING,
    STATE_FULL_DISCHARGE,
    STATE_CHARGE_RECOVERY,
    STATE_CHARGE_BULK,
    STATE_BALANCING,
    STATE_FULL_IDLE,
    STATE_FAULT         
};

//Fault Flags 
enum FaultFlags {
    FAULT_NONE          = 0x00,
    FAULT_CELL_OVP      = 0x01, // Over-voltage
    FAULT_CELL_UVP      = 0x02, // Under-voltage
    FAULT_OVER_CURRENT  = 0x04, // Over-current
    FAULT_OVER_TEMP     = 0x08, // Over-temperature
    FAULT_TEMP_DELTA    = 0x10, //Temperature delta between charge/discharge sensors too high
    FAULT_STALE_DATA    = 0x20  // STALE_DATA: No UART updates 
};


struct Sensor {
    float value;
    uint32_t lastUpdate;
};

// BMS MAster Record 
struct BmsRecord {
    // Inputs 
    Sensor cellVolts[4];
    Sensor currentA;
    Sensor tempCharge;
    Sensor tempDischarge;
    bool chargerPhysicallyConnected; 
    
    // Core Logic Outputs 
    BmsState currentState;
    uint8_t faultFlags;
    
    float stateOfCharge;          //Between 0-100% 
    float capacityRemaining_mAh;  
    
    // Actuators
    bool cmdChargeRelay;
    bool cmdDischargeRelay;
};

#endif