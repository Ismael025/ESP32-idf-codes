#include "app-ledc.h"
#include "esp_log.h"
#include "driver/ledc.h"

static const char *TAG = "APP_LEDC application";

#define LEDC_SPEED_MODE     LEDC_LOW_SPEED_MODE
#define LEDC_TIMER_SEL      LEDC_TIMER_0
#define LEDC_RESOLUTION     LEDC_TIMER_10_BIT  
#define LEDC_MAX_DUTY       (1023)             
#define LEDC_FREQUENCY      (5000)             

void initPWMOutput(gpio_num_t gpio_pin, ledc_channel_t channel) {

    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_SPEED_MODE,
        .timer_num       = LEDC_TIMER_SEL,
        .duty_resolution = LEDC_RESOLUTION,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK
    };

    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(err));
        return;
    }

    ledc_channel_config_t ledc_channel = {
        .speed_mode  = LEDC_SPEED_MODE,
        .channel     = channel,
        .timer_sel   = LEDC_TIMER_SEL,
        .intr_type   = LEDC_INTR_DISABLE,
        .gpio_num    = gpio_pin,
        .duty        = 0, 
        .hpoint      = 0
    };

    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "PWM initialized successfully on GPIO %d (Channel %d)", gpio_pin, channel);
    }
}

void setPWMDutyPercentage(ledc_channel_t channel, uint8_t percentage) {
    if (percentage > 100) {
        percentage = 100;
    }

    uint32_t duty_value = (percentage * LEDC_MAX_DUTY) / 100;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, channel, duty_value));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, channel));
}