#pragma once
#include <stdint.h>

namespace Config {
    // =================================================================
    // 1. BATTERY HARDWARE SPECIFICATIONS
    // =================================================================
    // constexpr float PACK_CAPACITY_MAH = 5000.0f;   
    constexpr float PACK_CAPACITY_MAH = 1.0f; //Temporary 1mAh for SoC Testing
    constexpr int NUM_CELLS = 4;
    constexpr int NUM_TEMPS = 2;

    // =================================================================
    // 2. DUAL-THRESHOLD SAFETY LIMITS (SOFT vs HARD FAULTS)
    // =================================================================
    
    // --- Voltage Limits ---
    constexpr float VOLTAGE_CRIT_MAX = 4.25f;  // HARD: Instant Relay Kill (Fire risk)
    constexpr float VOLTAGE_WARN_MAX = 4.15f;  // SOFT: Stop charging, allow discharge
    
    constexpr float VOLTAGE_WARN_MIN = 3.00f;  // SOFT: Limp Mode (Limit motor power)
    constexpr float VOLTAGE_CRIT_MIN = 2.50f;  // HARD: Instant Kill (Prevent cell death)

    // --- Current Limits ---
    constexpr float CURRENT_CRIT_CHARGE    = 6.0f;   // HARD: Charger runaway
    constexpr float CURRENT_WARN_CHARGE    = 5.0f;   // SOFT: Throttle charger
    
    constexpr float CURRENT_CRIT_DISCHARGE = -30.0f; // HARD: Short circuit / Massive load
    constexpr float CURRENT_WARN_DISCHARGE = -20.0f; // SOFT: Throttle motor

    // --- Temperature Limits (Celsius) ---
    constexpr float TEMP_CRIT_MAX = 60.0f;     // HARD: Thermal runaway risk
    constexpr float TEMP_WARN_MAX = 50.0f;     // SOFT: Limp mode to cool down

    // =================================================================
    // 3. IMBALANCE & DEGRADATION (Triggers Soft Faults)
    // =================================================================
    constexpr float MAX_CELL_DELTA = 0.05f;    // 50mV difference triggers balancing/warning
    constexpr float MAX_TEMP_DELTA = 15.0f;    // 15C difference between sensors is a warning
    
    // =================================================================
    // 4. STATE MACHINE BEHAVIOR LIMITS
    // =================================================================
    constexpr float VOLTAGE_FULL = 4.20f;          // 100% SoC Calibrator
    constexpr float VOLTAGE_RECHARGE = 4.05f;      // Drop below this to restart bulk charge
    constexpr float VOLTAGE_RECOVERY_LIMIT = 3.0f; // Limit for trickle recovery
    constexpr float VOLTAGE_DEEP_SLEEP = 2.8f;     // UVLO lock-out state
    
    constexpr float TAIL_CURRENT = 0.1f;           // Current to declare "Full"
    constexpr float NOISE_FLOOR = 0.05f;           // Ignore sensor noise below 50mA

    // =================================================================
    // 5. TIMING & ZERO-TRUST (Updated for SIL Testing)
    // =================================================================
    // 15 seconds without a sensor update = FAULT_STALE_DATA
    constexpr uint32_t TIMEOUT_STALE_DATA = 15000; 

    // =================================================================
    // 6. NVS STORAGE CONFIG (ESP-IDF Native)
    // =================================================================
    constexpr bool ENABLE_NVS_STORAGE = false;     // Keep FALSE during PC SIL testing
    constexpr uint32_t SAVE_INTERVAL_MS = 60000;   // 60 seconds between SoC flash saves
}