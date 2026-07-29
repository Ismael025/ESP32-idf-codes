idf_component_register (
    SRCS "app-servo-configuration.c" "app-servo-motor-configuration.c"
    INCLUDE_DIRS "include"

    REQUIRES
    esp_driver_ledc
    esp_driver_gpio

)