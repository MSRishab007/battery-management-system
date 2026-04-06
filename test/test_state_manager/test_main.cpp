#include <unity.h>
#include "DataTypes.h"
#include "Config.h"
#include "../../src/Core/StateManager.h"

BmsRecord record;

void setUp(void) {
    record = BmsRecord(); // Reset 
    record.currentState = STATE_IDLE; 
    for(int i=0; i<NUM_CELLS; i++) {
        record.cellVoltages[i].value = 3.8f;
    }
    record.vMax = 3.8f;
    record.vMin = 3.8f;
    record.current.value = 0.0f;
    record.chargerConnected = false;
    record.faultFlags = FAULT_NONE;
}

void tearDown(void) {}

// TEST 1: The Standard Charge Trigger
void test_idle_to_charge_bulk() {
    record.chargerConnected = true;
    StateManager::evaluateState(record); 
    TEST_ASSERT_EQUAL(STATE_CHARGE_BULK, record.currentState);
    StateManager::evaluateState(record); 
    TEST_ASSERT_TRUE(record.cmdChargeRelay); 
}
// TEST 2: The Tail Current Termination 
void test_charge_bulk_to_full_idle() {
    record.currentState = STATE_CHARGE_BULK;
    record.chargerConnected = true;
    for(int i=0; i<NUM_CELLS; i++) record.cellVoltages[i].value = 4.20f;
    record.vMax = 4.20f; 
    record.vMin = 4.20f; 
    record.current.value = 0.05f; 
    
    StateManager::evaluateState(record);
    
    TEST_ASSERT_EQUAL(STATE_FULL_IDLE, record.currentState);
    
    StateManager::evaluateState(record); 
    TEST_ASSERT_FALSE(record.cmdChargeRelay);
}

// TEST 3: Under-Voltage Lockout (The Self-Preservation Drop)
void test_idle_to_deep_sleep() {
    // Battery is sitting idle, but self-discharge pulls a cell down to 2.7V
    record.vMin = 2.7f; 
    
    StateManager::evaluateState(record);
    
    // The system must lock into DEEP_SLEEP to prevent destroying the chemistry
    TEST_ASSERT_EQUAL(STATE_DEEP_SLEEP, record.currentState);
}
// TEST 4: Recovery Success 
void test_recovery_to_bulk() {
    record.currentState = STATE_CHARGE_RECOVERY;
    record.chargerConnected = true;
    
    for(int i=0; i<NUM_CELLS; i++) record.cellVoltages[i].value = 3.1f;
    record.vMin = 3.1f;

    StateManager::evaluateState(record);
    TEST_ASSERT_EQUAL(STATE_CHARGE_BULK, record.currentState);
    
    StateManager::evaluateState(record);
    TEST_ASSERT_TRUE(record.cmdChargeRelay);    
    TEST_ASSERT_FALSE(record.cmdRecoveryRelay);
}

// TEST 5: Balancing Trigger (Cell Drift Detected)
void test_bulk_to_balancing() {
    record.currentState = STATE_CHARGE_BULK;
    record.chargerConnected = true;
    
    // Near full (4.15V), but Cell 0 is 4.15V and Cell 1 is 4.08V (Delta > 50mV)
    record.cellVoltages[0].value = 4.15f;
    record.cellVoltages[1].value = 4.08f;
    record.vMax = 4.15f;
    record.vMin = 4.08f;

    StateManager::evaluateState(record);
    
    TEST_ASSERT_EQUAL(STATE_BALANCING, record.currentState);
    // In balancing, we keep charging but enable bleed resistors
    TEST_ASSERT_TRUE(record.cmdChargeRelay);
}

// TEST 6: Balancing Resolution 
void test_balancing_to_full_idle() {
    record.currentState = STATE_BALANCING;
    record.chargerConnected = true;
    
    for(int i=0; i<NUM_CELLS; i++) record.cellVoltages[i].value = 4.20f;
    record.vMax = 4.20f;
    record.vMin = 4.20f;
    record.current.value = 0.05f; 
    StateManager::evaluateState(record);
    TEST_ASSERT_EQUAL(STATE_CHARGE_BULK, record.currentState);
    StateManager::evaluateState(record);
    TEST_ASSERT_EQUAL(STATE_FULL_IDLE, record.currentState);
    
    StateManager::evaluateState(record);
    TEST_ASSERT_FALSE(record.cmdChargeRelay);
} 
// TEST 7: Load Detection (Idle to Discharging)
void test_idle_to_discharging() {
    record.currentState = STATE_IDLE;
    
    // Simulate user turning on a motor (-5.0 Amps)
    record.current.value = -5.0f; 

    // Tick 1: Detect load
    StateManager::evaluateState(record);
    TEST_ASSERT_EQUAL(STATE_DISCHARGING, record.currentState);
    
    // Tick 2: Actuate Hardware
    StateManager::evaluateState(record);
    TEST_ASSERT_TRUE(record.cmdDischargeRelay);
    TEST_ASSERT_FALSE(record.cmdChargeRelay);
}

// TEST 8: Load Removed (Discharging to Idle)
void test_discharging_to_idle() {
    record.currentState = STATE_DISCHARGING;
    record.cmdDischargeRelay = true; // System is currently running the load
    
    // Simulate user turning off the motor (Current drops to 0.02A, which is < Noise Floor)
    record.current.value = 0.02f; 

    StateManager::evaluateState(record);
    
    TEST_ASSERT_EQUAL(STATE_IDLE, record.currentState);
}

// TEST 9: Charger Abruptly Disconnected
void test_bulk_to_idle_on_disconnect() {
    record.currentState = STATE_CHARGE_BULK;
    record.cmdChargeRelay = true;
    
    // User unplugs the charger mid-charge
    record.chargerConnected = false; 

    StateManager::evaluateState(record);
    
    TEST_ASSERT_EQUAL(STATE_IDLE, record.currentState);
}

// TEST 10: Universal Fault Override
void test_fault_priority_override() {
    record.currentState = STATE_CHARGE_BULK;
    record.cmdChargeRelay = true;
    
    // Simulate the ProtectionEngine suddenly flagging an over-temp fault
    record.faultFlags = FAULT_OVER_TEMP; 

    StateManager::evaluateState(record);
    
    // The StateManager MUST immediately abort the charge and lock down
    TEST_ASSERT_EQUAL(STATE_FAULT, record.currentState);
    TEST_ASSERT_FALSE(record.cmdChargeRelay);
    TEST_ASSERT_FALSE(record.cmdDischargeRelay);
}
int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_idle_to_charge_bulk);
    RUN_TEST(test_charge_bulk_to_full_idle);
    RUN_TEST(test_idle_to_deep_sleep);
    RUN_TEST(test_recovery_to_bulk);
    RUN_TEST(test_bulk_to_balancing);
    RUN_TEST(test_balancing_to_full_idle);
    RUN_TEST(test_idle_to_discharging);
    RUN_TEST(test_discharging_to_idle);
    RUN_TEST(test_bulk_to_idle_on_disconnect);
    RUN_TEST(test_fault_priority_override);
    
    return UNITY_END();
}