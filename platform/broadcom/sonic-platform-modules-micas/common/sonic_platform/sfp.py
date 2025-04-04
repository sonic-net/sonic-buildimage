#!/usr/bin/python
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
import fcntl
import syslog
import traceback
from abc import abstractmethod

configfile_pre = "/usr/local/bin/"
sys.path.append(configfile_pre)

try:
    from platform_intf import *
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from plat_hal.baseutil import baseutil

except ImportError as error:
    raise ImportError(str(error) + "- required module not found") from error

LOG_DEBUG_LEVEL = 1
LOG_INFO_LEVEL = 2
LOG_NOTICE_LEVEL = 3
LOG_WARNING_LEVEL = 4
LOG_ERROR_LEVEL = 5


class Sfp(SfpOptoeBase):

    OPTOE_DRV_TYPE1 = 1
    OPTOE_DRV_TYPE2 = 2
    OPTOE_DRV_TYPE3 = 3

    # index must start at 1
    def __init__(self, index):
        SfpOptoeBase.__init__(self)
        self.sfp_type = None
        self.presence = False
        sfp_config = baseutil.get_config().get("sfps", None)
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
        elif vers == 3:
            self._sfp_api = SfpV3CPO(index)
        else:
            self._sfplog(LOG_ERROR_LEVEL, "Get SfpVer Error!")

    def get_eeprom_path(self):
        return self._sfp_api._get_eeprom_path()

    def read_eeprom(self, offset, num_bytes):
        return self._sfp_api.read_eeprom(offset, num_bytes)

    def set_power_class(self):
        try:
            if self.sfp_type is None:
                self.refresh_xcvr_api()
            if self.sfp_type != "QSFP":
                return
            identify_code = self.read_eeprom(0, 1)
            if identify_code[0] != 0x11:
                return
            ext_identify_code = self.read_eeprom(129, 1)
            if (ext_identify_code[0] & 0x3) == 0x0:
                return
            power_class_ctrl = self.read_eeprom(93, 1)
            power_class_ctrl = power_class_ctrl[0] | 0x4
            self._sfp_api.write_eeprom(93, 1, bytearray([power_class_ctrl]))
        except Exception as e:
            print(traceback.format_exc())

    def write_eeprom(self, offset, num_bytes, write_buffer):
        return self._sfp_api.write_eeprom(offset, num_bytes, write_buffer)

    def get_presence(self):
        presence = self._sfp_api.get_presence()
        if (presence == True) and (self.presence != True):
            self.set_power_class()
        self.presence = presence
        return presence

    def get_transceiver_info(self):
        api_get = self._sfp_api.get_transceiver_info(SfpOptoeBase, self)
        return api_get if api_get is not None else super().get_transceiver_info()

    def get_transceiver_bulk_status(self):
        api_get = self._sfp_api.get_transceiver_bulk_status(SfpOptoeBase, self)
        return api_get if api_get is not None else super().get_transceiver_bulk_status()

    def get_transceiver_threshold_info(self):
        api_get = self._sfp_api.get_transceiver_threshold_info(SfpOptoeBase, self)
        return api_get if api_get is not None else super().get_transceiver_threshold_info()
    
    def get_transceiver_status(self):
        api_get = self._sfp_api.get_transceiver_status(SfpOptoeBase, self)
        return api_get if api_get is not None else super().get_transceiver_status()
    
    def get_transceiver_loopback(self):
        api_get = self._sfp_api.get_transceiver_loopback(SfpOptoeBase, self)
        return api_get if api_get is not None else super().get_transceiver_loopback()
    
    def get_transceiver_pm(self):
        api_get = self._sfp_api.get_transceiver_pm(SfpOptoeBase, self)
        return api_get if api_get is not None else super().get_transceiver_pm()

    def get_reset_status(self):
        if self.get_presence() is False:
            return False

        if self.sfp_type is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'SFP':
            self._sfplog(LOG_ERROR_LEVEL, 'SFP does not support reset')
            return False

        ret = self._sfp_api.get_reset_status()
        return ret

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

        if self.sfp_type == 'QSFP' or self.sfp_type == 'QSFP_DD':
            return SfpOptoeBase.get_lpmode(self)

            self._sfplog(LOG_WARNING_LEVEL, 'SFP does not support lpmode')
            return False

    def set_lpmode(self, lpmode):
        if self.get_presence() is False:
            return False

        if self.sfp_type is None or self._xcvr_api is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'QSFP_DD' or self.sfp_type == 'QSFP':
            return SfpOptoeBase.set_lpmode(self, lpmode)

        self._sfplog(LOG_WARNING_LEVEL, 'SFP does not support lpmode')
        return False

    def get_tx_disable(self):
        if self.get_presence() is False:
            return False

        if self.sfp_type is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'SFP':
            return self._sfp_api.get_tx_disable()

        return SfpOptoeBase.get_tx_disable(self)

    def get_tx_disable_channel(self):
        if self.get_presence() is False:
            return False

        if self.sfp_type is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'SFP':
            return self._sfp_api.get_tx_disable_channel()

        return SfpOptoeBase.get_tx_disable_channel(self)

    def tx_disable(self, tx_disable):
        if self.get_presence() is False:
            return False

        if self.sfp_type is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'SFP':
            return self._sfp_api.set_tx_disable(tx_disable)

        return SfpOptoeBase.tx_disable(self, tx_disable)

    def tx_disable_channel(self, channel, disable):
        if self.get_presence() is False:
            return False

        if self.sfp_type is None:
            self.refresh_xcvr_api()

        if self.sfp_type == 'SFP':
            self._sfplog(LOG_WARNING_LEVEL, 'SFP does not support tx disable channel')
            return False

        return SfpOptoeBase.tx_disable_channel(self, channel, disable)

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
            self.sfp_type = 'QSFP_DD'
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
                if log_level == LOG_INFO_LEVEL:
                    syslog.syslog(syslog.LOG_INFO, msg)
                if log_level == LOG_NOTICE_LEVEL:
                    syslog.syslog(syslog.LOG_NOTICE, msg)
                elif log_level == LOG_WARNING_LEVEL:
                    syslog.syslog(syslog.LOG_WARNING, msg)
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
        sfp_config = baseutil.get_config().get("sfps", None)
        self.log_level_config = sfp_config.get("log_level", LOG_WARNING_LEVEL)
        self._port_id = index
        self.eeprom_retry_times = sfp_config.get("eeprom_retry_times", 0)
        self.eeprom_retry_break_sec = sfp_config.get("eeprom_retry_break_sec", 0)

    def _get_eeprom_path(self):
        return self.eeprom_path or None

    def get_eeprom_path(self):
        return self._get_eeprom_path()

    @abstractmethod
    def _pre_get_transceiver_info(self):
        pass

    @abstractmethod
    def get_presence(self):
        pass

    def get_transceiver_info(self, class_optoeBase, class_sfp):
        # temporary solution for a sonic202111 bug
        transceiver_info = class_optoeBase.get_transceiver_info(class_sfp)
        try:
            if transceiver_info == None:
                return None
            if transceiver_info['cable_type'] == None:
                transceiver_info['cable_type'] = 'N/A'
            if transceiver_info["vendor_rev"] is not None:
                transceiver_info["hardware_rev"] = transceiver_info["vendor_rev"]
        except BaseException:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            return None
        return transceiver_info

    def get_transceiver_bulk_status(self, class_optoeBase, class_sfp):
        pass

    def get_transceiver_threshold_info(self, class_optoeBase, class_sfp):
        pass

    def get_transceiver_status(self, class_optoeBase, class_sfp):
        pass

    def get_transceiver_loopback(self, class_optoeBase, class_sfp):
        pass

    def get_transceiver_pm(self, class_optoeBase, class_sfp):
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
        for i in range(self.eeprom_retry_times):
            try:
                with open(self._get_eeprom_path(), mode='r+b', buffering=0) as f:
                    f.seek(offset)
                    f.write(write_buffer[0:num_bytes])
                return True
            except (OSError, IOError):
                time.sleep(self.eeprom_retry_break_sec)
                pass
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
                if log_level == LOG_INFO_LEVEL:
                    syslog.syslog(syslog.LOG_INFO, msg)
                if log_level == LOG_NOTICE_LEVEL:
                    syslog.syslog(syslog.LOG_NOTICE, msg)
                elif log_level == LOG_WARNING_LEVEL:
                    syslog.syslog(syslog.LOG_WARNING, msg)
                elif log_level == LOG_ERROR_LEVEL:
                    syslog.syslog(syslog.LOG_ERR, msg)
                syslog.closelog()

            except BaseException:
                print(traceback.format_exc())


