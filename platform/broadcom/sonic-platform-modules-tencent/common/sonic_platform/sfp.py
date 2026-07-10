#!/usr/bin/python3
#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################
import time
import syslog
import traceback

try:
    from platform_intf import platform_get_optoe_type, platform_set_optoe_type
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

LOG_DEBUG_LEVEL  = 1
LOG_WARNING_LEVEL  = 2
LOG_ERROR_LEVEL  = 3

EEPROM_RETRY = 5
EEPROM_RETRY_BREAK_SEC = 0.2

SYSFS_VAL_IS_PRESENT = 1
SYSFS_VAL_IS_RESET = 1
SYSFS_VAL_IS_LPMODE = 1

class Sfp(SfpOptoeBase):

    QSFP_DEVICE_TYPE = 1
    SFP_DEVICE_TYPE  = 2
    CMIS_DEVICE_TYPE = 3

    def __init__(self, index):
        SfpOptoeBase.__init__(self)
        self._port_id = index + 1 if self._get_config("port_index_start") == 0 else index
        self._device_type = None
        self.log_level = 1
        self.sfp_type = None
        # self._set_log_level(self._get_config("sfp_log_level"))

    def get_eeprom_path(self):
        return self._get_config("eeprom_path") % self._port_id

    def read_eeprom(self, offset, num_bytes):
        eeprom_raw = None
        try:
            for i in range(EEPROM_RETRY):
                eeprom_raw = SfpOptoeBase.read_eeprom(self, offset, num_bytes)
                if eeprom_raw is None:
                    time.sleep(EEPROM_RETRY_BREAK_SEC)
                    continue
                break
        except Exception as e:
            self._log_error(traceback.format_exc(e))
        return eeprom_raw

    def write_eeprom(self, offset, num_bytes, write_buffer):
        try:
            for i in range(EEPROM_RETRY):
                ret = SfpOptoeBase.write_eeprom(self, offset, num_bytes, write_buffer)
                if ret is False:
                    time.sleep(EEPROM_RETRY_BREAK_SEC)
                    continue
                break
        except Exception as e:
            self._log_error(traceback.format_exc(e))
            ret = False

        return ret

    def get_presence(self):
        try:
            presence_sysfs_path = self._get_config("presence_path") % self._port_id
            ret, result = self._read_sysfs(presence_sysfs_path)
            if ret is False:
                return False
            return result == SYSFS_VAL_IS_PRESENT
        except Exception as err:
            self._log_error(traceback.format_exc(err))

        return False

    def reset(self):
        if self.get_presence() is False:
            return False

        if self.sfp_type is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'SFP':
            self._log_warning('SFP does not support reset')
            return False

        self._log_debug('port_num:%d resetting...' % self._port_id)
        ret = self._set_reset(True)
        if ret:
            time.sleep(0.5)
            ret = self._set_reset(False)

        return ret

    def get_lpmode(self):
        if not self.get_presence():
            return False

        if self.sfp_type is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'SFP':
            self._log_warning('SFP does not support reset')
            return False

        try:
            lpmode_sysfs_path = self._get_config("lpmode_path") % self._port_id
            ret, result = self._read_sysfs(lpmode_sysfs_path)
            if ret is False:
                return False
            return result == SYSFS_VAL_IS_LPMODE
        except Exception as err:
            self._log_error(traceback.format_exc(err))

        return False

    def set_lpmode(self, lpmode):
        if not self.get_presence():
            return False

        if self.sfp_type is None or self._xcvr_api is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'QSFP-DD':
            return SfpOptoeBase.set_lpmode(self, lpmode)
        elif self.sfp_type == 'QSFP':
            if lpmode:
                return self._xcvr_api.set_power_override(True, lpmode)
            else:
                return self._xcvr_api.set_power_override(False, lpmode)
        else:
            self._log_warning('SFP does not support reset')
            return False

    def set_optoe_write_max(self, write_max):
        """
        This func is declared and implemented by SONiC but we're not supported
        so override it as NotImplemented
        """
        self._log_debug("set_optoe_write_max NotImplemented")
        return

    def refresh_xcvr_api(self):
        """
        Updates the XcvrApi associated with this SFP
        """
        self._xcvr_api = self._xcvr_api_factory.create_xcvr_api()

        class_name = self._xcvr_api.__class__.__name__
        self._set_optoe_driver(class_name)

