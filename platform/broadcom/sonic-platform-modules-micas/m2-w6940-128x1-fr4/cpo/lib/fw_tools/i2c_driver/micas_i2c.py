import sys
import io
import fcntl
from array import array
import struct
from GS_timing import *

I2C_SLAVE = 0x0703
I2C_SLAVE_FORCE	= 0x0706

class i2c:
    def __init__(self, bus):
        self.bus = bus
        self.fr = None
        self.fw = None
        # print(self.fw)
        # set device address

    def __enter__(self):
        return

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def set_bus(self, bus:int):
        self.bus = bus

    def open(self):
        self.fr = io.open("/dev/i2c-" + str(self.bus), "rb", buffering=0)
        self.fw = io.open("/dev/i2c-" + str(self.bus), "wb", buffering=0)

    def set_device_address(self, device_addr):
        if self.bus == 1:
            fcntl.ioctl(self.fr, I2C_SLAVE, device_addr)
            fcntl.ioctl(self.fw, I2C_SLAVE, device_addr)
        else:
            fcntl.ioctl(self.fr, I2C_SLAVE_FORCE, device_addr)
            fcntl.ioctl(self.fw, I2C_SLAVE_FORCE, device_addr)

    def write(self, byte_data):
        self.fw.write(byte_data)

    def read(self, byte_num):
        return self.fr.read(byte_num)

    def close(self):
        self.fw.close()
        self.fr.close()


class I2cHost(i2c):
    frame_max_size = 128

    def __init__(self, addr=0x50, bus=2, frm_max=128):
        super().__init__(bus=bus)
        self.bus = bus
        self.frame_max_size=frm_max

    def host_read(self, addr, n, verbose=True):
        try:
            try:
                self.set_device_address(device_addr=addr)
            except:
                self.close()
                raise I2CException("host_read() cannot set dev addr")
            data_in = [0] * n
            idx = 0
            read_size = 0
            while n:
                if n <= self.frame_max_size:
                    read_size = n
                    try:
                        data_raw = self.read(read_size)
                    except:
                        self.close()
                        raise I2CException("host_read() exception on i2c read")
                    n = 0
                else:
                    read_size = self.frame_max_size
                    try:
                        data_raw = self.read(read_size)
                    except:
                        self.close()
                        raise I2CException("host_read() exception on i2c read")
                    n = n - read_size
                # print(data_raw)
                data_readback = struct.unpack('B' * read_size, data_raw)
                # print(data_readback)
                for i in range (read_size):
                    data_in[idx + i] = data_readback[i]
                idx = idx + read_size
            return idx, data_in
        except (IOError, OSError):
            self.close()
            if verbose:
                print("I/O error occurred on read", flush=True)
            raise I2CException("host_read() exception on i2c read")

    def host_write(self, addr, data, verbose=True):
        try:
            self.set_device_address(device_addr=addr)
        except:
            self.close()
            raise I2CException("host_write() cannot set dev addr")
        try:
            data_raw = struct.pack('B' * len(data), *data)
            self.write(data_raw)
        except (IOError, OSError):
            self.close()
            if verbose:
                print("I/O error occurred on write", flush=True)
            raise I2CException("host_write() exception on i2c write")

    def dev_write(self, addr, data, verbose=True):
        try:
            data_raw = struct.pack('B' * len(data), *data)
            self.write(data_raw)
        except IOError:
            self.close()
            if verbose:
                print("I/O error occurred on write", flush=True)
            raise I2CException("dev_write() exception on i2c write")

    def ack_poll(self, addr, timeout_ms=100, delay_ms=10):
        print("ack_poll address: %02X" % addr)
        n = 0
        data_out = array('B', [0])
        while n < timeout_ms:
            try:
                data_in = self.reg_read(addr, 0x00, 1, verbose=False)
                data_out[0] = data_in[0]
                self.reg_write(addr, 0x00, data_out, verbose=False)
                return n < timeout_ms
            except (I2CException, IOError, OSError):                            # bypass I2CException due to Nack
                pass
            delay(delay_ms)
            n += delay_ms
        if n >= timeout_ms:
            return False
        return True

    def reg_write(self, addr, offset, data, verbose=True):
        try:
            self.open()
        except:
            self.close()
            raise I2CException("reg_write() Cannot Open I2C Dev")
        try:
            iter(data)
        except TypeError:
            data = [data]
        n = len(data)
        idx = 0
        while n:
            if n <= (self.frame_max_size - 1):
                data_out = [0] * (n + 1)
                data_out[0] = offset
                for i in range(n):
                    data_out[i + 1] = data[idx+i]
                offset = offset + n
                idx = idx + n
                n = 0
            else:
                data_out = [0] * self.frame_max_size
                data_out[0] = offset
                for i in range(self.frame_max_size-1):
                    data_out[i + 1] = data[idx+i]
                offset = offset + (self.frame_max_size - 1)
                idx = idx + (self.frame_max_size - 1)
                n = n - (self.frame_max_size - 1)
            self.host_write(addr, data_out, verbose=verbose)
        self.close()

    def word_write(self, addr, offset, data):
        data_out = [0] * 3
        data_out[0] = offset
        data_out[1] = (data & 0xFF00) >> 8
        data_out[2] = (data & 0xFF)
        self.write(addr, data_out)

    def reg_read(self, addr, offset, n, verbose=True):
        try:
            self.open()
        except:
            self.close()
            raise I2CException("reg_read() Cannot Open I2C Dev")
        self.host_write(addr, [offset], verbose=verbose)
        status, data_in = self.host_read(addr, n, verbose=verbose)
        self.close()
        return data_in


class I2CException(Exception):
    def __init__(self, detailed_error_message):
        super(I2CException, self).__init__(detailed_error_message)

