# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import os
    import time
    import fcntl
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    I2C_MAX_ATTEMPT = 50
    LOCK_MAX_ATTEMPT = 100

    BUS_START = 24
    BUS_END = 31
    BUS_NUM = 8

    def __init__(self):
        SfpUtilBase.__init__(self)

    pidfile = None

    def file_rw_lock(self, file_path):
        global pidfile
        pidfile = open(file_path, "r")
        try:
            fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
            return True
        except Exception:
            if pidfile is not None:
                pidfile.close()
                pidfile = None
            return False

    def file_rw_unlock(self):
        try:
            global pidfile

            if pidfile is not None:
                fcntl.flock(pidfile, fcntl.LOCK_UN)
                pidfile.close()
                pidfile = None
            return True
        except Exception as e:
            return False

    def _sfp_write_file_path(self, file_path, offset, num_bytes, val):
        attempts = 0
        while attempts < self.I2C_MAX_ATTEMPT:
            try:
                file_path.seek(offset)
                file_path.write(bytearray([val])[0:num_bytes])
            except:
                attempts += 1
                time.sleep(0.05)
            else:
                return True
        return False

    def _sfp_read_file_path(self, file_path, offset, num_bytes, target_page):
        attempts = 0
        page_offset = 127
        while attempts < self.I2C_MAX_ATTEMPT:
            try:
                if offset > page_offset:
                    # verify page
                    file_path.seek(page_offset)
                    read_buf = file_path.read(1)
                    cur_page = read_buf[0]

                    if cur_page != target_page:
                        self._sfp_write_file_path(file_path, page_offset, 1, target_page)

                file_path.seek(offset)
                read_buf = file_path.read(num_bytes)

            except:
                attempts += 1
                time.sleep(0.05)
            else:
                return True, read_buf
        return False, None

    def _get_port_eeprom_path(self, devid):
        sysfs_sfp_i2c_client_eeprom_path = "/sys/bus/i2c/devices/i2c-%d/%d-0050/eeprom" % (devid, devid)
        return sysfs_sfp_i2c_client_eeprom_path

    def _read_eeprom_specific_bytes(self, sysfsfile_eeprom, offset, num_bytes, page):
        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        rv, raw = self._sfp_read_file_path(sysfsfile_eeprom, offset, num_bytes, page)
        if rv == False:
            return None

        try:
            for n in range(0, num_bytes):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except:
            return None

        return eeprom_raw

    def get_highest_temperature_cpo_oe(self):
        hightest_temperature = -99999

        read_eeprom_flag = False
        temperature_valid_flag = False

        for bus in range(self.BUS_START, self.BUS_START+self.BUS_NUM):
            offset = 14
            page = 0
            eeprom_path = self._get_port_eeprom_path(bus)
            try:
                if os.path.exists(eeprom_path) is False:
                    break

                with open(eeprom_path, mode="rb", buffering=0) as eeprom:
                    read_eeprom_flag = True
                    eeprom_raw = self._read_eeprom_specific_bytes(eeprom, offset, 2, page)
                    if len(eeprom_raw) != 0:
                        msb = int(eeprom_raw[0], 16)
                        lsb = int(eeprom_raw[1], 16)

                        result = (msb << 8) | (lsb & 0xff)
                        # To support temperature below 
                        if ((result & (1 << (16 - 1))) != 0):
                            result = result - (1 << 16)
                        result = float(result / 256.0)
                        if -50 <= result <= 200:
                            temperature_valid_flag = True
                            if hightest_temperature < result:
                                hightest_temperature = result
            
            except Exception as e:
                pass

        # all port read eeprom fail
        if read_eeprom_flag == False:
            hightest_temperature = -99999

        # all port temperature invalid
        elif read_eeprom_flag == True and temperature_valid_flag == False:
            hightest_temperature = -100000

        hightest_temperature = round(hightest_temperature, 2)

        return hightest_temperature

    def get_highest_temperature_cpo_rlm(self):
        hightest_temperature = -99999

        read_eeprom_flag = False
        temperature_valid_flag = False

        for bus in range(self.BUS_START, self.BUS_START+self.BUS_NUM):

            eeprom_path = self._get_port_eeprom_path(bus)

            if os.path.exists(eeprom_path) is False:
                break

            for rlm_page in [176, 180]:     # two rlm per oe
                offset = 150
                page = rlm_page
                try:
                    for i in range(0, self.LOCK_MAX_ATTEMPT):
                        ret = self.file_rw_lock(eeprom_path)
                        if ret is True:
                            break
                        time.sleep(0.001)
            
                    if ret is False:
                        pass

                    with open(eeprom_path, mode="r+b", buffering=0) as eeprom:
                        read_eeprom_flag = True
                        eeprom_raw = self._read_eeprom_specific_bytes(eeprom, offset, 2, page)

                        if len(eeprom_raw) != 0:
                            msb = int(eeprom_raw[0], 16)
                            lsb = int(eeprom_raw[1], 16)

                            result = (msb << 8) | (lsb & 0xff)
                            # To support temperature below 0
                            if ((result & (1 << (16 - 1))) != 0):
                                result = result - (1 << 16)
                            result = float(result / 256.0)
                            if -50 <= result <= 200:
                                temperature_valid_flag = True
                                if hightest_temperature < result:
                                    hightest_temperature = result
                    self.file_rw_unlock()

                except Exception as e:
                    self.file_rw_unlock()
                    pass

        # all port read eeprom fail
        if read_eeprom_flag == False:
            hightest_temperature = -99999

        # all port temperature invalid
        elif read_eeprom_flag == True and temperature_valid_flag == False:
            hightest_temperature = -100000

        hightest_temperature = round(hightest_temperature, 2)

        return hightest_temperature
