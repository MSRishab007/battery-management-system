#include "Display.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h" 
#include <u8g2.h>
#include <stdio.h>
#include "NetManager.h"
static const char* TAG = "DISPLAY";

// Hardware Definitions
#define I2C_MASTER_PORT I2C_NUM_0
#define I2C_SDA_PIN 7
#define I2C_SCL_PIN 6
#define AXP2101_ADDR 0x34

u8g2_t u8g2;

// =================================================================
// 1. ESP-IDF NATIVE I2C BRIDGE FOR U8G2
// =================================================================
uint8_t u8x8_byte_esp32_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static i2c_cmd_handle_t handle;
    uint8_t* data = (uint8_t*)arg_ptr;

    switch(msg) {
        case U8X8_MSG_BYTE_SEND:
            i2c_master_write(handle, data, arg_int, true);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            handle = i2c_cmd_link_create();
            i2c_master_start(handle);
            i2c_master_write_byte(handle, u8x8_GetI2CAddress(u8x8) | I2C_MASTER_WRITE, true);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            i2c_master_stop(handle);
            i2c_master_cmd_begin(I2C_MASTER_PORT, handle, pdMS_TO_TICKS(100));
            i2c_cmd_link_delete(handle);
            break;
        default:
            break;
    }
    return 1;
}

uint8_t u8x8_gpio_and_delay_esp32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_DELAY_MILLI: esp_rom_delay_us(arg_int * 1000); break;
        case U8X8_MSG_DELAY_10MICRO: esp_rom_delay_us(10); break;
        case U8X8_MSG_DELAY_100NANO: esp_rom_delay_us(1); break;
        default: break;
    }
    return 1;
}

// =================================================================
// 2. BMS DISPLAY TASK LOGIC
// =================================================================
void Display::startTask(BmsRecord* record) {
    xTaskCreatePinnedToCore(displayTask, "Display_Task", 6144, (void*)record, 2, NULL, 0);
    ESP_LOGI(TAG, "Display Task Started on Core 0");
}

void Display::initI2C() {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA_PIN;
    conf.scl_io_num = I2C_SCL_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000; // 400kHz
    
    i2c_param_config(I2C_MASTER_PORT, &conf);
    i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
    ESP_LOGI(TAG, "I2C Master Initialized");
}

void Display::initPMU() {
    uint8_t data[2];
    esp_err_t err;
    
    // Set ALDO1 and ALDO4 to 3.3V
    data[0] = 0x92; data[1] = 0x1C; i2c_master_write_to_device(I2C_MASTER_PORT, AXP2101_ADDR, data, 2, pdMS_TO_TICKS(100));
    data[0] = 0x95; data[1] = 0x1C; i2c_master_write_to_device(I2C_MASTER_PORT, AXP2101_ADDR, data, 2, pdMS_TO_TICKS(100));
    
    // Read-Modify-Write the Power Enable Register
    uint8_t reg_addr = 0x90;
    uint8_t current_val = 0;
    err = i2c_master_write_read_device(I2C_MASTER_PORT, AXP2101_ADDR, &reg_addr, 1, &current_val, 1, pdMS_TO_TICKS(100));
    
    if (err == ESP_OK) {
        current_val |= 0x09; // Turn ON bit 0 (ALDO1) and bit 3 (ALDO4)
        data[0] = 0x90; data[1] = current_val;
        i2c_master_write_to_device(I2C_MASTER_PORT, AXP2101_ADDR, data, 2, pdMS_TO_TICKS(100));
        ESP_LOGI(TAG, "AXP2101 Power Enabled Safely");
    }
}

const char* Display::getStateString(SystemState state) {
    switch(state) {
        case STATE_BOOT: return "BOOTING";
        case STATE_MAINTENANCE: return "MAINTENANCE";
        case STATE_IDLE: return "STANDBY";
        case STATE_DEEP_SLEEP: return "DEEP SLEEP";
        case STATE_DISCHARGING: return "DISCHARGING";
        case STATE_CHARGE_RECOVERY: return "RECOVERY";
        case STATE_CHARGE_BULK: return "CHARGING";
        case STATE_BALANCING: return "BALANCING";
        case STATE_FULL_IDLE: return "100% FULL";
        case STATE_LIMP_MODE: return "LIMP MODE";
        case STATE_FAULT: return "FAULT!";
        default: return "UNKNOWN";
    }
}

