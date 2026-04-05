#include "Display.h"
#include <Wire.h>
#include <U8g2lib.h>

#define I2C_SDA 7
#define I2C_SCL 6
#define AXP2101_ADDR 0x34

#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>

static XPowersPMU PMU;
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void Display::init() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    if (PMU.begin(Wire, AXP2101_ADDR, I2C_SDA, I2C_SCL)) {
        PMU.setALDO1Voltage(3300); PMU.enableALDO1();
        PMU.setALDO4Voltage(3300); PMU.enableALDO4();
    }
    
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(10, 30, "BMS BOOTING...");
    u8g2.sendBuffer();
}

void Display::update(const BmsRecord& record, String timeStr, bool isWiFiConnected, bool isMqttConnected) {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setCursor(0, 8);
    u8g2.print(timeStr.c_str());

    if (isWiFiConnected) u8g2.drawStr(90, 8, "WIFI");
    if (isMqttConnected) u8g2.drawStr(115, 8, "MQ");
    u8g2.drawLine(0, 10, 128, 10);

    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.setCursor(0, 24);
    switch(record.currentState) {
        case STATE_BOOT:            u8g2.print("BOOTING"); break;
        case STATE_MAINTENANCE:     u8g2.print("MAINTENANCE"); break;
        case STATE_IDLE_STANDBY:    u8g2.print("STANDBY"); break;
        case STATE_IDLE_LIGHT:      u8g2.print("LIGHT SLEEP"); break;
        case STATE_DEEP_SLEEP:      u8g2.print("DEEP SLEEP"); break;
        case STATE_PRE_CHARGE:      u8g2.print("PRE-CHARGE"); break;
        case STATE_DISCHARGING:     u8g2.print("DISCHARGING"); break;
        case STATE_FULL_DISCHARGE:  u8g2.print("FULL (DISCHG)"); break;
        case STATE_CHARGE_RECOVERY: u8g2.print("RECOVERY CHG"); break;
        case STATE_CHARGE_BULK:     u8g2.print("BULK CHARGE"); break;
        case STATE_BALANCING:       u8g2.print("BALANCING"); break;
        case STATE_FULL_IDLE:       u8g2.print("FULL (IDLE)"); break;
        case STATE_FAULT:           u8g2.print("FAULT LOCKED!"); break;
        default:                    u8g2.print("UNKNOWN"); break;
    }

    u8g2.setFont(u8g2_font_6x13_tf);
    u8g2.setCursor(0, 38);
    u8g2.printf("V1: %.2fV  I: %.2fA", record.cellVolts[0].value, record.currentA.value);

    u8g2.setCursor(0, 50);
    u8g2.printf("SoC: %02.0f%%  T: %.1fC", record.stateOfCharge, record.tempCharge.value);

    u8g2.setCursor(0, 62);
    u8g2.printf("R: %d|%d    Err: 0x%02X", record.cmdChargeRelay, record.cmdDischargeRelay, record.faultFlags);

    u8g2.sendBuffer();
}
