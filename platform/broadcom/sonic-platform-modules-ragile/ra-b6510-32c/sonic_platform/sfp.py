#!/usr/bin/python
# -*- coding: UTF-8 -*-

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#
# *_device.py config version instruction:
#      ver 1.0 - platform api:
#           "presence_cpld": {
#               "dev_id": {
#                   [dev_id]: {
#                       "offset": {
#                           [offset]: [port_id]
#                       }
#                    }
#                 }
#              }
#           "reset_cpld": {
#               "dev_id": {
#                   [dev_id]: {
#                       "offset": {
#                           [offset]: [port_id]
#                       }
#                    }
#                 }
#           }
#      ver 2.0 - rg_plat:
#               "presence_path": "/xx/rg_plat/xx[port_id]/present"
#               "eeprom_path": "/sys/bus/i2c/devices/i2c-[bus]/[bus]-0050/eeprom"
#               "reset_path": "/xx/rg_plat/xx[port_id]/reset"
#############################################################################
import sys
import time
import syslog
import traceback
from abc import abstractmethod

try:
    import os
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from .sfp_config import *

except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

LOG_DEBUG_LEVEL  = 1
LOG_WARNING_LEVEL  = 2
LOG_ERROR_LEVEL  = 3

CONFIG_DB_PATH = "/etc/sonic/config_db.json"

def getonieplatform(path):
    if not os.path.isfile(path):
        return ""
    machine_vars = {}
    with open(path) as machine_file:
        for line in machine_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            machine_vars[tokens[0]] = tokens[1].strip()
    return machine_vars.get("onie_platform")

def getplatform_config_db():
    if not os.path.isfile(CONFIG_DB_PATH):
        return ""
    val = os.popen("sonic-cfggen -j %s -v DEVICE_METADATA.localhost.platform" % CONFIG_DB_PATH).read().strip()
    if len(val) <= 0:
        return ""
    else:
        return val

def getplatform_name():
    if os.path.isfile('/host/machine.conf'):
        return getonieplatform('/host/machine.conf')
    elif os.path.isfile('/usr/share/sonic/hwsku/machine.conf'):
        return getonieplatform('/usr/share/sonic/hwsku/machine.conf')
    else:
        return getplatform_config_db()

def get_sfp_config():
    dev = getplatform_name()
    return cust_sfp_cfg.get(dev, None)

