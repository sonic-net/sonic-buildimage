'''
listen for the SFP change event and return to chassis.
'''
import time
from sonic_py_common import logger

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
    SFP_PORT_START = 1
    SFP_PORT_END = 12
    QSFP_PORT_START = 13
    QSFP_PORT_END = 14

    # The Virgo MB CPLD (bus 18, addr 0x75) is managed by the virgo2_cpld
    # kernel driver, which exports per-port presence as flat sysfs attributes.
    CPLD_SYSFS_DIR = "/sys/bus/i2c/devices/18-0075"

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

    def _read_present_attr(self, prefix, port_num):
        """Read the driver's normalized presence bit (1 = present)."""
        path = "{}/{}{}_present".format(self.CPLD_SYSFS_DIR, prefix, port_num)
        try:
            with open(path, "r") as f:
                return int(f.read().strip()) == 1
        except (OSError, ValueError):
            sonic_logger.log_error("Failed to read presence from {}".format(path))
            return False

    def _get_transceivers_presence_bitmap(self):
        """
        Returns:
            A list of boolean values, representing the SFP presence status
            of each SFP/QSFP port channel (True = module inserted).
            E.g., [False, False, True, False, ...]
        """
        presence_list = []

        for port in range(self.SFP_PORT_START, self.SFP_PORT_END + 1):
            presence_list.append(self._read_present_attr("sfp", port))

        for port in range(self.QSFP_PORT_START, self.QSFP_PORT_END + 1):
            presence_list.append(self._read_present_attr("qsfp", port))

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
                return True, port_change

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
