#include "UartLink.h"

// Define exactly what the 5-byte payload looks like
struct UartPacket {
    uint8_t sensorId;
    float value;
} __attribute__((packed));

void UartLink::processIncoming(BmsRecord& record) {
    // We need 1 Header byte + 5 bytes of payload = 6 bytes total
    if (Serial.available() >= 6) {
        
        if (Serial.read() == 0xAA) { // Check Header
            UartPacket packet;
            Serial.readBytes((uint8_t*)&packet, sizeof(UartPacket));
            
            uint32_t now = millis();

            // Map the incoming ID to the correct variable in our struct
            switch(packet.sensorId) {
                case 0x01: record.cellVolts[0] = {packet.value, now}; break;
                case 0x02: record.cellVolts[1] = {packet.value, now}; break;
                case 0x03: record.cellVolts[2] = {packet.value, now}; break;
                case 0x04: record.cellVolts[3] = {packet.value, now}; break;
                
                case 0x0A: record.currentA = {packet.value, now}; break;
                
                case 0x0B: record.tempCharge = {packet.value, now}; break;
                case 0x0C: record.tempDischarge = {packet.value, now}; break;
                
                // For bools, we just send 1.0 for true, 0.0 for false
                case 0x0D: record.chargerPhysicallyConnected = (packet.value > 0.5f); break;
            }
        }
    }
}