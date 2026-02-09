#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import sys
    import time
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """

    SYSLED_DEV_NAME = "SYS_LED"

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)
        self.sfp_state = []

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (0-based) index <index>
        Args:
            index: An integer, the index (0-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 0.
            For example, 0 for Ethernet0, 1 for Ethernet4 and so on.
        Returns:
            An object derived from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            # The 'index' starts from 1 for this platform
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

    def get_serial_number(self):
        return self.get_serial()

    def initizalize_system_led(self):
        return True

    def get_status_led(self):
        return self.pddf_obj.get_system_led_color(self.SYSLED_DEV_NAME)

    def set_status_led(self, color):
        return self.pddf_obj.set_system_led_color(self.SYSLED_DEV_NAME, color)

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level

        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.

        Returns:
            (bool, dict):
                - True if call successful, False if not;
                - A nested dictionary where key is a device type,
                  value is a dictionary with key:value pairs in the format of
                  {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
                  Specifically for SFP event, besides SFP plug in and plug out,
                  there are some other error event could be raised from SFP, when
                  these error happened, SFP eeprom will not be avalaible, XCVRD shall
                  stop to read eeprom before SFP recovered from error status.
                      status='2' I2C bus stuck,
                      status='3' Bad eeprom,
                      status='4' Unsupported cable,
                      status='5' High Temperature,
                      status='6' Bad cable.
        """
        change_event_dict = {"sfp": {}}
        sfp_status, sfp_change_dict = self.get_transceiver_change_event(timeout)
        change_event_dict["sfp"] = sfp_change_dict
        if sfp_status is True:
            return True, change_event_dict

        return False, {}

    def get_transceiver_change_event(self, timeout=0):
        start_time = time.time()
        # SFP status definition from xcvrd
        SFP_STATUS_INSERTED = '1'
        SFP_STATUS_REMOVED = '0'

        timeout = (timeout/1000)
        end_time = start_time + timeout
        while (timeout >= 0):
            new_sfp_state = []
            change_dict = {}
            for index in range(self.get_num_sfps()):
                # get current status
                state = self._sfp_list[index].get_presence()
                new_sfp_state.append(state)

                port_index = self._sfp_list[index].port_index
                if self.sfp_state == []:
                    change_dict[port_index] = SFP_STATUS_INSERTED if state == True else SFP_STATUS_REMOVED
                elif state != self.sfp_state[index]:
                    change_dict[port_index] = SFP_STATUS_INSERTED if state == True else SFP_STATUS_REMOVED

            self.sfp_state = new_sfp_state
            current_time = time.time()

            if bool(change_dict):
                return True, change_dict
            elif timeout == 0 or current_time < end_time:
                time.sleep(1)
                continue
            else:
                return True, {}

        return False, {}