class SfpV1(SfpCust):
    def _init_config(self, index):
        super()._init_config(index)
        # init presence path
        sfp_config = baseutil.get_config().get("sfps", None)

        eeprom_path_config = sfp_config.get("eeprom_path", None)
        eeprom_path_key = sfp_config.get("eeprom_path_key")[self._port_id - 1]
        self.eeprom_path = None if eeprom_path_config is None else eeprom_path_config % (
            eeprom_path_key, eeprom_path_key)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init eeprom path: %s" % self.eeprom_path)

        self.presence_cpld = sfp_config.get("presence_cpld", None)
        self.presence_val_is_present = sfp_config.get("presence_val_is_present", 0)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init presence path")

        # init reset path
        self.reset_cpld = sfp_config.get("reset_cpld", None)
        self.reset_val_is_reset = sfp_config.get("reset_val_is_reset", 0)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init cpld path")

        # init tx_disable path
        self.txdis_cpld = sfp_config.get("txdis_cpld", None)
        self.txdisable_val_is_on = sfp_config.get("txdisable_val_is_on", 0)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init cpld tx_disable path")

    def get_presence(self):
        if self.presence_cpld is None:
            self._sfplog(LOG_ERROR_LEVEL, "presence_cpld is None!")
            return False
        try:
            dev_id, offset, offset_bit = self._get_sfp_cpld_info(self.presence_cpld)
            if dev_id == -1:
                return False
            ret, info = platform_reg_read(0, dev_id, offset, 1)
            if ((ret is False) or (info is None)):
                return False
            return info[0] & (1 << offset_bit) == self.presence_val_is_present
        except SystemExit:
            self._sfplog(LOG_WARNING_LEVEL, "SystemExit")
            return False
        except BaseException:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            return False

    def get_reset_status(self):
        if self.reset_cpld is None:
            self._sfplog(LOG_ERROR_LEVEL, "reset_cpld is None!")
            return False
        try:
            dev_id, offset, offset_bit = self._get_sfp_cpld_info(self.reset_cpld)
            if dev_id == -1:
                return False
            ret, info = platform_reg_read(0, dev_id, offset, 1)
            if (ret is False
                    or info is None):
                self._sfplog(LOG_ERROR_LEVEL, "platform_reg_read error!")
                return False

            return (info[0] & (1 << offset_bit) == self.reset_val_is_reset)
        except SystemExit:
            self._sfplog(LOG_WARNING_LEVEL, "SystemExit")
            return False
        except BaseException:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            return False

    def get_tx_disable(self):
        if self.reset_cpld is None:
            self._sfplog(LOG_ERROR_LEVEL, "txdis_cpld is None!")
            return None

        try:
            tx_disable_list = []
            dev_id, offset, offset_bit = self._get_sfp_cpld_info(self.txdis_cpld)
            if dev_id == -1:
                return False
            ret, info = platform_reg_read(0, dev_id, offset, 1)
            if (ret is False
                    or info is None):
                self._sfplog(LOG_ERROR_LEVEL, "platform_reg_read error!")
                return None
            if self.txdisable_val_is_on == 1:
                tx_disable_list.append(info[0] & (1 << offset_bit) != 0)
            else:
                tx_disable_list.append(info[0] & (1 << offset_bit) == 0)
        except SystemExit:
            self._sfplog(LOG_WARNING_LEVEL, "SystemExit")
            return None
        except BaseException:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            return None

        return tx_disable_list

    def get_tx_disable_channel(self):
        tx_disable_list = []
        tx_disable_list = self.get_tx_disable()
        if tx_disable_list is None:
            return 0

        tx_disabled = 0
        for i in range(len(tx_disable_list)):
            if tx_disable_list[i]:
                tx_disabled |= 1 << i

        return tx_disabled

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
                    eeprom_raw.append(0)
                for n in range(0, len(info)):
                    eeprom_raw[n] = info[n]
                # temporary solution for a sonic202111 bug
                if len(eeprom_raw) < num_bytes:
                    eeprom_raw = eeprom_raw[::-1].zfill(num_bytes)[::-1]
                return bytearray(eeprom_raw)
        except BaseException:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
        return None

    def set_optoe_type(self, optoe_type):
        ret, info = platform_get_optoe_type(self._port_id)
        if ret is True and info != optoe_type:
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
            if dev_id == -1:
                return False
            ret, info = platform_reg_read(0, dev_id, offset, 1)
            if (ret is False
                    or info is None):
                self._sfplog(LOG_ERROR_LEVEL, "platform_reg_read error!")
                return False

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
        except SystemExit:
            self._sfplog(LOG_WARNING_LEVEL, "SystemExit")
            return False
        except BaseException:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            return False

        return True

    def set_tx_disable(self, tx_disable):
        if self.txdis_cpld is None:
            self._sfplog(LOG_ERROR_LEVEL, "txdis_cpld is None!")
            return False
        try:
            val = []
            dev_id, offset, offset_bit = self._get_sfp_cpld_info(self.txdis_cpld)
            if dev_id == -1:
                return False
            ret, info = platform_reg_read(0, dev_id, offset, 1)
            if (ret is False
                    or info is None):
                self._sfplog(LOG_ERROR_LEVEL, "platform_reg_read error!")
                return False

            if self.txdisable_val_is_on == 0:
                if tx_disable:
                    val.append(info[0] & (~(1 << offset_bit)))
                else:
                    val.append(info[0] | (1 << offset_bit))
            else:
                if tx_disable:
                    val.append(info[0] | (1 << offset_bit))
                else:
                    val.append(info[0] & (~(1 << offset_bit)))

            ret, info = platform_reg_write(0, dev_id, offset, val)
            if ret is False:
                self._sfplog(LOG_ERROR_LEVEL, "platform_reg_write error!")
                return False
        except SystemExit:
            self._sfplog(LOG_WARNING_LEVEL, "SystemExit")
            return False
        except BaseException:
            self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            return False

        return True

    def _get_sfp_cpld_info(self, cpld_config):
        dev_id = -1
        offset = -1
        offset_bit = -1
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
        # init eeprom path
        sfp_config = baseutil.get_config().get("sfps", None)
        eeprom_path_config = sfp_config.get("eeprom_path", None)
        eeprom_path_key = sfp_config.get("eeprom_path_key")[self._port_id - 1]
        self.eeprom_path = None if eeprom_path_config is None else eeprom_path_config % (
            eeprom_path_key, eeprom_path_key)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init eeprom path: %s" % self.eeprom_path)

        # init presence path
        self.presence_path = None if sfp_config.get("presence_path",
                                                    None) is None else sfp_config.get("presence_path") % self._port_id
        self.presence_val_is_present = sfp_config.get("presence_val_is_present", 0)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init presence path: %s" % self.presence_path)

        # init optoe driver path
        optoe_driver_path = sfp_config.get("optoe_driver_path", None)
        optoe_driver_key = sfp_config.get("optoe_driver_key")[self._port_id - 1]
        self.dev_class_path = None if optoe_driver_path is None else optoe_driver_path % (
            optoe_driver_key, optoe_driver_key)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init optoe driver path: %s" % self.dev_class_path)

        # init reset path
        self.reset_path = None if sfp_config.get(
            "reset_path",
            None) is None else sfp_config.get(
            "reset_path",
            None) % self._port_id
        self.reset_val_is_reset = sfp_config.get("reset_val_is_reset", 0)
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

