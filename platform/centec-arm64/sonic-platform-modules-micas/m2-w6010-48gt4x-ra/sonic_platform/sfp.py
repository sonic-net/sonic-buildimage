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
#      ver 2.0 - wb_plat:
#               "presence_path": "/xx/wb_plat/xx[port_id]/present"
#               "eeprom_path": "/sys/bus/i2c/devices/i2c-[bus]/[bus]-0050/eeprom"
#               "reset_path": "/xx/wb_plat/xx[port_id]/reset"
#############################################################################
import sys
import time
import os
import syslog
import traceback
from abc import abstractmethod

configfile_pre = "/usr/local/bin/"
sys.path.append(configfile_pre)

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

except ImportError as error:
    raise ImportError(str(error) + "- required module not found") from error

LOG_DEBUG_LEVEL = 1
LOG_WARNING_LEVEL = 2
LOG_ERROR_LEVEL = 3


class Sfp(SfpOptoeBase):

    OPTOE_DRV_TYPE1 = 1
    OPTOE_DRV_TYPE2 = 2
    OPTOE_DRV_TYPE3 = 3

    # index must start at 1
    def __init__(self, index):
        SfpOptoeBase.__init__(self)
        self.sfp_type = None
        self.log_level_config = LOG_WARNING_LEVEL
        # Init instance of SfpCust
        self._sfp_api = SfpV2(index)

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
        except BaseException:
            print(traceback.format_exc())
            return None
        return transceiver_info

    def set_optoe_write_max(self, write_max):
        """
        This func is declared and implemented by SONiC but we're not supported
        so override it as NotImplemented
        """
        self._sfplog(LOG_DEBUG_LEVEL, "set_optoe_write_max NotImplemented")

    def refresh_xcvr_api(self):
        """
        Updates the XcvrApi associated with this SFP
        """
        self._xcvr_api = self._xcvr_api_factory.create_xcvr_api()
        class_name = self._xcvr_api.__class__.__name__
        optoe_type = None
        # set sfp_type
        if 'CmisApi' in class_name:
            self.sfp_type = 'QSFP-DD'
            optoe_type = self.OPTOE_DRV_TYPE3
        elif 'Sff8472Api' in class_name:
            self.sfp_type = 'SFP'
            optoe_type = self.OPTOE_DRV_TYPE2
        elif ('Sff8636Api' in class_name or 'Sff8436Api' in class_name):
            self.sfp_type = 'QSFP'
            optoe_type = self.OPTOE_DRV_TYPE1
        # set optoe driver
        if optoe_type is not None:
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

            except BaseException:
                print(traceback.format_exc())


class SfpCust():
    def __init__(self, index):
        self.eeprom_path = None
        self._init_config(index)

    def _init_config(self, index):
        self.log_level_config = LOG_WARNING_LEVEL
        self._port_id = index
        self.eeprom_retry_times = 5
        self.eeprom_retry_break_sec = 0.2

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
                    if result is not None:
                        return bytearray(result)
                    time.sleep(self.eeprom_retry_break_sec)
                    continue

        except BaseException:
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
        except BaseException:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            return False

    @abstractmethod
    def set_optoe_type(self, optoe_type):
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

            except BaseException:
                print(traceback.format_exc())


class SfpV2(SfpCust):
    def _init_config(self, index):
        super()._init_config(index)
        # init eeprom path
        sfp_pt2ee_path_list = [0] * 53
        sfp_pt2ee_path_list[49:53] = [9, 10, 11, 12]

        eeprom_path_config = "/sys/bus/i2c/devices/i2c-%d/%d-0050/eeprom"
        eeprom_path_key = sfp_pt2ee_path_list[self._port_id]
        self.eeprom_path = None if eeprom_path_config is None or eeprom_path_key == 0 else eeprom_path_config % (
            eeprom_path_key, eeprom_path_key)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init eeprom path: %s" % self.eeprom_path)

        # init presence path
        self.presence_path = "/sys/bus/i2c/devices/3-0030/sfp_presence1"
        self.presence_val_is_present = 0
        self._sfplog(LOG_DEBUG_LEVEL, "Done init presence path: %s" % self.presence_path)

        # init optoe driver path
        optoe_driver_path = "/sys/bus/i2c/devices/i2c-%d/%d-0050/dev_class"
        optoe_driver_key = sfp_pt2ee_path_list[self._port_id]
        self.dev_class_path = None if optoe_driver_path is None or optoe_driver_key == 0 else optoe_driver_path % (
            optoe_driver_key, optoe_driver_key)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init optoe driver path: %s" % self.dev_class_path)

        # init reset path
        self.reset_val_is_reset = 0

        new_device_path = "/sys/bus/i2c/devices/i2c-%d/new_device"
        new_device_key = sfp_pt2ee_path_list[self._port_id]
        self.new_class_path = None if new_device_path is None or new_device_key == 0 else new_device_path % new_device_key
        self._sfplog(LOG_DEBUG_LEVEL, "Done init new_class path: %s" % self.new_class_path)

        if sfp_pt2ee_path_list[self._port_id] != 0:
            self.add_new_sfp_device(self._port_id, 0x50)
            self._sfplog(LOG_DEBUG_LEVEL, "Done add_new_sfp_device 0x50 port %d" % self._port_id)

    def sfp_add_dev(self, new_device_path, devaddr, devtype):
        try:
            # Write device address to new_device file
            nd_file = open(new_device_path, "w")
            nd_str = "%s %s" % (devtype, hex(devaddr))
            nd_file.write(nd_str)
            nd_file.close()
        except Exception as err:
            self._sfplog(LOG_ERROR_LEVEL, ("Error writing to new device file: %s" % str(err)))
            return 1
        else:
            return 0

    def add_new_sfp_device(self, port_num, devid):
        if os.path.exists(self.dev_class_path):
            return

        ret = self.sfp_add_dev(self.new_class_path, devid, "optoe2")
        if ret != 0:
            self._sfplog(LOG_ERROR_LEVEL, "Error adding sfp device")

    def get_presence(self):
        sfp_ls = [49, 50, 51, 52]
        if self._port_id not in sfp_ls or self.presence_path is None:
            self._sfplog(LOG_ERROR_LEVEL, "presence_path is None!")
            return False
        try:
            with open(self.presence_path, "rb") as data:
                presence_data = data.read(2)
                if presence_data == "":
                    return False
                result = int(presence_data, 16)

            # ModPrsL is active low
            presence_offset = sfp_ls.index(self._port_id)
            if result & (1 << presence_offset) == 0:
                return True
            return False
        except BaseException:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            return False

    def set_reset(self, reset):
        return True

    def set_optoe_type(self, optoe_type):
        if self.dev_class_path is None:
            self._sfplog(LOG_ERROR_LEVEL, "dev_class_path is None!")
            return False
        try:
            with open(self.dev_class_path, "r+") as dc_file:
                dc_file_val = dc_file.read(1)
                if int(dc_file_val) != optoe_type:
                    dc_str = "%s" % str(optoe_type)
                    dc_file.write(dc_str)
                    # dc_file.close()
        except BaseException:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            return False
        return True
