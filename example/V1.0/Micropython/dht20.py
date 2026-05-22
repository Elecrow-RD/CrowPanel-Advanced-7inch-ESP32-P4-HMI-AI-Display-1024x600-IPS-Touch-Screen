from machine import I2C
from time import sleep_ms
class DHT20(object):
    # I2C 设备地址
    I2C_ADDR = 0x38

    def __init__(self, i2c, addr=I2C_ADDR):
        self.i2c = i2c
        if (self.read_status() & 0x80) == 0x80:
            self.dht20_init()

    def measure(self):
        self.i2c.write(bytes([0xac,0x33,0x00]))
        sleep_ms(80)
        cnt = 0
        while (self.read_status() & 0x80) == 0x80:
            sleep_ms(1)
            if cnt >= 100:
                cnt += 1
                break
        data = self.i2c.read(7, True)
        n = []
        for i in data[:]:
            n.append(i)
        self.data = n

    def read_status(self):
        data = self.i2c.read(1, True)
        return data[0]

    def dht20_init(self):
        self.i2c.write(bytes([0xa8,0x00,0x00]))
        sleep_ms(10)
        self.i2c.write(bytes([0xbe,0x08,0x00]))

    def calc_crc8(self,data):
        crc = 0xff
        for i in data[:-1]:
            crc ^= i
            for j in range(8):
                if crc & 0x80:
                    crc = (crc << 1) ^ 0x31
                else:
                    crc = (crc << 1)
        return crc

    def temperature(self):
        data = self.data
        Temper = 0
        if 1:
            Temper = (Temper | data[3]) << 8
            Temper = (Temper | data[4]) << 8
            Temper = Temper | data[5]
            Temper = Temper & 0xfffff
            Temper = Temper / 1048576 * 200 - 50
        return Temper

    def humidity(self):
        data = self.data
        humidity = 0
        if 1:
            humidity = (humidity | data[1]) << 8
            humidity = (humidity | data[2]) << 8
            humidity = humidity | data[3]
            humidity = humidity >> 4
            humidity = humidity / 1048576 * 100
        return humidity