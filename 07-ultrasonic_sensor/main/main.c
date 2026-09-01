#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#define TRIG_PIN     GPIO_NUM_5
#define ECHO_PIN     GPIO_NUM_18

#define SOUND_SPEED  0.034f 
#define TIMEOUT_US   30000  

void ultrasonic_init(void) {
    gpio_config_t io_conf = {};
    
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << TRIG_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << ECHO_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // Optional internal pull-up
    gpio_config(&io_conf);
    
    gpio_set_level(TRIG_PIN, 0);
}

float measure_distance(void) {
    gpio_set_level(TRIG_PIN, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    int64_t start_waiting = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if ((esp_timer_get_time() - start_waiting) > TIMEOUT_US) {
            return -1.0f;
        }
    }

    int64_t start_time = esp_timer_get_time();

    while (gpio_get_level(ECHO_PIN) == 1) {
        if ((esp_timer_get_time() - start_time) > TIMEOUT_US) {
            return -1.0f; // Timeout waiting for end (out of range)
        }
    }

    int64_t end_time = esp_timer_get_time();
    int64_t duration = end_time - start_time;

    float distance = (duration * SOUND_SPEED) / 2.0f;
    return distance;
}

void app_main(void) {

    ultrasonic_init();
    printf("HC-SR04 Ultrasonic Sensor Initialized via ESP-IDF\n");

    while (1) {
        float distance = measure_distance();
        
        if (distance < 0) {
            printf("Measurement Error / Out of Range\n");
        } else {
            printf("Distance: %.2f cm\n", distance);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}