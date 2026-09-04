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
        ESP_LOGE(TAG, "MFRC522 initialization failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "RFID reader ready");

    uint8_t uid[4];

    while (1) {
        if (mfrc522_is_card_present()) {
            err = mfrc522_read_uid(uid);

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Card UID: %02X %02X %02X %02X", uid[0], uid[1], uid[2], uid[3]);

                /*
                 * Prevent printing the same card
                 * hundreds of times immediately.
                 */
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}