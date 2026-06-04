# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# this driver uses a special i2c bus implimentation I have written.
# This implimentation takes into consideration the ESP32 and it having
# threading available. It also has some convience methods built into it
# that figure out what is wanting to be done automatically.
# read more about it's use in the stub files.

from micropython import const  # NOQA
import machine  # NOQA
import time
import lvgl as lv  # NOQA

_CMD_REG = const(0x8040)
_CMD_CHECK_REG = const(0x8046)
_CMD_READ_DATA = const(0x01)

_ESD_CHECK_REG = const(0x8041)

_STATUS_REG = const(0x814E)
_POINT_1_REG = const(0x8150)

_PRODUCT_ID_REG = const(0x8140)
_FIRMWARE_VERSION_REG = const(0x8144)
_VENDOR_ID_REG = const(0x814A)

_X_CORD_RES_REG = const(0x8146)
_Y_CORD_RES_REG = const(0x8148)

I2C_ADDR_1 = 0x5D
I2C_ADDR_2 = 0x14


class GT911():

    I2C_ADDR = I2C_ADDR_1
    PRESSED = lv.INDEV_STATE.PRESSED  # NOQA
    RELEASED = lv.INDEV_STATE.RELEASED  # NOQA

    def _read_reg(self, reg, num_bytes=None, buf=None):
        self._tx_buf[0] = reg >> 8
        self._tx_buf[1] = reg & 0xFF
        if num_bytes is not None:
            self._device.write_readinto(self._tx_mv[:2], self._rx_mv[:num_bytes])
        else:
            self._device.write_readinto(self._tx_mv[:2], buf)

    def _write_reg(self, reg, value=None, buf=None):
        if value is not None:
            self._tx_buf[0] = value
            self._device.write_mem(reg, self._tx_mv[:1])
        elif buf is not None:
            self._device.write_mem(reg, buf)

    def __init__(
        self,
        device,
        reset_pin=None,
        interrupt_pin=None,
        touch_cal=None,
        debug=False
    ):
        self._tx_buf = bytearray(3)
        self._tx_mv = memoryview(self._tx_buf)
        self._rx_buf = bytearray(6)
        self._rx_mv = memoryview(self._rx_buf)

        self._device = device

        if isinstance(reset_pin, int):
            reset_pin = machine.Pin(reset_pin, machine.Pin.OUT)

        if isinstance(interrupt_pin, int):
            interrupt_pin = machine.Pin(interrupt_pin, machine.Pin.OUT)

        self._reset_pin = reset_pin
        self._interrupt_pin = interrupt_pin

        self.hw_reset()

    def hw_reset(self):
        if self._interrupt_pin and self._reset_pin:
            self._interrupt_pin.init(self._interrupt_pin.OUT)
            self._interrupt_pin(0)
            self._reset_pin(0)
            time.sleep_ms(10)  # NOQA
            self._interrupt_pin(0)
            time.sleep_ms(1)  # NOQA
            self._reset_pin(1)
            time.sleep_ms(5)  # NOQA
            self._interrupt_pin(0)
            time.sleep_ms(50)  # NOQA
            self._interrupt_pin.init(self._interrupt_pin.IN)
            time.sleep_ms(50)  # NOQA

        self._write_reg(_ESD_CHECK_REG, 0x00)
        self._write_reg(_CMD_CHECK_REG, _CMD_READ_DATA)
        self._write_reg(_CMD_REG, _CMD_READ_DATA)

        self._read_reg(_PRODUCT_ID_REG, 4)

        product_id = ''
        for item in self._rx_buf[:4]:
            try:
                product_id += chr(item)
            except:  # NOQA
                break

        print('Touch Product id:', product_id)

        self._read_reg(_FIRMWARE_VERSION_REG, 2)
        print(
            'Touch Firmware version:',
            hex(self._rx_buf[0] + (self._rx_buf[1] << 8))
            )

        self._read_reg(_VENDOR_ID_REG, 1)
        print(f'Touch Vendor id: 0x{hex(self._rx_buf[0])[2:].upper()}')
        x, y = self.hw_size
        print(f'Touch resolution: width={x}, height={y}')

    @property
    def hw_size(self):
        self._read_reg(_X_CORD_RES_REG, 2)
        x = self._rx_buf[0] + (self._rx_buf[1] << 8)

        self._read_reg(_Y_CORD_RES_REG, 2)
        y = self._rx_buf[0] + (self._rx_buf[1] << 8)

        return x, y

    def _get_coords(self, indev, data):
        self._read_reg(_STATUS_REG, 1)
        touch_cnt = self._rx_buf[0] & 0x0F
        status = self._rx_buf[0] & 0x80
        if status:
            if touch_cnt == 1:
                self._read_reg(_POINT_1_REG, 6)

                data.state = lv.INDEV_STATE.PRESSED
                data.point.x = self._rx_buf[0] + (self._rx_buf[1] << 8)
                data.point.y = self._rx_buf[2] + (self._rx_buf[3] << 8)

                self._write_reg(_STATUS_REG, 0x00)

            elif touch_cnt == 0:
                data.state = lv.INDEV_STATE.RELEASED

            self._write_reg(_STATUS_REG, 0x00)
        #print(data.state, data.point.x, data.point.y)


