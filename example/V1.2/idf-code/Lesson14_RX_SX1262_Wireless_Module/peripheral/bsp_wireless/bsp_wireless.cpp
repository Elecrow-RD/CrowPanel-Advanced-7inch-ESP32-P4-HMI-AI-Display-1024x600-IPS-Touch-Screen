/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_wireless.h"
#include <RadioLib.h>
#include "EspHal.h"
#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------
 * SX1262 radio state and class
 *--------------------------------------------------------------*/
#ifdef CONFIG_BSP_SX1262_ENABLED

/**
 * @brief Wrapper around the RadioLib SX1262 driver.
 *
 * Provides TX/RX init, packet send and packet receive helpers
 * exposed to C code via extern "C" shims.
 */
class BSP_SX1262
{
public:
    BSP_SX1262() {};
    ~BSP_SX1262() {};

    esp_err_t Sx1262_tx_init();
    void Sx1262_tx_deinit();
    bool Send_pack_radio();

    esp_err_t Sx1262_rx_init();
    void Sx1262_rx_deinit();
    void Received_pack_radio(size_t len);

protected:
private:
    static Module *bsp_sx_mod;
    static SX1262 *bsp_sx_radio;
};

EspHal lora_hal;
Module *BSP_SX1262::bsp_sx_mod = nullptr;
SX1262 *BSP_SX1262::bsp_sx_radio = nullptr;

/* TX/RX completion flags set from radio IRQ callbacks. */
static int lora_transmissionState = RADIOLIB_ERR_NONE;
volatile bool lora_transmittedFlag = true;
volatile bool lora_receivedFlag = false;
static size_t lora_received_len = 0;

/* Application-level callback invoked when a packet is received. */
static void (*rx_data_callback)(const char *data, size_t len, float rssi, float snr) = NULL;
#endif

/*---------------------------------------------------------------
 * nRF24 radio state and class
 *--------------------------------------------------------------*/
#ifdef CONFIG_BSP_NRF2401_ENABLED

/**
 * @brief Wrapper around the RadioLib nRF24 driver.
 */
class BSP_NRF2401
{
public:
    BSP_NRF2401() {};
    ~BSP_NRF2401() {};

    esp_err_t NRF24_tx_init();
    void NRF24_tx_deinit();
    bool Send_pack_radio();

    esp_err_t NRF24_rx_init();
    void NRF24_rx_deinit();
    void Received_pack_radio(size_t len);

protected:
private:
    static Module *bsp_nrf_mod;
    static nRF24 *bsp_nrf_radio;
};

EspHal nrf_hal;
Module *BSP_NRF2401::bsp_nrf_mod = nullptr;
nRF24 *BSP_NRF2401::bsp_nrf_radio = nullptr;

static int radio24_transmissionState = RADIOLIB_ERR_NONE;
volatile bool radio24_transmittedFlag = true;
volatile bool radio24_receivedFlag = false;
#endif

/*---------------------------------------------------------------
 * Optional UART transponder (transparent passthrough)
 *--------------------------------------------------------------*/
#ifdef CONFIG_BSP_UART_TRANSPOND_ENABLED

/**
 * @brief Initialise UART1 for transparent passthrough at 115200-8N1.
 */
extern "C" esp_err_t uart_transpond_init()
{
    esp_err_t err = ESP_OK;
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    err = uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        WIRELESS_UART_ERROR("uart driver install fail");
        return err;
    }
    uart_set_pin(UART_NUM_1, UART_GPIO_TXD, UART_GPIO_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    err = uart_param_config(UART_NUM_1, &uart_config);
    return err;
}

extern "C" void uart_transpond_deinit()
{
    uart_driver_delete(UART_NUM_1);
}
#endif

/*---------------------------------------------------------------
 * SX1262 implementation
 *--------------------------------------------------------------*/
#ifdef CONFIG_BSP_SX1262_ENABLED

static uint32_t sx1262_tx_counter = 0;

/**
 * @brief IRQ callback: signal that a TX packet has finished sending.
 */
static void set_sx1262_tx_flag(void)
{
    lora_transmittedFlag = true;
}

/**
 * @brief Initialise the SX1262 in TX mode.
 *
 * Brings up SPI at 8 MHz, creates the RadioLib Module and SX1262
 * objects, and configures LoRa: 915 MHz, 125 kHz BW, SF7, CR7,
 * private sync word, 22 dBm, 8-symbol preamble, 1.6 kHz coding rate.
 *
 * @return ESP_OK on success, ESP_FAIL on radio init error.
 */
