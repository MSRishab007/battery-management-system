#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>
#define I2C_SDA             7
#define I2C_SCL             6
#define AXP2101_SLAVE_ADDRESS 0x34
struct BmsPacket {
    float value;
} __attribute__((packed)); 
XPowersPMU PMU;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
BmsPacket receivedData;

void setup() {
    Serial.begin(115200);
    delay(2000); 
    Wire.begin(I2C_SDA, I2C_SCL);
    if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) {
        Serial.println("PMU Fail");
        while(1) delay(100);
    }
    PMU.setALDO1Voltage(3300); PMU.enableALDO1();
    PMU.setALDO4Voltage(3300); PMU.enableALDO4();
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(0, 30, "READY.");
    u8g2.sendBuffer();
}
void loop() {
    // FIX: Check for the exact size of YOUR struct + 1 header byte
    // sizeof(BmsPacket) is 4 bytes. + 1 Header = 5 bytes.
    if (Serial.available() >= (sizeof(BmsPacket) + 1)) {
        
        // Check for Header
        if (Serial.read() == 0xAA) {
            
            // Read 4 bytes (float)
            Serial.readBytes((uint8_t*)&receivedData, sizeof(BmsPacket));
            
            // Debug
            Serial.printf("Got Packet: %.2fV\n", receivedData.value);
            
            // Display
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_helvB10_tf);
            u8g2.drawStr(0, 12, "UART Read");
            u8g2.drawLine(0, 15, 128, 15);
            
            u8g2.setFont(u8g2_font_6x13_tf);
            u8g2.setCursor(0, 35);
            u8g2.printf("Val: %.2f", receivedData.value);
            
            u8g2.sendBuffer();
        }
    }
}