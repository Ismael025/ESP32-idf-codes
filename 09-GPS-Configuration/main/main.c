#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define GPS_UART_NUM      UART_NUM_1
#define BUF_SIZE          (1024)
#define ECHO_TEST_TXD     (GPIO_NUM_17)
#define ECHO_TEST_RXD     (GPIO_NUM_16)
#define ECHO_TEST_RTS     (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS     (UART_PIN_NO_CHANGE)

static const char *TAG = "GPS_UART";

void init_gps_uart(void) {
    uart_config_t uart_config = {
        .baud_rate = 9600, 
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    
    // Set UART pins (TX, RX, RTS, CTS)
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
    
    ESP_LOGI(TAG, "GPS UART initialized on RX pin %d", ECHO_TEST_RXD);
}

void gps_task(void *pvParameters) {
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    
    while (1) {
  
        int len = uart_read_bytes(GPS_UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(2000));
        
        if (len > 0) {
            data[len] = '\0'; // Null-terminate the string

            printf("%s", (char *)data);
        } else {

            ESP_LOGW(TAG, "Waiting for GPS data... (Ensure clear view of sky)");
        }
    }
    free(data);
}

void app_main(void) {
    init_gps_uart();
    xTaskCreate(gps_task, "gps_task", 4096, NULL, 5, NULL);
}