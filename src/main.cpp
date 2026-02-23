#include <Arduino.h>
#include "../include/DataTypes.h"
#include "../include/Config.h"
#include "Core/Protection.h"
#include "HAL/Relays.h"
#include "HAL/UartLink.h"
#include "HAL/Display.h"         
#include "Services/NetManager.h"

BmsRecord systemRecord;
uint32_t lastDebugPrint = 0;
uint32_t lastMqttPublish = 0;
uint32_t lastDisplayUpdate = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Relays::init();
    Display::init();             
    NetManager::init(); 
    
    Serial.println("BMS V2.0 Initialized.");
}

void loop() {
    uint32_t currentMillis = millis();

    NetManager::loop();
    UartLink::processIncoming(systemRecord);

    Protection::runDiagnostics(systemRecord, currentMillis);
    systemRecord.currentState = Protection::evaluateState(systemRecord);

    Relays::setCharge(systemRecord.cmdChargeRelay);
    Relays::setDischarge(systemRecord.cmdDischargeRelay);

    // Update Display every 500ms
    if (currentMillis - lastDisplayUpdate > 500) {
        lastDisplayUpdate = currentMillis;
        Display::update(systemRecord, NetManager::getTimeString(), NetManager::isWiFiConnected(), NetManager::isMqttConnected());
    }

    if (currentMillis - lastDebugPrint > 1000) {
        lastDebugPrint = currentMillis;
        Serial.printf("State: %d | Faults: 0x%02X\n", systemRecord.currentState, systemRecord.faultFlags);
    }

    if (currentMillis - lastMqttPublish > 3000) {
        lastMqttPublish = currentMillis;
        NetManager::publishTelemetry(systemRecord);
    }
}