esp_err_t BSP_SX1262::Sx1262_tx_init()
{
    lora_hal.setSpiPins(RADIO_GPIO_CLK, RADIO_GPIO_MISO, RADIO_GPIO_MOSI);
    lora_hal.setSpiFrequency(8000000);
    lora_hal.spiBegin();

    bsp_sx_mod = new Module(&lora_hal, SX1262_GPIO_NSS, SX1262_GPIO_IRQ, SX1262_GPIO_NRST, SX1262_GPIO_BUSY);
    bsp_sx_radio = new SX1262(bsp_sx_mod);
    int state = bsp_sx_radio->begin(915.0, 125.0, 7, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 22, 8, 1.6);
    if (state != RADIOLIB_ERR_NONE) {
        SX1262_ERROR("radio tx init failed, code :%d", state);
        lora_hal.spiEnd();
        return ESP_FAIL;
    }
    bsp_sx_radio->setPacketSentAction(set_sx1262_tx_flag);
    return ESP_OK;
}

/**
 * @brief Stop TX, clear the IRQ action and put the radio in standby.
 */
void BSP_SX1262::Sx1262_tx_deinit()
{
    lora_transmittedFlag = true;
    bsp_sx_radio->finishTransmit();
    bsp_sx_radio->clearPacketSentAction();
    bsp_sx_radio->standby();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    lora_hal.spiEnd();
}

/**
 * @brief Send one "TX_Hello World:N" packet if the previous TX finished.
 *
 * Uses the IRQ-driven flag so it never blocks waiting for the radio;
 * returns true only when a transmission was actually started.
 *
 * @return true if a packet was transmitted this call.
 */
bool BSP_SX1262::Send_pack_radio()
{
    static char text[32];

    /* Only send when the previous transmission has completed. */
    if (lora_transmittedFlag) {
        lora_transmittedFlag = false;

        if (lora_transmissionState == RADIOLIB_ERR_NONE) {
            SX1262_INFO("transmission finished!");
        } else {
            SX1262_INFO("failed, code :%d", lora_transmissionState);
        }

        bsp_sx_radio->finishTransmit();

        sx1262_tx_counter++;
        snprintf(text, sizeof(text), "TX_Hello World:%lu", (unsigned long)sx1262_tx_counter);
        size_t tx_len = strlen(text);
        lora_transmissionState = bsp_sx_radio->startTransmit((uint8_t *)text, tx_len + 1);

        if (lora_transmissionState != RADIOLIB_ERR_NONE) {
            SX1262_INFO("startTransmit failed, code: %d", lora_transmissionState);
        }
        return true;
    }
    return false;
}

extern "C" uint32_t sx1262_get_tx_counter(void)
{
    return sx1262_tx_counter;
}

extern "C" esp_err_t sx1262_tx_init()
{
    BSP_SX1262 obj;
    return obj.Sx1262_tx_init();
}

extern "C" void sx1262_tx_deinit()
{
    BSP_SX1262 obj;
    obj.Sx1262_tx_deinit();
}

extern "C" bool send_lora_pack_radio()
{
    BSP_SX1262 obj;
    return obj.Send_pack_radio();
}

/**
 * @brief IRQ callback: signal that a packet has been received.
 */
static void set_sx1262_rx_flag(void)
{
    lora_receivedFlag = true;
}

/**
 * @brief Initialise the SX1262 in RX mode.
 *
 * Same LoRa parameters as TX so the two ends can talk. Enables
 * boosted gain and starts continuous receive.
 *
 * @return ESP_OK on success, ESP_FAIL on radio error.
 */
