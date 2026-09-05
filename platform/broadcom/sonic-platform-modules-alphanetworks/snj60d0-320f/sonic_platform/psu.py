#############################################################################
# Alphanetworks
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSU status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from sonic_eeprom import eeprom_base
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Psu(PsuBase):
    """Platform-specific PSU class"""

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        # initialize PSU Fan
        fan = Fan(psu_index, True)
        self._fan_list.append(fan)
        self.index = psu_index + 1
        
        self.pus_type = "AC"
        # driver attribute
        self.psu_presence = "/psu{}_present".format(psu_index+1)
        self.psu_oper_status = "/psu{}_power_good".format(psu_index+1)
        self.psu_model_name = "/psu_model_name"
        self.psu_mfr_id = "/psu_mfr_id"
        self.psu_v_in = "/psu_v_in"
        self.psu_v_out = "/psu_v_out"
        self.psu_i_in = "/psu_i_in"
        self.psu_i_out = "/psu_i_out"
        self.psu_p_in = "/psu_p_in"
        self.psu_p_out = "/psu_p_out"
        self.psu_temp1_input = "/psu_temp1_input"
        self.psu_mfr_pout_max = "/psu_mfr_pout_max"
        self.psu_serial_num = "/eeprom"

        # psu eeprom info
        self._PSU_EEPROM_SERIAL_NUM_OFFSET = 0x35
        self._PSU_EEPROM_SERIAL_NUM_LENGTH = 19

        # driver path
        psu_bus_num = [10, 11]
        psu_eeprom_address = [50, 51]
        psu_pmbus_address = [58, 59]
        psu_path = "/sys/bus/i2c/devices/"
        self.psu_mapping = psu_path + "1-005e"
        self.psu_eeprom = psu_path + "{}-00{}".format(psu_bus_num[psu_index], psu_eeprom_address[psu_index])
        self.psu_pmbus = psu_path +"{}-00{}".format(psu_bus_num[psu_index], psu_pmbus_address[psu_index])


    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU{}".format(self.index)

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        status = 0
        node = self.psu_mapping + self.psu_presence
        try:
            with open(node, 'r') as presence_status:
                status = int(presence_status.read())
        except IOError:
            return False
        return status == 1

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = 0
        node = self.psu_mapping + self.psu_oper_status
        try:
            with open(node, 'r') as power_status:
                status = int(power_status.read())
        except IOError:
            return False
        return status == 1

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        model = ""
        node = self.psu_pmbus + self.psu_model_name
        try:
            with open(node, 'r') as model_name:
                model = model_name.read()
        except IOError:
            return None
        return model.rstrip()

    def get_mfr_id(self):
        """
        Retrieves the manufacturer's name (or id) of the device

        Returns:
            string: Manufacturer's id of device
        """
        mfr = ""
        node = self.psu_pmbus + self.psu_mfr_id
        try:
            with open(node, 'r') as mfr_id:
                mfr = mfr_id.read()
        except IOError:
            return None
        return mfr.rstrip()

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        serial = ""
        node = self.psu_eeprom + self.psu_serial_num
        try:
            psu_eeprom = eeprom_base.EepromDecoder(node, None, 0, '', True)
            serial = psu_eeprom.read_eeprom_bytes(self._PSU_EEPROM_SERIAL_NUM_LENGTH, self._PSU_EEPROM_SERIAL_NUM_OFFSET)
            if len(serial) != self._PSU_EEPROM_SERIAL_NUM_LENGTH:
                return None
        except IOError:
            return None
        return serial.decode("utf-8") 

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts, 
            e.g. 12.1 
        """
        vout = 0.0
        node = self.psu_pmbus + self.psu_v_out
        try:
            with open(node, 'r') as v_out:
                vout = int(v_out.read())
        except IOError:
            return vout
        return float(vout) / 1000

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        iout = 0.0
        node = self.psu_pmbus + self.psu_i_out
        try:
            with open(node, 'r') as i_out:
                iout = int(i_out.read())
        except IOError:
            return iout
        return float(iout) / 1000

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        pout = 0.0
        node = self.psu_pmbus + self.psu_p_out
        try:
            with open(node, 'r') as p_out:
                pout = int(p_out.read())
        except IOError:
            return pout
        return float(pout) / 1000

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        status = 0
        node = self.psu_mapping + self.psu_oper_status
        try:
            with open(node, 'r') as powergood_status:
                status = int(powergood_status.read())
        except IOError:
            return False
        return status == 1


    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED

        Args:
            color: A string representing the color with which to set the
                   PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        from .led import PsuLed
        psuled = PsuLed.get_psuLed()
        return psuled.update_status()

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        from .led import PsuLed
        psuled = PsuLed.get_psuLed()
        return psuled.get_status()

    def get_input_voltage(self):
        """
        Retrieves current input voltage to the PSU

        Returns:
            A float number, the input voltage in volts,
            e.g. 20.3
        """
        vin = 0.0
        node = self.psu_pmbus + self.psu_v_in
        try:
            with open(node, 'r') as v_in:
                vin = int(v_in.read())
        except IOError:
            return vin
        return float(vin) / 1000
        

    def get_input_current(self):
        """
        Retrieves present electric current supplied to the PSU

        Returns:
            A float number, the electric current in amperes, e.g 13.7
        """
        iin = 0.0
        node = self.psu_pmbus + self.psu_i_in
        try:
            with open(node, 'r') as i_in:
                iin = int(i_in.read())
        except IOError:
            return iin
        return float(iin) / 1000

    def get_input_power(self):
        """
        Retrieves current energy supplied to the PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        pin = 0.0
        node = self.psu_pmbus + self.psu_p_in
        try:
            with open(node, 'r') as p_in:
                pin = int(p_in.read())
        except IOError:
            return pin
        return float(pin) / 1000

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temp1in = 0.0
        node = self.psu_pmbus + self.psu_temp1_input
        try:
            with open(node, 'r') as temp1_in:
                temp1in = int(temp1_in.read())
        except IOError:
            return temp1in
        return float(temp1in) / 1000
        
    def get_type(self):
        """
        Gets the type of the PSU

        Returns:
            A string, the type of PSU (AC/DC)
        """
        return self.pus_type

    def get_capacity(self):
        """
        Gets the capacity (maximum output power) of the PSU in watts

        Returns:
            An integer, the capacity of PSU
        """
        poutmax = 0
        node = self.psu_pmbus + self.psu_mfr_pout_max
        try:
            with open(node, 'r') as pout_max:
                poutmax = int(pout_max.read())
        except IOError:
            return poutmax
        return int(float(poutmax) / 1000)