class Sfp(SfpOptoeBase):

    OPTOE_DRV_TYPE1 = 1
    OPTOE_DRV_TYPE2 = 2
    OPTOE_DRV_TYPE3 = 3

    # index must start at 1
    def __init__(self, index, a=None, b=None):
        SfpOptoeBase.__init__(self)
        self.sfp_type = None
        sfp_config = get_sfp_config()
        self.log_level_config = sfp_config.get("log_level", LOG_WARNING_LEVEL)
        # Init instance of SfpCust
        ver = sfp_config.get("ver", None)
        if ver is None:
            self._sfplog(LOG_ERROR_LEVEL, "Get Ver Config Error!")
        vers = int(float(ver))
        if vers == 1:
            self._sfp_api = SfpV1(index)
        elif vers == 2:
            self._sfp_api = SfpV2(index)
        else:
            self._sfplog(LOG_ERROR_LEVEL, "Get SfpVer Error!")

    def get_eeprom_path(self):
        return self._sfp_api._get_eeprom_path()

    def read_eeprom(self, offset, num_bytes):
        return self._sfp_api.read_eeprom(offset, num_bytes)

    def write_eeprom(self, offset, num_bytes, write_buffer):
        return self._sfp_api.write_eeprom(offset, num_bytes, write_buffer)

    def get_presence(self):
        return self._sfp_api.get_presence()

    def get_transceiver_info(self):
        # temporary solution for a sonic202111 bug
        transceiver_info = super().get_transceiver_info()
        try:
            if transceiver_info["vendor_rev"] is not None:
                transceiver_info["hardware_rev"] = transceiver_info["vendor_rev"]
            return transceiver_info
        except Exception as e:
            print(traceback.format_exc())

    def reset(self):
        if self.get_presence() is False:
            return False

        if self.sfp_type is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'SFP':
            self._sfplog(LOG_ERROR_LEVEL, 'SFP does not support reset')
            return False

        self._sfplog(LOG_DEBUG_LEVEL, 'resetting...')
        ret = self._sfp_api.set_reset(True)
        if ret:
            time.sleep(0.5)
            ret = self._sfp_api.set_reset(False)

        return ret

    def get_lpmode(self):
        if self.get_presence() is False:
            return False

        if self.sfp_type is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'SFP':
            self._sfplog(LOG_WARNING_LEVEL, 'SFP does not support lpmode')
            return False

        #implement in future

        return False

    def set_lpmode(self, lpmode):
        if self.get_presence() is False:
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
            self._sfplog(LOG_WARNING_LEVEL, 'SFP does not support lpmode')
            return False

    def set_optoe_write_max(self, write_max):
        """
        This func is declared and implemented by SONiC but we're not supported
        so override it as NotImplemented
        """
        self._sfplog(LOG_DEBUG_LEVEL, "set_optoe_write_max NotImplemented")
        pass

    def refresh_xcvr_api(self):
        """
        Updates the XcvrApi associated with this SFP
        """
        self._xcvr_api = self._xcvr_api_factory.create_xcvr_api()
        class_name = self._xcvr_api.__class__.__name__
        optoe_type = None
        # set sfp_type
        if (class_name == 'CmisApi'):
            self.sfp_type = 'QSFP-DD'
            optoe_type = self.OPTOE_DRV_TYPE3
        elif (class_name == 'Sff8472Api'):
            self.sfp_type = 'SFP'
            optoe_type = self.OPTOE_DRV_TYPE2
        elif (class_name == 'Sff8636Api' or class_name == 'Sff8436Api'):
            self.sfp_type = 'QSFP'
            optoe_type = self.OPTOE_DRV_TYPE1

        if optoe_type is not None:
            # set optoe driver
            self._sfp_api.set_optoe_type(optoe_type)

    def _sfplog(self, log_level, msg):
        if log_level >= self.log_level_config:
            try:
                syslog.openlog("Sfp")
                if log_level == LOG_DEBUG_LEVEL:
                    syslog.syslog(syslog.LOG_DEBUG, msg)
                elif log_level == LOG_WARNING_LEVEL:
                    syslog.syslog(syslog.LOG_DEBUG, msg)
                elif log_level == LOG_ERROR_LEVEL:
                    syslog.syslog(syslog.LOG_ERR, msg)
                syslog.closelog()

            except Exception as e:
                print(traceback.format_exc())

class SfpCust():
    def __init__(self, index):
        self.eeprom_path = None
        self._init_config(index)

    def _init_config(self, index):
        sfp_config = get_sfp_config()
        self.log_level_config = sfp_config.get("log_level", LOG_WARNING_LEVEL)
        self._port_id = index
        self.eeprom_retry_times = sfp_config.get("eeprom_retry_times", 0)
        self.eeprom_retry_break_sec = sfp_config.get("eeprom_retry_break_sec", 0)

    def combine_format_str(self, str, key):
        count_format = str.count('%')
        if count_format > 0:
            args_k = []
            for i in range(count_format):
                args_k.append(key)
            return str % (tuple(args_k))
        else:
            return str

    def _get_eeprom_path(self):
        return self.eeprom_path or None

    @abstractmethod
    def get_presence(self):
        pass

    def read_eeprom(self, offset, num_bytes):
        try:
            for i in range(self.eeprom_retry_times):
                with open(self._get_eeprom_path(), mode='rb', buffering=0) as f:
                    f.seek(offset)
                    result = f.read(num_bytes)
                    # temporary solution for a sonic202111 bug
                    if len(result) < num_bytes:
                        result = result[::-1].zfill(num_bytes)[::-1]
                    if result != None:
                        return bytearray(result)
                    else:
                        time.sleep(self.eeprom_retry_break_sec)
                        continue

        except Exception as e:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())

        return None


    def write_eeprom(self, offset, num_bytes, write_buffer):
        try:
            for i in range(self.eeprom_retry_times):
                ret = SfpOptoeBase.write_eeprom(self, offset, num_bytes, write_buffer)
                if ret is False:
                    time.sleep(self.eeprom_retry_break_sec)
                    continue
                break

            return ret

        except Exception as e:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())


    @abstractmethod
    def set_optoe_type(self, class_name):
        pass

    @abstractmethod
    def set_reset(self, reset):
        pass

    def _convert_str_range_to_int_arr(self, range_str):
        if not range_str:
            return []

        int_range_strs = range_str.split(',')
        range_res = []
        for int_range_str in int_range_strs:
            if '-' in int_range_str:
                range_s = int(int_range_str.split('-')[0])
                range_e = int(int_range_str.split('-')[1]) + 1
            else:
                range_s = int(int_range_str)
                range_e = int(int_range_str) + 1

            range_res = range_res + list(range(range_s, range_e))

        return range_res

    def _sfplog(self, log_level, msg):
        if log_level >= self.log_level_config:
            try:
                syslog.openlog("SfpCust")
                if log_level == LOG_DEBUG_LEVEL:
                    syslog.syslog(syslog.LOG_DEBUG, msg)
                elif log_level == LOG_WARNING_LEVEL:
                    syslog.syslog(syslog.LOG_DEBUG, msg)
                elif log_level == LOG_ERROR_LEVEL:
                    syslog.syslog(syslog.LOG_ERR, msg)
                syslog.closelog()

            except Exception as e:
                print(traceback.format_exc())

