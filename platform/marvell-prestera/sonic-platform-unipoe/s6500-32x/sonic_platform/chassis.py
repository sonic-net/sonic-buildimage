#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import sys
    import time
    import subprocess
    from . import component
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """
    sfp_status = {}
    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)
        
        for port in range(1, self.platform_inventory['num_ports'] + 1):
            self.sfp_status[port] = self.get_sfp(port).get_presence()

        for index in range(self.platform_inventory['num_component']):
            component_obj = component.Component(index)
            self._component_list.append(component_obj)

    # Provide the functions/variables below for which implementation is to be overwritten

    @staticmethod
    def initizalize_system_led():
        return True

    def get_status_led(self):
        return self.get_system_led("SYS_LED")

    def set_status_led(self, color):
        if color == self.get_status_led():
            return False
        result = self.set_system_led("SYS_LED", color)
        return result

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of physical SFP ports in a
            chassis starting from 1.

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            if index == 0:
                raise IndexError
            sfp = self._sfp_list[index - 1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                index, len(self._sfp_list)))

        return sfp

    def get_change_event(self, timeout=0):
        # SFP event
        sfp_dict = {}
        start_time = time.time()
        time_period = timeout / float(1000)  # Convert msecs to secss

        while time.time() < (start_time + time_period) or timeout == 0:
            for port in range(1, self.platform_inventory['num_ports'] + 1):
                exist = self.get_sfp(port).get_presence()
                if self.sfp_status[port] != exist:
                   self.sfp_status[port] = exist
                   sfp_dict[port] = '1' if exist else '0'

            if sfp_dict:
                return True, {'sfp': sfp_dict}

            time.sleep(1)

        return True, {'sfp': {}}  # Timeout

    def _run_i2c_command(self, cmd, is_get=False):
        try:
            result = subprocess.run(cmd,check=True,capture_output=True,text=True)
            if is_get: 
                return result.stdout.strip()
            return True
        except Exception as e:
            print(f"Unexpected error: {str(e)}")
        return None

    def get_reboot_cause(self):
        set_cmd1 = ['i2cset', '-y', '-f', '0', '0x75', '0x00', '0x04']
        set_cmd2 = ['i2cset', '-y', '-f', '0', '0x75', '0x00', '0x00']
        get_cmd = ['i2cget', '-y', '-f', '0', '0x60', '0x07']
        self._run_i2c_command(set_cmd1)
        hw_reboot_cause = self._run_i2c_command(get_cmd,is_get=True)
        self._run_i2c_command(set_cmd2)

        if hw_reboot_cause == "0x11": 
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
            description = 'The last reset is Power on reset.'
        elif hw_reboot_cause == "0x33":
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'The last reset is CPU cold reset'
        elif hw_reboot_cause == "0x44":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'The last reset is CPU warm reset'
        elif hw_reboot_cause == "0x77":
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
            description = 'The last reset is power off reset'
        else: 
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'Hardware reason'
        
        return (reboot_cause, description)

