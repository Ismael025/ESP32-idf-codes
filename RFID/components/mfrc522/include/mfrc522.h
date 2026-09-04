#ifndef MFRC522_H
#define MFRC522_H

#include <stdint.h>
#include  <stdbool.h>


#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define MFRC522_SPI_HOST SPI2_HOST

#define MFRC522_MOSI_GPIO GPIO_NUM_23
#define MFRC522_MISO_GPIO GPIO_NUM_19
#define MFRC522_SCLK_GPIO GPIO_NUM_18

#define MFRC522_CS_GPIO   GPIO_NUM_25
#define MFRC522_RST_GPIO  GPIO_NUM_22

esp_err_t mfrc522_init(void);

uint8_t mfrc522_read_register(uint8_t reg);

void mfrc522_write_register(uint8_t reg, uint8_t value);

bool mfrc522_is_card_present(void);

esp_err_t mfrc522_read_uid(uint8_t uid[4]);


#endif