#################### inner api ####################

    def _get_device_type(self, class_name):
        if (class_name == 'CmisApi'):
            self._device_type = self.CMIS_DEVICE_TYPE
            self.sfp_type = 'QSFP-DD'
        elif (class_name == 'Sff8472Api'):
            self._device_type = self.SFP_DEVICE_TYPE
            self.sfp_type = 'SFP'
        elif (class_name == 'Sff8636Api'):
            self._device_type = self.QSFP_DEVICE_TYPE
            self.sfp_type = 'QSFP'
        else:
            self._log_error("get_device_type error, class_name not supported:%s" % class_name)
            self._device_type = None
            self.sfp_type = None

    def _set_reset(self, reset):
        try:
            reset_sysfs_path = self._get_config("reset_path") % self._port_id
            ret, result = self._read_sysfs(reset_sysfs_path)
            if ret is False:
                return False
            if reset:
                result = SYSFS_VAL_IS_RESET
            else:
                result = 0

            ret = self._write_sysfs(reset_sysfs_path, result)
            if ret is False:
                return False
        except Exception as err:
            self._log_error(traceback.format_exc())
            return False
        return True

    def _read_sysfs(self, sysfs_path):
        val = 0
        if sysfs_path is None:
            self._log_error("sysfs_path is None")
            return False, 0

        try:
            with open(sysfs_path, "rb") as data:
                sysfs_data = data.read(2)
                if sysfs_data != "":
                    val = int(sysfs_data, 16)
        except Exception as err:
            self._log_error(traceback.format_exc(err))
            return False, 0

        return True, val

    def _write_sysfs(self, sysfs_path, val):
        if sysfs_path is None:
            self._log_error("sysfs_path is None")
            return False

        try:
            with open(sysfs_path, "r+") as data:
                data.seek(0)
                sres = hex(val)[2:]
                data.write(sres)
        except Exception as err:
            self._log_error(traceback.format_exc(err))
            return False

        self._log_debug("write sysfs_path:%s success, val:%d" % (sysfs_path, val))
        return True

    def _set_optoe_driver(self, class_name):
        self._get_device_type(class_name)
        if self._device_type is None:
            return
        try:
            ret, output = platform_get_optoe_type(self._port_id)
            if ret is False:
                self._log_error("port %s get optoe type fail, output:%s" % (self._port_id, output))
            if int(output) != self._device_type:
                ret, output = platform_set_optoe_type(self._port_id, self._device_type)
                if ret is False:
                    self._log_error("port %s set optoe type fail, output:%s" % (self._port_id, output))
                self._log_debug("port_num:%d optoe_type:%s set success" % (self._port_id, self._device_type))
        except Exception as err:
            print("Set optoe err %s" % err)
        return

    def _get_config(self, key=None, default_val=None):
        config = {
            "port_index_start": 1,
            "sfp_log_level": 1,
            "presence_path": "/sys/s3ip/transceiver/eth%d/present",
            "eeprom_path": "/sys/s3ip/transceiver/eth%d/eeprom",
            "lpmode_path": "/sys/s3ip/transceiver/eth%d/low_power_mode",
            "reset_path": "/sys/s3ip/transceiver/eth%d/reset",
        }
        return config.get(key, default_val)

    ####### SfpLog class #####

    def _set_log_level(self, level):
        if (level == LOG_DEBUG_LEVEL
            or level == LOG_ERROR_LEVEL):
            self.log_level = level
        else:
            self.log_error("unavailable loglevel:%d" % level)

    def _log_debug(self, msg):
        if self.log_level <= LOG_DEBUG_LEVEL:
            try:
                syslog.openlog("Sfp")
                syslog.syslog(syslog.LOG_DEBUG, msg)
                syslog.closelog()

            except Exception as e:
                msg = traceback.format_exc(e)
                print("Exception_info:\n%s" % msg)

    def _log_warning(self, msg):
        if self.log_level <= LOG_WARNING_LEVEL:
            try:
                syslog.openlog("Sfp")
                syslog.syslog(syslog.LOG_WARNING, msg)
                syslog.closelog()

            except Exception as e:
                msg = traceback.format_exc(e)
                print("Exception_info:\n%s" % msg)

    def _log_error(self, msg):
        if self.log_level <= LOG_ERROR_LEVEL:
            try:
                syslog.openlog("Sfp")
                syslog.syslog(syslog.LOG_ERR, msg)
                syslog.closelog()

            except Exception as e:
                msg = traceback.format_exc(e)
                print("Exception_info:\n%s" % msg)

