#include "Protection.h"
#include "esp_timer.h" // Native ESP-IDF microsecond timer
#include "esp_log.h"   // Native ESP-IDF logging

static const char* TAG = "PROTECTION";

uint32_t Protection::runDiagnostics(BmsRecord& record) {
    uint32_t newFaults = FAULT_NONE;

    // =================================================================
    // 1. ACQUIRE THREAD-SAFE MUTEX LOCK
    // Wait up to 10 ticks for access to the Master Record
    // =================================================================
    if (xSemaphoreTake(record.lock, pdMS_TO_TICKS(10)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire BmsRecord lock! Skipping cycle.");
        return FAULT_NONE; // Can't read memory safely, abort this cycle.
    }

    // =================================================================
    // 2. MQTT MANUAL RESET REQUEST
    // =================================================================
    if (record.requestFaultReset) {
        ESP_LOGW(TAG, "Manual Fault Reset requested via MQTT.");
        record.faultFlags = FAULT_NONE; 
        record.requestFaultReset = false; // Consume the request
    }

    // Get current time in milliseconds (esp_timer is in microseconds)
    uint32_t currentMillis = (uint32_t)(esp_timer_get_time() / 1000ULL);

    // =================================================================
    // 3. ZERO-TRUST STALE DATA CHECK (Sensors offline?)
    // =================================================================
    bool isStale = false;
    for(int i = 0; i < Config::NUM_CELLS; i++) {
        if ((currentMillis - record.cellVoltages[i].lastUpdate) > Config::TIMEOUT_STALE_DATA) {
            isStale = true;
        }
    }
    for(int i = 0; i < Config::NUM_TEMPS; i++) {
        if ((currentMillis - record.temperatures[i].lastUpdate) > Config::TIMEOUT_STALE_DATA) {
            isStale = true;
        }
    }
    
    if (isStale) newFaults |= FAULT_STALE_DATA;

    // =================================================================
    // 4. VOLTAGE CHECKS (Dual Threshold)
    // =================================================================
    record.vMax = 0.0f;
    record.vMin = 5.0f; 
    
    for(int i = 0; i < Config::NUM_CELLS; i++) {
        float v = record.cellVoltages[i].value;
        
        if (v > record.vMax) record.vMax = v;
        if (v < record.vMin) record.vMin = v;

        // Over-Voltage
        if (v > Config::VOLTAGE_CRIT_MAX)      newFaults |= FAULT_CRIT_OVER_VOLTAGE;
        else if (v > Config::VOLTAGE_WARN_MAX) newFaults |= FAULT_WARN_OVER_VOLTAGE;
        
        // Under-Voltage
        if (v < Config::VOLTAGE_CRIT_MIN)      newFaults |= FAULT_CRIT_UNDER_VOLTAGE;
        else if (v < Config::VOLTAGE_WARN_MIN) newFaults |= FAULT_WARN_UNDER_VOLTAGE;
    }

    // =================================================================
    // 5. CURRENT CHECKS (Dual Threshold)
    // Positive = Charge, Negative = Discharge
    // =================================================================
    float current = record.current.value;
    
    if (current > Config::CURRENT_CRIT_CHARGE || current < Config::CURRENT_CRIT_DISCHARGE) {
        newFaults |= FAULT_CRIT_OVER_CURRENT;
    } 
    else if (current > Config::CURRENT_WARN_CHARGE || current < Config::CURRENT_WARN_DISCHARGE) {
        newFaults |= FAULT_WARN_OVER_CURRENT;
    }

    // =================================================================
    // 6. TEMPERATURE CHECKS & DELTA (Dual Threshold)
    // =================================================================
    float tMax = -100.0f;
    float tMin = 100.0f;
    
    for(int i = 0; i < Config::NUM_TEMPS; i++) {
        float t = record.temperatures[i].value;
        
        if (t > tMax) tMax = t;
        if (t < tMin) tMin = t;

        if (t > Config::TEMP_CRIT_MAX)      newFaults |= FAULT_CRIT_OVER_TEMP;
        else if (t > Config::TEMP_WARN_MAX) newFaults |= FAULT_WARN_OVER_TEMP;
    }

    // Localized fire/degradation check
    if ((tMax - tMin) > Config::MAX_TEMP_DELTA) {
        newFaults |= FAULT_TEMP_DELTA;
    }

    // =================================================================
    // 7. LATCH NEW FAULTS & RELEASE LOCK
    // =================================================================
    
    // We bitwise OR the new faults so that we don't accidentally erase 
    // a hardware latched fault (like FAULT_CONTACTOR_WELDED) which 
    // might be set by the Relays module elsewhere.
    record.faultFlags |= newFaults;

    // VERY IMPORTANT: Give the lock back so other tasks can run!
    xSemaphoreGive(record.lock);

    return newFaults;
}