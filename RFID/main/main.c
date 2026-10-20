#include <stdio.h>
#include "mfrc522.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    esp_err_t err = mfrc522_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "RFID reader ready");

    uint8_t uid[4];
    const uint8_t default_key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const uint8_t payload[16] = "ESP32_WRITE_OK!";
    const uint8_t target_block = 4;

    while (1) {
        if (mfrc522_is_card_present()) {
            err = mfrc522_read_uid(uid);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Card UID: %02X %02X %02X %02X", uid[0], uid[1], uid[2], uid[3]);

                err = mfrc522_write_card_data(uid, target_block, default_key, payload);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
                } else {
                    ESP_LOGI(TAG, "Successfully wrote data to Block %d", target_block);
                }

                vTaskDelay(pdMS_TO_TICKS(1500));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}