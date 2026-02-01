#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. HARDWARE PINS (LilyGo T-Camera S3 PIR)
// ==========================================

// I2C Bus (PMU & Display)
#define I2C_SDA             7
#define I2C_SCL             6

// Power Management Unit (AXP2101)
#define PMU_IRQ_PIN         2   // Was PMU_INPUT_PIN in your file

// Camera & Sensors (Reserved for later)
#define PIR_INPUT_PIN       17
#define USER_BUTTON_PIN     0

// UART Sensor Interface (The Battery Module)
// We need to pick free pins for UART connection to the battery.
// Based on your board, pins 16 and 15 are labeled "EXTERN".
#define BMS_RX_PIN          16  // EXTERN_PIN1
#define BMS_TX_PIN          15  // EXTERN_PIN2


// ==========================================
// 2. BMS SETTINGS (Thresholds)
// ==========================================

// Voltage Safety Limits
#define MAX_CELL_VOLTAGE    4.20f
#define MIN_CELL_VOLTAGE    2.50f
#define MAX_PACK_VOLTAGE    16.80f // 4S
#define MIN_PACK_VOLTAGE    10.00f

// Current Safety Limits
#define MAX_CHARGE_A        5.0f
#define MAX_DISCHARGE_A     20.0f

// Temperature Safety Limits (Celsius)
#define MAX_TEMP_C          60.0f
#define MIN_TEMP_C          0.0f
#define TEMP_HYSTERESIS     5.0f   // Must cool down by 5C to reset fault

// System Timings
#define UART_TIMEOUT_MS     2500   // If no data for 2.5s, trigger fault
#define LOOP_DELAY_MS       100

#endif