void Display::displayTask(void* pvParameters) {
    BmsRecord* record = (BmsRecord*)pvParameters;

    initI2C();
    initPMU();
    
    // Let the PMU voltage stabilize before talking to the OLED
    esp_rom_delay_us(100000); // 100ms

    // Connect U8g2 to our custom hardware bridge
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2, U8G2_R0, 
        u8x8_byte_esp32_hw_i2c, 
        u8x8_gpio_and_delay_esp32
    );
    
    // CRITICAL FIX: Restored to 0x3C which worked in the Hello World test
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x3C << 1);
    
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0); 
    u8g2_SetContrast(&u8g2, 255); 

    while (true) {
        uint32_t currentMillis = (uint32_t)(esp_timer_get_time() / 1000ULL);
        drawUI(record, currentMillis);
        
        vTaskDelay(pdMS_TO_TICKS(200)); // Update at 5Hz
    }
}

void Display::drawUI(BmsRecord* record, uint32_t currentMillis) {
    // Snapshot the data
    if (xSemaphoreTake(record->lock, pdMS_TO_TICKS(50)) != pdTRUE) return;
    
    uint32_t faultFlags = record->faultFlags;
    SystemState state = record->currentState;
    float current = record->current.value;
    float delta = record->vMax - record->vMin;
    float t1 = record->temperatures[0].value;
    float t2 = record->temperatures[1].value;
    float soc = record->stateOfCharge;
    
    float packVolts = 0;
    for(int i=0; i<Config::NUM_CELLS; i++) packVolts += record->cellVoltages[i].value;
    
    xSemaphoreGive(record->lock);

    bool isWiFiConnected = NetManager::isWiFiConnected(); 
    bool isMqttConnected = NetManager::isMqttConnected();

    u8g2_ClearBuffer(&u8g2);

    // --- 1. FAULT OVERRIDE ALARM ---
    bool blinkFast = (currentMillis / 250) % 2 == 0;
    if (faultFlags != FAULT_NONE) {
        if (blinkFast) {
            u8g2_DrawBox(&u8g2, 0, 0, 128, 64);
            u8g2_SetDrawColor(&u8g2, 0); 
        }
        u8g2_SetFont(&u8g2, u8g2_font_helvB14_tf);
        u8g2_DrawStr(&u8g2, 25, 30, "FAULT");
        
        u8g2_SetFont(&u8g2, u8g2_font_6x13_tf);
        char faultBuf[32];
        snprintf(faultBuf, sizeof(faultBuf), "CODE: 0x%02lX", faultFlags);
        u8g2_DrawStr(&u8g2, 20, 50, faultBuf);
        
        u8g2_SetDrawColor(&u8g2, 1); 
        u8g2_SendBuffer(&u8g2);
        return; 
    }

    // --- RESTORED: 2. TOP STATUS BAR ---
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
    
    // Generate a quick uptime string (HH:MM:SS)
    char timeStr[16];
    NetManager::getTimeString(timeStr, sizeof(timeStr));
    u8g2_DrawStr(&u8g2, 0, 8, timeStr);

    if (isWiFiConnected) u8g2_DrawStr(&u8g2, 90, 8, "WIFI");
    if (isMqttConnected) u8g2_DrawStr(&u8g2, 115, 8, "MQ");
    u8g2_DrawLine(&u8g2, 0, 10, 128, 10); 

    // --- 3. SYSTEM STATE (Re-aligned downward so it doesn't hit the line) ---
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tf);
    u8g2_DrawStr(&u8g2, 0, 23, getStateString(state));

    // --- 4. DYNAMIC BATTERY ICON ---
    // Moved slightly down to fit better
    u8g2_DrawRFrame(&u8g2, 108, 16, 20, 34, 2); 
    u8g2_DrawBox(&u8g2, 114, 13, 8, 3);         
    
    int fillPixels = (int)((soc / 100.0f) * 30.0f);
    u8g2_DrawBox(&u8g2, 110, 48 - fillPixels, 16, fillPixels);

    // --- 5. DATA TELEMETRY (Re-spaced coordinates to prevent text overlap) ---
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tf);
    
    char buf[32];
    snprintf(buf, sizeof(buf), "Pack: %.2fV", packVolts);
    u8g2_DrawStr(&u8g2, 0, 35, buf);
    
    snprintf(buf, sizeof(buf), "Cur: %.2fA", current);
    u8g2_DrawStr(&u8g2, 0, 45, buf);

    snprintf(buf, sizeof(buf), "dlt: %.0fmV", delta * 1000.0f);
    u8g2_DrawStr(&u8g2, 55, 45, buf); 

    snprintf(buf, sizeof(buf), "T1:%.1f T2:%.1f", t1, t2);
    u8g2_DrawStr(&u8g2, 0, 56, buf);

    u8g2_SendBuffer(&u8g2);
}