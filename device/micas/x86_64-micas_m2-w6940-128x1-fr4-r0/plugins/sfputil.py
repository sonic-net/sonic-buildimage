# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import os
    import time
    import fcntl
    import syslog
    import sonic_platform
    from datetime import datetime
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

SYSLOG_LEVEL = syslog.LOG_WARNING

def write_syslog(lvl, msg):
    # EMERG=0, ALERT=1, CRIT=2, ERR=3, WARNING=4, NOTICE=5, INFO=6, DEBUG=7
    if lvl <= SYSLOG_LEVEL:
        syslog.syslog(lvl, msg)

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    I2C_MAX_ATTEMPT = 50
    LOCK_MAX_ATTEMPT = 100
    TEMP_VALID_LOW = -50
    TEMP_VALID_HIGH = 200
    TEMP_INVALID = -100000
    TEMP_ERROR = -99999

    BUS_START = 24
    BUS_END = 31
    BUS_NUM = 8

    def __init__(self):
        self.pidfile_dict = dict()
        SfpUtilBase.__init__(self)
        syslog.openlog("CPO_TEMP_DEBUG", syslog.LOG_PID)

    def file_rw_lock(self, file_path):
        pidfile = self.pidfile_dict.get(file_path, None)
        if pidfile == None:
            pidfile = open(file_path, "r")
            self.pidfile_dict[file_path] = pidfile
        if pidfile == None:
            return False
        # Retry 100 times to lock file
        for i in range(0, 100):
            try:
                fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
                return True
            except Exception:
                time.sleep(0.05)
                continue

        pidfile.close()
        self.pidfile_dict[file_path] = None
        return False

    def file_rw_unlock(self, file_path):
        pidfile = self.pidfile_dict.get(file_path, None)
        if pidfile == None:
            return True
        try:
            fcntl.flock(pidfile, fcntl.LOCK_UN)
            pidfile.close()
            self.pidfile_dict[file_path] = None
            return True
        except Exception as e:
            print("file unlock err, msg:%s" % (str(e)))
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
        highest_temperature = self.TEMP_INVALID
        try:
            platform_chassis = sonic_platform.platform.Platform().get_chassis()
            if platform_chassis is None:
                write_syslog(syslog.LOG_ERR, "platform_chassis is none")
                return self.TEMP_ERROR
            cpo_list = platform_chassis.get_all_cpos()
        except Exception as e:
            write_syslog(syslog.LOG_ERR, "get cpo list for oe temperature failed:{}".format(e))
            return self.TEMP_ERROR

        seen_oes = set()
        read_error = False
        for cpo in cpo_list:
            try:
                oe = cpo.oe
                oe_name = oe.get_name()
                if oe_name in seen_oes:
                    continue
                seen_oes.add(oe_name)
                api = oe.get_api()
                if api is None:
                    raise OSError("OE API is unavailable: {}".format(oe_name))
                temperature = api.get_module_temperature()
                if isinstance(temperature, bool) or not isinstance(temperature, (int, float)):
                    raise ValueError("OE temperature is unavailable: {}".format(oe_name))
                write_syslog(syslog.LOG_DEBUG, "OE:{},temp:{}".format(oe_name, temperature))
                if not self.TEMP_VALID_LOW <= temperature <= self.TEMP_VALID_HIGH:
                    write_syslog(syslog.LOG_ERR, "OE Invalid temp:{} oe:{}".format(temperature, oe_name))
                    continue
                highest_temperature = max(highest_temperature, temperature)
            except Exception as e:
                write_syslog(syslog.LOG_ERR, "get oe temp error:{}".format(e))
                read_error = True

        if highest_temperature == self.TEMP_INVALID and read_error:
            return self.TEMP_ERROR
        return highest_temperature

    def get_highest_temperature_cpo_rlm(self):
        highest_temperature = self.TEMP_INVALID
        try:
            platform_chassis = sonic_platform.platform.Platform().get_chassis()
            if platform_chassis is None:
                write_syslog(syslog.LOG_ERR, "platform_chassis is none")
                return self.TEMP_ERROR
            cpo_list = platform_chassis.get_all_cpos()
        except Exception as e:
            write_syslog(syslog.LOG_ERR, "get cpo list for els temperature failed:{}".format(e))
            return self.TEMP_ERROR

        seen_els = set()
        read_error = False
        for cpo in cpo_list:
            try:
                els = cpo.elsfp
                els_name = els.get_name()
                if els_name in seen_els:
                    continue
                seen_els.add(els_name)
                if not els.get_presence():
                    continue
                api = els.get_api()
                if api is None:
                    raise OSError("ELS API is unavailable: {}".format(els_name))
                temperature = api.get_module_temperature()
                if isinstance(temperature, bool) or not isinstance(temperature, (int, float)):
                    raise ValueError("ELS temperature is unavailable: {}".format(els_name))
                write_syslog(syslog.LOG_DEBUG, "ELS:{},temp:{}".format(els_name, temperature))
                if not self.TEMP_VALID_LOW <= temperature <= self.TEMP_VALID_HIGH:
                    write_syslog(syslog.LOG_ERR, "ELS Invalid temp:{} els:{}".format(temperature, els_name))
                    continue
                highest_temperature = max(highest_temperature, temperature)
            except Exception as e:
                write_syslog(syslog.LOG_ERR, "get els temp error:{}".format(e))
                read_error = True

        if highest_temperature == self.TEMP_INVALID and read_error:
            return self.TEMP_ERROR
        return highest_temperature
