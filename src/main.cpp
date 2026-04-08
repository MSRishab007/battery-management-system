#include <Arduino.h>

// Include our centralized Record and Config
#include "../include/DataTypes.h"
#include "../include/Config.h"

// Include Core Brain
#include "Core/Protection.h"
#include "Core/StateManager.h"
#include "Core/CoulombCounter.h"

// Include Hardware Senses (HAL)
#include "HAL/UartLink.h"
#include "HAL/Relays.h"
#include "HAL/Display.h"

// Include External Services
#include "Services/NetManager.h"
#include "Services/StorageManager.h"
BmsRecord record;

uint32_t lastCoreTick = 0;
uint32_t lastDisplayTick = 0;
uint32_t lastTelemetryTick = 0;

void setup() {
    delay(3000);
    // 1. Boot the Physical Serial Port for Python SIL
    Serial.begin(115200);
    delay(1000); // Give the UART bus a moment to stabilize
    Serial.println("\n--- ESP32 BMS CONDUCTOR BOOTING ---");

    // 2. Initialize Hardware Abstraction Layer
    UartLink::init();
    Relays::init();
    Display::init();

    // 3. Initialize Services (Network & Memory)
    NetManager::init();
    StorageManager::init(record);

    // 4. Initialize Core Logic (Anchors the Coulomb Counter)
    CoulombCounter::init(record);

    Serial.println("[CONDUCTOR] Boot Sequence Complete. Entering Main Loop.");
}

void loop() {
    uint32_t currentMillis = millis();

    NetManager::loop(currentMillis);
    
    // Pulls data off the UART wire the millisecond it arrives
    UartLink::readSerial(record, currentMillis);
    if (currentMillis - lastCoreTick >= 50) {
        lastCoreTick = currentMillis;

        Protection::runDiagnostics(record, currentMillis);
        CoulombCounter::update(record, currentMillis);
        StateManager::evaluateState(record);
        Relays::update(record);
        StorageManager::update(record, currentMillis);
    }

    if (currentMillis - lastDisplayTick >= 250) {
        lastDisplayTick = currentMillis;
        
        Display::update(
            record, 
            NetManager::getTimeString(), 
            NetManager::isWiFiConnected(), 
            NetManager::isMqttConnected(),
            currentMillis
        );
    }

    if (currentMillis - lastTelemetryTick >= 1000) {
        lastTelemetryTick = currentMillis;
        
        NetManager::publishTelemetry(record);
    }
}