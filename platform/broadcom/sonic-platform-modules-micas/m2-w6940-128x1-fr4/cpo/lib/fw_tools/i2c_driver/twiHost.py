import sys
from array import array
import micas_i2c as i2c_dev
import struct
import logging
from GS_timing import *


class TwiHostObj(object):
    frame_max_size = 11

    def __init__(self, bus=1, verbose=False):
        self.bus = bus

    def read(self, addr, n, verbose=True):
        try:
            data_in = [0] * n 
            idx = 0
            read_size = 0
            try:
                dev = i2c_dev.i2c(self.bus)
            except:
                dev.close()
            while n:
                if n <= self.frame_max_size:
                    read_size = n
                    try:
                        data_raw = dev.read(read_size)
                    except:
                        dev.close()
                    n = 0
                else:
                    read_size = self.frame_max_size
                    try:
                        data_raw = dev.read(read_size)
                    except:
                        dev.close()
                    n = n - read_size
                data_readback = struct.unpack('B' * read_size, data_raw)
                for i in range (read_size):
                    data_in[idx + i] = data_readback[i]
                idx = idx + read_size
            dev.close()
            return idx, data_in
        except IOError:
            if verbose:
                print("I/O error occurred on read", flush=True)
            return 0, None

    def write(self, addr, data, verbose=True):
        try:
            data_raw = struct.pack('B' * len(data), *data)
            dev = i2c_dev.i2c(self.bus)
            dev.host_write(addr, data_raw)
            dev.close()
        except IOError:
            if verbose:
                print("I/O error occurred on write", flush=True)
            dev.close()

    def dev_write(self, addr, data, dev=None, verbose=True):
        try:
            data_raw = struct.pack('B' * len(data), *data)
            if dev is None:
                dev = i2c_dev.i2c(self.bus)
            dev.host_write(addr, data_raw)
            # dev.close()
        except IOError:
            if verbose:
                print("I/O error occurred on write", flush=True)
            dev.close()

    def ack_poll(self, addr, timeout_ms=100, delay_ms=10):
        n = 0
        data_out = array('B', [0])
        while n < timeout_ms:
            print('acknowledge_poll counts: %d' % n)
            try:
                data_in = self.reg_read(addr, 0x00, 1, verbose=False)
                data_out[0] = data_in[0]
                self.reg_write(addr, 0x00, data_out, verbose=False)
                return n < timeout_ms
            except IOError:                            # bypass I2CException due to Nack
                pass
            delay(delay_ms)
            n += delay_ms
        if n >= timeout_ms:
            return False
        return True

    def reg_write(self, addr, offset, data, verbose=True):
        try:
            dev = i2c_dev.i2c(addr, self.bus)
        except:
            dev.close()
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
            self.dev_write(addr, data_out, dev=dev, verbose=verbose)
        dev.close()

    def word_write(self, addr, offset, data):
        data_out = [0] * 3
        data_out[0] = offset
        data_out[1] = (data & 0xFF00) >> 8
        data_out[2] = (data & 0xFF)
        self.write(addr, data_out)

    def reg_read(self, addr, offset, n, verbose=True):
        self.write(addr, [offset])
        status, data_in = self.read(addr, n, verbose=verbose)
        return data_in


class TwiHostException(Exception):
    def __init__(self, detailed_error_message):
        super(TwiHostException, self).__init__(detailed_error_message)