class SfpV1(SfpCust):
    def _init_config(self, index):
        super()._init_config(index)
        sfp_config = get_sfp_config()

        # init presence path
        self.presence_cpld = sfp_config.get("presence_cpld", None)
        self.presence_val_is_present = sfp_config.get("presence_val_is_present", 0)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init presence path")

        # init reset path
        self.reset_cpld = sfp_config.get("reset_cpld", None)
        self.reset_val_is_reset = sfp_config.get("reset_val_is_reset", 0)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init cpld path")

    def get_presence(self):
        if self.presence_cpld is None:
            self._sfplog(LOG_ERROR_LEVEL, "presence_cpld is None!")
            return False
        try:
            dev_id, offset, offset_bit = self._get_sfp_cpld_info(self.presence_cpld)
            ret, info = platform_reg_read(0, dev_id, offset, 1)
            return (info[0] & (1 << offset_bit) == self.presence_val_is_present)
        except Exception as err:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())

    def read_eeprom(self, offset, num_bytes):
        try:
            for i in range(self.eeprom_retry_times):
                ret, info = platform_sfp_read(self._port_id, offset, num_bytes)
                if (ret is False
                    or info is None):
                    time.sleep(self.eeprom_retry_break_sec)
                    continue
                eeprom_raw = []
                for i in range(0, num_bytes):
                    eeprom_raw.append("0x00")
                for n in range(0, len(info)):
                    eeprom_raw[n] = info[n]
                # temporary solution for a sonic202111 bug
                if len(eeprom_raw) < num_bytes:
                    eeprom_raw = eeprom_raw[::-1].zfill(num_bytes)[::-1]
                return bytearray(eeprom_raw)
        except Exception as e:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
        return None

    def write_eeprom(self, offset, num_bytes, write_buffer):
        try:
            for i in range(self.eeprom_retry_times):
                # TODO: write_buffer is bytearray, need to convert to int array
                val_list = []
                if isinstance(write_buffer, list):
                    val_list = write_buffer
                else:
                    val_list.append(write_buffer)
                ret, info = platform_sfp_write(self._port_id, offset, val_list)
                if ret is False:
                    time.sleep(self.eeprom_retry_break_sec)
                    continue
                return True
        except Exception as e:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())

        return False

    def set_optoe_type(self, optoe_type):
        ret, info = platform_get_optoe_type(self._port_id)
        if info != optoe_type:
            try:
                ret, _ = platform_set_optoe_type(self._port_id, optoe_type)
            except Exception as err:
                self._sfplog(LOG_ERROR_LEVEL, "Set optoe err %s" % err)

    def set_reset(self, reset):
        if self.reset_cpld is None:
            self._sfplog(LOG_ERROR_LEVEL, "reset_cpld is None!")
            return False
        try:
            val = []
            dev_id, offset, offset_bit = self._get_sfp_cpld_info(self.reset_cpld)
            ret, info = platform_reg_read(0, dev_id, offset, 1)
            if self.reset_val_is_reset == 0:
                if reset:
                    val.append(info[0] & (~(1 << offset_bit)))
                else:
                    val.append(info[0] | (1 << offset_bit))
            else:
                if reset:
                    val.append(info[0] | (1 << offset_bit))
                else:
                    val.append(info[0] & (~(1 << offset_bit)))

            ret, info = platform_reg_write(0, dev_id, offset, val)
            if ret is False:
                self._sfplog(LOG_ERROR_LEVEL, "platform_reg_write error!")
                return False

        except Exception as err:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            return False

        return True

    def _get_sfp_cpld_info(self, cpld_config):
        dev_id = 0
        offset = 0

        for dev_id_temp in cpld_config["dev_id"]:
            for offset_temp in cpld_config["dev_id"][dev_id_temp]["offset"]:
                port_range_str = cpld_config["dev_id"][dev_id_temp]["offset"][offset_temp]
                port_range_int = self._convert_str_range_to_int_arr(port_range_str)
                if self._port_id in port_range_int:
                    dev_id = dev_id_temp
                    offset = offset_temp
                    offset_bit = port_range_int.index(self._port_id)
                    break

        return dev_id, offset, offset_bit

