#ifdef ARDUINO
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
#ifdef ARDUINO
String getStateString(uint8_t stateCode) {
    switch(stateCode) {
        case 0: return "BOOT";
        case 1: return "STANDBY";
        case 2: return "CHARGE";
        case 3: return "DISCHARGE";
        case 4: return "FULL_IDLE";
        case 5: return "DEEP_SLEEP";
        case 6: return "BALANCING";
        case 7: return "RECOVERY";
        case 8: return "LIMP_MODE";
        case 9: return "FAULT!";
        default: return "UNKNOWN (" + String(stateCode) + ")";
    }
}

String getFaultString(uint32_t faultFlags) { 
    if (faultFlags == 0) return "OK";
    
    String out = "0x";
    if (faultFlags < 16) out += "0"; 
    out += String(faultFlags, HEX) + " [";
    
    if (faultFlags & FAULT_OVER_VOLTAGE)      out += "OVP ";
    if (faultFlags & FAULT_UNDER_VOLTAGE)     out += "UVP ";
    if (faultFlags & FAULT_TEMP_DELTA)        out += "dTMP "; 
    if (faultFlags & FAULT_OVER_TEMP)         out += "OTP ";
    if (faultFlags & FAULT_STALE_DATA)        out += "STALE "; 
    if (faultFlags & FAULT_CONTACTOR_WELDED)  out += "WELD ";  
    
    out += "]";
    return out;
}
#endif

void Display::init() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Initialize the T-Display S3 PMU
    if (PMU.begin(Wire, AXP2101_ADDR, I2C_SDA, I2C_SCL)) {
        PMU.setALDO1Voltage(3300); PMU.enableALDO1();
        PMU.setALDO4Voltage(3300); PMU.enableALDO4();
    }
    
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(10, 35, "BMS BOOTING...");
    u8g2.sendBuffer();
}

void Display::update(const BmsRecord& record, const String& timeStr, 
                     bool isWiFiConnected, bool isMqttConnected, 
                     uint32_t currentMillis) {
    u8g2.clearBuffer();

    // --- 1. THE FAULT OVERRIDE (Flashing Alarm) ---
    bool blinkFast = (currentMillis / 250) % 2 == 0;
    if (record.faultFlags != FAULT_NONE) {
        if (blinkFast) {
            u8g2.drawBox(0, 0, 128, 64);
            u8g2.setDrawColor(0); // Draw text in black over white box
        }
        u8g2.setFont(u8g2_font_helvB14_tf);
        u8g2.drawStr(25, 30, "FAULT");
        u8g2.setFont(u8g2_font_6x13_tf);
        u8g2.setCursor(20, 50);
        u8g2.printf("CODE: 0x%02X", record.faultFlags);
        
        u8g2.setDrawColor(1); // Reset to normal
        u8g2.sendBuffer();
        return; // Skip drawing the rest of the UI
    }

    // --- 2. TOP STATUS BAR ---
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setCursor(0, 8);
    u8g2.print(timeStr.c_str());

    if (isWiFiConnected) u8g2.drawStr(90, 8, "WIFI");
    if (isMqttConnected) u8g2.drawStr(115, 8, "MQ");
    u8g2.drawLine(0, 10, 128, 10); // Separator

    // --- 3. SYSTEM STATE (Big & Bold) ---
    u8g2.setFont(u8g2_font_helvB08_tf);
    u8g2.setCursor(0, 23);
    switch(record.currentState) {
        case STATE_BOOT:            u8g2.print("BOOTING"); break;
        case STATE_MAINTENANCE:     u8g2.print("MAINTENANCE"); break;
        case STATE_IDLE:            u8g2.print("STANDBY"); break;
        case STATE_DEEP_SLEEP:      u8g2.print("DEEP SLEEP (UVLO)"); break;
        case STATE_DISCHARGING:     u8g2.print("DISCHARGING"); break;
        case STATE_CHARGE_RECOVERY: u8g2.print("TRICKLE RECOVERY"); break;
        case STATE_CHARGE_BULK:     u8g2.print("BULK CHARGING"); break;
        case STATE_BALANCING:       u8g2.print("CELL BALANCING"); break;
        case STATE_FULL_IDLE:       u8g2.print("100% CHARGED"); break;
        default:                    u8g2.print("UNKNOWN"); break;
    }

    // --- 4. DYNAMIC BATTERY ICON ---
    // Draw battery outline on the far right
    u8g2.drawRFrame(108, 16, 20, 34, 2); // Body
    u8g2.drawBox(114, 13, 8, 3);         // Positive terminal
    

    // Map vMin from 3.0V (0%) to 4.2V (100%)
    float displaySoC = record.stateOfCharge / 100.0f;
    
    int fillPixels = (int)(displaySoC * 30.0f);
    u8g2.drawBox(110, 48 - fillPixels, 16, fillPixels);

    // Charging Animation Chevron
    bool blinkSlow = (currentMillis / 500) % 2 == 0;
    if ((record.currentState == STATE_CHARGE_BULK || 
         record.currentState == STATE_CHARGE_RECOVERY || 
         record.currentState == STATE_BALANCING) && blinkSlow) {
        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(95, 35, ">>");
    }

    // --- 5. DATA TELEMETRY ---
    // Calculate total pack voltage
    float packVolts = 0;
    for(int i=0; i<NUM_CELLS; i++) packVolts += record.cellVoltages[i].value;

    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.setCursor(0, 35);
    u8g2.printf("Pack: %.2fV", packVolts);
    
    u8g2.setCursor(0, 45);
    u8g2.printf("Current: %.2fA", record.current.value);

    // Show Cell Delta (vMax - vMin) and Max Temp
    u8g2.setCursor(0, 55);
    u8g2.printf("Delta: %.2fmV", (record.vMax - record.vMin) * 1000.0f);

    u8g2.setCursor(0, 64); // Bottom line
    u8g2.printf("T1:%.1fC T2:%.1fC", record.temperatures[0].value, record.temperatures[1].value);

    u8g2.sendBuffer();
}
#endif