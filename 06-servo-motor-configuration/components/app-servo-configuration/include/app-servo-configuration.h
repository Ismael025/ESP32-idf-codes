#pragma once

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define LED_GPIO        GPIO_NUM_2
#define SERVO_GPIO      GPIO_NUM_18


static void led_timer_init(void);

static void led_channel_init(void);

static void servo_timer_init(void);

