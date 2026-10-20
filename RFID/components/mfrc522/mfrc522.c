#include "mfrc522.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MFRC522";

/* =========================================================
 * SPI DEVICE HANDLE
 * =========================================================
 */
static spi_device_handle_t rfid_device;

/* =========================================================
 * MFRC522 REGISTERS
 * =========================================================
 */
#define CommandReg        0x01
#define ComIEnReg         0x02
#define DivIEnReg         0x03
#define ComIrqReg         0x04
#define DivIrqReg         0x05
#define ErrorReg          0x06
#define Status1Reg        0x07
#define Status2Reg        0x08
#define FIFODataReg       0x09
#define FIFOLevelReg      0x0A
#define WaterLevelReg     0x0B
#define ControlReg        0x0C
#define BitFramingReg     0x0D
#define CollReg           0x0E

#define ModeReg           0x11
#define TxModeReg         0x12
#define RxModeReg         0x13
#define TxControlReg      0x14
#define TxASKReg          0x15

#define TModeReg          0x2A
#define TPrescalerReg     0x2B
#define TReloadRegH       0x2C
#define TReloadRegL       0x2D

#define VersionReg        0x37

/* =========================================================
 * MFRC522 PCD COMMANDS
 * =========================================================
 */
#define PCD_IDLE          0x00
#define PCD_TRANSCEIVE    0x0C
#define PCD_MFAUTHENT     0x0E

/* =========================================================
 * PICC / RFID CARD COMMANDS
 * =========================================================
 */
#define PICC_REQA         0x26
#define PICC_ANTICOLL_CL1 0x93
#define PICC_CMD_MF_AUTH  0x60
#define PICC_CMD_MF_WRITE 0xA0
#define PICC_CMD_MF_READ  0x30
#define PICC_CMD_HALT     0x50

/* =========================================================
 * PRIVATE FUNCTION PROTOTYPES
 * =========================================================
 */
static void mfrc522_set_bit_mask(uint8_t reg, uint8_t mask);
static void mfrc522_clear_bit_mask(uint8_t reg, uint8_t mask);
static esp_err_t mfrc522_transceive(uint8_t *send_data, uint8_t send_len, uint8_t *back_data, uint8_t *back_len, uint8_t tx_last_bits);

/* =========================================================
 * INITIALIZATION
 * =========================================================
 */
esp_err_t mfrc522_init(void)
{
    /* STEP 1: Configure SPI bus pins. */
    spi_bus_config_t bus_config = {
        .mosi_io_num = MFRC522_MOSI_GPIO,
        .miso_io_num = MFRC522_MISO_GPIO,
        .sclk_io_num = MFRC522_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    /* STEP 2: Initialize the ESP32 SPI bus. */
    esp_err_t err = spi_bus_initialize(MFRC522_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        return err;
    }

    /* STEP 3: Configure MFRC522 as an SPI device. */
    spi_device_interface_config_t device_config = {
        .clock_speed_hz = 1000000,
        .mode = 0,
        .spics_io_num = MFRC522_CS_GPIO,
        .queue_size = 1,
    };

    /* STEP 4: Add MFRC522 to the SPI bus. */
    err = spi_bus_add_device(MFRC522_SPI_HOST, &device_config, &rfid_device);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        return err;
    }

    /* STEP 5: Configure MFRC522 reset pin. */
    gpio_config_t rst_config = {
        .pin_bit_mask = (1ULL << MFRC522_RST_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    err = gpio_config(&rst_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        return err;
    }

    /* STEP 6: Hardware-reset the MFRC522. */
    gpio_set_level(MFRC522_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(MFRC522_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* STEP 7: Configure MFRC522 internal timer. */
    mfrc522_write_register(TModeReg, 0x80);
    mfrc522_write_register(TPrescalerReg, 0xA9);
    mfrc522_write_register(TReloadRegH, 0x03);
    mfrc522_write_register(TReloadRegL, 0xE8);

    /* STEP 8: 100% ASK modulation. */
    mfrc522_write_register(TxASKReg, 0x40);

    /* STEP 9: Configure CRC preset. */
    mfrc522_write_register(ModeReg, 0x3D);

    /* STEP 10: Turn RF antenna ON. */
    mfrc522_set_bit_mask(TxControlReg, 0x03);

    ESP_LOGI(TAG, "MFRC522 initialized successfully");
    return ESP_OK;
}

/* =========================================================
 * WRITE REGISTER
 * =========================================================
 */
void mfrc522_write_register(uint8_t reg, uint8_t value)
{
    uint8_t tx_data[2] = {0};

    tx_data[0] = (reg << 1) & 0x7E;
    tx_data[1] = value;

    spi_transaction_t transaction = {
        .length = 16,
        .tx_buffer = tx_data,
    };

    esp_err_t err = spi_device_transmit(rfid_device, &transaction);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
    }
}

/* =========================================================
 * READ REGISTER
 * =========================================================
 */
uint8_t mfrc522_read_register(uint8_t reg)
{
    uint8_t tx_data[2] = {0};
    uint8_t rx_data[2] = {0};

    tx_data[0] = ((reg << 1) & 0x7E) | 0x80;
    tx_data[1] = 0x00;

    spi_transaction_t transaction = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    esp_err_t err = spi_device_transmit(rfid_device, &transaction);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        return 0x00;
    }

    return rx_data[1];
}

/* =========================================================
 * SET REGISTER BITS
 * =========================================================
 */
static void mfrc522_set_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t value = mfrc522_read_register(reg);
    mfrc522_write_register(reg, value | mask);
}

/* =========================================================
 * CLEAR REGISTER BITS
 * =========================================================
 */
static void mfrc522_clear_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t value = mfrc522_read_register(reg);
    mfrc522_write_register(reg, value & (~mask));
}

