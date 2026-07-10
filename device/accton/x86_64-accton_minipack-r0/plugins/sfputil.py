# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
    import os
    import sys
    from minipack.pimutil import PimUtil
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 128

    LOCAL_OOM_PATH = "/usr/local/bin/minipack_qsfp/port%d_eeprom"

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.PORT_START, self.PORT_END + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def sfp_map(self, index):
        port = index + 1
        base = ((port-1)/8*8) + 10
        index = (port - 1) % 8
        index = 7 - index
        if (index % 2):
            index = index - 1
        else:
            index = index + 1
        bus = base + index
        return bus

    def __init__(self):
        for x in range(0, self.port_end):
            self.port_to_eeprom_mapping[x] = self.LOCAL_OOM_PATH % x

        SfpUtilBase.__init__(self)
        pim = PimUtil()
        pim.init_pim_fpga()

    def __del__(self):
        self.value = 0

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        pim = PimUtil()
        status = pim.get_qsfp_presence(port_num)
        return status

    def get_low_power_mode(self, port_num):
        if port_num < self.port_start or port_num > self.port_end:
            return False

        pim = PimUtil()
        return pim.get_low_power_mode(port_num)

    def set_low_power_mode(self, port_num, lpmode):
        if port_num < self.port_start or port_num > self.port_end:
            return False
        pim = PimUtil()
        pim.set_low_power_mode(port_num, lpmode)
        return True

    def reset(self, port_num):
        if port_num < self.port_start or port_num > self.port_end:
            return False
        pim = PimUtil()
        pim.reset(port_num)
        return True

    def get_transceiver_change_event(self, timeout=0):
        pim = PimUtil()
        start_time = time.time()
        port_dict = {}
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000)  # Convert to secs
        else:
            print("get_transceiver_change_event:Invalid timeout value", timeout)
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print('get_transceiver_change_event:'
                  'time wrap / invalid timeout value', timeout)

            return False, {}  # Time wrap or possibly incorrect timeout

        while timeout >= 0:
            change_status = 0
            port_dict = pim.get_qsfp_interrupt()
            present = 0
            for key, value in port_dict.items():
                if value == 1:
                    present = self.get_presence(key)
                    change_status = 1
                    if present:
                        port_dict[key] = '1'
                    else:
                        port_dict[key] = '0'

            if change_status:
                return True, port_dict
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
        print("get_evt_change_event: Should not reach here.")
        return False, {}
