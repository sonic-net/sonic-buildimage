'''
listen for the SFP change event and return to chassis.
'''
import time
from sonic_py_common import logger
from sonic_py_common.general import getstatusoutput_noshell

smbus_present = 1

try:
    import smbus
except ImportError as e:
    smbus_present = 0

# system level event/error
EVENT_ON_ALL_SFP = '-1'
SYSTEM_NOT_READY = 'system_not_ready'
SYSTEM_READY = 'system_become_ready'
SYSTEM_FAIL = 'system_fail'

# SFP PORT numbers
SFP_PORT_START = 49
SFP_PORT_END = 52

SYSLOG_IDENTIFIER = "sfp_event"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)


class sfp_event:
    ''' Listen to plugin/plugout cable events '''

    def __init__(self):
        self.handle = None

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
        if smbus_present == 0:
            sonic_logger.log_info("  PMON - smbus ERROR - DEBUG sfp_event   ")
            cmdstatus, sfpstatus = getstatusoutput_noshell(['sudo', 'i2cget', '-y', '0', '0x41', '0x3'])
            sfpstatus = int(sfpstatus, 16)
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x41
            DEVICE_REG = 0x3
            sfpstatus = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)

            sfpstatus = ~sfpstatus
            sfpstatus = sfpstatus & 0xF

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
