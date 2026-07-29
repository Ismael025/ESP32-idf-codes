idf_component_register(
    SRCS "app-analog-input.c"

    INCLUDE_DIRS "include"

    REQUIRES
    esp_adc
    esp_driver_gpio

)