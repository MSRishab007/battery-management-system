#include "Relays.h"
#include "esp_log.h"

static const char* TAG = "RELAYS";


#define HARDWARE_ACTIVE 0 

// GPIO Pin Definitions
#define PIN_RELAY_CHARGE    GPIO_NUM_15
#define PIN_RELAY_DISCHARGE GPIO_NUM_2
#define PIN_RELAY_RECOVERY  GPIO_NUM_4
#define PIN_BALANCE_1       GPIO_NUM_16
#define PIN_BALANCE_2       GPIO_NUM_17
#define PIN_BALANCE_3       GPIO_NUM_18
#define PIN_BALANCE_4       GPIO_NUM_19

void Relays::init() {
    if (!HARDWARE_ACTIVE) {
        ESP_LOGW(TAG, "Hardware Air-Gapped. Relay pins disabled to protect USB Serial.");
        return;
    }

    ESP_LOGI(TAG, "Configuring Physical Relay & Balance GPIOs...");

    // Create a bitmask of all our output pins
    uint64_t pin_mask = (1ULL << PIN_RELAY_CHARGE) | 
                        (1ULL << PIN_RELAY_DISCHARGE) | 
                        (1ULL << PIN_RELAY_RECOVERY) | 
                        (1ULL << PIN_BALANCE_1) | 
                        (1ULL << PIN_BALANCE_2) | 
                        (1ULL << PIN_BALANCE_3) | 
                        (1ULL << PIN_BALANCE_4);

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = pin_mask;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Ensure all relays and balancers default to OFF (0V) on boot
    gpio_set_level(PIN_RELAY_CHARGE, 0);
    gpio_set_level(PIN_RELAY_DISCHARGE, 0);
    gpio_set_level(PIN_RELAY_RECOVERY, 0);
    gpio_set_level(PIN_BALANCE_1, 0);
    gpio_set_level(PIN_BALANCE_2, 0);
    gpio_set_level(PIN_BALANCE_3, 0);
    gpio_set_level(PIN_BALANCE_4, 0);
}

void Relays::update(BmsRecord& record) {
    if (!HARDWARE_ACTIVE) return;

    // Snapshot the logic states
    if (xSemaphoreTake(record.lock, pdMS_TO_TICKS(10)) == pdTRUE) {
        
        // Drive Main Relays
        gpio_set_level(PIN_RELAY_CHARGE, record.cmdChargeRelay ? 1 : 0);
        gpio_set_level(PIN_RELAY_DISCHARGE, record.cmdDischargeRelay ? 1 : 0);
        gpio_set_level(PIN_RELAY_RECOVERY, record.cmdRecoveryRelay ? 1 : 0);

        // Drive Cell Balancers
        gpio_set_level(PIN_BALANCE_1, record.balanceEnables[0] ? 1 : 0);
        gpio_set_level(PIN_BALANCE_2, record.balanceEnables[1] ? 1 : 0);
        gpio_set_level(PIN_BALANCE_3, record.balanceEnables[2] ? 1 : 0);
        gpio_set_level(PIN_BALANCE_4, record.balanceEnables[3] ? 1 : 0);

        xSemaphoreGive(record.lock);
    }
}