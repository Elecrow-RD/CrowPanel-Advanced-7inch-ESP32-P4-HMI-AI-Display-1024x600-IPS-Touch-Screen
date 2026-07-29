/**
 * @file EspHal.h
 * @brief Teaching example that demonstrates how to transmit numbered packets with the SX1262 LoRa radio.
 *
 * Comments emphasize program structure, hardware intent, and call order
 * while preserving the original executable behavior.
 */

#pragma once

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/rtc_io.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class EspHal : public RadioLibHal
{
private:
  struct
  {
    int8_t sck, miso, mosi;
  } _spiPins = {-1, -1, -1};
  spi_device_handle_t _spiHandle;
  bool _spiInitialized = false;
  uint32_t _spiFrequency = 8000000; // 8MHz

public:
  /**
   * @brief Construct the ESP32 hardware adapter with RadioLib-compatible constants.
   *
   * Parameters: None.
   * @return A ready hardware-abstraction object; SPI is initialized later.
   * @note Constructed when the wireless driver creates its global HAL object.
   */
  EspHal() : RadioLibHal(
                 GPIO_MODE_INPUT,   // input mode
                 GPIO_MODE_OUTPUT,  // output mode
                 0,                 // low level
                 1,                 // high level
                 GPIO_INTR_POSEDGE, // rising edge
                 GPIO_INTR_NEGEDGE  // falling edge
             )
  {
  }

  /**
   * @brief Configure one GPIO for the mode requested by RadioLib.
   *
   * @param pin GPIO number to access.
   * @param mode GPIO or interrupt mode requested by RadioLib.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void pinMode(uint32_t pin, uint32_t mode) override
  {
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode = static_cast<gpio_mode_t>(mode),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .hys_ctrl_mode = GPIO_HYS_SOFT_DISABLE,
    };
    gpio_config(&cfg);
  }

  /**
   * @brief Drive a GPIO to the logic level requested by RadioLib.
   *
   * @param pin GPIO number to access.
   * @param value Logic level to write.
   * @return None.
   * @note Called when the lesson requests the corresponding data operation.
   */
  void digitalWrite(uint32_t pin, uint32_t value) override
  {
    gpio_set_level(static_cast<gpio_num_t>(pin), value);
  }

  /**
   * @brief Read a GPIO level for RadioLib.
   *
   * @param pin GPIO number to access.
   * @return Current GPIO logic level.
   * @note Called when the lesson needs to obtain or process new data.
   */
  uint32_t digitalRead(uint32_t pin) override
  {
    return gpio_get_level(static_cast<gpio_num_t>(pin));
  }

  /**
   * @brief Connect a RadioLib callback to an ESP32 GPIO interrupt.
   *
   * @param interruptNum GPIO number used as the interrupt source.
   * @param interruptCb Callback invoked by the GPIO interrupt.
   * @param mode GPIO or interrupt mode requested by RadioLib.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override
  {
    if (interruptNum == RADIOLIB_NC)
    {
      return;
    }

    gpio_install_isr_service((int)ESP_INTR_FLAG_IRAM);
    gpio_set_intr_type((gpio_num_t)interruptNum, (gpio_int_type_t)(mode & 0x7));

    // this uses function typecasting, which is not defined when the functions have different signatures
    // untested and might not work
    gpio_isr_handler_add((gpio_num_t)interruptNum, (void (*)(void *))interruptCb, NULL);
  }

  /**
   * @brief Remove a previously attached RadioLib GPIO interrupt.
   *
   * @param interruptNum GPIO number used as the interrupt source.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void detachInterrupt(uint32_t interruptNum) override
  {
    if (interruptNum == RADIOLIB_NC)
    {
      return;
    }

    gpio_isr_handler_remove((gpio_num_t)interruptNum);
    gpio_wakeup_disable((gpio_num_t)interruptNum);
    gpio_set_intr_type((gpio_num_t)interruptNum, GPIO_INTR_DISABLE);
  }

  /**
   * @brief Suspend the current task for a millisecond delay requested by RadioLib.
   *
   * @param ms Delay duration in milliseconds.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void delay(RadioLibTime_t ms) override
  {
    vTaskDelay(pdMS_TO_TICKS(ms));
  }

  /**
   * @brief Provide the short busy-wait delay required by RadioLib timing.
   *
   * @param us Delay duration in microseconds.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void delayMicroseconds(RadioLibTime_t us) override
  {
    uint64_t end = esp_timer_get_time() + us;
    while (esp_timer_get_time() < end)
      ;
  }

  /**
   * @brief Return the elapsed scheduler time in milliseconds.
   *
   * Parameters: None.
   * @return Elapsed time in milliseconds.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  RadioLibTime_t millis() override
  {
    return pdTICKS_TO_MS(xTaskGetTickCount());
  }

  /**
   * @brief Return the elapsed high-resolution time in microseconds.
   *
   * Parameters: None.
   * @return Elapsed time in microseconds.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  RadioLibTime_t micros() override
  {
    return esp_timer_get_time();
  }

  /**
   * @brief Measure how long a GPIO remains at the requested logic level.
   *
   * @param pin GPIO number to access.
   * @param state Logic level whose pulse width will be measured.
   * @param timeout Maximum time to wait, expressed in FreeRTOS ticks.
   * @return Measured pulse width in microseconds, or 0 on timeout.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  long pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) override
  {
    const RadioLibTime_t start = micros();
    while (digitalRead(pin) != state)
    {
      if (micros() - start > timeout)
        return 0;
    }
    const RadioLibTime_t pulseStart = micros();
    while (digitalRead(pin) == state)
    {
      if (micros() - start > timeout)
        return 0;
    }
    return micros() - pulseStart;
  }

  /**
   * @brief Initialize the ESP32 SPI bus and attach the RadioLib device.
   *
   * Parameters: None.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void spiBegin() override
  {
    if (_spiInitialized)
      return;

    spi_bus_config_t buscfg = {
        .mosi_io_num = _spiPins.mosi,
        .miso_io_num = _spiPins.miso,
        .sclk_io_num = _spiPins.sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096};

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_speed_hz = (int)_spiFrequency,
        .spics_io_num = -1,
        .queue_size = 7};

    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &devcfg, &_spiHandle));
    _spiInitialized = true;
  }

  /**
   * @brief Begin a RadioLib SPI transaction; bus setup is already persistent.
   *
   * Parameters: None.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void spiBeginTransaction() override
  {
  }

  /**
   * @brief Transfer one RadioLib data block over the ESP32 SPI bus.
   *
   * @param out Bytes transmitted to the SPI device.
   * @param len Number of bytes available or expected.
   * @param in Buffer that receives bytes from the SPI device.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void spiTransfer(uint8_t *out, size_t len, uint8_t *in) override
  {
    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = len * 8,
        .rxlength = 0,
        .user = nullptr,
        .tx_buffer = out,
        .rx_buffer = in};
    ESP_ERROR_CHECK(spi_device_transmit(_spiHandle, &t));
  }

  /**
   * @brief End a RadioLib SPI transaction; no per-transaction cleanup is required.
   *
   * Parameters: None.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void spiEndTransaction() override
  {
  }

  /**
   * @brief Detach the RadioLib device and release the ESP32 SPI bus.
   *
   * Parameters: None.
   * @return None.
   * @note Called when the related hardware service is being stopped or reconfigured.
   */
  void spiEnd() override
  {
    if (!_spiInitialized)
      return;
    spi_bus_remove_device(_spiHandle);
    spi_bus_free(SPI3_HOST);
    _spiInitialized = false;
  }

  /**
   * @brief Initialize the hardware abstraction layer for RadioLib.
   *
   * Parameters: None.
   * @return None.
   * @note Called during lesson setup before the related hardware is used.
   */
  void init() override
  {
    spiBegin();
  }

  /**
   * @brief Shut down the hardware abstraction layer for RadioLib.
   *
   * Parameters: None.
   * @return None.
   * @note Called when the related hardware service is being stopped or reconfigured.
   */
  void term() override
  {
    spiEnd();
  }

  /**
   * @brief Store the SPI pin mapping that will be used during bus initialization.
   *
   * @param sck SPI clock GPIO.
   * @param miso SPI controller input GPIO.
   * @param mosi SPI controller output GPIO.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void setSpiPins(int8_t sck, int8_t miso, int8_t mosi)
  {
    _spiPins = {sck, miso, mosi};
  }

  /**
   * @brief Store the SPI clock frequency used for later transactions.
   *
   * @param freq SPI clock frequency in hertz.
   * @return None.
   * @note Called by the lesson workflow when this helper operation is required.
   */
  void setSpiFrequency(uint32_t freq)
  {
    _spiFrequency = freq;
  }
};