#include <Arduino.h>
#include "../include/DataTypes.h"
#include "../include/Config.h"

// Core Logic
#include "Core/Protection.h"
#include "Core/StateManager.h"
#include "Core/CoulombCounter.h"

// HAL & Services
#include "HAL/Relays.h"
#include "HAL/UartLink.h"
#include "HAL/Display.h"         
#include "Services/NetManager.h"
// #include "Services/StorageManager.h

BmsRecord systemRecord;
uint32_t lastDebugPrint = 0;
uint32_t lastMqttPublish = 0;
uint32_t lastDisplayUpdate = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Configure the physical reset button for hard faults
    pinMode(Config::PIN_BUTTON_RESET, INPUT_PULLUP);
    
    // Initialize Core Logic
    // StorageManager::init(systemRecord);
    StateManager::init(systemRecord);
    systemRecord.capacityRemaining_mAh = -1.0; 
    CoulombCounter::init(systemRecord);
    
    // Initialize Hardware & Services
    Relays::init();
    Display::init();             
    NetManager::init(); 
    
    Serial.println("BMS V2.0 Initialized.");
}

void loop() {
    uint32_t currentMillis = millis();

    // 1. READ INPUTS
    NetManager::loop();
    UartLink::processIncoming(systemRecord);

    // Manual Fault Reset (LilyGo Button)
    if (digitalRead(Config::PIN_BUTTON_RESET) == LOW) {
        Serial.println("Manual Reset: Clearing faults...");
        systemRecord.faultFlags = FAULT_NONE;
        // StorageManager::clearFaults(); 
        delay(200); 
    }

    // 2. CORE LOGIC
    // Gatekeeper -> Brain -> Analytics
    Protection::runDiagnostics(systemRecord);
    StateManager::evaluateState(systemRecord);
    CoulombCounter::update(systemRecord);

    // 3. WRITE OUTPUTS
    StorageManager::update(systemRecord);
    Relays::setCharge(systemRecord.cmdChargeRelay);
    Relays::setDischarge(systemRecord.cmdDischargeRelay);

    // Update Display every 500ms
    if (currentMillis - lastDisplayUpdate > 500) {
        lastDisplayUpdate = currentMillis;
        Display::update(systemRecord, NetManager::getTimeString(), NetManager::isWiFiConnected(), NetManager::isMqttConnected());
    }

    // Serial Debug Print every 1000ms
    if (currentMillis - lastDebugPrint > 1000) {
        lastDebugPrint = currentMillis;
        Serial.printf("State: %d | Faults: 0x%02X | SoC: %.1f%%\n", 
            systemRecord.currentState, 
            systemRecord.faultFlags, 
            systemRecord.stateOfCharge);
    }

    // MQTT Publish every 3000ms
    if (currentMillis - lastMqttPublish > 3000) {
        lastMqttPublish = currentMillis;
        NetManager::publishTelemetry(systemRecord);
    }
}