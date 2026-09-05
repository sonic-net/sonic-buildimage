#! /usr/bin/python
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
    import sys
    sys.path.append('/usr/lib/python2.7/dist-packages/sonic_sfp/')
    from sff8472 import sff8472InterfaceId
    from sff8472 import sff8472Dom
    from sff8436 import sff8436InterfaceId
    from sff8436 import sff8436Dom
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 53
    PORTS_IN_BLOCK = 54

    EEPROM_OFFSET = 11

    _port_to_eeprom_mapping = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.PORT_START + 48, self.PORTS_IN_BLOCK))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"

        for x in range(0, self.port_end + 1):
            self._port_to_eeprom_mapping[x] = eeprom_path.format(x + self.EEPROM_OFFSET)

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        bit_mask = port_num % 8

        if port_num <= 7:
            presence_path = "/sys/bus/i2c/devices/3-0031/sfp_mod_abs1"
        elif 8 <= port_num and port_num <= 15:
            presence_path = "/sys/bus/i2c/devices/3-0031/sfp_mod_abs2"
        elif 16 <= port_num and port_num <= 23:
            presence_path = "/sys/bus/i2c/devices/3-0031/sfp_mod_abs3"
        elif 24 <= port_num and port_num <= 27:
            presence_path = "/sys/bus/i2c/devices/3-0031/sfp_mod_abs4"
        elif 28 <= port_num and port_num <= 31:
            presence_path = "/sys/bus/i2c/devices/4-0032/sfp_mod_abs1"
            bit_mask = bit_mask - 4
        elif 32 <= port_num and port_num <= 39:
            presence_path = "/sys/bus/i2c/devices/4-0032/sfp_mod_abs2"
        elif 40 <= port_num and port_num <= 47:
            presence_path = "/sys/bus/i2c/devices/4-0032/sfp_mod_abs3"
        elif 48 <= port_num and port_num <= 71:
            presence_path = "/sys/bus/i2c/devices/4-0032/qsfp_modprs"
        else:
            return False

        try:
            reg_file = open(presence_path, "rb")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()
        reg_value = int(content, 16)
        reg_file.close()

        if reg_value & (1 << bit_mask) == 0:
            return True
        else:
            return False

    def get_low_power_mode(self, port_num):
        if port_num in self.qsfp_ports:
            bit_mask = port_num % 8
        else:
            return False

        try:
            reg_file = open("/sys/bus/i2c/devices/4-0032/qsfp_lpmode")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

        content = reg_file.readline().rstrip()
        reg_value = int(content, 16)
        reg_file.close()

        if reg_value & (1 << bit_mask) == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):
        if port_num in self.qsfp_ports:
            bit_mask = port_num % 8
        else:
            return False

        try:
            reg_file = open("/sys/bus/i2c/devices/4-0032/qsfp_lpmode", "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()
        reg_value = int(content, 16)

        if lpmode is True:
            reg_value = reg_value | (1 << bit_mask)
        else:
            reg_value = reg_value & ~(1 << bit_mask)

        reg_file.seek(0)
        reg_file.write(str(reg_value))
        reg_file.close()

        return True

    def reset(self, port_num):
        if port_num in self.qsfp_ports:
            bit_mask = (port_num % 8) + 2
        else:
            return False

        try:
            reg_file = open("/sys/bus/i2c/devices/4-0032/reset_control", "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()
        reg_value = int(content, 16)
        reg_value = reg_value & ~(1 << bit_mask)

        reg_file.seek(0)
        reg_file.write(str(reg_value))
        reg_file.close()

        time.sleep(1)

        try:
            reg_file = open("/sys/bus/i2c/devices/4-0032/reset_control", "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = reg_value | (1 << bit_mask)
        reg_file.seek(0)
        reg_file.write(str(reg_value))
        reg_file.close()

        return True

    def get_eeprom_dict(self, port_num):
        if not self.get_presence(port_num):
            return None

        sfp_data = {}

        eeprom_ifraw = self.get_eeprom_raw(port_num)
        eeprom_domraw = self.get_eeprom_dom_raw(port_num)

        if eeprom_ifraw is None:
            return None

        if port_num in self.qsfp_ports:
            sfpi_obj = sff8436InterfaceId(eeprom_ifraw)
            if sfpi_obj is not None:
                sfp_data['interface'] = sfpi_obj.get_data_pretty()

            sfpd_obj = sff8436Dom(eeprom_ifraw)
            if sfpd_obj is not None:
                sfp_data['dom'] = sfpd_obj.get_data_pretty()
            return sfp_data

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
