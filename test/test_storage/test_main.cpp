#include <unity.h>
#include "DataTypes.h"
#include "../../src/Services/StorageManager.h"

BmsRecord record;

void setUp(void) {
    record = BmsRecord();
    StorageManager::mockFlashFaults = FAULT_NONE;
    StorageManager::mockFlashSoC = 50.0f; 
    StorageManager::init(record);
}

void tearDown(void) {}

void test_fault_saves_instantly() {
    record.faultFlags = FAULT_OVER_VOLTAGE;
    
    // Pass in a time of 0 (no time has passed)
    StorageManager::update(record, 0); 
    
    // The fault MUST bypass the timer and save instantly
    TEST_ASSERT_EQUAL_UINT32(FAULT_OVER_VOLTAGE, StorageManager::mockFlashFaults);
}

void test_soc_respects_wear_leveling() {
    record.stateOfCharge = 50.5f; // Changed by 0.5%
    
    // Fast forward past the 60-second timer
    StorageManager::update(record, Config::SAVE_INTERVAL_MS + 10);
    
    // It should REJECT the save because the change was < 1.0%
    TEST_ASSERT_EQUAL_FLOAT(50.0f, StorageManager::mockFlashSoC);
    
    // Now change it by 2.0% and try again
    record.stateOfCharge = 52.0f;
    StorageManager::update(record, (Config::SAVE_INTERVAL_MS * 2) + 10);
    
    // Now it should save
    TEST_ASSERT_EQUAL_FLOAT(52.0f, StorageManager::mockFlashSoC);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_fault_saves_instantly);
    RUN_TEST(test_soc_respects_wear_leveling);
    return UNITY_END();
}