import os
import fcntl
import time
import io
import struct
from cpo_platform_config import CPO_PLATFORM_CONFIG, OE_I2C_PATH_MAP

class oe_cmis_rw():

    def cmis_oe_bus_get(self, oe_index):
        return CPO_PLATFORM_CONFIG.get("oe_bus_map")[oe_index]

    def cmis_read(self, oe_index, bank, page, offset, rd_len, lock):
        raise NotImplementedError

    def cmis_write(self, oe_index, bank, page, offset, wr_data, lock):
        raise NotImplementedError

class oe_cmis_sysfs_rw(oe_cmis_rw):

    def acquire_lock(self, fd):
        fcntl.flock(fd, fcntl.LOCK_EX)

    def release_lock(self, fd):
        fcntl.flock(fd, fcntl.LOCK_UN)

    def dfd_i2c_sysfs_read(self, fd, offset, rd_len):
        val_list = []
        try:
            os.lseek(fd, offset, os.SEEK_SET)
            ret = os.read(fd, rd_len)
            for item in ret:
                val_list.append(item)
            return True, val_list
        except Exception as e:
            return False, str(e)

    def dfd_i2c_sysfs_write(self, fd, offset, wr_buf):
        try:
            os.lseek(fd, offset, os.SEEK_SET)
            ret = os.write(fd, bytes(wr_buf))
            if ret == len(wr_buf):
                return True, ""
            return False, "write buf len: %s, write len indeed: %s" % (len(wr_buf), ret)
        except Exception as e:
            return False, str(e)
        
    def cmis_read(self, oe_index, bank, page, offset, rd_len, lock=True):
        """
        Read cmis data from /sys/bus/i2c/devices/XX-0050/eeprom

        Args :
            bank: cmis bank
            page: cmis page
            offset: read data offset
            rd_len: read length

        Returns:
            True, value if read is successfully, False, errmsg if not
        """

        val_list = []
        msg = ""
        ret = ""
        fd = -1
        i2c_path = OE_I2C_PATH_MAP.get(oe_index)
        if not os.path.exists(i2c_path):
            msg = i2c_path + " not found!"
            return False, msg
        time.sleep(0.1)
        try:
            fd = os.open(i2c_path, os.O_RDWR)
            if fd < 0:
                msg = "cmis_sysfs_byte_read open %s failed, fd: %s" % (i2c_path, fd)
                return False, msg
            if (lock):
                self.acquire_lock(fd)
            # set bank
            status, log = self.dfd_i2c_sysfs_write(fd, 0x7e, [bank])
            if status is False:
                return status, log
            # set page
            status, log = self.dfd_i2c_sysfs_write(fd, 0x7f, [page])
            if status is False:
                return status, log
            # read data
            for i in range(0, rd_len):
                status, val = self.dfd_i2c_sysfs_read(fd, offset+i, 1)
                if status is False:
                    return status, val
                val_list.extend(val)
            return True, val_list
        except Exception as e:
            return False, str(e)
        finally:
            if (fd > 0) and lock:
                self.release_lock(fd)
            if fd > 0:
                os.close(fd)

    def cmis_write(self, oe_index, bank, page, offset, wr_data, lock=True):
        """
        Write cmis data through /sys/bus/i2c/devices/XX-0050/eeprom

        Args :
            bank: cmis bank
            page: cmis page
            offset: write data offset
            wr_data: write data

        Returns:
            True, "" if read is successfully, False, errmsg if not
        """
        val_list = []
        msg = ""
        ret = ""
        fd = -1
        i2c_path = OE_I2C_PATH_MAP.get(oe_index)
        if not os.path.exists(i2c_path):
            msg = i2c_path + " not found!"
            return False, msg
        # time.sleep(0.1)
        try:
            fd = os.open(i2c_path, os.O_RDWR)
            if fd < 0:
                msg = "cmis_sysfs_byte_read open %s failed, fd: %s" % (i2c_path, fd)
                return False, msg
            if (lock):
                self.acquire_lock(fd)
            # set bank
            status, log = self.dfd_i2c_sysfs_write(fd, 0x7e, [bank])
            if status is False:
                return status, log
            # set page
            status, log = self.dfd_i2c_sysfs_write(fd, 0x7f, [page])
            if status is False:
                return status, log
            # write data
            status, log = self.dfd_i2c_sysfs_write(fd, offset, wr_data)
            return status, log
        except Exception as e:
            return False, str(e)
        finally:
            if (fd > 0) and lock:
                self.release_lock(fd)
            if fd > 0:
                os.close(fd)

