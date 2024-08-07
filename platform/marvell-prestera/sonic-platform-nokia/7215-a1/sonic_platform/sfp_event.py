'''
listen for the SFP change event and return to chassis.
'''
import os
import time
from sonic_py_common import logger
from sonic_py_common.general import getstatusoutput_noshell

# system level event/error
EVENT_ON_ALL_SFP = '-1'
SYSTEM_NOT_READY = 'system_not_ready'
SYSTEM_READY = 'system_become_ready'
SYSTEM_FAIL = 'system_fail'

# SFP PORT numbers
SFP_PORT_START = 49
SFP_PORT_END = 52
CPLD_DIR = "/sys/bus/i2c/devices/0-0041/"

SYSLOG_IDENTIFIER = "sfp_event"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)


class sfp_event:
    ''' Listen to plugin/plugout cable events '''

    def __init__(self):
        self.handle = None

    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv
    
    def initialize(self):
        self.modprs_register = 0
        # Get Transceiver status
        time.sleep(5)
        self.modprs_register = self._get_transceiver_status()
        sonic_logger.log_info("Initial SFP presence=%d" % self.modprs_register)

    def deinitialize(self):
        if self.handle is None:
            return

    def _get_transceiver_status(self):

        pos = [1, 2, 4, 8]
        sfpstatus = 0
        for port in range (SFP_PORT_START,SFP_PORT_END+1):
            status = self._read_sysfs_file(CPLD_DIR+"sfp{}_present".format(port))  
            bit_pos = pos[port-SFP_PORT_START]
            sfpstatus = sfpstatus + (bit_pos * (int(status)))

        return sfpstatus

    def check_sfp_status(self, port_change, timeout):
        """
        check_sfp_status called from get_change_event, this will return correct
            status of all 4 SFP ports if there is a change in any of them
        """
        start_time = time.time()
        port = SFP_PORT_START
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
            reg_value = self._get_transceiver_status()
            if (reg_value != self.modprs_register):
                changed_ports = (self.modprs_register ^ reg_value)
                while (port >= SFP_PORT_START and port <= SFP_PORT_END):
                    # Mask off the bit corresponding to our port
                    mask = (1 << port-SFP_PORT_START)
                    if (changed_ports & mask):
                        # ModPrsL is active high
                        if reg_value & mask == 0:
                            port_change[port] = '0'
                        else:
                            port_change[port] = '1'
                    port += 1

                # Update reg value
                self.modprs_register = reg_value
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
