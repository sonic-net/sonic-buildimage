#############################################################################
# Edgecore
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#
#############################################################################

import time
import sys
import subprocess
from ctypes import create_string_buffer

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CPLD_I2C_PATH = "/sys/bus/i2c/devices/3-0060/"

class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    # Port number
    PORT_START = 49
    PORT_END = 54

    # Path to sysfs
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"
    HOST_CHK_CMD = ["which", "systemctl"]
        
    PLATFORM = "x86_64-accton_as4630_54pe-r0"
    HWSKU = "Accton-AS4630-54PE"

    _port_to_i2c_mapping = {
           49: 18,
           50: 19,
           51: 20,
           52: 21,
           53: 22,
           54: 23,
    }

    def __init__(self, sfp_index=0):
        SfpOptoeBase.__init__(self)
        self._api_helper=APIHelper()
        # Init index
        self.index = sfp_index
        self.port_num = self.index + 1
        # Init eeprom path
        eeprom_path = '/sys/bus/i2c/devices/{0}-0050/eeprom'
        self.port_to_eeprom_mapping = {}
        for x in range(self.PORT_START, self.PORT_END + 1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(self._port_to_i2c_mapping[x])

    def get_eeprom_path(self):
        return self.port_to_eeprom_mapping[self.port_num]

    def __is_host(self):
        return subprocess.call(self.HOST_CHK_CMD) == 0

    def __get_path_to_port_config_file(self):
        platform_path = "/".join([self.PLATFORM_ROOT_PATH, self.PLATFORM])
        hwsku_path = "/".join([platform_path, self.HWSKU]
                              ) if self.__is_host() else self.PMON_HWSKU_PATH
        return "/".join([hwsku_path, "port_config.ini"])

    def __read_eeprom_specific_bytes(self, offset, num_bytes):
        sysfsfile_eeprom = None
        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom_mapping[self.port_num]
        try:
            sysfsfile_eeprom = open(
                sysfs_sfp_i2c_client_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            if sys.version_info[0] >= 3:
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
            else:
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(ord(raw[n]))[2:].zfill(2)
        except Exception:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_raw

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if self.port_num < 53: #Copper port and sfp ports are suported.
            return False

        reset_path="{}{}{}".format(CPLD_I2C_PATH , "module_reset_" , str(self.port_num))
        val = self._api_helper.read_txt_file(reset_path)
        
        if val is not None:
            return int(val, 10) == 1
        else:        
            return False

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP
        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        rx_los = False
        
        if self.port_num < 49: #Copper port, no sysfs
            return False
            
        if self.port_num < 53:
            rx_path = "{}{}{}".format(CPLD_I2C_PATH, '/module_rx_los_', self.port_num)
            rx_los=self._api_helper.read_txt_file(rx_path)
            if rx_los is None:
                return False
        else:
            rx_los_list = []
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                QSFP_CHANNL_RX_LOS_STATUS_OFFSET, QSFP_CHANNL_RX_LOS_STATUS_WIDTH) if self.get_presence() else None
            if dom_channel_monitor_raw is not None:
                rx_los_data = int(dom_channel_monitor_raw[0], 16)
                rx_los_list.append(rx_los_data & 0x01 != 0)
                rx_los_list.append(rx_los_data & 0x02 != 0)
                rx_los_list.append(rx_los_data & 0x04 != 0)
                rx_los_list.append(rx_los_data & 0x08 != 0)
                rx_los = rx_los_list[0] and rx_los_list[1] and rx_los_list[2] and rx_los_list[3]
        return rx_los

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP
        Returns:
            A Boolean, True if SFP has TX fault, False if not
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        tx_fault = False
        if self.port_num < 49: #Copper port, no sysfs
            return False
        
        if self.port_num < 53:
            tx_path = "{}{}{}".format(CPLD_I2C_PATH, '/module_tx_fault_', self.port_num)
            tx_fault=self._api_helper.read_txt_file(tx_path)
            if tx_fault is None:
                return False
        else:
            tx_fault_list = []
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                QSFP_CHANNL_TX_FAULT_STATUS_OFFSET, QSFP_CHANNL_TX_FAULT_STATUS_WIDTH) if self.get_presence() else None
            if dom_channel_monitor_raw is not None:
                tx_fault_data = int(dom_channel_monitor_raw[0], 16)
                tx_fault_list.append(tx_fault_data & 0x01 != 0)
                tx_fault_list.append(tx_fault_data & 0x02 != 0)
                tx_fault_list.append(tx_fault_data & 0x04 != 0)
                tx_fault_list.append(tx_fault_data & 0x08 != 0)
                tx_fault = tx_fault_list[0] and tx_fault_list[1] and tx_fault_list[2] and tx_fault_list[3]

        return tx_fault

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP
        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """
        if self.port_num < 49: #Copper port, no sysfs
            return False
        
        if self.port_num < 53: 
            tx_disable = False            
                
            tx_path = "{}{}{}".format(CPLD_I2C_PATH, '/module_tx_disable_', self.port_num)
            tx_disable=self._api_helper.read_txt_file(tx_path)
            
            if tx_disable is not None:
                return tx_disable 
            else:
                return False
                   
        else:
            tx_disable_list = []
    
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return False
    
            dom_control_raw = self.__read_eeprom_specific_bytes(
                QSFP_CONTROL_OFFSET, QSFP_CONTROL_WIDTH) if self.get_presence() else None
            if dom_control_raw is not None:
                dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
                tx_disable_list.append(
                    'On' == dom_control_data['data']['TX1Disable']['value'])
                tx_disable_list.append(
                    'On' == dom_control_data['data']['TX2Disable']['value'])
                tx_disable_list.append(
                    'On' == dom_control_data['data']['TX3Disable']['value'])
                tx_disable_list.append(
                    'On' == dom_control_data['data']['TX4Disable']['value'])
    
            return tx_disable_list

    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP
        Returns:
            A hex of 4 bits (bit 0 to bit 3 as channel 0 to channel 3) to represent
            TX channels which have been disabled in this SFP.
            As an example, a returned value of 0x5 indicates that channel 0
            and channel 2 have been disabled.
        """
        if self.port_num < 53: 
            # SFP doesn't support this feature
            return False
        else:
            tx_disable_list = self.get_tx_disable()
            if tx_disable_list is None:
                return 0
            tx_disabled = 0
            for i in range(len(tx_disable_list)):
                if tx_disable_list[i]:
                    tx_disabled |= 1 << i
            return tx_disabled

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if self.port_num < 53: 
            # SFP doesn't support this feature
            return False
        else:
            power_set=self.get_power_set()
            power_override = self.get_power_override()
            return power_set and power_override
      
    
    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        # Check for invalid port_num
        if self.port_num < 53: #Copper port and  sfp ports are not supported.
            return False

        reset_path = "{}{}{}".format(CPLD_I2C_PATH, 'module_reset_', self.port_num)
        ret = self._api_helper.write_txt_file(reset_path, 1)
        if ret is not True:
            return ret
        
        time.sleep(0.01)
        ret = self._api_helper.write_txt_file(reset_path, 0)
        time.sleep(0.2)
        
        return ret
        
    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.
        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if self.port_num < 49: #Copper port, no sysfs
            return False
        
        if self.port_num < 53:
            tx_path = "{}{}{}".format(CPLD_I2C_PATH, '/module_tx_disable_', self.port_num)      
            ret = self._api_helper.write_txt_file(tx_path,  1 if tx_disable else 0)
            if ret is not None:
                time.sleep(0.01)
                return ret
            else:
                return False
        
        else:
            if not self.get_presence():
                return False
            sysfsfile_eeprom = None
            try:
                tx_disable_ctl = 0xf if tx_disable else 0x0
                buffer = create_string_buffer(1)
                if sys.version_info[0] >= 3:
                    buffer[0] = tx_disable_ctl
                else:
                    buffer[0] = chr(tx_disable_ctl)
                # Write to eeprom
                sysfsfile_eeprom = open(
                    self.port_to_eeprom_mapping[self.port_num], "r+b")
                sysfsfile_eeprom.seek(QSFP_CONTROL_OFFSET)

                sysfsfile_eeprom.write(buffer[0])
            except IOError as e:
                print ('Error: unable to open file: ',str(e))
                return False
            finally:
                if sysfsfile_eeprom is not None:
                    sysfsfile_eeprom.close()
                    time.sleep(0.01)
        return True

    def tx_disable_channel(self, channel, disable):
        """
        Sets the tx_disable for specified SFP channels
        Args:
            channel : A hex of 4 bits (bit 0 to bit 3) which represent channel 0 to 3,
                      e.g. 0x5 for channel 0 and channel 2.
            disable : A boolean, True to disable TX channels specified in channel,
                      False to enable
        Returns:
            A boolean, True if successful, False if not
        """
        
        if self.port_num < 53:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False

            sysfsfile_eeprom = None
            try:
                channel_state = self.get_tx_disable_channel()

                for i in range(4):
                    channel_mask = (1 << i)
                    if not (channel & channel_mask):
                        continue

                    if disable:
                        channel_state |= channel_mask
                    else:
                        channel_state &= ~channel_mask

                buffer = create_string_buffer(1)
                if sys.version_info[0] >= 3:
                    buffer[0] = channel_state
                else:
                    buffer[0] = chr(channel_state)
                # Write to eeprom
                sysfsfile_eeprom = open(
                    self.port_to_eeprom_mapping[self.port_num], "r+b")
                sysfsfile_eeprom.seek(QSFP_CONTROL_OFFSET)
                sysfsfile_eeprom.write(buffer[0])
            except IOError as e:
                print ('Error: unable to open file: ', str(e))
                return False
            finally:
                if sysfsfile_eeprom is not None:
                    sysfsfile_eeprom.close()
                    time.sleep(0.01)
            return True

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if self.port_num < 53:
            return False # SFP doesn't support this feature
        else:
            if lpmode is True:
                self.set_power_override(True, True)
            else:
                self.set_power_override(False, False)
    
            return True
       
    def set_power_override(self, power_override, power_set):
        """
        Sets SFP power level using power_override and power_set
        Args:
            power_override :
                    A Boolean, True to override set_lpmode and use power_set
                    to control SFP power, False to disable SFP power control
                    through power_override/power_set and use set_lpmode
                    to control SFP power.
            power_set :
                    Only valid when power_override is True.
                    A Boolean, True to set SFP to low power mode, False to set
                    SFP to high power mode.
        Returns:
            A boolean, True if power-override and power_set are set successfully,
            False if not
        """
        if self.port_num < 53:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False
            try:
                power_override_bit = (1 << 0) if power_override else 0
                power_set_bit      = (1 << 1) if power_set else (1 << 3)
    
                buffer = create_string_buffer(1)
                if sys.version_info[0] >= 3:
                    buffer[0] = (power_override_bit | power_set_bit)
                else:
                    buffer[0] = chr(power_override_bit | power_set_bit)
                # Write to eeprom
                with open(self.port_to_eeprom_mapping[self.port_num], "r+b") as fd:
                    fd.seek(QSFP_POWEROVERRIDE_OFFSET)
                    fd.write(buffer[0])
                    time.sleep(0.01)
            except IOError as e:
                print ('Error: unable to open file: ', str(e))
                return False
            return True

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        name = None
        sfputil_helper = SfpUtilHelper()
        sfputil_helper.read_porttab_mappings(
            self.__get_path_to_port_config_file())
        name = sfputil_helper.logical[self.index] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        if self.port_num < 49: #Copper port, no sysfs
            return False
            
        present_path = "{}{}{}".format(CPLD_I2C_PATH, '/module_present_', self.port_num)
        val=self._api_helper.read_txt_file(present_path)
        if val is not None:
            return int(val, 10)==1
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.port_num

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True
