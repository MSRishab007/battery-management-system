#include <unity.h>
#include "DataTypes.h"
#include "../../src/HAL/Relays.h"

BmsRecord record;

void setUp(void) {
    record = BmsRecord(); 
    Relays::init();       
}

void tearDown(void) {}

// TEST 1: The Initialization Safety Check
void test_relays_init_safely() {
    TEST_ASSERT_FALSE(Relays::mockChargePin);
    TEST_ASSERT_FALSE(Relays::mockDischargePin);
    TEST_ASSERT_FALSE(Relays::mockRecoveryPin);
    for(int i=0; i<NUM_CELLS; i++) {
        TEST_ASSERT_FALSE(Relays::mockBalancePins[i]);
    }
}

// TEST 2: Active Discharging Mapping
void test_discharge_hardware_mapping() {
    record.cmdDischargeRelay = true; 
    
    Relays::update(record);
    
    TEST_ASSERT_TRUE(Relays::mockDischargePin);  
    TEST_ASSERT_FALSE(Relays::mockChargePin);    
}

// TEST 3: Cell Balancing MOSFET Mapping
void test_balancing_hardware_mapping() {
    record.cmdChargeRelay = true;
    record.balanceEnables[0] = true;  
    record.balanceEnables[3] = true;  
    
    Relays::update(record);
    
    TEST_ASSERT_TRUE(Relays::mockChargePin);
    TEST_ASSERT_TRUE(Relays::mockBalancePins[0]);
    TEST_ASSERT_FALSE(Relays::mockBalancePins[1]);
    TEST_ASSERT_FALSE(Relays::mockBalancePins[2]);
    TEST_ASSERT_TRUE(Relays::mockBalancePins[3]);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_relays_init_safely);
    RUN_TEST(test_discharge_hardware_mapping);
    RUN_TEST(test_balancing_hardware_mapping);
    return UNITY_END();
}