#include <unity.h>
#include "DataTypes.h"
#include "../../src/Core/CoulombCounter.h"

BmsRecord record;

void setUp(void) {
    record = BmsRecord(); 
}

void tearDown(void) {}

// TEST 1: Initialization Fallback (Voltage Guessing)
void test_init_from_voltage() {
    record.capacityRemaining_mAh = -1.0f; // Simulate wiped memory
    record.vMin = 3.6f; // Exactly halfway between 3.0V and 4.2V
    
    CoulombCounter::init(record);
    
    TEST_ASSERT_EQUAL_FLOAT(50.0f, record.stateOfCharge);
    TEST_ASSERT_EQUAL_FLOAT(2500.0f, record.capacityRemaining_mAh); // 50% of 5000
}

// TEST 2: The Integration Math
void test_coulomb_integration() {
    // Start perfectly full
    record.capacityRemaining_mAh = 5000.0f;
    CoulombCounter::init(record);
    
    // Tick 1: Setup the timer
    CoulombCounter::update(record, 1000); 
    
    // Simulate drawing -5.0 Amps for exactly 1 hour (3,600,000 ms)
    record.current.value = -5.0f;
    CoulombCounter::update(record, 1000 + 3600000);
    
    // Drawing 5 Amps for 1 hour should consume exactly 5000 mAh
    // The battery should now be completely dead (0 mAh)
    TEST_ASSERT_EQUAL_FLOAT(0.0f, record.capacityRemaining_mAh);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, record.stateOfCharge);
}

// TEST 3: Auto-Calibration Anchors
void test_auto_calibration() {
    record.capacityRemaining_mAh = 4800.0f; // 96%
    CoulombCounter::init(record);
    
    // Tick 1
    CoulombCounter::update(record, 1000);
    
    // Suddenly, the StateManager declares the battery is 100% full
    record.currentState = STATE_FULL_IDLE;
    CoulombCounter::update(record, 1050);
    
    // The Coulomb Counter MUST correct its drift and snap to 100% (5000 mAh)
    TEST_ASSERT_EQUAL_FLOAT(5000.0f, record.capacityRemaining_mAh);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, record.stateOfCharge);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_init_from_voltage);
    RUN_TEST(test_coulomb_integration);
    RUN_TEST(test_auto_calibration);
    return UNITY_END();
}