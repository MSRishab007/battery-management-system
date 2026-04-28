#pragma once
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"


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
    STATE_LIMP_MODE,          
    STATE_FAULT               
};

enum FaultFlags : uint32_t {
    FAULT_NONE                = 0x00000000,
    
    // --- WARNINGS (Soft Faults -> Triggers LIMP_MODE) ---
    FAULT_WARN_OVER_VOLTAGE   = (1 << 0),  // 0x01
    FAULT_WARN_UNDER_VOLTAGE  = (1 << 1),  // 0x02
    FAULT_WARN_OVER_TEMP      = (1 << 2),  // 0x04
    FAULT_WARN_OVER_CURRENT   = (1 << 3),  // 0x08
    FAULT_TEMP_DELTA          = (1 << 4),  // 0x10
    FAULT_STALE_DATA          = (1 << 5),  // 0x20

    // --- CRITICALS (Hard Faults -> Triggers FAULT / Instant Kill) ---
    FAULT_CRIT_OVER_VOLTAGE   = (1 << 6),  // 0x40
    FAULT_CRIT_UNDER_VOLTAGE  = (1 << 7),  // 0x80
    FAULT_CRIT_OVER_TEMP      = (1 << 8),  // 0x100
    FAULT_CRIT_OVER_CURRENT   = (1 << 9),  // 0x200
    FAULT_CONTACTOR_WELDED    = (1 << 10)  // 0x400
};

// RTOS Helper Masks for quick grouping in the State Machine
constexpr uint32_t MASK_SOFT_FAULTS = (FAULT_WARN_OVER_VOLTAGE | FAULT_WARN_UNDER_VOLTAGE | 
                                       FAULT_WARN_OVER_TEMP | FAULT_WARN_OVER_CURRENT | 
                                       FAULT_TEMP_DELTA | FAULT_STALE_DATA);

constexpr uint32_t MASK_HARD_FAULTS = (FAULT_CRIT_OVER_VOLTAGE | FAULT_CRIT_UNDER_VOLTAGE | 
                                       FAULT_CRIT_OVER_TEMP | FAULT_CRIT_OVER_CURRENT | 
                                       FAULT_CONTACTOR_WELDED);

constexpr int NUM_CELLS = 4; 
constexpr int NUM_TEMPS = 2;

// The Granular Zero-Trust Wrapper
struct Sensor {
    float value = 0.0f;
    uint32_t lastUpdate = 0; // Will use esp_timer_get_time() in IDF
};

// THE MASTER RECORD (Now Thread-Safe)
struct BmsRecord {
    // --- RTOS MUTEX ---
    // Any FreeRTOS Task must take this lock before reading/writing!
    SemaphoreHandle_t lock;

    // --- Inputs (Wrapped in Sensor struct) ---
    Sensor cellVoltages[NUM_CELLS];
    Sensor packVoltage;
    Sensor current;
    Sensor temperatures[NUM_TEMPS];
    
    bool chargerConnected = false;
    bool requestFaultReset = false; // <-- NEW: MQTT manual reset flag
    
    // --- State & Calculations ---
    SystemState currentState = STATE_BOOT;
    float stateOfCharge = 0.0f;
    float vMax = 0.0f;
    float vMin = 0.0f;
    uint32_t faultFlags = FAULT_NONE; // Holds our active bitmasks
    
    // --- Outputs ---
    bool cmdChargeRelay = false;
    bool cmdDischargeRelay = false;
    bool cmdRecoveryRelay = false;       
    bool balanceEnables[NUM_CELLS] = {false}; 

    float capacityRemaining_mAh = -1.0f; 
};