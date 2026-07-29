#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"

char* TAG = "CONFIGURATION";

void app_main(void)
{

    esp_err_t err = gpio_dump_io_configuration(stdout, SOC_GPIO_VALID_GPIO_MASK);

    ESP_LOGE( TAG, "error is: %s", esp_err_to_name(err));
     
}