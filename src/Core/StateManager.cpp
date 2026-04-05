#include "StateManager.h"

uint32_t StateManager::idleStartTime = 0;

void StateManager::init(BmsRecord& record) {
    record.currentState = STATE_BOOT;
    idleStartTime = 0;
}

void StateManager::evaluateState(BmsRecord& record) {
    if (record.faultFlags != FAULT_NONE) {
        record.currentState = STATE_FAULT;
        record.cmdChargeRelay = false;
        record.cmdDischargeRelay = false;
        return; 
    }

    float vMax = record.cellVolts[0].value; 
    float current = record.currentA.value;
    bool isCurrentZero = (abs(current) < CURRENT_NOISE_FLOOR);

    switch (record.currentState) {
        
        case STATE_BOOT:
            record.currentState = STATE_IDLE_STANDBY;
            idleStartTime = millis();
            break;

        case STATE_IDLE_STANDBY:
            record.cmdChargeRelay = false;
            record.cmdDischargeRelay = false;

            if (!isCurrentZero && current < 0) {
                record.currentState = STATE_PRE_CHARGE; 
            } 
            else if (record.chargerPhysicallyConnected) {
                record.currentState = STATE_CHARGE_BULK;
            }
            else if (isCurrentZero && (millis() - idleStartTime > Config::TIME_STANDBY_TO_LIGHT)) {
                record.currentState = STATE_IDLE_LIGHT;
            }
            break;

        case STATE_IDLE_LIGHT:
            if (record.chargerPhysicallyConnected || !isCurrentZero) {
                record.currentState = STATE_IDLE_STANDBY;
                idleStartTime = millis();
            }
            break;

        case STATE_PRE_CHARGE:
            record.currentState = STATE_DISCHARGING;
            break;

        case STATE_DISCHARGING:
            record.cmdChargeRelay = false;
            record.cmdDischargeRelay = true;

            if (isCurrentZero) {
                record.currentState = STATE_IDLE_STANDBY;
                idleStartTime = millis();
            }
            if (vMax <= Config::VOLTAGE_DEEP_SLEEP) {
                record.currentState = STATE_DEEP_SLEEP;
            }
            break;

        case STATE_CHARGE_BULK:
            record.cmdChargeRelay = true;
            record.cmdDischargeRelay = (!isCurrentZero && current < 0);

            if (vMax >= Config::VOLTAGE_FULL && abs(current) < TAIL_CURRENT) {
                record.currentState = STATE_FULL_IDLE;
            }
            break;

        case STATE_FULL_IDLE:
            record.cmdChargeRelay = false;
            record.cmdDischargeRelay = false;

            if (vMax <= Config::VOLTAGE_RECHARGE) {
                record.currentState = STATE_CHARGE_BULK;
            }
            else if (!isCurrentZero && current < 0) {
                record.currentState = STATE_FULL_DISCHARGE;
            }
            break;

        case STATE_FULL_DISCHARGE:
            record.cmdChargeRelay = false;
            record.cmdDischargeRelay = true;

            if (isCurrentZero) {
                record.currentState = STATE_FULL_IDLE;
            }
            break;

        case STATE_DEEP_SLEEP:
            record.cmdChargeRelay = false;
            record.cmdDischargeRelay = false;
            if (record.chargerPhysicallyConnected) {
                record.currentState = STATE_IDLE_STANDBY;
            }
            break;

        case STATE_FAULT:
            // Handled at the top. Stuck here until human presses reset.
            break;
            
        default:
            record.currentState = STATE_IDLE_STANDBY;
            break;
    }
    
    if (record.currentState != STATE_IDLE_STANDBY) {
        idleStartTime = millis();
    }
}