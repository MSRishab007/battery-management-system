#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

namespace Config {
    // --- Hardware Pins ---
    const uint8_t PIN_RELAY_CHARGE = 25;    
    const uint8_t PIN_RELAY_DISCHARGE = 26; 
    const uint8_t PIN_BUTTON_RESET = 0;    

    // Protection thresholds
    const float OVP_LIMIT = 4.25;           // Over-Voltage Protection (Per Cell)
    const float UVP_LIMIT = 2.80;           // Under-Voltage Protection (Per Cell)
    const float OTP_LIMIT = 60.0;           // Over-Temperature Protection (Celsius)
    const float MAX_DELTA_T = 15.0;         // Max diff between charge/discharge sensors

    // State Machine Thresholds 
    const float VOLTAGE_FULL = 4.20;        // 100% Charged
    const float VOLTAGE_RECHARGE = 4.05;   
    const float VOLTAGE_DEEP_SLEEP = 3.00;  
    const float VOLTAGE_RECOVERY = 3.10;    

    // Timeouts
    const uint32_t TIME_STALE_DATA = 3000;         // 3s without UART data 
    const uint32_t TIME_STANDBY_TO_LIGHT = 300000; // 5 mins of inactivity before going to light sleep
    const uint32_t TIME_PRE_CHARGE = 2000;         // 2s soft-start for capacitors

    // 
    const float PACK_CAPACITY_MAH = 5000.0; 
}

#endif