#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"
#define LED_PIN GPIO_NUM_2

char* TAG = " GPIO CONFIGURATION";

void app_main(void)
{

    gpio_config_t conf = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&conf);

    if( err != ESP_OK) {
        ESP_LOGE( TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "configuration successfully");

    while(1){
        gpio_set_level(LED_PIN,1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(LED_PIN,0);
        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}