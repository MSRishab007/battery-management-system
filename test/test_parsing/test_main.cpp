#include <unity.h>
#include <string.h>
#include <stdio.h> 
#include "DataTypes.h"
#include "../../src/HAL/UartLink.h"

BmsRecord record;

void setUp(void) {
    record = BmsRecord(); // Reset record
}
void tearDown(void) {}

void buildFrame(char* buffer, const char* payload) {
    uint8_t chk = UartLink::calculateChecksum(payload);
    sprintf(buffer, "%s*%02X", payload, chk);
}

// TEST 1: Valid Checksum + Full Frame
void test_valid_frame_parsing() {
    char validFrame[50];
    buildFrame(validFrame, "v1=3.5,v2=3.6"); 
    uint32_t mockTime = 1000;

    bool success = UartLink::processFrame(validFrame, record, mockTime);

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_FLOAT(3.5f, record.cellVoltages[0].value);
    TEST_ASSERT_EQUAL_FLOAT(3.6f, record.cellVoltages[1].value);
    TEST_ASSERT_EQUAL_UINT32(1000, record.cellVoltages[0].lastUpdate);
}

// TEST 2: Invalid Checksum Rejection
void test_invalid_checksum_rejected() {
    char badFrame[] = "v1=3.5,v2=3.6*FF"; 
    uint32_t mockTime = 2000;

    bool success = UartLink::processFrame(badFrame, record, mockTime);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, record.cellVoltages[0].value); 
}

// TEST 3: Partial Frame Updates 
void test_partial_frame_updates() {
    char partialFrame[50];
    buildFrame(partialFrame, "v3=4.1,c=-1.5"); 
    uint32_t mockTime = 3000;

    UartLink::processFrame(partialFrame, record, mockTime);

    TEST_ASSERT_EQUAL_FLOAT(4.1f, record.cellVoltages[2].value);
    TEST_ASSERT_EQUAL_FLOAT(-1.5f, record.current.value);
    TEST_ASSERT_EQUAL_UINT32(0, record.cellVoltages[0].lastUpdate);
    TEST_ASSERT_EQUAL_UINT32(3000, record.cellVoltages[2].lastUpdate);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_frame_parsing);
    RUN_TEST(test_invalid_checksum_rejected);
    RUN_TEST(test_partial_frame_updates);
    return UNITY_END();
}