class SfpV3CPO(SfpCust):
    def _init_config(self, index):
        super()._init_config(index)
        sfp_config = baseutil.get_config().get("sfps", None)

        eeprom_path_config = sfp_config.get("eeprom_path", None)
        eeprom_path_key = sfp_config.get("eeprom_path_key")[self._port_id - 1]
        self.eeprom_path = None if eeprom_path_config is None else eeprom_path_config % (
            eeprom_path_key, eeprom_path_key)
        self._sfplog(LOG_DEBUG_LEVEL, "Done init eeprom path: %s" % self.eeprom_path)

    # CPO always present
    def get_presence(self):
        return True

    def read_eeprom(self, offset, num_bytes):
        # temp solution for CPO byte0 bug, remove me when it fixed
        if offset == 0:
            if self._file_rw_lock() is False:
                return None
            self._switch_page(0)
            result = self._read_eeprom(offset, num_bytes)
            self._file_rw_unlock()
            return result

        if offset < 128:    # page 0L
            return self._read_eeprom(offset, num_bytes)

        if self._file_rw_lock() is False:
            return None

        # for other page, need to convert flat_mem offset to single page offset
        result = self._convert_to_single_page_offset_read(offset, num_bytes)
        self._file_rw_unlock()
        return result

    def write_eeprom(self, offset, num_bytes, write_buffer):
        # temp solution for CPO byte0 bug, remove me when it fixed
        if offset == 0:
            if self._file_rw_lock() is False:
                return None
            self._switch_page(0)
            result = self._write_eeprom(offset, num_bytes, write_buffer)
            self._file_rw_unlock()
            return result

        if offset < 128:    # page 0L
            return self._write_eeprom(offset, num_bytes, write_buffer)

        if self._file_rw_lock() is False:
            return None

        # for other page, need to convert flat_mem offset to single page offset
        result = self._convert_to_single_page_offset_write(offset, num_bytes, write_buffer)
        self._file_rw_unlock()
        return result

    def set_optoe_type(self, optoe_type):
        ret, info = platform_get_optoe_type(self._port_id)
        if ret is True and info != optoe_type:
            try:
                ret, _ = platform_set_optoe_type(self._port_id, optoe_type)
            except Exception as err:
                self._sfplog(LOG_ERROR_LEVEL, "Set optoe err %s" % err)

    def _read_eeprom(self, offset, num_bytes):    
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

    def _write_eeprom(self, offset, num_bytes, write_buffer):
        for i in range(self.eeprom_retry_times):
            try:
                with open(self._get_eeprom_path(), mode='r+b', buffering=0) as f:
                    f.seek(offset)
                    f.write(write_buffer[0:num_bytes])
                return True
            except BaseException:
                print(traceback.format_exc())
                time.sleep(self.eeprom_retry_break_sec)
                pass
        return False

    def _switch_page(self, page):
        page_offset = 127 #0x7f
        num_bytes = 1
        cur_page = self._read_eeprom(page_offset, num_bytes)[0]
        if cur_page is None:
            self._sfplog(LOG_ERROR_LEVEL, "CPO read PAGE ERROR!")
            return False

        if cur_page == page:
            return True

        return self._write_eeprom(page_offset, num_bytes, bytearray([page]))

    def _switch_bank(self, bank):
        bank_offset = 126 #0x7e
        num_bytes = 1
        cur_bank = self._read_eeprom(bank_offset, num_bytes)[0]
        if cur_bank is None:
            self._sfplog(LOG_ERROR_LEVEL, "CPO read BANK ERROR!")
            return False
    
        if cur_bank == bank:
            return True

        return self._write_eeprom(bank_offset, num_bytes, bytearray([bank]))

    def _convert_to_single_page_offset_read(self, offset, num_bytes):
        page_list = [0, 1, 2, 16, 17, 19, 20, 159]  # page 0 1 2 10h 11h 13h 14h 9Fh
        for p in page_list:
            if (256 + (p - 1) * 128) <= offset < (256 + 128 * p):
                self._switch_page(p)
                single_page_offset = offset - 128 * p

                if p in [16, 17, 19, 20]:   # need to switch Bank
                    port_id_abs = (self._port_id - 1) % 16
                    bank_id = port_id_abs // 2
                    self._switch_bank(bank_id)

                return self._read_eeprom(single_page_offset, num_bytes)

        self._sfplog(LOG_WARNING_LEVEL, "cannot find page! offset: %d num_bytes: %d" % (offset, num_bytes))
        return None

    def _convert_to_single_page_offset_write(self, offset, num_bytes, write_buffer):
        page_list = [0, 1, 2, 16, 17, 19, 20, 159]  # page 0 1 2 10h 11h 13h 14h 9Fh
        for p in page_list:
            if (256 + (p - 1) * 128) <= offset < (256 + 128 * p):
                self._switch_page(p)
                single_page_offset = offset - 128 * p

                if p in [16, 17, 19, 20]:   # need to switch Bank
                    port_id_abs = (self._port_id - 1) % 16
                    bank_id = port_id_abs // 2
                    self._switch_bank(bank_id)

                return self._write_eeprom(single_page_offset, num_bytes, write_buffer)

        self._sfplog(LOG_WARNING_LEVEL, "cannot find page! offset: %d num_bytes: %d" % (offset, num_bytes))
        return None

    pidfile = None

    def _file_rw_lock(self):
        global pidfile
        pidfile = open(self._get_eeprom_path(), "r")
        file_lock_flag = False
        # Retry 100 times to lock file
        for i in range(0, 100):
            try:
                fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
                file_lock_flag = True
                self._sfplog(LOG_DEBUG_LEVEL, "file lock success")
                return True
            except Exception:
                time.sleep(0.001)
                continue

        if file_lock_flag == False:
            if pidfile is not None:
                pidfile.close()
                pidfile = None
            return False

    def _file_rw_unlock(self):
        try:
            global pidfile
            if pidfile is not None:
                fcntl.flock(pidfile, fcntl.LOCK_UN)
                pidfile.close()
                pidfile = None
                self._sfplog(LOG_DEBUG_LEVEL, "file unlock success")
            else:
                self._sfplog(LOG_DEBUG_LEVEL, "pidfile is invalid, do nothing")
            return True
        except Exception as e:
            self._sfplog(LOG_ERROR_LEVEL, "file unlock err, msg:%s" % (str(e)))
            return False