/* =========================================================
 * TRANSCEIVE
 * =========================================================
 */
static esp_err_t mfrc522_transceive(uint8_t *send_data, uint8_t send_len, uint8_t *back_data, uint8_t *back_len, uint8_t tx_last_bits)
{
    mfrc522_write_register(CommandReg, PCD_IDLE);
    mfrc522_write_register(ComIrqReg, 0x7F);
    mfrc522_write_register(FIFOLevelReg, 0x80);

    for (uint8_t i = 0; i < send_len; i++) {
        mfrc522_write_register(FIFODataReg, send_data[i]);
    }

    mfrc522_write_register(BitFramingReg, tx_last_bits & 0x07);
    mfrc522_write_register(CommandReg, PCD_TRANSCEIVE);
    mfrc522_set_bit_mask(BitFramingReg, 0x80);

    bool response_received = false;

    for (uint32_t i = 0; i < 2000; i++) {
        uint8_t irq = mfrc522_read_register(ComIrqReg);

        if (irq & 0x30) {
            response_received = true;
            break;
        }

        if (irq & 0x01) {
            mfrc522_clear_bit_mask(BitFramingReg, 0x80);
            return ESP_ERR_TIMEOUT;
        }
    }

    mfrc522_clear_bit_mask(BitFramingReg, 0x80);

    if (!response_received) {
        return ESP_ERR_TIMEOUT;
    }

    uint8_t error = mfrc522_read_register(ErrorReg);
    if (error & 0x13) {
        return ESP_FAIL;
    }

    uint8_t fifo_level = mfrc522_read_register(FIFOLevelReg);
    
    if (back_data && back_len) {
        if (fifo_level > *back_len) {
            return ESP_ERR_INVALID_SIZE;
        }

        *back_len = fifo_level;

        for (uint8_t i = 0; i < fifo_level; i++) {
            back_data[i] = mfrc522_read_register(FIFODataReg);
        }
    }

    return ESP_OK;
}

/* =========================================================
 * CHECK IF CARD IS PRESENT
 * =========================================================
 */
bool mfrc522_is_card_present(void)
{
    uint8_t command = PICC_REQA;
    uint8_t response[2] = {0};
    uint8_t response_len = sizeof(response);

    esp_err_t err = mfrc522_transceive(&command, 1, response, &response_len, 7);
    if (err != ESP_OK) {
        return false;
    }

    return response_len == 2;
}

/* =========================================================
 * READ 4-BYTE UID
 * =========================================================
 */
