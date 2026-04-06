#include "StateManager.h"
#include <cmath>

void StateManager::evaluateState(BmsRecord& record) {
    
    // Safety & Fault Overrides 
    if (record.faultFlags != FAULT_NONE && record.currentState != STATE_MAINTENANCE) {
        record.currentState = STATE_FAULT;
        record.cmdChargeRelay = false;
        record.cmdDischargeRelay = false;
        record.cmdRecoveryRelay = false;
        return;
    }

    if (record.vMin <= Config::VOLTAGE_DEEP_SLEEP && 
        record.currentState != STATE_CHARGE_RECOVERY && 
        record.currentState != STATE_BOOT) {
        record.currentState = STATE_DEEP_SLEEP;
        record.cmdChargeRelay = false;
        record.cmdDischargeRelay = false;
        record.cmdRecoveryRelay = false;
        return; 
    }

    //  State Machine Transitions & Actions
    switch (record.currentState) {
        
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
            for(int i = 0; i < NUM_CELLS; i++) {
                record.balanceEnables[i] = (record.cellVoltages[i].value > (record.vMin + 0.01f));
            }

            if (!record.chargerConnected) record.currentState = STATE_IDLE;
            else if ((record.vMax - record.vMin) <= 0.01f) { // Balanced to within 10mV
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
}