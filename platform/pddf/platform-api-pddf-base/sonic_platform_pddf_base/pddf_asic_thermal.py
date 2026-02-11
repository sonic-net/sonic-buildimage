#############################################################################
# PDDF
#
# PDDF ASIC thermal class for monitoring ASIC temperature sensors
#############################################################################

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from swsscommon.swsscommon import SonicV2Connector
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class PddfAsicThermal(ThermalBase):
    """PDDF ASIC Thermal class"""

    ASIC_TEMP_INFO = "ASIC_TEMPERATURE_INFO"

    def get_thermal_obj_name(self):
        return "ASIC_TEMP{}".format(self.thermal_index)

    def __init__(self, index, position_offset, pddf_data=None):
        self.thermal_index = index + 1
        self.thermal_position_in_parent = index + position_offset + 1
        # The sensors are 0-indexed in the DB.
        self.sensor_db_index = index
        thermal_obj = pddf_data.data[self.get_thermal_obj_name()]

        self.thermal_name = thermal_obj['dev_attr']['display_name']
        self.high_threshold = thermal_obj['dev_attr']['temp1_high_threshold']
        self.high_crit_threshold = thermal_obj['dev_attr']['temp1_high_crit_threshold']

    def get_name(self):
        return self.thermal_name

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_status(self):
        return self.get_presence()

    def get_presence(self):
        # Check the DB for the sensor
        db = SonicV2Connector()
        db.connect(db.STATE_DB)
        data_dict = db.get_all(db.STATE_DB, self.ASIC_TEMP_INFO)
        return "temperature_{}".format(self.sensor_db_index) in data_dict

    def get_temperature(self):
        db = SonicV2Connector()
        db.connect(db.STATE_DB)
        data_dict = db.get_all(db.STATE_DB, self.ASIC_TEMP_INFO)
        return float(data_dict["temperature_{}".format(self.sensor_db_index)])

    def get_high_threshold(self):
        val = self.high_threshold
        if val:
            return float(val)
        return None

    def get_high_critical_threshold(self):
        val = self.high_crit_threshold
        if val:
            return float(val)
        return None

    def get_low_threshold(self):
        raise NotImplementedError

    def set_high_threshold(self, temperature):
        raise NotImplementedError

    def set_low_threshold(self, temperature):
        raise NotImplementedError

    def get_temp_label(self):
        return None

    def dump_sysfs(self):
        return ''

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.thermal_position_in_parent

    def is_replaceable(self):
        """
        Indicate whether Thermal is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        # Usually thermal sensor is not replaceable
        return False
