idf_component_register(
            SRCS "app-ledc.c"
            INCLUDE_DIRS "include"

            REQUIRES
            esp_driver_ledc
)