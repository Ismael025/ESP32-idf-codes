#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_err.h"

void app_main(void)
{
    const uart_port_t uart_num = UART_NUM_0;
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    // 2. Set Pins (TX: 17, RX: 16, RTS: 18, CTS: 19)
    ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, 
                                        uart_buffer_size, 10, &uart_queue, 0));

    ESP_ERROR_CHECK(uart_set_rx_full_threshold(uart_num, 100));
    ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, 10));

     while(1) {
        
    char* test_str = "HELLO WORLD.\n";
    uart_write_bytes(uart_num, (const char*)test_str, strlen(test_str));
    vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_ERROR_CHECK(uart_wait_tx_done(uart_num, pdMS_TO_TICKS(100)));

    // 6. Read Data Example
    uint8_t data[128];
    size_t length = 0;
    
    ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, &length));
    if (length > 0) {
        int read_len = uart_read_bytes(uart_num, data, (length > 128 ? 128 : length), pdMS_TO_TICKS(100));
        printf("Read %d bytes\n", read_len);
    }
}