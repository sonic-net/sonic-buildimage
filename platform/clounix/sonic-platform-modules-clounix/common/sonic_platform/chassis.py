#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import os
    import sys
    import time
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.thermal import Thermal
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'

def get_platform_cpu_num():
    num = 0
    try:        
        from re import findall
        from subprocess import getstatusoutput
        cmd = "ls /sys/devices/platform/*coretemp*/hwmon/hwmon*/temp*"
        _, ret = getstatusoutput(cmd)
        match = findall(r".*temp[0-9]*_input", ret)
        num = len(match)
    except:
        pass
    return num

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)

        self.__api_helper = APIHelper()
        self.chassis_conf = self.__api_helper.get_attr_conf("chassis")
        self.__initialize_components()

        # VRM THERMALs(for temp sensor mp2882/xdpe)
        num = self.platform_inventory['num_pmbus_vrm'] if 'num_pmbus_vrm' in self.platform_inventory else 0
        self.num_temp_per_vrm = 2
        for i in range(num):
            for j in range(self.num_temp_per_vrm):
                thermal = Thermal(0, self.pddf_obj, self.plugin_data)
                thermal.thermal_index = j + 1
                thermal.thermal_obj_name = "PMBUS-VRM_{}".format(i + 1)
                thermal.thermal_obj = self.pddf_obj.data[thermal.thermal_obj_name]
                thermal.is_vc_sensor_thermal = True
                self._thermal_list.append(thermal)

        # CORE THERMALs
        for i in range(get_platform_cpu_num()):
            thermal = Thermal(0, self.pddf_obj, self.plugin_data)
            thermal.thermal_index = i + 1
            if i == 0:
                thermal.thermal_obj_name = "CPU / Package"
            else:    
                thermal.thermal_obj_name = "CPU / Core {}".format(i-1)
            thermal.thermal_obj = None
            thermal.is_core_thermal = True
            self._thermal_list.append(thermal)
        
        # fpga pvt thermal
        num = self.platform_inventory['num_fpga_pvt_temp'] if 'num_fpga_pvt_temp' in self.platform_inventory else 0 
        for i in range(num):
            thermal = Thermal(0, self.pddf_obj, self.plugin_data)
            thermal.thermal_index = i + 1
            thermal.thermal_obj_name = "FPGA_PVT_TEMP_{}".format(i + 1)
            thermal.thermal_obj = None
            thermal.is_fpga_pvt_thermal = True
            self._thermal_list.append(thermal)
        
    # Provide the functions/variables below for which implementation is to be overwritten
    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis
        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        if self._watchdog is None:
            self._watchdog = Watchdog()

        return self._watchdog
    
    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot
        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """
        THERMAL_OVERLOAD_POSITION_FILE = "/host/reboot-cause/platform/thermal_overload_position"
        ADDITIONAL_FAULT_CAUSE_FILE = "/host/reboot-cause/platform/additional_fault_cause"

        reboot_cause = (self.REBOOT_CAUSE_NON_HARDWARE, "Unknown")
        #thermal policy reboot cause
        if os.path.isfile(THERMAL_OVERLOAD_POSITION_FILE):
            thermal_overload_pos = self.__api_helper.read_one_line_file(
                THERMAL_OVERLOAD_POSITION_FILE) or "Unknown"
            if thermal_overload_pos != "Unknown":
                str = thermal_overload_pos
                if str.find('cpu') >= 0:
                    reboot_cause = (
                        self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, 'Thermal Overload: CPU')
                elif str.find('asic') >= 0:
                    reboot_cause = (
                        self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC, 'Thermal Overload: ASIC')
                else:
                    reboot_cause = (
                        self.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER, thermal_overload_pos)

                os.remove(THERMAL_OVERLOAD_POSITION_FILE)
                # print("thermal reboot_cause {0}".format(reboot_cause))
                return reboot_cause
         #ADM1166 cause
        if os.path.isfile(ADDITIONAL_FAULT_CAUSE_FILE):
            addational_fault_cause = self.__api_helper.read_one_line_file(
                ADDITIONAL_FAULT_CAUSE_FILE) or "Unknown"
            if addational_fault_cause != "Unknown":
                reboot_cause = (self.REBOOT_CAUSE_HARDWARE_OTHER,
                                addational_fault_cause)
                os.remove(ADDITIONAL_FAULT_CAUSE_FILE)
                # print("add reboot_cause {0}".format(reboot_cause))
                return reboot_cause
        # print(" reboot_cause {0}".format(reboot_cause))
        return reboot_cause
     
    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager
		
    def __initialize_components(self):
        from sonic_platform.component import Component

        for index in range(len(self.chassis_conf['components'])):
            component = Component(index,self.chassis_conf['components'])
            self._component_list.append(component) 
			
    @property
    def _get_presence_bitmap(self):

        bits = []

        for x in self._sfp_list:
          bits.append(str(int(x.get_presence())))

        rev = "".join(bits[::-1])
        return int(rev,2)
    
    data = {'present':0}
    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}

        if timeout == 0:
            cd_ms = sys.maxsize
        else:
            cd_ms = timeout

        #poll per second
        while cd_ms > 0:
            reg_value = self._get_presence_bitmap
            changed_ports = self.data['present'] ^ reg_value
            if changed_ports != 0:
                break
            time.sleep(1)
            cd_ms = cd_ms - 1000

        if changed_ports != 0:
            for port in range(0, len(self._sfp_list)):
                # Mask off the bit corresponding to our port
                mask = (1 << (port - 0))
                if changed_ports & mask:
                    if (reg_value & mask) == 0:
                        port_dict[port] = SFP_STATUS_REMOVED
                    else:
                        port_dict[port] = SFP_STATUS_INSERTED

            # Update cache
            self.data['present'] = reg_value
            return True, port_dict
        else:
            return True, {}
        return False, {}

    def get_change_event(self, timeout=0):
        res_dict = {
            'component': {},
            'fan': {},
            'module': {},
            'psu': {},
            'sfp': {},
            'thermal': {},
        }
        ''' get transceiver change event '''
        res_dict['sfp'].clear()
        status, res_dict['sfp'] = self.get_transceiver_change_event(timeout)
        return status, res_dict

    def initizalize_system_led(self):
        return True
    
    def get_status_led(self):
        return self.get_system_led('SYS_LED')
    
    def set_status_led(self, color):
        return self.set_system_led('SYS_LED',color)
