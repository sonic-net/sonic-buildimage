'''
listen for the SFP change event and return to chassis.
'''
import sys
import time
from sonic_py_common import logger

smbus_present = 1

try:
    import smbus
except ImportError as e:
    smbus_present = 0

if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd

# system level event/error
EVENT_ON_ALL_SFP = '-1'
SYSTEM_NOT_READY = 'system_not_ready'
SYSTEM_READY = 'system_become_ready'
SYSTEM_FAIL = 'system_fail'

SYSLOG_IDENTIFIER = "sfp_event"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)


class sfp_event:
    ''' Listen to plugin/plugout cable events '''

    # SFP PORT numbers
    SFP_PORT_START = 5
    CPLD_ADDRESS = 0x70

    def __init__(self):
        self.handle = None

    def initialize(self):
        self.modprs_list = []
        # Get Transceiver status
        time.sleep(5)
        self.modprs_list = self._get_transceivers_presence_bitmap()
        sonic_logger.log_info("Initial SFP presence={}".format(self.modprs_list))

    def deinitialize(self):
        if self.handle is None:
            return

    def _read_sfp_register(self, reg_addr):
        value = 0
        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, value = cmd.getstatusoutput('sudo i2cget -y 0 {} {}'.format(self.CPLD_ADDRESS, reg_addr))
            if cmdstatus:
                sonic_logger.log_error("Failed to retrieve register value %s", cmdstatus)
                return None
            value = int(value, 16)
        else:
            bus = smbus.SMBus(0)
            try:
                value = bus.read_byte_data(self.CPLD_ADDRESS, reg_addr)
            except Exception as e:
                sonic_logger.log_error("Failed to retrieve register value")

        return value

    def _get_transceivers_presence_bitmap(self):
        """
        Returns:
            A list of boolean values, representing the SFP presence status
            of each SFP port channel
            E.g., [False, False, True, False, ...]
        """
        presence_list = []

        """
        in case of sfp we have 1 registers with 8 bits that represent ports 5-12 presence
        value:
            0:Module inserted
            1:Module absent
        """
        SFP_ctrl_reg_3_value = self._read_sfp_register(0x13); # 5-12
        for i in range(8):
            presence_list.append((SFP_ctrl_reg_3_value & (1 << i)) == 0)

        """
        in case of zsfp we have 3 registers for all 6 of the zsfp`s.
        Everyone with 3 different statuses
        [bit 0] - presence of ZSFP N
        [bit 4] - presence of ZSFP N+1
        value:
            0:Module inserted
            1:Module absent
        """
        ZSFP_ctrl_reg_3_value = self._read_sfp_register(0x0e); # 13-14
        presence_list.append((ZSFP_ctrl_reg_3_value & (1 << 0)) == 0)
        presence_list.append((ZSFP_ctrl_reg_3_value & (1 << 4)) == 0)
        ZSFP_ctrl_reg_2_value = self._read_sfp_register(0x0d); # 15-16
        presence_list.append((ZSFP_ctrl_reg_2_value & (1 << 0)) == 0)
        presence_list.append((ZSFP_ctrl_reg_2_value & (1 << 4)) == 0)
        ZSFP_ctrl_reg_1_value = self._read_sfp_register(0x0c); # 17-18
        presence_list.append((ZSFP_ctrl_reg_1_value & (1 << 0)) == 0)
        presence_list.append((ZSFP_ctrl_reg_1_value & (1 << 4)) == 0)

        return presence_list

    def check_sfp_status(self, port_change, timeout):
        """
        check_sfp_status called from get_change_event, this will return correct
            status of all SFP ports if there is a change in any of them
        """
        start_time = time.time()
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000)  # Convert to secs
        else:
            return False, {}
        end_time = start_time + timeout

        if (start_time > end_time):
            return False, {}  # Time wrap or possibly incorrect timeout

        while (timeout >= 0):
            # Check for OIR events and return updated port_change
            presence_list = self._get_transceivers_presence_bitmap()
            if (presence_list != self.modprs_list):
                if (len(presence_list) != len(self.modprs_list)):
                    sonic_logger.log_error("Invalid length of the sfp presence list - {}".format(len(presence_list)))
                    return False, {}
                # Update in modules presence detected
                for i in range(len(presence_list)):
                    if presence_list[i] != self.modprs_list[i]:
                        port_change[self.SFP_PORT_START + i] = '1' if presence_list[i] else '0'

                # Update presence list
                self.modprs_list = presence_list
                return True, {}

            if forever:
                time.sleep(1)
            else:
                timeout = end_time - time.time()
                if timeout >= 1:
                    time.sleep(1)  # We poll at 1 second granularity
                else:
                    if timeout > 0:
                        time.sleep(timeout)
                    return True, {}
        return False, {}
