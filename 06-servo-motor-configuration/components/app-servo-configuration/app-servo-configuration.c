#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"


static const char *TAG = "APP-SERVO-CONFIGURATION";

/*----------------------------------------------------------
 * LED Configuration
 *---------------------------------------------------------*/

#define LED_TIMER       LEDC_TIMER_0
#define LED_CHANNEL     LEDC_CHANNEL_0
#define LED_MODE        LEDC_HIGH_SPEED_MODE
#define LED_FREQUENCY   1000
#define LED_RESOLUTION  LEDC_TIMER_13_BIT

/*----------------------------------------------------------
 * Servo Configuration
 *---------------------------------------------------------*/

#define SERVO_TIMER      LEDC_TIMER_1
#define SERVO_CHANNEL    LEDC_CHANNEL_1
#define SERVO_MODE       LEDC_HIGH_SPEED_MODE
#define SERVO_FREQUENCY  50
#define SERVO_RESOLUTION LEDC_TIMER_16_BIT

static uint32_t servo_angle_to_duty(uint8_t angle)
{
    uint32_t pulse_width_us =
        500 + ((2000 * angle) / 180);

    uint32_t max_duty =
        (1 << SERVO_RESOLUTION);

    uint32_t duty =
        (pulse_width_us * max_duty) / 20000;

    return duty;
}

static void led_timer_init(void)
{
    ledc_timer_config_t timer = {

        .speed_mode = LED_MODE,

        .timer_num = LED_TIMER,

        .duty_resolution = LED_RESOLUTION,

        .freq_hz = LED_FREQUENCY,

        .clk_cfg = LEDC_AUTO_CLK
    };

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(err));
        return;
    }

}

static void led_channel_init(void)
{
    ledc_channel_config_t channel = {

        .gpio_num = LED_GPIO,

        .speed_mode = LED_MODE,

        .channel = LED_CHANNEL,

        .intr_type = LEDC_INTR_DISABLE,

        .timer_sel = LED_TIMER,

        .duty = 0,

        .hpoint = 0
    };

    err = ledc_channel_config(&channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "PWM initialized successfully on GPIO %d (Channel %d)", gpio_pin, channel);
    }
}

static void servo_timer_init(void)
{
    ledc_timer_config_t timer = {

        .speed_mode = SERVO_MODE,

        .timer_num = SERVO_TIMER,

        .duty_resolution = SERVO_RESOLUTION,

        .freq_hz = SERVO_FREQUENCY,

        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&timer));
}

static void servo_channel_init(void)
{
    ledc_channel_config_t channel = {

        .gpio_num = SERVO_GPIO,

        .speed_mode = SERVO_MODE,

        .channel = SERVO_CHANNEL,

        .intr_type = LEDC_INTR_DISABLE,

        .timer_sel = SERVO_TIMER,

        .duty = 0,

        .hpoint = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

