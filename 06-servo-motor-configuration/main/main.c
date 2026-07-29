#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
     led_timer_init();

    servo_timer_init();

    led_channel_init();

    servo_channel_init();

    ESP_ERROR_CHECK(
        ledc_fade_func_install(0)
    );

    while (1)
    {
        /*-------------------------------
         * LED Fade ON
         *------------------------------*/

        ledc_set_fade_with_time(
            LED_MODE,
            LED_CHANNEL,
            8191,
            2000);

        ledc_fade_start(
            LED_MODE,
            LED_CHANNEL,
            LEDC_FADE_NO_WAIT);

        /*-------------------------------
         * Servo 0°
         *------------------------------*/

        ledc_set_duty(
            SERVO_MODE,
            SERVO_CHANNEL,
            servo_angle_to_duty(0));

        ledc_update_duty(
            SERVO_MODE,
            SERVO_CHANNEL);

        vTaskDelay(pdMS_TO_TICKS(2000));

        /*-------------------------------
         * LED Fade OFF
         *------------------------------*/

        ledc_set_fade_with_time(
            LED_MODE,
            LED_CHANNEL,
            0,
            2000);

        ledc_fade_start(
            LED_MODE,
            LED_CHANNEL,
            LEDC_FADE_NO_WAIT);

        /*-------------------------------
         * Servo 90°
         *------------------------------*/

        ledc_set_duty(
            SERVO_MODE,
            SERVO_CHANNEL,
            servo_angle_to_duty(90));

        ledc_update_duty(
            SERVO_MODE,
            SERVO_CHANNEL);

        vTaskDelay(pdMS_TO_TICKS(2000));

        /*-------------------------------
         * Servo 180°
         *------------------------------*/

        ledc_set_duty(
            SERVO_MODE,
            SERVO_CHANNEL,
            servo_angle_to_duty(180));

        ledc_update_duty(
            SERVO_MODE,
            SERVO_CHANNEL);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}