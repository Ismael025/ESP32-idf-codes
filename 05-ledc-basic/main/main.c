#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "app-ledc.h"

static const char *TAG = "MAIN LEDC";

#define LEDC_GPIO_PIN    GPIO_NUM_3  
#define LEDC_CHANNEL     LEDC_CHANNEL_0

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 LEDC PWM Example...");

    uint8_t current_percentage = 0;
    int8_t step = 10;

    while (1) {
        ESP_LOGI(TAG, "Setting PWM duty cycle to: %d%%", current_percentage);
        
        setPWMDutyPercentage(LEDC_CHANNEL, current_percentage);

        if (current_percentage >= 100 || current_percentage == 0) {
            step = -step;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}