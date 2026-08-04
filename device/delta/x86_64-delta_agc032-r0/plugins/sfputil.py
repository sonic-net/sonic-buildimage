# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
    from sonic_sfp.sff8472 import sff8472InterfaceId
    from sonic_sfp.sff8472 import sff8472InterfaceId
    from sonic_sfp.sff8436 import sff8436InterfaceId
    from sonic_sfp.sff8436 import sff8436Dom
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 33
    PORTS_IN_BLOCK = 32

    EEPROM_OFFSET = 1

    CPLD_SWITCH = 0

    _port_to_eeprom_mapping = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(0, self.PORTS_IN_BLOCK))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        eeprom_path = "/sys/bus/i2c/devices/{0}-0050/eeprom"

        for x in range(0, self.port_end + 1):
            self._port_to_eeprom_mapping[x] = eeprom_path.format(x + self.EEPROM_OFFSET)

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        # Retrieve file path of presence
        port = (str(port_num + 1)).zfill(2)

        # SWPLD2 for port 1~16, SWPLD3 for port 17~34
        if port_num < 16:
            present_path = "SWPLD2/qsfp_p{}_modprs".format(port)
        elif port_num < self.PORTS_IN_BLOCK:
            present_path = "SWPLD3/qsfp_p{}_modprs".format(port)
        else:
            present_path = "SWPLD3/sfp_p{}_modprs".format(str(port_num - self.PORTS_IN_BLOCK))

        try:
            with open("/sys/devices/platform/delta-agc032-swpld.0/" + present_path, 'r') as present:
                if int(present.readline()) == 0:
                    return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num >= self.PORTS_IN_BLOCK:
            return False

        # Retrieve file path of presence
        port = (str(port_num + 1)).zfill(2)

        # SWPLD2 for port 1~16, SWPLD3 for port 17~32
        if port_num < 16:
            lpmode_path = "SWPLD2/qsfp_p{}_lpmode".format(port)
        else:
            lpmode_path = "SWPLD3/qsfp_p{}_lpmode".format(port)

        try:
            with open("/sys/devices/platform/delta-agc032-swpld.0/" + lpmode_path, 'r') as lpmode:
                if int(lpmode.readline()) == 1:
                    return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return False

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num >= self.PORTS_IN_BLOCK:
            return False

        # Retrieve file path of presence
        port = (str(port_num + 1)).zfill(2)

        # SWPLD2 for port 1~16, SWPLD3 for port 17~32
        if port_num < 16:
            lpmode_path = "SWPLD2/qsfp_p{}_lpmode".format(port)
        else:
            lpmode_path = "SWPLD3/qsfp_p{}_lpmode".format(port)

        try:
            file = open("/sys/devices/platform/delta-agc032-swpld.0/" + lpmode_path, 'r+')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        file.seek(0)
        if lpmode is True:
            # "1" for lpmode on
            file.write('1')
        else:
            # "0" for lpmode off
            file.write('0')
        file.close()

        return True

    def reset(self, port_num):

        # Check for invalid port_num
        if port_num < self.port_start or port_num >= self.PORTS_IN_BLOCK:
            return False

        # Retrieve file path of presence
        port = (str(port_num + 1)).zfill(2)

        # SWPLD2 for port 1~16, SWPLD3 for port 17~32
        if port_num < 16:
            reset_path = "SWPLD2/qsfp_p{}_rst".format(port)
        else:
            reset_path = "SWPLD3/qsfp_p{}_rst".format(port)

        try:
            file = open("/sys/devices/platform/delta-agc032-swpld.0/" + reset_path, 'r+')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        file.seek(0)
        file.write('0')
        file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        # Flip the bit back high and write back to the register to take port out of reset
        try:
            file = open("/sys/devices/platform/delta-agc032-swpld.0/" + reset_path, 'r+')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        file.seek(0)
        file.write('1')
        file.close()

        return True

    def get_eeprom_dict(self, port_num):
        sfp_data = {}

        try:
            file = open("/sys/devices/platform/delta-agc032-cpupld.0/cpu_i2c_mux_sel", 'r+')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # Switch CPLD to FRONT-PORT EEPROM MUX
        file.seek(0)
        file.write('3')
        file.close()

        eeprom_ifraw = self.get_eeprom_raw(port_num)
        eeprom_domraw = self.get_eeprom_dom_raw(port_num)

        try:
            file = open("/sys/devices/platform/delta-agc032-cpupld.0/cpu_i2c_mux_sel", 'r+')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # Switch CPLD to FRONT-PORT EEPROM MUX
        file.seek(0)
        file.write('1')
        file.close()

        if eeprom_ifraw is None:
            return None

        if port_num in self.osfp_ports:
            sfpi_obj = inf8628InterfaceId(eeprom_ifraw)
            if sfpi_obj is not None:
                sfp_data['interface'] = sfpi_obj.get_data_pretty()
            return sfp_data
        elif port_num in self.qsfp_ports:
            sfpi_obj = sff8436InterfaceId(eeprom_ifraw)
            if sfpi_obj is not None:
                sfp_data['interface'] = sfpi_obj.get_data_pretty()
            # For Qsfp's the dom data is part of eeprom_if_raw
            # The first 128 bytes

            sfpd_obj = sff8436Dom(eeprom_ifraw)
            if sfpd_obj is not None:
                sfp_data['dom'] = sfpd_obj.get_data_pretty()

            return sfp_data
        else:
            sfpi_obj = sff8472InterfaceId(eeprom_ifraw)
            if sfpi_obj is not None:
                sfp_data['interface'] = sfpi_obj.get_data_pretty()
                cal_type = sfpi_obj.get_calibration_type()

            if eeprom_domraw is not None:
                sfpd_obj = sff8472Dom(eeprom_domraw, cal_type)
                if sfpd_obj is not None:
                    sfp_data['dom'] = sfpd_obj.get_data_pretty()

            return sfp_data

    def get_transceiver_change_event(self):
        """
        TODO: This function need to be implemented
        when decide to support monitoring SFP(Xcvrd)
        on this platform.
        """
        raise NotImplementedError
