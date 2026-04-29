#include "SoCEstimator.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char* TAG = "SOC";

uint64_t SoCEstimator::lastUpdateTime = 0;

void SoCEstimator::init(BmsRecord& record) {
    if (xSemaphoreTake(record.lock, portMAX_DELAY) == pdTRUE) {
        float avgVoltage = 0;
        for(int i=0; i<Config::NUM_CELLS; i++) {
            avgVoltage += record.cellVoltages[i].value;
        }
        avgVoltage /= Config::NUM_CELLS;

        float startingSoC = 0.0f;
        if (avgVoltage >= 4.2f) startingSoC = 100.0f;
        else if (avgVoltage <= 3.0f) startingSoC = 0.0f;
        else startingSoC = ((avgVoltage - 3.0f) / (4.2f - 3.0f)) * 100.0f;

        record.stateOfCharge = startingSoC;
        ESP_LOGI(TAG, "Boot OCV Estimated SoC: %.1f%% based on %.2fV/cell", startingSoC, avgVoltage);
        
        xSemaphoreGive(record.lock);
    }
    lastUpdateTime = esp_timer_get_time();
}

void SoCEstimator::update(BmsRecord& record) {
    uint64_t now = esp_timer_get_time();
    float dt_seconds = (float)(now - lastUpdateTime) / 1000000.0f;
    lastUpdateTime = now;

    if (xSemaphoreTake(record.lock, pdMS_TO_TICKS(10)) == pdTRUE) {
        
        float hours = dt_seconds / 3600.0f;
        
        // Calculate the capacity moved
        float deltaAh = record.current.value * hours;
        float deltaSoC = (deltaAh / Config::PACK_CAPACITY_MAH) * 100.0f;

        // THE FIX: Subtract the delta (Assuming positive current = Discharge)
        // If your hardware uses negative current for discharge, change this back to +=
        record.stateOfCharge -= deltaSoC; 

        // Clamp to physical boundaries
        if (record.stateOfCharge > 100.0f) record.stateOfCharge = 100.0f;
        if (record.stateOfCharge < 0.0f) record.stateOfCharge = 0.0f;

        // DEBUG LOG: Print the raw, unrounded float every 2 seconds
        static int logCounter = 0;
        if (++logCounter % 4 == 0) { 
            ESP_LOGI(TAG, "Raw SoC: %f%% | Drained: %f%%", record.stateOfCharge, deltaSoC);
        }

        xSemaphoreGive(record.lock);
    }
}