class I2CException(Exception):
    def __init__(self, detailed_error_message):
        super(I2CException, self).__init__(detailed_error_message)

I2C_SLAVE = 0x0703
I2C_SLAVE_FORCE	= 0x0706

class oe_cmis_i2c_rw(oe_cmis_rw):

    frame_max_size = 256

    def write(self, byte_data):
        self.i2c_fw.write(byte_data)

    def read(self, byte_num):
        return self.i2c_fr.read(byte_num)

    def host_read(self, rd_len, verbose = False):
        try:
            data_in = [0] * rd_len
            idx = 0
            read_size = 0
            while rd_len:
                if rd_len <= self.frame_max_size:
                    read_size = rd_len
                    try:
                        data_raw = self.read(read_size)
                    except:
                        raise I2CException("host_read() exception on i2c read")
                    rd_len = 0
                else:
                    read_size = self.frame_max_size
                    try:
                        data_raw = self.read(read_size)
                    except:
                        raise I2CException("host_read() exception on i2c read")
                    rd_len = rd_len - read_size
                # print(data_raw)
                data_readback = struct.unpack('B' * read_size, data_raw)
                # print(data_readback)
                for i in range (read_size):
                    data_in[idx + i] = data_readback[i]
                idx = idx + read_size
            return idx, data_in
        except (IOError, OSError):
            if verbose:
                print("I/O error occurred on read", flush=True)
            raise I2CException("host_read() exception on i2c read")

    def host_write(self, data, verbose = False):
        try:
            data_raw = struct.pack('B' * len(data), *data)
            self.write(data_raw)
        except (IOError, OSError):
            if verbose:
                print("I/O error occurred on write", flush=True)
            raise I2CException("host_write() exception on i2c write")

    def reg_read(self, offset, rd_len, verbose = False):
        self.host_write([offset], verbose=verbose)
        status, data = self.host_read(rd_len, verbose=verbose)
        return status, data

    def reg_write(self, offset, data, verbose = False):
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
            self.host_write(data_out, verbose=verbose)

    def set_page(self, page):
        self.reg_write(0x7F, [page])

    def set_bank(self, bank):
        self.reg_write(0x7E, [bank])

    def set_device_address(self, device_addr):
        try:
            if self.bus == 1:
                fcntl.ioctl(self.i2c_fr, I2C_SLAVE, device_addr)
                fcntl.ioctl(self.i2c_fw, I2C_SLAVE, device_addr)
            else:
                fcntl.ioctl(self.i2c_fr, I2C_SLAVE_FORCE, device_addr)
                fcntl.ioctl(self.i2c_fw, I2C_SLAVE_FORCE, device_addr)
        except:
            raise I2CException("cmis_read() cannot set dev addr")

    def cmis_read(self, oe_index, bank, page, offset, rd_len, lock=True):
        self.bus = self.cmis_oe_bus_get(oe_index=oe_index)
        try:
            self.i2c_fr = io.open("/dev/i2c-" + str(self.bus), "rb", buffering=0)
            self.i2c_fw = io.open("/dev/i2c-" + str(self.bus), "wb", buffering=0)
            self.set_device_address(0x50)
            if (offset >= 128):
                self.set_page(page=page)
                self.set_bank(bank=bank)
            status , data = self.reg_read(offset=offset, rd_len=rd_len)
            return True if status > 0 else False, data
        except Exception as e:
            return False, str(e)
        finally:
            self.i2c_fr.close()
            self.i2c_fw.close()

    def cmis_write(self, oe_index, bank, page, offset, wr_data, lock=True):
        self.bus = self.cmis_oe_bus_get(oe_index=oe_index)
        try:
            self.i2c_fr = io.open("/dev/i2c-" + str(self.bus), "rb", buffering=0)
            self.i2c_fw = io.open("/dev/i2c-" + str(self.bus), "wb", buffering=0)
            self.set_device_address(0x50)
            if (offset >= 128):
                self.set_page(page=page)
                self.set_bank(bank=bank)
            self.reg_write(offset=offset, data=wr_data)
            return True , ""
        except Exception as e:
            return False, str(e)
        finally:
            self.i2c_fr.close()
            self.i2c_fw.close()