esp_err_t BSP_SX1262::Sx1262_rx_init()
{
    lora_hal.setSpiPins(RADIO_GPIO_CLK, RADIO_GPIO_MISO, RADIO_GPIO_MOSI);
    lora_hal.setSpiFrequency(8000000);
    lora_hal.spiBegin();

    bsp_sx_mod = new Module(&lora_hal, SX1262_GPIO_NSS, SX1262_GPIO_IRQ, SX1262_GPIO_NRST, SX1262_GPIO_BUSY);
    bsp_sx_radio = new SX1262(bsp_sx_mod);
    int state = bsp_sx_radio->begin(915.0, 125.0, 7, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 22, 8, 1.6);
    if (state != RADIOLIB_ERR_NONE) {
        SX1262_ERROR("radio rx init failed, code :%d", state);
        return ESP_FAIL;
    }
    bsp_sx_radio->setPacketReceivedAction(set_sx1262_rx_flag);
    bsp_sx_radio->setRxBoostedGainMode(true);
    state = bsp_sx_radio->startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        SX1262_ERROR("radio start receive failed, code :%d", state);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief Stop RX, clear the IRQ action and put the radio in standby.
 */
void BSP_SX1262::Sx1262_rx_deinit()
{
    lora_receivedFlag = false;
    bsp_sx_radio->clearPacketReceivedAction();
    bsp_sx_radio->standby();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    lora_hal.spiEnd();
}

/**
 * @brief Read a received packet and dispatch it via the callback.
 *
 * If the packet length from the radio is valid it is used, otherwise
 * the passed-in length is the fallback. CRC errors are reported but
 * do not crash.
 *
 * @param len Expected packet length (fallback).
 */
void BSP_SX1262::Received_pack_radio(size_t len)
{
    if (lora_receivedFlag) {
        lora_receivedFlag = false;

        size_t actual_len = bsp_sx_radio->getPacketLength();
        if (actual_len > 0) {
            lora_received_len = actual_len;
        } else {
            lora_received_len = len;
        }

        uint8_t data[255];
        int state = bsp_sx_radio->readData(data, lora_received_len);
        if (state == RADIOLIB_ERR_NONE) {
            SX1262_INFO("Received packet!");
            SX1262_INFO("Valid Data : %.*s", lora_received_len, (char *)data);
            SX1262_INFO("RSSI:%.2f dBm", bsp_sx_radio->getRSSI());
            SX1262_INFO("SNR:%.2f dB", bsp_sx_radio->getSNR());
            SX1262_INFO("Frequency error:%.2f", bsp_sx_radio->getFrequencyError());

            if (rx_data_callback != NULL) {
                rx_data_callback((const char *)data, lora_received_len,
                                 bsp_sx_radio->getRSSI(), bsp_sx_radio->getSNR());
            }
        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            SX1262_ERROR("CRC error!");
        } else {
            SX1262_ERROR("radio receive failed, code :%d", state);
        }
    }
}

extern "C" esp_err_t sx1262_rx_init()
{
    BSP_SX1262 obj;
    return obj.Sx1262_rx_init();
}

extern "C" void sx1262_rx_deinit()
{
    BSP_SX1262 obj;
    obj.Sx1262_rx_deinit();
}

extern "C" void received_lora_pack_radio(size_t len)
{
    BSP_SX1262 obj;
    obj.Received_pack_radio(len);
}

extern "C" void sx1262_set_rx_callback(void (*callback)(const char *data, size_t len, float rssi, float snr))
{
    rx_data_callback = callback;
}

extern "C" size_t sx1262_get_received_len(void)
{
    return lora_received_len;
}

extern "C" bool sx1262_is_data_received(void)
{
    return lora_receivedFlag;
}

#endif

/*---------------------------------------------------------------
 * nRF24 implementation
 *--------------------------------------------------------------*/
#ifdef CONFIG_BSP_NRF2401_ENABLED

/**
 * @brief Initialise the nRF24 in TX mode.
 *
 * SPI at 8 MHz, 2400 MHz channel, 250 kbps, address width 5, pipe
 * address 0x01 0x02 0x11 0x12 0xFF.
 *
 * @return ESP_OK on success.
 */
esp_err_t BSP_NRF2401::NRF24_tx_init()
{
    nrf_hal.setSpiPins(RADIO_GPIO_CLK, RADIO_GPIO_MISO, RADIO_GPIO_MOSI);
    nrf_hal.setSpiFrequency(8000000);
    nrf_hal.spiBegin();

    bsp_nrf_mod = new Module(&nrf_hal, NRF24_GPIO_CS, NRF24_GPIO_IRQ, NRF24_GPIO_CE, -1);
    bsp_nrf_radio = new nRF24(bsp_nrf_mod);
    int state = bsp_nrf_radio->begin(2400, 250, 0, 5);
    if (state != RADIOLIB_ERR_NONE) {
        NRF2401_ERROR("radio tx init failed, code :%d", state);
        return ESP_FAIL;
    }
    uint8_t addr[] = {0x01, 0x02, 0x11, 0x12, 0xFF};
    state = bsp_nrf_radio->setTransmitPipe(addr);
    if (state != RADIOLIB_ERR_NONE) {
        NRF2401_ERROR("radio tx init failed, code :%d", state);
        return ESP_FAIL;
    }
    return ESP_OK;
}

void BSP_NRF2401::NRF24_tx_deinit()
{
    bsp_nrf_radio->finishTransmit();
    bsp_nrf_radio->standby();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    nrf_hal.spiEnd();
}

/**
 * @brief Transmit the fixed payload "#Hello World!" synchronously.
 * @return true after the transmit call returns.
 */
bool BSP_NRF2401::Send_pack_radio()
{
    const uint8_t tx_buff[] = "#Hello World!";
    bsp_nrf_radio->transmit(tx_buff, 14, 0);
    NRF2401_INFO("transmission finished!");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return true;
}

extern "C" esp_err_t nrf24_tx_init()
{
    BSP_NRF2401 obj;
    return obj.NRF24_tx_init();
}

extern "C" void nrf24_tx_deinit()
{
    BSP_NRF2401 obj;
    obj.NRF24_tx_deinit();
}

extern "C" bool send_nrf24_pack_radio()
{
    BSP_NRF2401 obj;
    return obj.Send_pack_radio();
}

/**
 * @brief IRQ callback: signal that an nRF24 packet was received.
 */
static void set_rx_flag(void)
{
    radio24_receivedFlag = true;
}

/**
 * @brief Initialise the nRF24 in RX mode with the same parameters as TX.
 * @return ESP_OK on success.
 */
esp_err_t BSP_NRF2401::NRF24_rx_init()
{
    nrf_hal.setSpiPins(RADIO_GPIO_CLK, RADIO_GPIO_MISO, RADIO_GPIO_MOSI);
    nrf_hal.setSpiFrequency(8000000);
    nrf_hal.spiBegin();

    bsp_nrf_mod = new Module(&nrf_hal, NRF24_GPIO_CS, NRF24_GPIO_IRQ, NRF24_GPIO_CE, -1);
    bsp_nrf_radio = new nRF24(bsp_nrf_mod);
    int state = bsp_nrf_radio->begin(2400, 250, 0, 5);
    if (state != RADIOLIB_ERR_NONE) {
        NRF2401_ERROR("radio rx init failed, code :%d", state);
        return ESP_FAIL;
    }
    uint8_t addr[] = {0x01, 0x02, 0x11, 0x12, 0xFF};
    state = bsp_nrf_radio->setReceivePipe(0, addr);
    if (state != RADIOLIB_ERR_NONE) {
        NRF2401_ERROR("radio rx init failed, code :%d", state);
        return ESP_FAIL;
    }
    bsp_nrf_radio->setPacketReceivedAction(set_rx_flag);
    state = bsp_nrf_radio->startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        NRF2401_ERROR("radio start receive failed, code :%d", state);
        return ESP_FAIL;
    }
    return ESP_OK;
}

