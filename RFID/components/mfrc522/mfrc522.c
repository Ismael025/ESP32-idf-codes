#include "mfrc522.h"
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

/* =========================================================
 * PICC / RFID CARD COMMANDS
 * =========================================================
 */
#define PICC_REQA         0x26
#define PICC_ANTICOLL_CL1 0x93

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
        ESP_LOGE(TAG, "SPI bus initialization failed: %s", esp_err_to_name(err));
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
        ESP_LOGE(TAG, "SPI device configuration failed: %s", esp_err_to_name(err));
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
        ESP_LOGE(TAG, "Reset GPIO configuration failed: %s", esp_err_to_name(err));
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

    /*
     * MFRC522 SPI write address format:
     * bit 7 = 0
     * bits 6:1 = register address
     * bit 0 = 0
     */
    tx_data[0] = (reg << 1) & 0x7E;
    tx_data[1] = value;

    spi_transaction_t transaction = {
        .length = 16,
        .tx_buffer = tx_data,
    };

    esp_err_t err = spi_device_transmit(rfid_device, &transaction);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Register write failed: %s", esp_err_to_name(err));
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

    /*
     * MFRC522 SPI read address format:
     * bit 7 = 1
     * bits 6:1 = register address
     * bit 0 = 0
     */
    tx_data[0] = ((reg << 1) & 0x7E) | 0x80;
    tx_data[1] = 0x00; /* Dummy byte */

    spi_transaction_t transaction = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    esp_err_t err = spi_device_transmit(rfid_device, &transaction);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Register read failed: %s", esp_err_to_name(err));
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
 * Send bytes through MFRC522 RF interface & get response.
 * =========================================================
 */
static esp_err_t mfrc522_transceive(uint8_t *send_data, uint8_t send_len, uint8_t *back_data, uint8_t *back_len, uint8_t tx_last_bits)
{
    /* Stop previous command. */
    mfrc522_write_register(CommandReg, PCD_IDLE);

    /* Clear interrupt flags. */
    mfrc522_write_register(ComIrqReg, 0x7F);

    /* Flush MFRC522 FIFO. */
    mfrc522_write_register(FIFOLevelReg, 0x80);

    /* Load outgoing bytes into FIFO. */
    for (uint8_t i = 0; i < send_len; i++) {
        mfrc522_write_register(FIFODataReg, send_data[i]);
    }

    /* Configure number of valid bits in final byte (REQA uses 7 bits). */
    mfrc522_write_register(BitFramingReg, tx_last_bits & 0x07);

    /* Start Transceive command. */
    mfrc522_write_register(CommandReg, PCD_TRANSCEIVE);

    /* Set StartSend bit. */
    mfrc522_set_bit_mask(BitFramingReg, 0x80);

    bool response_received = false;

    /* Wait for card response. */
    for (uint32_t i = 0; i < 2000; i++) {
        uint8_t irq = mfrc522_read_register(ComIrqReg);

        /* RxIRq or IdleIRq */
        if (irq & 0x30) {
            response_received = true;
            break;
        }

        /* Timer interrupt. */
        if (irq & 0x01) {
            mfrc522_clear_bit_mask(BitFramingReg, 0x80);
            return ESP_ERR_TIMEOUT;
        }
    }

    /* Clear StartSend. */
    mfrc522_clear_bit_mask(BitFramingReg, 0x80);

    if (!response_received) {
        return ESP_ERR_TIMEOUT;
    }

    /* Check MFRC522 errors. */
    uint8_t error = mfrc522_read_register(ErrorReg);
    if (error & 0x13) {
        return ESP_FAIL;
    }

    /* Determine number of received bytes. */
    uint8_t fifo_level = mfrc522_read_register(FIFOLevelReg);
    if (fifo_level > *back_len) {
        return ESP_ERR_INVALID_SIZE;
    }

    *back_len = fifo_level;

    /* Read response bytes from FIFO. */
    for (uint8_t i = 0; i < fifo_level; i++) {
        back_data[i] = mfrc522_read_register(FIFODataReg);
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

    /* REQA contains only 7 valid bits. */
    esp_err_t err = mfrc522_transceive(&command, 1, response, &response_len, 7);
    if (err != ESP_OK) {
        return false;
    }

    /* Valid ATQA response is 2 bytes. */
    return response_len == 2;
}

/* =========================================================
 * READ 4-BYTE UID
 * =========================================================
 */
esp_err_t mfrc522_read_uid(uint8_t uid[4])
{
    /* Cascade Level 1 anticollision: 0x93 = SEL_CL1, 0x20 = NVB */
    uint8_t command[2] = {PICC_ANTICOLL_CL1, 0x20};
    uint8_t response[5] = {0};
    uint8_t response_len = sizeof(response);

    esp_err_t err = mfrc522_transceive(command, 2, response, &response_len, 0);
    if (err != ESP_OK) {
        return err;
    }

    if (response_len != 5) {
        ESP_LOGE(TAG, "Unexpected UID response length: %u", response_len);
        return ESP_FAIL;
    }

    /* Calculate BCC. */
    uint8_t calculated_bcc = response[0] ^ response[1] ^ response[2] ^ response[3];

    /* Compare calculated BCC with received BCC. */
    if (calculated_bcc != response[4]) {
        ESP_LOGE(TAG, "UID BCC verification failed");
        return ESP_FAIL;
    }

    /* Copy UID. */
    uid[0] = response[0];
    uid[1] = response[1];
    uid[2] = response[2];
    uid[3] = response[3];

    return ESP_OK;
}