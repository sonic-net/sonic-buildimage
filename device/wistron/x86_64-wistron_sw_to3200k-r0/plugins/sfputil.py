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

    BASE_OOM_PATH = "/sys/bus/i2c/devices/0-00{0}/"
    BASE_CPLD1_PATH = "/sys/bus/i2c/devices/0-0061/"
    BASE_CPLD2_PATH = "/sys/bus/i2c/devices/0-0062/"

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}
    _port_to_i2c_mapping = {
           0: [1, 10],
           1: [2, 11],
           2: [3, 12],
           3: [4, 13],
           4: [5, 14],
           5: [6, 15],
           6: [7, 16],
           7: [8, 17],
           8: [9, 18],
           9: [10, 19],
           10: [11, 20],
           11: [12, 21],
           12: [13, 22],
           13: [14, 23],
           14: [15, 24],
           15: [16, 25],
           16: [17, 26],
           17: [18, 27],
           18: [19, 28],
           19: [20, 29],
           20: [21, 30],
           21: [22, 31],
           22: [23, 32],
           23: [24, 33],
           24: [25, 34],
           25: [26, 35],
           26: [27, 36],
           27: [28, 37],
           28: [29, 38],
           29: [30, 39],
           30: [31, 40],
           31: [32, 41],
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
        eeprom_path = self.BASE_OOM_PATH + "eeprom1"

        for x in range(0, self.port_end+1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(self._port_to_i2c_mapping[x][1])

        self._transceiver_presence = self._get_sfp_presence()
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num < 16 :
            present_path = self.BASE_CPLD1_PATH + "module_present_" + str(port_num+1)
        else:
            present_path = self.BASE_CPLD2_PATH + "module_present_" + str(port_num+1)
        self.__port_to_is_present = present_path

        try:
            val_file = open(self.__port_to_is_present)
            content = val_file.readline().rstrip()
            val_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            return False

        if content == "1":
            return True

        return False

    def get_low_power_mode(self, port_num):
        raise NotImplementedError

    def set_low_power_mode(self, port_num, lpmode):
        raise NotImplementedError

    def reset(self, port_num):
        raise NotImplementedError

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
                if cur_presence[port] == 1:
                    port_dict[port]='1'
                else:
                    port_dict[port]='0'

                if cur_presence[port] != self._transceiver_presence[port]:
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
