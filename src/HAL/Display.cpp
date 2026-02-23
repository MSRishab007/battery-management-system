#include "Display.h"
#include <Wire.h>

// PMU and Display specific pins for LilyGo T-Camera S3
#define I2C_SDA 7
#define I2C_SCL 6
#define AXP2101_ADDR 0x34

#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>

static XPowersPMU PMU;
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void Display::init() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Power up the display via PMU
    if (PMU.begin(Wire, AXP2101_ADDR, I2C_SDA, I2C_SCL)) {
        PMU.setALDO1Voltage(3300); PMU.enableALDO1();
        PMU.setALDO4Voltage(3300); PMU.enableALDO4();
    }
    
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(10, 30, "SYSTEM BOOT");
    u8g2.sendBuffer();
}

void Display::update(const BmsRecord& record, String timeStr, bool isWiFiConnected, bool isMqttConnected) {
    u8g2.clearBuffer();

    // --- Top Bar: Time and Network Status ---
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setCursor(0, 8);
    u8g2.print(timeStr.c_str());

    if (isWiFiConnected) u8g2.drawStr(90, 8, "WIFI");
    if (isMqttConnected) u8g2.drawStr(115, 8, "MQ");
    u8g2.drawLine(0, 10, 128, 10);

    // --- Middle: System State ---
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.setCursor(0, 26);
    switch(record.currentState) {
        case STATE_BOOT:        u8g2.print("BOOTING"); break;
        case STATE_IDLE:        u8g2.print("IDLE"); break;
        case STATE_CHARGING:    u8g2.print("CHARGING..."); break;
        case STATE_DISCHARGING: u8g2.print("DISCHARGING"); break;
        case STATE_FAULT_HARD:  u8g2.print("CRITICAL FAULT!"); break;
        default:                u8g2.print("UNKNOWN"); break;
    }

    // --- Data: Voltage and Temp ---
    u8g2.setFont(u8g2_font_6x13_tf);
    u8g2.setCursor(0, 44);
    u8g2.printf("V1: %.2fV  T: %.1fC", record.cellVolts[0].value, record.tempCharge.value);

    // --- Bottom: Relays and Fault Code ---
    u8g2.setCursor(0, 60);
    u8g2.printf("Relays: %d|%d  Err: %02X", record.cmdChargeRelay, record.cmdDischargeRelay, record.faultFlags);

    u8g2.sendBuffer();
}