#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/usb_serial_jtag.h" 
#include "nvs_flash.h"

// Include our custom BMS modules
#include "include/DataTypes.h"
#include "include/Config.h"
#include "Core/Protection.h"
#include "Core/StateManager.h"
#include "HAL/UartLink.h"
#include "HAL/Display.h"
#include "Services/NetManager.h"
#include "Core/SoCEstimator.h"
#include "HAL/Relays.h"

static const char *TAG = "MAIN";

// =================================================================
// 1. THE GLOBAL MASTER RECORD
// =================================================================
BmsRecord bmsRecord;

// =================================================================
// 2. THE CORE BMS FREERTOS TASK
// =================================================================
void bms_core_task(void *pvParameters) {
    ESP_LOGI(TAG, "BMS Core Task Started on Core %d!", xPortGetCoreID());

    while (true) {
        SoCEstimator::update(bmsRecord);
        // 1. Run the safety math (Checks thresholds, updates active faults)
        Protection::runDiagnostics(bmsRecord);

        // 2. Evaluate state machine transitions (Acts on the faults)
        StateManager::evaluateState(bmsRecord);
        Relays::update(bmsRecord);
        // 3. Debug Print to Monitor (Safely reading the memory)
        if (xSemaphoreTake(bmsRecord.lock, pdMS_TO_TICKS(10)) == pdTRUE) {
            ESP_LOGI(TAG, "State: %2d | Faults: 0x%08lX | vMax: %.2fV | vMin: %.2fV",
                     bmsRecord.currentState,
                     bmsRecord.faultFlags,
                     bmsRecord.vMax,
                     bmsRecord.vMin);
            xSemaphoreGive(bmsRecord.lock);
        }

        // Run this task at exactly 2Hz (Every 500ms)
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// =================================================================
// 3. APP_MAIN (The ESP-IDF Boot Sequence)
// =================================================================
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "--- ESP32-S3 BMS Booting ---");

    // A. Initialize the RTOS Mutex FIRST
    bmsRecord.lock = xSemaphoreCreateMutex();
    if (bmsRecord.lock == NULL) {
        ESP_LOGE(TAG, "Failed to create Master Mutex! Halting.");
        while(1) vTaskDelay(100); 
    }

    // B. INSTALL THE USB DRIVER (Prevents the CPU Panic)
    usb_serial_jtag_driver_config_t usb_config = {};
    usb_config.tx_buffer_size = 256;
    usb_config.rx_buffer_size = 256;
    
    esp_err_t ret = usb_serial_jtag_driver_install(&usb_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB driver! Error: %d", ret);
    } else {
        ESP_LOGI(TAG, "Native USB Driver Installed Successfully.");
    }
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);

    // C. Initialize dummy sensor data to prevent instant faults
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
    if (xSemaphoreTake(bmsRecord.lock, portMAX_DELAY) == pdTRUE) {
        for(int i = 0; i < Config::NUM_CELLS; i++) {
            bmsRecord.cellVoltages[i].value = 3.8f;
            bmsRecord.cellVoltages[i].lastUpdate = now;
        }
        for(int i = 0; i < Config::NUM_TEMPS; i++) {
            bmsRecord.temperatures[i].value = 25.0f;
            bmsRecord.temperatures[i].lastUpdate = now;
        }
        bmsRecord.current.value = 0.0f;
        bmsRecord.current.lastUpdate = now;
        
        xSemaphoreGive(bmsRecord.lock);
    }
    SoCEstimator::init(bmsRecord);
    Relays::init();

    // D. Spawn the Background Tasks
    // Pass the memory address of our global record so UartLink can write to it safely
    UartLink::startTask(&bmsRecord);
    Display::startTask(&bmsRecord);
    NetManager::startTask(&bmsRecord);

    // Pin the safety core task to Core 1
    xTaskCreatePinnedToCore(bms_core_task, "BMS_Core", 4096, NULL, 5, NULL, 1);
}