void BSP_NRF2401::NRF24_rx_deinit()
{
    radio24_receivedFlag = false;
    bsp_nrf_radio->clearPacketReceivedAction();
    bsp_nrf_radio->standby();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    nrf_hal.spiEnd();
}

/**
 * @brief Read a received nRF24 packet and restart receive.
 * @param len Expected packet length.
 */
void BSP_NRF2401::Received_pack_radio(size_t len)
{
    if (radio24_receivedFlag) {
        radio24_receivedFlag = false;
        uint8_t data[len];
        int state = bsp_nrf_radio->readData(data, len);
        if (state == RADIOLIB_ERR_NONE) {
            NRF2401_INFO("Received packet!");
            NRF2401_INFO("Valid Data : %.*s", len, (char *)data);
        } else {
            NRF2401_ERROR("radio receive failed, code :%d", state);
        }
        bsp_nrf_radio->startReceive();
    }
}

extern "C" esp_err_t nrf24_rx_init()
{
    BSP_NRF2401 obj;
    return obj.NRF24_rx_init();
}

extern "C" void nrf24_rx_deinit()
{
    BSP_NRF2401 obj;
    obj.NRF24_rx_deinit();
}

extern "C" void received_nrf24_pack_radio(size_t len)
{
    BSP_NRF2401 obj;
    obj.Received_pack_radio(len);
}
#endif
