#include <stdio.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "app-analog-input.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"

const char* TAG = "ANALOG_INPUT";

adc_oneshot_unit_handle_t adc1_handle;

// Now it accepts an ADC channel instead of being void!
void initAnalogInput(adc_channel_t channel)
{ 
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, 
        .atten = ADC_ATTEN_DB_12,         
    };
    
    // FIXED: Instead of hardcoded ADC_CHANNEL_3, we pass the 'channel' variable!
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channel, &config));
}
    
     void readAnalogInput(adc_channel_t channel)
     {

 // The loop safely enclosed within a function block
    while (true)
    {
        int raw_value = 0;
        
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &raw_value));
        // Use the official ANALOG reading function, NOT the digital gpio_get_level
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, channel,raw_value);

        esp_err_t err = adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &raw_value);
        
        if (err == ESP_OK) {
            // Will print a smooth spectrum between 0 and 4095
            ESP_LOGI(TAG, "Analog Raw Level is: %d", raw_value);
        } else {
            ESP_LOGE(TAG, "Failed to read analog value: %s", esp_err_to_name(err));
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }


}
