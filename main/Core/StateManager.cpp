#include "StateManager.h"
#include <cmath>
#include "esp_log.h"

static const char* TAG = "STATE_MANAGER";

void StateManager::evaluateState(BmsRecord& record) {
    // Acquire the lock before reading/modifying the record
    if (xSemaphoreTake(record.lock, pdMS_TO_TICKS(10)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire lock! Skipping state evaluation.");
        return;
    }

    // =================================================================
    // 1. HARD FAULT OVERRIDE (Brick Wall)
    // =================================================================
    if ((record.faultFlags & MASK_HARD_FAULTS) != 0 && record.currentState != STATE_MAINTENANCE) {
        if (record.currentState != STATE_FAULT) {
            ESP_LOGE(TAG, "CRITICAL FAULT DETECTED. Entering STATE_FAULT.");
        }
        record.currentState = STATE_FAULT;
        record.cmdChargeRelay = false;
        record.cmdDischargeRelay = false;
        record.cmdRecoveryRelay = false;
        for (int i = 0; i < Config::NUM_CELLS; i++) record.balanceEnables[i] = false;
        
        xSemaphoreGive(record.lock);
        return; // Halt all further state evaluation
    }

    // =================================================================
    // 2. SOFT FAULT OVERRIDE (Limp Mode)
    // =================================================================
    if ((record.faultFlags & MASK_SOFT_FAULTS) != 0 && record.currentState != STATE_MAINTENANCE) {
        if (record.currentState != STATE_LIMP_MODE) {
            ESP_LOGW(TAG, "WARNING FAULT DETECTED. Entering STATE_LIMP_MODE.");
        }
        record.currentState = STATE_LIMP_MODE;
        // In Limp Mode, we keep relays ON, but the system (or external controller) 
        // knows power is restricted based on this state.
        record.cmdChargeRelay = true;
        record.cmdDischargeRelay = true;
        record.cmdRecoveryRelay = false;
        
        xSemaphoreGive(record.lock);
        return;
    }

    // =================================================================
    // 3. DEEP SLEEP / UVLO OVERRIDE
    // =================================================================
    if (record.vMin <= Config::VOLTAGE_DEEP_SLEEP && 
        record.currentState != STATE_CHARGE_RECOVERY && 
        record.currentState != STATE_BOOT) {
        
        record.currentState = STATE_DEEP_SLEEP;
        record.cmdChargeRelay = false;
        record.cmdDischargeRelay = false;
        record.cmdRecoveryRelay = false;
        
        xSemaphoreGive(record.lock);
        return; 
    }

    // =================================================================
    // 4. NORMAL STATE MACHINE TRANSITIONS
    // =================================================================
    switch (record.currentState) {
        
        // If we were in FAULT or LIMP_MODE, and the flags were cleared (via MQTT reset),
        // gracefully return to IDLE to re-evaluate the safe state.
        case STATE_FAULT:
        case STATE_LIMP_MODE:
        case STATE_BOOT:
            if (record.faultFlags == FAULT_NONE) {
                ESP_LOGI(TAG, "Faults cleared. Returning to IDLE.");
                record.currentState = STATE_IDLE;
            }
            break;

        case STATE_IDLE:
        case STATE_FULL_IDLE:
            record.cmdChargeRelay = false;
            record.cmdDischargeRelay = false;
            record.cmdRecoveryRelay = false;
            
            if (record.chargerConnected) {
                if (record.vMin < Config::VOLTAGE_RECOVERY_LIMIT) {
                    record.currentState = STATE_CHARGE_RECOVERY;
                } else if (record.vMax < Config::VOLTAGE_RECHARGE) { 
                    record.currentState = STATE_CHARGE_BULK;
                }
            } else if (record.current.value < -Config::NOISE_FLOOR) {
                record.currentState = STATE_DISCHARGING;
            }
            break;

        case STATE_CHARGE_RECOVERY:
            record.cmdChargeRelay = false;    // Main charge relay OFF
            record.cmdRecoveryRelay = true;   // Trickle/Recovery relay ON
            
            if (!record.chargerConnected) record.currentState = STATE_IDLE;
            else if (record.vMin >= Config::VOLTAGE_RECOVERY_LIMIT) {
                record.currentState = STATE_CHARGE_BULK;
            }
            break;

        case STATE_CHARGE_BULK:
            record.cmdChargeRelay = true;
            record.cmdRecoveryRelay = false;

            if (!record.chargerConnected) record.currentState = STATE_IDLE;
            else if (record.vMax >= 4.10f && (record.vMax - record.vMin) > Config::MAX_CELL_DELTA) {
                record.currentState = STATE_BALANCING;
            }
            else if (record.vMax >= Config::VOLTAGE_FULL && std::abs(record.current.value) < Config::TAIL_CURRENT) {
                record.currentState = STATE_FULL_IDLE;
            }
            break;

        case STATE_BALANCING:
            record.cmdChargeRelay = true;
            for(int i = 0; i < Config::NUM_CELLS; i++) {
                record.balanceEnables[i] = (record.cellVoltages[i].value > (record.vMin + 0.01f));
            }

            if (!record.chargerConnected) record.currentState = STATE_IDLE;
            else if ((record.vMax - record.vMin) <= 0.01f) { 
                record.currentState = STATE_CHARGE_BULK;
            }
            break;

        case STATE_DISCHARGING:
            record.cmdDischargeRelay = true;
            if (std::abs(record.current.value) < Config::NOISE_FLOOR) record.currentState = STATE_IDLE;
            break;
            
        case STATE_DEEP_SLEEP:
            if (record.chargerConnected) record.currentState = STATE_CHARGE_RECOVERY;
            break;

        default:
            break;
    }

    xSemaphoreGive(record.lock);
}