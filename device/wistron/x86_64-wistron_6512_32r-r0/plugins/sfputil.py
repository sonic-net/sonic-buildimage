# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""
    SONIC_PORT_NAME_PREFIX = "Ethernet"
    PORT_START = 0
    PORT_END = 31
    PORTS_IN_BLOCK = 32

    BASE_OOM_PATH = "/sys/bus/i2c/devices/0-00"
    BASE_CPLD1_PATH = "/sys/bus/i2c/devices/0-0006/"
    BASE_CPLD2_PATH = "/sys/bus/i2c/devices/0-0007/"

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {
        0: '10',
        1: '11',
        2: '12',
        3: '13',
        4: '14',
        5: '15',
        6: '16',
        7: '17',
        8: '18',
        9: '19',
        10: '1a',
        11: '1b',
        12: '1c',
        13: '1d',
        14: '1e',
        15: '1f',
        16: '20',
        17: '21',
        18: '22',
        19: '23',
        20: '24',
        21: '25',
        22: '26',
        23: '27',
        24: '28',
        25: '29',
        26: '2a',
        27: '2b',
        28: '2c',
        29: '2d',
        30: '2e',
        31: '2f',
    }

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return range(self.PORT_START, self.PORTS_IN_BLOCK + 1)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def is_logical_port(self, port_name):
        return True

    def get_logical_to_physical(self, port_name):
        if not port_name.startswith(self.SONIC_PORT_NAME_PREFIX):
            return None

        port_idx = int(port_name[len(self.SONIC_PORT_NAME_PREFIX):])
        port_idx = port_idx // 8
        return [port_idx]

    def __init__(self):

        for x in range(0, self.port_end+1):
            self.port_to_eeprom_mapping[x] = self.BASE_OOM_PATH + self.port_to_i2c_mapping[x] + "/eeprom1"

        self._transceiver_presence = {}
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num < 16:
            present_path = self.BASE_CPLD1_PATH + "port" + str(port_num+1) + "_present"
        else:
            present_path = self.BASE_CPLD2_PATH + "port" + str(port_num+1) + "_present"
        self.__port_to_is_present = present_path

        try:
            val_file = open(self.__port_to_is_present)
            content = val_file.readline().rstrip()
            val_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            val_file.close()
            return False

        if content == "1":
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num < 16:
            lpmode_path = self.BASE_CPLD1_PATH + "port" + str(port_num+1) + "_lpmode"
        else:
            lpmode_path = self.BASE_CPLD2_PATH + "port" + str(port_num+1) + "_lpmode"
        self.__port_to_is_lpmode = lpmode_path

        try:
            val_file = open(self.__port_to_is_lpmode)
            content = val_file.readline().rstrip()
            val_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            val_file.close()
            return False

        if content == "1":
            return True

        return False

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num < 16:
            lpmode_path = self.BASE_CPLD1_PATH + "port" + str(port_num+1) + "_lpmode"
        else:
            lpmode_path = self.BASE_CPLD2_PATH + "port" + str(port_num+1) + "_lpmode"
        self.__port_to_is_lpmode = lpmode_path

        try:
            val_file = open(self.__port_to_is_lpmode, 'w')
            val_file.write('1' if lpmode else '0')
            val_file.close()
            return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            val_file.close()
            return False

    def reset(self, port_num):
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if not self.get_presence(port_num):
            return False  # Port is not present, unable to set reset

        if port_num < 16:
            reset_path = self.BASE_CPLD1_PATH + "port" + str(port_num+1) + "_reset"
        else:
            reset_path = self.BASE_CPLD2_PATH + "port" + str(port_num+1) + "_reset"

        self.__port_to_mod_rst = reset_path

        try:
            val_file = open(self.__port_to_mod_rst, 'w')
            val_file.write('1')
            time.sleep(1)
            val_file.write('0')
            time.sleep(1)
            val_file.close()
            return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            val_file.close()
            return False


    def _get_sfp_presence(self):
        port_pres = {}
        for port in range(0, self.port_end+1):
            port_pres[port] = self.get_presence(port)

        return port_pres

    def get_transceiver_change_event(self, timeout=0):
        start_time = time.time()
        port_dict = {}
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            print("get_transceiver_change_event:Invalid timeout value", timeout)
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print('get_transceiver_change_event:' \
                       'time wrap / invalid timeout value', timeout)

            return False, {} # Time wrap or possibly incorrect timeout

        while timeout >= 0:
            change_status = False

            cur_presence = self._get_sfp_presence()
            for port in range(0, self.port_end+1):
                if cur_presence[port] != self._transceiver_presence[port]:
                    if cur_presence[port] == 1:
                        port_dict[port]='1'
                    else:
                        port_dict[port]='0'
                    change_status = True

            self._transceiver_presence = cur_presence
            if change_status:
                return True, port_dict

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
        print("get_evt_change_event: Should not reach here.")
        return False, {}
