# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    from socket import *
    from select import *
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 30
    PORTS_IN_BLOCK = 30
    SFP_PORT_START = 1
    SFP_PORT_END = 28

    EEPROM_OFFSET = 14

    _port_to_eeprom_mapping = {}
    _port_i2c_mapping  = {
            1: 27,
            2: 28,
            3: 29,
            4: 30,
            5: 31,
            6: 32,
            7: 33,
            8: 34,
            9: 35,
            10: 36,
            11: 37,
            12: 38,
            13: 39,
            14: 40,
            15: 41,
            16: 42,
            17: 43,
            18: 44,
            19: 45,
            20: 46,
            21: 47,
            22: 48,
            23: 49,
            24: 50,
            25: 20,
            26: 21,
            27: 22,
            28: 23,
            29: 24,
            30: 25
    }
    port_dict = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self) :
        return range(self.SFP_PORT_END+1, self.PORT_END+1)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    @property
    def get_transceiver_status(self):

        try:
            sfp_modprs_path = "/sys/devices/platform/dell-e3224f-cpld.0/sfp_modprs"
            reg_file = open(sfp_modprs_path)

        except IOError as e:
            print ("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        reg_file.close()

        return int(content, 16)


    def __init__(self):

        sfpplus_eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"

        for x in range(self.SFP_PORT_START, self.PORT_END + 1):
            self.port_to_eeprom_mapping[x] = sfpplus_eeprom_path.format(self._port_i2c_mapping[x])
        # Get Transceiver status
        self.modprs_register = self.get_transceiver_status

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        if port_num < self.SFP_PORT_START :
            return False
        port_num -= self.SFP_PORT_START
        try:
            sfp_modprs_path = "/sys/devices/platform/dell-e3224f-cpld.0/sfp_modprs"
            reg_file = open(sfp_modprs_path)
        except IOError as e:
            print ("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # ModPrsL is active low
        if (reg_value & mask) == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        return False

    def set_low_power_mode(self, port_num, lpmode):
        return False

    def reset(self, port_num):
        return False

    def get_transceiver_change_event(self, timeout=0):

        start_time = time.time()
        port = self.SFP_PORT_START
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            print ('get_transceiver_change_event:Invalid timeout value', timeout)
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print ('get_transceiver_change_event:' \
                       'time wrap / invalid timeout value', timeout)

            return False, {} # Time wrap or possibly incorrect timeout

        while timeout >= 0:
            # Check for OIR events and return updated port_dict
            reg_value = self.get_transceiver_status
            if reg_value != self.modprs_register:
                changed_ports = self.modprs_register ^ reg_value
                while port >= self.SFP_PORT_START and port <= self.SFP_PORT_END:

                    # Mask off the bit corresponding to our port
                    mask = (1 << (port - self.SFP_PORT_START))

                    if changed_ports & mask:
                        # ModPrsL is active low
                        if reg_value & mask == 0:
                            self.port_dict[port] = '1'
                        else:
                            self.port_dict[port] = '0'

                    port += 1

                # Update reg value
                self.modprs_register = reg_value
                return True, self.port_dict

            if forever:
                time.sleep(1)
            else:
                timeout = end_time - time.time()
                if timeout >= 1:
                    time.sleep(1) # We poll at 1 second granularity
                else:
                    if timeout > 0:
                        time.sleep(timeout)
                    return True, {}
        print ("get_transceiver_change_event: Should not reach here.")
        return False, {}
