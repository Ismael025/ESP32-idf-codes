#pragma once

#include "driver/ledc.h"
#include "hal/gpio_types.h"

void initPWMOutput( gpio_num_t gpio_pin, ledc_channel_t channel);

void setPWMDutyPercentage( ledc_channel_t channel, uint8_t percentage);
