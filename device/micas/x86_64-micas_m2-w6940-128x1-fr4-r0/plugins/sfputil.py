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
        hightest_temperature = self.TEMP_INVALID
        try:
            platform_chassis = sonic_platform.platform.Platform().get_chassis()
            if platform_chassis is None:
                write_syslog(syslog.LOG_ERR, "platform_chassis is none")
                return self.TEMP_ERROR
            sfp_list = platform_chassis._sfp_list
            last_oe_id = -1
            for idx in range(1, len(sfp_list)):
                sfp = sfp_list[idx]
                # skip same oe
                oe_id = sfp._oe_id
                if oe_id == last_oe_id:
                    continue
                last_oe_id = oe_id
                api = sfp.get_xcvr_api()
                if api is None:
                    write_syslog(syslog.LOG_ERR, "oe api is none:{}".format(idx))
                    continue
                temperature = api.get_module_temperature()
                write_syslog(syslog.LOG_DEBUG, "OE:{} sfp:{},temp:{}".format(oe_id, idx, temperature))
                if temperature < self.TEMP_VALID_LOW or temperature > self.TEMP_VALID_HIGH:
                    write_syslog(syslog.LOG_ERR, "OE Invalid temp:{} idx:{}".format(temperature, idx))
                    continue
                if hightest_temperature < temperature:
                    hightest_temperature = temperature
        except Exception as e:
            write_syslog(syslog.LOG_ERR, "get oe temp error:{}".format(e))
            return self.TEMP_ERROR

        return hightest_temperature

    def get_highest_temperature_cpo_rlm(self):
        hightest_temperature = self.TEMP_INVALID
        try:
            platform_chassis = sonic_platform.platform.Platform().get_chassis()
            if platform_chassis is None:
                write_syslog(syslog.LOG_ERR, "platform_chassis is none")
                return self.TEMP_ERROR
            sfp_list = platform_chassis._sfp_list
            last_rlm_id = -1
            for idx in range(1, len(sfp_list)):
                sfp = sfp_list[idx]
                # skip same rlm
                rlm_id = sfp._els_id
                if rlm_id == last_rlm_id:
                    continue
                last_rlm_id = rlm_id
                api = sfp.get_xcvr_api()
                if api is None:
                    write_syslog(syslog.LOG_ERR, "rlm api is none:{}".format(idx))
                    continue
                temperature = api.get_rlm_temperature()
                write_syslog(syslog.LOG_DEBUG, "RLM:{} sfp:{},temp:{}".format(rlm_id, idx, temperature))
                if temperature < self.TEMP_VALID_LOW or temperature > self.TEMP_VALID_HIGH:
                    write_syslog(syslog.LOG_ERR, "RLM Invalid temp:{} idx:{}".format(temperature, idx))
                    continue
                if hightest_temperature < temperature:
                    hightest_temperature = temperature
        except Exception as e:
            write_syslog(syslog.LOG_ERR, "get rlm temp error:{}".format(e))
            return self.TEMP_ERROR

        return hightest_temperature
