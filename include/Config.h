#ifndef CONFIG_H
#define CONFIG_H

// --- HARDWARE PINS ---
#define PIN_RELAY_CHARGE    10 
#define PIN_RELAY_DISCHARGE 11 
#define PIN_BTN_RESET       0

// --- PHYSICS THRESHOLDS ---
#define LIMIT_OVP           4.25f  // Volts
#define LIMIT_UVP           2.80f  // Volts
#define LIMIT_TEMP_MAX      60.0f  // Celsius
#define LIMIT_TEMP_DELTA    15.0f  // Max diff between the 2 temp sensors
#define LIMIT_CURRENT_MAX   20.0f  // Amps

// --- TIMINGS ---
#define TIMEOUT_SENSOR_MS   3000   // 3 seconds without data = Fault

#endif