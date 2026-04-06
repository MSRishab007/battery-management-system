#pragma once
#include <stdint.h>

// The Lean 10-State Architecture
enum SystemState {
    STATE_BOOT = 0,
    STATE_MAINTENANCE,
    STATE_IDLE,
    STATE_DEEP_SLEEP,
    STATE_DISCHARGING,
    STATE_CHARGE_RECOVERY,
    STATE_CHARGE_BULK,
    STATE_BALANCING,
    STATE_FULL_IDLE,
    STATE_FAULT
};

// Fault Bitmask
enum FaultFlags : uint32_t {
    FAULT_NONE                  = 0x00000000,
    FAULT_OVER_VOLTAGE          = 0x00000001,
    FAULT_UNDER_VOLTAGE         = 0x00000002,
    FAULT_TEMP_DELTA            = 0x00000004, 
    FAULT_OVER_TEMP             = 0x00000008,
    FAULT_STALE_DATA            = 0x00000010, 
    FAULT_CONTACTOR_WELDED      = 0x00000020
};

constexpr int NUM_CELLS = 4; 
constexpr int NUM_TEMPS = 2;

// The Granular Zero-Trust Wrapper
struct Sensor {
    float value = 0.0f;
    uint32_t lastUpdate = 0;
};

// THE MASTER RECORD
struct BmsRecord {
    // --- Inputs (Wrapped in Sensor struct) ---
    Sensor cellVoltages[NUM_CELLS];
    Sensor packVoltage;
    Sensor current;
    Sensor temperatures[NUM_TEMPS];
    
    bool chargerConnected = false;
    
    // --- State & Calculations ---
    SystemState currentState = STATE_BOOT;
    float stateOfCharge = 0.0f;
    float vMax = 0.0f;
    float vMin = 0.0f;
    uint32_t faultFlags = FAULT_NONE;
    
    // --- Outputs ---
    bool cmdChargeRelay = false;
    bool cmdDischargeRelay = false;
    bool cmdRecoveryRelay = false;       
    bool balanceEnables[NUM_CELLS] = {false}; 
};