class SfpV2(SfpCust):
    def _init_config(self, index):
        super()._init_config(index)
        sfp_config = get_sfp_config()

        # init eeprom path
        eeprom_path_config = sfp_config.get("eeprom_path", None)
        eeprom_path_key = sfp_config.get("eeprom_path_key")[self._port_id - 1]
        self.eeprom_path = self.combine_format_str(eeprom_path_config, eeprom_path_key)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init eeprom path: %s" % self.eeprom_path)

        # init presence path
        presence_path_config = sfp_config.get("presence_path", None)
        presence_path_key = sfp_config.get("presence_path_key")[self._port_id - 1]
        self.presence_path = self.combine_format_str(presence_path_config, presence_path_key)
        self.presence_val_is_present = sfp_config.get("presence_val_is_present", 0)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init presence path: %s" % self.presence_path)

        # init optoe driver path
        optoe_driver_path_config = sfp_config.get("optoe_driver_path", None)
        optoe_driver_key = sfp_config.get("optoe_driver_key")[self._port_id - 1]
        self.dev_class_path = self.combine_format_str(optoe_driver_path_config, optoe_driver_key)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init optoe driver path: %s" % self.dev_class_path)

        # init txdisable path
        txdisable_path_config = sfp_config.get("txdisable_path", None)
        if txdisable_path_config is not None:
            txdisable_path_key = sfp_config.get("txdisable_path_key")[self._port_id - 1]
            self.txdisable_path = self.combine_format_str(txdisable_path_config, txdisable_path_key)
            self.txdisable_val_is_on = sfp_config.get("txdisable_val_is_on", 0)
            self._sfplog(LOG_DEBUG_LEVEL, "Done init optoe driver path: %s" % self.dev_class_path)

        # init reset path
        reset_path_config = sfp_config.get("reset_path", None)
        if reset_path_config is not None:
            reset_path_key = sfp_config.get("reset_path_key")[self._port_id - 1]
            self.reset_path = self.combine_format_str(reset_path_config, reset_path_key)
            self.reset_val_is_on = sfp_config.get("reset_val_is_on", 0)
            self._sfplog(LOG_DEBUG_LEVEL, "Done init reset path: %s" % self.reset_path)

    def get_presence(self):
        if self.presence_path is None:
            self._sfplog(LOG_ERROR_LEVEL, "presence_path is None!")
            return False
        try:
            with open(self.presence_path, "rb") as data:
                sysfs_data = data.read(1)
                if sysfs_data != "":
                    result = int(sysfs_data, 16)
            return result == self.presence_val_is_present
        except Exception as err:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())

    def set_reset(self, reset):
        return True

    def set_optoe_type(self, optoe_type):
        if self.dev_class_path is None:
            self._sfplog(LOG_ERROR_LEVEL, "dev_class_path is None!")
            return False
        try:
            dc_file = open(self.dev_class_path, "r+")
            dc_file_val = dc_file.read(1)
            if int(dc_file_val) != optoe_type:
                dc_str = "%s" % str(optoe_type)
                dc_file.write(dc_str)
                dc_file.close()
        except Exception as err:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
