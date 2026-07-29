#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* I2C configuration */
#define I2C_PORT           0
#define I2C_SDA_PIN        21
#define I2C_SCL_PIN        22
#define I2C_FREQUENCY      100000

/* Change to 0x3F if your LCD does not use 0x27 */
#define LCD_ADDRESS        0x25

/* Common PCF8574 LCD backpack bit connections */
#define LCD_RS             0x01
#define LCD_ENABLE         0x04
#define LCD_BACKLIGHT      0x08

#define LCD_COMMAND        0x00
#define LCD_CHARACTER      LCD_RS

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t lcd_device;


/* Send one byte to the LCD I2C backpack */
static void lcd_i2c_write(uint8_t data)
{
    ESP_ERROR_CHECK(
        i2c_master_transmit(
            lcd_device,
            &data,
            1,
            1000
        )
    );
}


/* Send four bits and pulse the LCD Enable pin */
static void lcd_send_nibble(uint8_t nibble, uint8_t mode)
{
    uint8_t data =
        (nibble & 0xF0) |
        mode |
        LCD_BACKLIGHT;

    /* Enable HIGH */
    lcd_i2c_write(data | LCD_ENABLE);

    vTaskDelay(pdMS_TO_TICKS(1));

    /* Enable LOW: LCD accepts the data */
    lcd_i2c_write(data & ~LCD_ENABLE);

    vTaskDelay(pdMS_TO_TICKS(1));
}


/* Send a complete eight-bit command or character */
static void lcd_send_byte(uint8_t byte, uint8_t mode)
{
    /* Send high four bits */
    lcd_send_nibble(byte & 0xF0, mode);

    /* Send low four bits */
    lcd_send_nibble((byte << 4) & 0xF0, mode);
}


/* Send a complete string to the LCD */
static void lcd_send_string(const char *text)
{
    while (*text != '\0')
    {
        lcd_send_byte((uint8_t)*text, LCD_CHARACTER);
        text++;
    }
}


/* Initialize the ESP32 I2C controller */
static void i2c_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(
        i2c_new_master_bus(
            &bus_config,
            &i2c_bus
        )
    );

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_ADDRESS,
        .scl_speed_hz = I2C_FREQUENCY,
    };

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            i2c_bus,
            &device_config,
            &lcd_device
        )
    );
}


/* Initialize the LCD in four-bit mode */
static void lcd_init(void)
{
    /* Wait for the LCD to power up */
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Put LCD into a known state */
    lcd_send_nibble(0x30, LCD_COMMAND);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x30, LCD_COMMAND);
    vTaskDelay(pdMS_TO_TICKS(1));

    lcd_send_nibble(0x30, LCD_COMMAND);

    /* Switch to four-bit mode */
    lcd_send_nibble(0x20, LCD_COMMAND);

    /* Four-bit mode, two lines, 5x8 font */
    lcd_send_byte(0x28, LCD_COMMAND);

    /* Display ON, cursor OFF, blinking OFF */
    lcd_send_byte(0x0C, LCD_COMMAND);

    /* Move cursor to the right after each character */
    lcd_send_byte(0x06, LCD_COMMAND);

    /* Clear display */
    lcd_send_byte(0x01, LCD_COMMAND);
    vTaskDelay(pdMS_TO_TICKS(2));
}


void app_main(void)
{
    /* First initialize the I2C communication */
    i2c_init();

    /* Then initialize the LCD */
    lcd_init();

    /* Finally display the message */
    lcd_send_string("Hello World Ismael");
}