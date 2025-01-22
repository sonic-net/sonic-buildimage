#############################################################################
# SuperMicro SSE-T7132S
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

IPMI_SENSOR_NETFN = "0x04"
IPMI_SS_READ_CMD = "0x2D {}"
IPMI_SS_THRESHOLD_CMD = "0x27 {}"
HIGH_TRESHOLD_SET_KEY = "ucr"
LOW_TRESHOLD_SET_KEY = "lcr"
HIGH_CRIT_TRESHOLD_SET_KEY = "unr"
LOW_CRIT_TRESHOLD_SET_KEY = "lnr"


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index):
        ThermalBase.__init__(self)
        self._api_helper = APIHelper()
        self.index = thermal_index
        self.THERMAL_LIST = [
            ('CPU_Temp',        'CPU Temperature Sensor',                     '0x01'),
            ('PCH_Temp',        'PCH Temperature Sensor',                     '0x0a'),
            ('System_Temp',     'System Temperature Sensor',                  '0x0b'),
            ('Peripheral_Temp', 'Peripheral Temperature Sensor',              '0x0c'),
            ('Switch_Top-1',    'Switchboard Left Inlet Temperature Sensor',  '0xb4'),
            ('Switch_Buttom-1', 'Switchboard Left Outlet Temperature Sensor', '0xb5'),
            ('Switch_Top-2',    'Switchboard Right Inlet Temperature Sensor', '0xb6'),
            ('Switch_Buttom-2', 'Switchboard Right Outlet Temperature Sensor','0xb7'),
            ('Switch_Temp',     'Switch Temperature Sensor',                  '0xb8'),
        ]
        self.sensor_id = self.THERMAL_LIST[self.index][0]
        self.sensor_des = self.THERMAL_LIST[self.index][1]
        self.sensor_reading_addr = self.THERMAL_LIST[self.index][2]
        self.minimum_thermal = self.get_temperature()
        self.maximum_thermal = self.get_temperature()

    def __set_threshold(self, key, value):
        print('{} {}'.format(key, value))

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temperature = 0.0
        status, raw_ss_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(self.sensor_reading_addr))
        if status and len(raw_ss_read.split()) > 0:
            ss_read = raw_ss_read.split()[0]
            temperature = float(int(ss_read, 16))
        return temperature

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        high_threshold = 0.0
        status, raw_up_thres_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_THRESHOLD_CMD.format(self.sensor_reading_addr))
        if status and len(raw_up_thres_read.split()) > 6:
            ss_read = raw_up_thres_read.split()[5]
            high_threshold = float(int(ss_read, 16))
        return high_threshold

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        low_threshold = 0.0
        status, raw_up_thres_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_THRESHOLD_CMD.format(self.sensor_reading_addr))
        if status and len(raw_up_thres_read.split()) > 6:
            ss_read = raw_up_thres_read.split()[2]
            low_threshold = float(int(ss_read, 16))
        return low_threshold

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        status, ret_txt = self._api_helper.ipmi_set_ss_thres(self.sensor_id, HIGH_TRESHOLD_SET_KEY, temperature)
        return status

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        status, ret_txt = self._api_helper.ipmi_set_ss_thres(self.sensor_id, LOW_TRESHOLD_SET_KEY, temperature)
        return status

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal
        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        high_critical_threshold = 0.0
        status, raw_up_thres_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_THRESHOLD_CMD.format(self.sensor_reading_addr))
        if status and len(raw_up_thres_read.split()) > 6:
            ss_read = raw_up_thres_read.split()[6]
            high_critical_threshold = float(int(ss_read, 16))
        return high_critical_threshold

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal
        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        critical_low_threshold = 0.0
        status, raw_up_thres_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_THRESHOLD_CMD.format(self.sensor_reading_addr))
        if status and len(raw_up_thres_read.split()) > 6:
            ss_read = raw_up_thres_read.split()[3]
            critical_low_threshold = float(int(ss_read, 16))
        return critical_low_threshold

    def set_high_critical_threshold(self, temperature):
        """
        Sets the critical high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        status, ret_txt = self._api_helper.ipmi_set_ss_thres(self.sensor_id, HIGH_CRIT_TRESHOLD_SET_KEY, temperature)
        return status

    def set_low_critical_threshold(self, temperature):
        """
        Sets the critical low threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        status, ret_txt = self._api_helper.ipmi_set_ss_thres(self.sensor_id, LOW_CRIT_TRESHOLD_SET_KEY, temperature)
        return status

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return self.THERMAL_LIST[self.index][0]

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        return True if self.get_temperature() > 0 else False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self.sensor_des

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return "Unknown"

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return (self.index + 1)

    def is_replaceable(self):
        """
        Indicate whether this Thermal is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal
        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp < self.minimum_thermal:
            self.minimum_thermal = tmp
        return self.minimum_thermal

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal
        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp > self.maximum_thermal:
            self.maximum_thermal = tmp
        return self.maximum_thermal
