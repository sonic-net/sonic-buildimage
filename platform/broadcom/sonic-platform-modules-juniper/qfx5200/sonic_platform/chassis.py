#!/usr/bin/env python
#
# Name: chassis.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
# which provide the chassis specific details
#
# Copyright (c) 2020, Juniper Networks, Inc.
# All rights reserved.
#

try:
    import os
    import subprocess
    import sys
    import time
    import syslog
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.thermal import Thermal
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.component import Component
    import sonic_platform.fpga_access as fpga

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_THERMAL = 8
NUM_PSU = 2
NUM_FAN_TRAY = 5
NUM_FAN = 10
NUM_COMPONENT = 2

SYSLOG_IDENTIFIER = "Juniper-Chassis"

def log_info(msg):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_INFO, msg)
    syslog.closelog()

cmd = '/var/run/sfp_presence'
QFX5200_PORT_START = 0
QFX5200_PORT_END = 31

#  Writing to a file from a list
def write_to_file(file_name, from_list):
    try:
        fp1 = open(file_name, 'w')
    except IOError as e:
        print("Error: unable to open file: %s" % str(e))
        return False

    for i in from_list:
        fp1.write(i)
        fp1.write('\n')

    fp1.close()
    return True
 
# Reading from a file to a list
def read_from_file(file_name):
    try:
        fp = open(file_name, 'r')
    except IOError as e:
        print("Error: unable to open file: %s" % str(e))
        return False

    to_list = fp.readlines()
    to_list = [x.rstrip() for x in to_list]
    fp.close()
    return to_list

def sfp_detect():
    port = 0
    ret_dict = {}
    defl_dict = {}
    port_dict = {}
    ret_dict['sfp'] = port_dict
    current_sfp_values = [0] * 32
    previous_sfp_values = [0] * 32

    if not os.path.isfile(cmd):
        pass
    else:
        if (read_from_file(cmd) == False):
            return False, defl_dict
        else:
            previous_sfp_values = read_from_file(cmd)

    # Read the current values from sysfs
    for port in range(fpga.GPIO_PORT_START, fpga.GPIO_PORT_END + 1):
        sfp_present = fpga.gpio_sfp_presence_get(port)
        current_sfp_values[port] = str(sfp_present)
            
        if (current_sfp_values[port] != previous_sfp_values[port]):
            port_dict[port] = current_sfp_values[port]
            
    if(write_to_file(cmd, current_sfp_values) == True):
        return True, ret_dict
    else:
        return False, defl_dict

