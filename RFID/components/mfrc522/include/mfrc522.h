#ifndef MFRC522_H
#define MFRC522_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

/* Define GPIO pins according to your hardware layout */
#define MFRC522_SPI_HOST    SPI2_HOST
#define MFRC522_MOSI_GPIO   23
#define MFRC522_MISO_GPIO   19
#define MFRC522_SCLK_GPIO   18
#define MFRC522_CS_GPIO     5
#define MFRC522_RST_GPIO    22

/* Hardware Functions */
esp_err_t mfrc522_init(void);
void mfrc522_write_register(uint8_t reg, uint8_t value);
uint8_t mfrc522_read_register(uint8_t reg);

/* RFID Operations */
bool mfrc522_is_card_present(void);
esp_err_t mfrc522_read_uid(uint8_t uid[4]);
esp_err_t mfrc522_authenticate(uint8_t block_addr, const uint8_t *key, const uint8_t *uid);
esp_err_t mfrc522_write_block(uint8_t block_addr, const uint8_t *data_in);
void mfrc522_halt_card(void);

/* Clean All-in-One Write Function */
esp_err_t mfrc522_write_card_data(const uint8_t *uid, uint8_t block_addr, const uint8_t *key, const uint8_t *data_16bytes);

#endif // MFRC522_H