esp_err_t mfrc522_read_uid(uint8_t uid[4])
{
    uint8_t command[2] = {PICC_ANTICOLL_CL1, 0x20};
    uint8_t response[5] = {0};
    uint8_t response_len = sizeof(response);

    esp_err_t err = mfrc522_transceive(command, 2, response, &response_len, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        return err;
    }

    if (response_len != 5) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(ESP_ERR_INVALID_SIZE));
        return ESP_FAIL;
    }

    uint8_t calculated_bcc = response[0] ^ response[1] ^ response[2] ^ response[3];
    if (calculated_bcc != response[4]) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(ESP_ERR_INVALID_CRC));
        return ESP_FAIL;
    }

    uid[0] = response[0];
    uid[1] = response[1];
    uid[2] = response[2];
    uid[3] = response[3];

    return ESP_OK;
}

/* =========================================================
 * AUTHENTICATE BLOCK
 * =========================================================
 */
esp_err_t mfrc522_authenticate(uint8_t block_addr, const uint8_t *key, const uint8_t *uid)
{
    uint8_t buff[12];

    buff[0] = PICC_CMD_MF_AUTH;
    buff[1] = block_addr;
    for (int i = 0; i < 6; i++) {
        buff[2 + i] = key[i];
    }
    for (int i = 0; i < 4; i++) {
        buff[8 + i] = uid[i];
    }

    mfrc522_write_register(CommandReg, PCD_IDLE);
    mfrc522_write_register(ComIrqReg, 0x7F);
    mfrc522_write_register(FIFOLevelReg, 0x80);

    for (int i = 0; i < 12; i++) {
        mfrc522_write_register(FIFODataReg, buff[i]);
    }

    mfrc522_write_register(CommandReg, PCD_MFAUTHENT);

    TickType_t start_tick = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start_tick) < pdMS_TO_TICKS(100)) {
        uint8_t status2 = mfrc522_read_register(Status2Reg);
        if (status2 & 0x08) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(ESP_ERR_TIMEOUT));
    return ESP_FAIL;
}

/* =========================================================
 * WRITE 16 BYTES TO BLOCK
 * =========================================================
 */
esp_err_t mfrc522_write_block(uint8_t block_addr, const uint8_t *data_in)
{
    uint8_t cmd_buff[2] = {PICC_CMD_MF_WRITE, block_addr};
    uint8_t back_data[4];
    uint8_t back_len = sizeof(back_data);

    esp_err_t err = mfrc522_transceive(cmd_buff, 2, back_data, &back_len, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        return err;
    }

    if (back_len == 0 || (back_data[0] & 0x0F) != 0x0A) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(ESP_ERR_INVALID_RESPONSE));
        return ESP_FAIL;
    }

    uint8_t data_buff[16];
    memcpy(data_buff, data_in, 16);
    back_len = sizeof(back_data);

    err = mfrc522_transceive(data_buff, 16, back_data, &back_len, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        return err;
    }

    if (back_len == 0 || (back_data[0] & 0x0F) != 0x0A) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(ESP_ERR_INVALID_RESPONSE));
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* =========================================================
 * HALT CARD / STOP CRYPTO
 * =========================================================
 */
void mfrc522_halt_card(void)
{
    uint8_t command[2] = {PICC_CMD_HALT, 0x00};
    uint8_t response[2];
    uint8_t response_len = sizeof(response);

    mfrc522_transceive(command, 2, response, &response_len, 0);
    mfrc522_clear_bit_mask(Status2Reg, 0x08);
}

/* =========================================================
 * ALL-IN-ONE WRITE FUNCTION
 * =========================================================
 */
esp_err_t mfrc522_write_card_data(const uint8_t *uid, uint8_t block_addr, const uint8_t *key, const uint8_t *data_16bytes)
{
    esp_err_t err = mfrc522_authenticate(block_addr, key, uid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        mfrc522_halt_card();
        return err;
    }

    err = mfrc522_write_block(block_addr, data_16bytes);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "configuration unsuccessfully: %s", esp_err_to_name(err));
        mfrc522_halt_card();
        return err;
    }

    mfrc522_halt_card();
    return ESP_OK;
}