class Chassis(ChassisBase):

    PORT_START = 0
    PORT_END = 31
    PORTS_IN_BLOCK = 32
	
    port_to_i2cbus_mapping = {
        0: 14,
        1: 15,
        2: 16,
        3: 17,
        4: 18,
        5: 19,
        6: 20,
        7: 21,
        8: 22,
        9: 23,
        10: 24,
        11: 25,
        12: 26,
        13: 27,
        14: 28,
        15: 29,
        16: 30,
        17: 31,
        18: 32,
        19: 33,
        20: 34,
        21: 35,
        22: 36,
        23: 37,
        24: 38,
        25: 39,
        26: 40,
        27: 41,
        28: 42,
        29: 43,
        30: 44,
        31: 45
    }
    
    def __init__(self):
        ChassisBase.__init__(self)
        self.PORT_START = 0
        self.PORT_END = 31

        # initialize system eeprom
        self._eeprom = Eeprom()

        # Initialize GPIO drivers
        fpga.gpio_sfp_base_init()
        
        # Initialize SFPs
        eeprom_base = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
        for x in range(self.PORT_START, self.PORT_END + 1):
            eeprom_path = eeprom_base.format(self.port_to_i2cbus_mapping[x])
            sfp_node = Sfp(x, 'QSFP', eeprom_path) #lgtm [py/call/wrong-number-class-arguments]
            self._sfp_list.append(sfp_node)
		   
        if os.path.isfile(cmd):
            os.remove(cmd)

        # Initialize firmware components
        for idx in range(0, NUM_COMPONENT):
            comp = Component(idx) #lgtm [py/call/wrong-number-class-arguments]
            self._component_list.append(comp)

        # Initialize Thermal
        for idx in range(0, NUM_THERMAL):
            thermal = Thermal(idx) #lgtm [py/call/wrong-number-class-arguments]
            self._thermal_list.append(thermal)
        
        # Initialize PSUs
        for idx in range(0, NUM_PSU):
            psu_node = Psu(idx) #lgtm [py/call/wrong-number-class-arguments]
            self._psu_list.append(psu_node)

        # Initialize Fan drawer
        for idx in range(0, NUM_FAN_TRAY):
            fandrawer = FanDrawer(idx)  #lgtm [py/call/wrong-number-class-arguments]
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)
		
    def get_sfp(self, index):
        sfp = None
        try:
            sfp = self._sfp_list[index]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(index, len(self._sfp_list)))
        return sfp

    def get_change_event(self, timeout=2000):
        change_dict = {}
        start_ms = time.time() * 1000  #lgtm [py/unused-local-variable]
        status, change_dict = sfp_detect()
        return status, change_dict

        if timeout:  #lgtm [py/unreachable-statement]
            now_ms = time.time() * 1000
            if (now_ms - start_ms >= timeout):
                return True, change_dict  #lgtm [py/unreachable-statement]

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
           string: The name of the chassis
        """
        return self._eeprom.modelstr()

    def get_presence(self):
        """
        Retrieves the presence of the chassis
        Returns:
            bool: True if chassis is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.part_number_str()

    def get_serial(self):
        """
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_str()

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr()

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis
        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.serial_number_str()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis
        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.system_eeprom_info()

    def get_eeprom(self):
        """
        Retrieves the Sys Eeprom instance for the chassis.
        Returns :
            The instance of the Sys Eeprom
        """
        return self._eeprom

    def get_num_sfps(self):
        """
        Retrives the numnber of Media on the chassis.
        Returns:
            An integer represences the number of SFPs on the chassis.
        """
        return self._num_sfps
 
    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot
        """
        platform_name = self.get_platform_name()
        platform_name = platform_name.replace("\r","")
        platform_name = platform_name.replace("\n","")
        log_info("Juniper Platform name: {} and {}".format(self.get_platform_name(), platform_name))
        if str(platform_name) == "x86_64-juniper_networks_qfx5210-r0":
            log_info("Juniper Platform QFX5210 ")
            status, last_reboot_reason = subprocess.getstatusoutput("i2cget -f -y 0 0x65 0x24")
            if (status == 0):
                if last_reboot_reason == "0x80":
                    return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)
                elif last_reboot_reason == "0x40" or last_reboot_reason == "0x08":
                    return (ChassisBase.REBOOT_CAUSE_WATCHDOG, None)
                elif last_reboot_reason == "0x20":
                    return (ChassisBase.REBOOT_CAUSE_POWER_LOSS, None)
                elif last_reboot_reason == "0x10":
                    return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Swizzle Reset")
                else:
                    return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Unknown reason")
            else:
                time.sleep(3)
                status, last_reboot_reason = subprocess.getstatusoutput("i2cget -f -y 0 0x65 0x24")
                if last_reboot_reason == "0x80":
                    return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)
                elif last_reboot_reason == "0x40" or last_reboot_reason == "0x08":
                    return (ChassisBase.REBOOT_CAUSE_WATCHDOG, None)
                elif last_reboot_reason == "0x20":
                    return (ChassisBase.REBOOT_CAUSE_POWER_LOSS, None)
                elif last_reboot_reason == "0x10":
                    return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Swizzle Reset")
                else:
                    return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Unknown reason")

        elif str(platform_name) == "x86_64-juniper_networks_qfx5200-r0" :
            log_info("Juniper Platform QFX5200 ")
            status, major_version = subprocess.getstatusoutput("busybox devmem 0xFED50000 8")
            status, minor_version = subprocess.getstatusoutput("busybox devmem 0xFED50001 8")
            status, last_reboot_reason = subprocess.getstatusoutput("busybox devmem 0xFED50004 8")
            if (status == 0):
                if (major_version == "0x31") and (minor_version == "0x03") and (last_reboot_reason == "0x80"):
                    return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)
                elif (major_version == "0x31") and (minor_version == "0x05") and (last_reboot_reason == "0x02"):
                    return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)
                elif last_reboot_reason == "0x40" or last_reboot_reason == "0x08":
                    return (ChassisBase.REBOOT_CAUSE_WATCHDOG, None)
                elif last_reboot_reason == "0x20":
                    return (ChassisBase.REBOOT_CAUSE_POWER_LOSS, None)
                elif last_reboot_reason == "0x10":
                    return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Swizzle Reset")
                else:
                    return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Unknown reason")
            else:
                time.sleep(3)
                status, major_version = subprocess.getstatusoutput("busybox devmem 0xFED50000 8")
                status, minor_version = subprocess.getstatusoutput("busybox devmem 0xFED50001 8")
                status, last_reboot_reason = subprocess.getstatusoutput("busybox devmem 0xFED50004 8")
                if (status == 0):
                    if (major_version == "0x31") and (minor_version == "0x03") and (last_reboot_reason == "0x80"):
                        return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)
                    elif (major_version == "0x31") and (minor_version == "0x05") and (last_reboot_reason == "0x02"):
                        return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)
                    elif last_reboot_reason == "0x40" or last_reboot_reason == "0x08":
                        return (ChassisBase.REBOOT_CAUSE_WATCHDOG, None)
                    elif last_reboot_reason == "0x20":
                        return (ChassisBase.REBOOT_CAUSE_POWER_LOSS, None)
                    elif last_reboot_reason == "0x10":
                        return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Swizzle Reset")
                    else:
                        return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Unknown reason")
                else:
                    log_info("Error while reading Re-Fpga")
        else:
            reboot_reason_addr=[0xFC800208,0xFC800207,0xFC80020A]
            reason_string = [["FPGA reset","power cycle","catastrophic error/hw misc",
                               "MSMI error/hw misc","swizzle reset","reset-button reset",
                               "PCH cold reset", "software reboot"],
                             ["software halt","power failure","thermal shutdown"],
                             ["low battery","iTCO watchdog"]]
            reason_priority = [[10,25,40,40,30,30,10,20],
                               [20,30,50],
                               [30,40]
                              ]
            selected_reason_weight = 0
            reason_str_res = ""
            for i in range(len(reboot_reason_addr)):
                status, reason = subprocess.getstatusoutput("busybox devmem {} 8".format(reboot_reason_addr[i]))
                if status != 0:
                    log_info("Error while reading Re-Fpga")
                    return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Unknown reason")

                reason = int(reason,16)
                for j in range(8):
                    if (reason & (1 << j)):
                        if selected_reason_weight < reason_priority[i][j]:
                            selected_reason_weight = reason_priority[i][j]
                            reason_str_res = reason_string[i][j]

        return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, reason_str_res)

