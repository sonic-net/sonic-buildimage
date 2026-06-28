#!/usr/bin/env python
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
    import struct
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 55
    QSFP_PORT_START = 48
    QSFP_PORT_END = 55
    __xcvr_presence = {}

    EEPROM_OFFSET = 9
    PORT_INFO_PATH = '/sys/class/questone2_fpga'

    _port_name = ""
    _port_to_eeprom_mapping = {}
    _port_to_i2cbus_mapping = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return range(self.QSFP_PORT_START, self.QSFP_PORT_END + 1)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    @property
    def port_to_i2cbus_mapping(self):
        return self._port_to_i2cbus_mapping

    def get_port_name(self, port_num):
        if port_num in self.qsfp_ports:
            self._port_name = "QSFP" + str(port_num - self.QSFP_PORT_START + 1)
        else:
            self._port_name = "SFP" + str(port_num + 1)
        return self._port_name

    def get_eeprom_dom_raw(self, port_num):
        if port_num in self.qsfp_ports:
            # QSFP DOM EEPROM is also at addr 0x50 and thus also stored in eeprom_ifraw
            return None
        else:
            # Read dom eeprom at addr 0x51
            return self._read_eeprom_devid(port_num, self.DOM_EEPROM_ADDR, 256)

    def __init__(self):
        # Override port_to_eeprom_mapping for class initialization
        eeprom_path = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'

        for x in range(self.PORT_START, self.PORT_END+1):
            # port_index = 0 , it's path = /sys/bus/i2c/devices/i2c-10/10-0050/eeprom
            # port_index = 55, it's path = /sys/bus/i2c/devices/i2c-65/65-0050/eeprom
            # so the real offset is 10
            self.port_to_i2cbus_mapping[x] = (x + 1 + self.EEPROM_OFFSET)
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                x + 1 + self.EEPROM_OFFSET)
        SfpUtilBase.__init__(self)
        for x in range(self.PORT_START, self.PORT_END+1):
            self.__xcvr_presence[x] = self.get_presence(x)

    def _do_write_file(self, file_handle, offset, value):
        file_handle.seek(offset)
        file_handle.write(hex(value))
        file_handle.close()

    def get_presence(self, port_num):

        # Check for invalid port_num
        if port_num not in range(self.port_start, self.port_end + 1):
            return False

        # Get path for access port presence status
        port_name = self.get_port_name(port_num)
        sysfs_filename = "qsfp_modprs" if port_num in self.qsfp_ports else "sfp_modabs"
        reg_path = "/".join([self.PORT_INFO_PATH, port_name, sysfs_filename])

        # Read status
        try:
            reg_file = open(reg_path)
            content = reg_file.readline().rstrip()
            reg_value = int(content)
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False

        # Module present is active low
        if reg_value == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        if not self.get_presence(port_num):
            return None

        eeprom_raw = []
        eeprom_raw.append("0x00")

        lpmode = False
        eeprom_path = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'.format(port_num + 1 + self.EEPROM_OFFSET)
        if port_num >= 49:
            try:
                with open(eeprom_path, mode="rb", buffering=0) as eeprom:
                    eeprom.seek(93)
                    raw = eeprom.read(1)
                    eeprom.close()
            except Exception as err:
                return None

            if len(raw) == 0:
                return None
            eeprom_raw[0] = hex(ord(raw[0]))[2:].zfill(2)

            power_data = int(eeprom_raw[0], 16)
            # if lpmod, power-override bit and power-set bit are both setted
            #               bit0                bit1
            lpmode = power_data & 0x03 != 0
        else:
            return None
        
        return lpmode
 
    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid QSFP port_num
        if port_num not in self.qsfp_ports:
            return False
	
	if not self.get_presence(port_num):
            return False
        eeprom_path = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'.format(port_num + 1 + self.EEPROM_OFFSET)
        try:
            reg_file = open(eeprom_path, mode="wb+")
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False

	reg_file.seek(93)
        power_raw = reg_file.read(1)
        if power_raw is None:
            return False
        power_data = int(hex(ord(power_raw))[2:].zfill(2), 16)
	
        if lpmode:
            power_data |= 0x03
        else:
            power_data &= ~(0x03)
        
        reg_file.seek(93)
        reg_file.write(struct.pack('B', int(power_data)))
        reg_file.close()

        return True

    def reset(self, port_num):
        # Check for invalid QSFP port_num
        if port_num not in self.qsfp_ports:
            return False

        try:
            port_name = self.get_port_name(port_num)
            reg_file = open(
                "/".join([self.PORT_INFO_PATH, port_name, "qsfp_reset"]), "w")
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False

        # Convert our register value back to a hex string and write back
        reg_file.seek(0)
        reg_file.write(hex(0))
        reg_file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        # Flip the bit back high and write back to the register to take port out of reset
        try:
            reg_file = open(
                "/".join([self.PORT_INFO_PATH, port_name, "qsfp_reset"]), "w")
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False

        reg_file.seek(0)
        reg_file.write(hex(1))
        reg_file.close()

        return True

    def get_transceiver_change_event(self, timeout=0):
        """
        To detect if any transceiver change event happens.
        """
        start_ms = time.time() * 1000
        xcvr_change_event_dict = {}
        event = False

        while True:
            time.sleep(0.5)
            for port in range(self.port_start, self.port_end+1):
                curr_presence = self.get_presence(port)
                if curr_presence != self.__xcvr_presence[port]:
                    if curr_presence is True:
                        xcvr_change_event_dict[str(port)] = '1'
                        self.__xcvr_presence[port] = True
                    elif curr_presence is False:
                        xcvr_change_event_dict[str(port)] = '0'
                        self.__xcvr_presence[port] = False
                    event = True

            if event is True:
                return True, xcvr_change_event_dict

            if timeout:
                now_ms = time.time() * 1000
                if (now_ms - start_ms >= timeout):
                    return True, xcvr_change_event_dict

    def tx_disable(self, port_num, disable):
        """
        @param port_num index of physical port
        @param disable, True  -- disable port tx signal
                        False -- enable port tx signal
        @return True when operation success, False on failure.
        """
        TX_DISABLE_BYTE_OFFSET = 86
        if port_num not in range(self.port_start, self.port_end + 1) or type(disable) != bool:
            return False

        # QSFP, set eeprom to disable tx
        if port_num in self.qsfp_ports:
            presence = self.get_presence(port_num)
            if not presence:
                return True

            disable = b'\x0f' if disable else b'\x00'
            # open eeprom
            try:
                with open(self.port_to_eeprom_mapping[port_num], mode="wb", buffering=0) as sysfsfile:
                    sysfsfile.seek(TX_DISABLE_BYTE_OFFSET)
                    sysfsfile.write(bytearray(disable))
            except IOError:
                return False

        # SFP, set tx_disable pin
        else:
            try:
                disable = hex(1) if disable else hex(0)
                port_name = self.get_port_name(port_num)
                reg_file = open(
                    "/".join([self.PORT_INFO_PATH, port_name, "sfp_txdisable"]), "w")
                reg_file.write(disable)
                reg_file.close()
            except IOError as e:
                print "Error: unable to open file: %s" % str(e)
                return False

        return True

    def reset_all(self):
        result = True
        port_sysfs_path = []
        for port in range(self.port_start, self.port_end+1):
            if port not in self.qsfp_ports:
                continue

            presence = self.get_presence(port)
            if not presence:
                continue

            try:
                port_name = self.get_port_name(port)
                sysfs_path = "/".join([self.PORT_INFO_PATH,
                                       port_name, "qsfp_reset"])
                reg_file = open(sysfs_path, "w")
                port_sysfs_path.append(sysfs_path)
            except IOError as e:
                result = False
                continue

            self._do_write_file(reg_file, 0, 0)

        time.sleep(1)

        for sysfs_path in port_sysfs_path:
            try:
                reg_file = open(sysfs_path, "w")
            except IOError as e:
                result = False
                continue

            self._do_write_file(reg_file, 0, 1)

        return result
