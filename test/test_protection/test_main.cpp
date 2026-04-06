#include <unity.h>
#include "DataTypes.h"
#include "Config.h"
#include "../../src/Core/Protection.h" 

// Global test record
BmsRecord record;


void setUp(void) {
    record = BmsRecord(); 
    

    for(int i=0; i<NUM_CELLS; i++) {
        record.cellVoltages[i].value = 3.8f;
        record.cellVoltages[i].lastUpdate = 1000; 
    }
    for(int i=0; i<NUM_TEMPS; i++) {
        record.temperatures[i].value = 25.0f;
        record.temperatures[i].lastUpdate = 1000;
    }
}

void tearDown(void) {

}


void test_healthy_battery_passes() {
    uint32_t simulatedTime = 2500; 
    Protection::runDiagnostics(record, simulatedTime);
    
    TEST_ASSERT_EQUAL_HEX32(FAULT_NONE, record.faultFlags);
}

void test_thermal_delta_triggers_fault() {
    uint32_t simulatedTime = 2500;
    record.temperatures[0].value = 45.0f;
    record.temperatures[1].value = 25.0f; 
    
    Protection::runDiagnostics(record, simulatedTime);
    
    TEST_ASSERT_BIT_HIGH(2, record.faultFlags); 
}

void test_stale_data_triggers_fault() {
    uint32_t simulatedTime = 6000; 
    Protection::runDiagnostics(record, simulatedTime);
    
    TEST_ASSERT_BIT_HIGH(4, record.faultFlags); 
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_healthy_battery_passes);
    RUN_TEST(test_thermal_delta_triggers_fault);
    RUN_TEST(test_stale_data_triggers_fault);
    
    return UNITY_END();
}