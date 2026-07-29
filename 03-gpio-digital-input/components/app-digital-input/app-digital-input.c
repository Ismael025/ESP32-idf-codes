#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "app-digital-input.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char* TAG = "GPIO DIGITAL INPUTS";

void gpioGetDigitalLevel(gpio_num_t GPIO_PIN)
{

    gpio_config_t conf= {
        .pin_bit_mask = 1ULL << GPIO_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t err = gpio_config(&conf);

    if(err != ESP_OK){
        ESP_LOGE(TAG,"Configurations unsuccesfullly: %s", esp_err_to_name(err)); 
        return;
    }
    ESP_LOGI(TAG,"Configurations successfully");

    while (true)
    {
        ESP_LOGI(TAG,"Level is: %d",gpio_get_level(GPIO_PIN));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
}