# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import cisco.pacific.sfp
    import cisco.pacific.optics
    from cisco.sonic.optics import load_configuration
    from sonic_sfp.sfputilbase import SfpUtilBase
#   from cisco_sfp.sfputilcisco import SfpUtilCisco
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return range(0, self.PORTS_IN_BLOCK)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    @property
    def get_transceiver_status(self):
        status = '0' * self.PORTS_IN_BLOCK
        for k,v in self.sfp.sfp_presence('\d+').items():
            if v:
                port = int(k[4:])    # Removes QSFP prefix
                status = status[0:port] + '1' + status[port+1:]
        return status

    def __init__(self):
        self.sfp = cisco.pacific.sfp.sfp_mgr()
        self.optics = None
        self.PORTS_IN_BLOCK = self.sfp.num_sfps
        self.PORT_START = 0
        self.PORT_END = self.PORTS_IN_BLOCK - 1
        self.EEPROM_OFFSET = 20

        self._port_to_eeprom_mapping = self.sfp.eeprom_path('\d+')
        self.transceiver_status = '0' * self.PORTS_IN_BLOCK

        super(SfpUtil, self).__init__()

    def valid_port(self, port_num):
        return port_num >= self.PORT_START and port_num <= self.PORT_END

    def get_presence(self, port_num):
        return self.sfp.sfp_present(port_num)

    def get_low_power_mode(self, port_num):
        if not self.valid_port(port_num):
            return False

        # XXX/balsup: what to do here?
        return False

    def set_low_power_mode(self, port_num, lpmode):
        if not self.valid_port(port_num):
            return False

        # XXX/balsup: what to do here?
        return False

    def reset(self, port_num):
        if not self.valid_port(port_num):
            return False
        self.sfp.sfp_op('reset', port_num)
        return True

    @staticmethod
    def load_cfg():
        m = load_configuration()

        # Filter the breakout modes for unsupported configurations
        # XXX/balsup: this is 8201_SYS specific
        for port in range(24, 36, 2):
            even = m.get(port, None)
            odd = m.get(port + 1, None)
            # No problem if the odd is unconfigured (None), or
            # the even is unconfigured (None) or
            # the even is not in a breakout mode ('none')
            if odd is None:
                continue
            if odd.get('breakout_mode', None) is not None:
                print('?Illegal configuration: port {:u} does not support any breakout mode'.format(
                    port + 1))
                m[port + 1] = None
                continue
            if (even is None) or (even.get('breakout_mode', None) == None):
                continue
            # At this point, even is configured in a breakout
            # mode, and odd also has a configuration.
            # Odd must not be configured.
            print('?Illegal configuration: port {:u} cannot be configured when port {:u} is in any breakout mode'.format(
                    port + 1, port))
            m[port + 1] = None
        return m

    def get_transceiver_change_event(self, timeout=0):
        start_time = time.time()
        port = self.port_start
        forever = False

        if self.optics is None:
            self.optics = cisco.pacific.optics.optics_mgr(cfg = self.load_cfg())

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            return False, {}

        while timeout >= 0:
            v = self.get_transceiver_status
            if v != self.transceiver_status:
                result = {}
                delta = v
                for port in range(self.PORT_START, self.PORT_END + 1):
                    if v[port] == self.transceiver_status[port]:
                        d = '0'
                    else:
                        d = '1'
                        result[port] = v[port]
                    delta = delta[0:port] + d + delta[port+1:]
                self.transceiver_status = v
                self.optics.port_set_init(delta=delta, presence=v)
                self.optics.port_set_admin(delta=delta, presence=v)
                return True, result

            if forever:
                time.sleep(1)
                continue
            timeout = end_time - time.time()
            if timeout > 1:
                timeout = 1
            if timeout > 0:
                time.sleep(timeout)
        return True, {}
