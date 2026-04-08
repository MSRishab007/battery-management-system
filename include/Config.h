#pragma once
#include <stdint.h>

namespace Config {
    // Battery Chemistry Limits
    constexpr float VOLTAGE_MAX_CELL = 4.25f;      
    constexpr float VOLTAGE_FULL = 4.20f;          
    constexpr float VOLTAGE_RECHARGE = 4.05f;      
    constexpr float VOLTAGE_RECOVERY_LIMIT = 3.0f; 
    constexpr float VOLTAGE_DEEP_SLEEP = 2.8f;     
    constexpr float VOLTAGE_MIN_CELL = 2.5f;    
    constexpr float PACK_CAPACITY_MAH = 5000.0f;   
    
    // Balancing & Deltas
    constexpr float MAX_CELL_DELTA = 0.05f;        
    constexpr float MAX_TEMP_DELTA = 15.0f;        
    constexpr float TEMP_MAX_SAFE = 60.0f;         
    
    // Current Limits
    constexpr float CURRENT_MAX_CHARGE = 5.0f;     
    constexpr float CURRENT_MAX_DISCHARGE = -20.0f;
    constexpr float TAIL_CURRENT = 0.1f;           
    constexpr float NOISE_FLOOR = 0.05f;           
    
    // Timing & Zero-Trust
    constexpr uint32_t TIMEOUT_STALE_DATA = 3000;  // 3 seconds without an update = FAULT

    // NVS Storage Config
    constexpr bool ENABLE_NVS_STORAGE = false;     // Keep FALSE during testing
    constexpr uint32_t SAVE_INTERVAL_MS = 60000;   // 60 seconds between SoC saves
}