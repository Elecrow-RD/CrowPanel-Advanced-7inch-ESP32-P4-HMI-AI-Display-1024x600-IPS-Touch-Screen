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

  void digitalWrite(uint32_t pin, uint32_t value) override
  {
    gpio_set_level(static_cast<gpio_num_t>(pin), value);
  }

  uint32_t digitalRead(uint32_t pin) override
  {
    return gpio_get_level(static_cast<gpio_num_t>(pin));
  }

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

  void delay(RadioLibTime_t ms) override
  {
    vTaskDelay(pdMS_TO_TICKS(ms));
  }

  void delayMicroseconds(RadioLibTime_t us) override
  {
    uint64_t end = esp_timer_get_time() + us;
    while (esp_timer_get_time() < end)
      ;
  }

  RadioLibTime_t millis() override
  {
    return pdTICKS_TO_MS(xTaskGetTickCount());
  }

  RadioLibTime_t micros() override
  {
    return esp_timer_get_time();
  }

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

  void spiBeginTransaction() override
  {
  }

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

  void spiEndTransaction() override
  {
  }

  void spiEnd() override
  {
    if (!_spiInitialized)
      return;
    spi_bus_remove_device(_spiHandle);
    spi_bus_free(SPI3_HOST);
    _spiInitialized = false;
  }

  void init() override
  {
    spiBegin();
  }

  void term() override
  {
    spiEnd();
  }

  void setSpiPins(int8_t sck, int8_t miso, int8_t mosi)
  {
    _spiPins = {sck, miso, mosi};
  }

  void setSpiFrequency(uint32_t freq)
  {
    _spiFrequency = freq;
  }
};