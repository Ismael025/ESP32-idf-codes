idf_component_register( 
        SRCS "main.c"

        INCLUDE_DIRS "."
                    
         PRIV_REQUIRES 
         ledc-basics
         esp_driver_ledc
         esp_adc
         )