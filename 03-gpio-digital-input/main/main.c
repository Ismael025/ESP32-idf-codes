#include <stdio.h>
#include "app-digital-input.h"
#include "app-config.h"

void app_main(void)
{
        gpioGetDigitalLevel(BTN_PIN);
}