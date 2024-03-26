#############################################################################
# Edgecore
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

import os
import os.path
import glob

try:
    from .chassis import *
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_py_common.logger import Logger
    from swsscommon.swsscommon import SonicV2Connector
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = Logger()
# To enable messages of log_debug verbosity, uncomment the below line
# logger.set_min_log_priority_debug()

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""
    NUMBER_OF_THERMALS = 17
    ASIC_TEMP_SENSORS_OFFSET = 7
    ASIC_CALCULATED_TEMP_OFFSET = 15
    XCVR_TEMP_SENSORS_OFFSET = NUMBER_OF_THERMALS
    THERMAL_NAME_LIST = []
    TRANSCEIVER_LIST = []
    TRANSCEIVER_TEMP_LIST = []
    SYSFS_PATH = "/sys/bus/i2c/devices"
    Thermals_db = SonicV2Connector()
    Thermals_db.connect(Thermals_db.STATE_DB)

    # Find all transceivers in DB
    for i in range(0, PORT_END):
        db_field_temp = None
        db_field_warn = None
        db_field_alarm = None
        try:
            db_key = Thermals_db.get_all(Thermals_db.STATE_DB, 'TRANSCEIVER_DOM_SENSOR|Ethernet{}'.format(i * 8))
            db_field = db_key.get("temperature", None)
            db_field_temp = float(db_field)
            db_field = db_key.get("temphighwarning", None)
            db_field_warn = float(db_field)
            db_field = db_key.get("temphighalarm", None)
            db_field_alarm = float(db_field)
        except Exception as e:
            pass
        TRANSCEIVER_LIST.append([db_field_temp, db_field_warn, db_field_alarm, i])

    for i in range(0, len(TRANSCEIVER_LIST)):
        if isinstance(TRANSCEIVER_LIST[i][1], float):
            TRANSCEIVER_TEMP_LIST.append(TRANSCEIVER_LIST[i])

    NUMBER_OF_THERMALS += len(TRANSCEIVER_TEMP_LIST)

    def __init__(self, thermal_index=0):
        self.index = thermal_index
        # Add thermal name
        self.THERMAL_NAME_LIST.append("Temp sensor 1")
        self.THERMAL_NAME_LIST.append("Temp sensor 2")
        self.THERMAL_NAME_LIST.append("Temp sensor 3")
        self.THERMAL_NAME_LIST.append("Temp sensor 4")
        self.THERMAL_NAME_LIST.append("Temp sensor 5")
        self.THERMAL_NAME_LIST.append("Temp sensor 6")
        self.THERMAL_NAME_LIST.append("Temp sensor 7")
        self.THERMAL_NAME_LIST.append("ASIC sensor 1")
        self.THERMAL_NAME_LIST.append("ASIC sensor 2")
        self.THERMAL_NAME_LIST.append("ASIC sensor 3")
        self.THERMAL_NAME_LIST.append("ASIC sensor 4")
        self.THERMAL_NAME_LIST.append("ASIC sensor 5")
        self.THERMAL_NAME_LIST.append("ASIC sensor 6")
        self.THERMAL_NAME_LIST.append("ASIC sensor 7")
        self.THERMAL_NAME_LIST.append("ASIC sensor 8")
        self.THERMAL_NAME_LIST.append("ASIC  average")
        self.THERMAL_NAME_LIST.append("ASIC  maximum")
        # append transceiver names
        for i in range(0, len(Thermal.TRANSCEIVER_TEMP_LIST)):
            self.THERMAL_NAME_LIST.append("Temp xcvr  {:02d}".format(Thermal.TRANSCEIVER_TEMP_LIST[i][3]))

        if self.index < Thermal.ASIC_TEMP_SENSORS_OFFSET:
            # Set hwmon path
            i2c_path = {
                0: "18-0048/hwmon/hwmon*/",
                1: "18-0049/hwmon/hwmon*/",
                2: "18-004a/hwmon/hwmon*/",
                3: "18-004b/hwmon/hwmon*/",
                4: "18-004d/hwmon/hwmon*/",
                5: "18-004e/hwmon/hwmon*/",
                6: "18-004f/hwmon/hwmon*/",
            }.get(self.index, None)

            self.hwmon_path = "{}/{}".format(self.SYSFS_PATH, i2c_path)

        self.ss_key = self.THERMAL_NAME_LIST[self.index]
        self.ss_index = 1

        self.tbl = Thermal.Thermals_db.get_all(Thermal.Thermals_db.STATE_DB, 'ASIC_TEMPERATURE_INFO')

    def __read_txt_file(self, file_path):
        for filename in glob.glob(file_path):
            try:
                with open(filename, 'r') as fd:
                    data =fd.readline().rstrip()
                    return data
            except IOError as e:
                pass

        return None

    def __get_temp(self, temp_file):
        temp_file_path = os.path.join(self.hwmon_path, temp_file)
        raw_temp = self.__read_txt_file(temp_file_path)
        if raw_temp is not None:
            return float(raw_temp)/1000
        else:
            return 0


    def __set_threshold(self, file_name, temperature):
        if self.index < Thermal.ASIC_TEMP_SENSORS_OFFSET:
            temp_file_path = os.path.join(self.hwmon_path, file_name)
            for filename in glob.glob(temp_file_path):
                try:
                    with open(filename, 'w') as fd:
                        fd.write(str(temperature))
                    return True
                except IOError as e:
                    print("IOError: {} in file: {}".format(e, self.hwmon_path))
        else:
            return Flase


    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        if self.index < Thermal.ASIC_TEMP_SENSORS_OFFSET:
            temp_file = "temp{}_input".format(self.ss_index)
            return float(self.__get_temp(temp_file))
        elif self.index >= Thermal.ASIC_TEMP_SENSORS_OFFSET and self.index < Thermal.ASIC_CALCULATED_TEMP_OFFSET:
            return float(self.tbl.get("temperature_{}".format(self.index - Thermal.ASIC_TEMP_SENSORS_OFFSET), None))
        elif self.index == Thermal.ASIC_CALCULATED_TEMP_OFFSET:
            return float(self.tbl.get("average_temperature", None))
        elif self.index == Thermal.ASIC_CALCULATED_TEMP_OFFSET + 1:
            return float(self.tbl.get("maximum_temperature", None))
        elif self.index >= Thermal.XCVR_TEMP_SENSORS_OFFSET:
            return float(Thermal.TRANSCEIVER_TEMP_LIST[self.index - Thermal.XCVR_TEMP_SENSORS_OFFSET][0])
        else:
            return None

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.index < Thermal.ASIC_TEMP_SENSORS_OFFSET:
            temp_file = "temp{}_max".format(self.ss_index)
            return self.__get_temp(temp_file)
        elif self.index >= Thermal.ASIC_TEMP_SENSORS_OFFSET and self.index < Thermal.ASIC_CALCULATED_TEMP_OFFSET:
            return float(self.tbl.get("temperature_{}".format(self.index - Thermal.ASIC_TEMP_SENSORS_OFFSET), None))
        elif self.index >= Thermal.XCVR_TEMP_SENSORS_OFFSET:
            return Thermal.TRANSCEIVER_TEMP_LIST[self.index - Thermal.XCVR_TEMP_SENSORS_OFFSET][2]
        else:
            return None

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        if self.index < Thermal.ASIC_TEMP_SENSORS_OFFSET:
            temp_file = "temp{}_max".format(self.ss_index)
            temperature = temperature * 1000
            self.__set_threshold(temp_file, temperature)
        return True

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return self.THERMAL_NAME_LIST[self.index]

    def get_model(self):
        """
        Retrieves the model of the thermal device
            Returns:
            string: The model of the thermal device
        """
        if self.index < Thermal.ASIC_TEMP_SENSORS_OFFSET:
            return "lm75"
        else:
            return "XSight ASIC"

    def get_serial(self):
        """
        Retrieves the model of the thermal device
            Returns:
            string: The model of the thermal device
        """
        return "N/A"

    def is_replaceable(self):
        """
        Retrieves is the thermal device replaceable
            Returns:
            boolean: false
        """
        return False

    def get_presence(self):
        """
        Retrieves the presence of the Thermal
        Returns:
            bool: True if Thermal is present, False if not
        """
        if self.index < Thermal.ASIC_TEMP_SENSORS_OFFSET:
            temp_file = "temp{}_input".format(self.ss_index)
            temp_file_path = os.path.join(self.hwmon_path, temp_file)
            raw_txt = self.__read_txt_file(temp_file_path)
            if raw_txt is not None:
                return True
            else:
                return False
        else:
            return True

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.index < Thermal.ASIC_TEMP_SENSORS_OFFSET:
            file_str = "temp{}_input".format(self.ss_index)
            file_path = os.path.join(self.hwmon_path, file_str)
            raw_txt = self.__read_txt_file(file_path)
            if raw_txt is None:
                return False
            else:
                return int(raw_txt) != 0
        return True

    @classmethod
    def set_thermal_algorithm_status(cls, status, force=True):
        """
        Thermal policy activation/deactivation
        Returns:
            True if thermal algorithm status changed.
        """
        if not force and cls.thermal_algorithm_status == status:
            return False

        cls.thermal_algorithm_status = status
        logger.log_debug("Thermal:set_thermal_algorithm_status - {}".format(str(cls.thermal_algorithm_status)))
        return True

    @classmethod
    def check_module_temperature_trustable(cls):
        return 'untrust'

    @classmethod
    def get_temperatures(cls):
        """
        Representing important temperatures in device
        Returns:
            array of temperatures
        """
        from sonic_platform import platform
        if True == Thermal.get_transceiver_temperatures(True):
            logger.log_info("Detected changes in transceivers. Required reload of thermalctld")
            os.system("supervisorctl restart thermalctld")
        temperatures = []
        platform_chassis = platform.Platform().get_chassis()
        if platform_chassis is not None:
            thermals_num = platform_chassis.get_num_thermals()
            indX = 0
            for thermal_ins in platform_chassis.get_all_thermals():
                temperatures.append(thermal_ins.get_temperature())
                indX += 1
        else:
            logger.log_error("Thermal: Chassis is not available !")
        logger.log_debug("Thermal:get_temperatures {}".format(temperatures))
        return temperatures

    @classmethod
    def check_thermal_zone_temperature(cls):
        """
        Compare thermal zone temperature to the threshold

        Returns:
            True if all thermal zones temperatures are below the high threshold.
        TODO: Check temperature of modules
        """
        # Check Inbox thermperatures
        from sonic_platform import platform
        platform_chassis = platform.Platform().get_chassis()
        if platform_chassis is not None:
            thermals_num = platform_chassis.get_num_thermals()
            logger.log_debug("Thermal:check_thermal_zone_temperature: Got {} Thermal Objects".format(thermals_num))
            for thermal_ins in platform_chassis.get_all_thermals():
                if (thermal_ins.get_high_threshold() <= thermal_ins.get_temperature()):
                    logger.log_warning("Thermal: Critical temperature on {}".format(thermal_ins.get_name()))
                    return False
        else:
            logger.log_error("Thermal: Chassis is not available !")
        logger.log_debug("Thermal:check_thermal_zone_temperature: All temperatures looks OK")
        return True

    @staticmethod
    def get_transceiver_temperatures(update_class_list = False):
        """
        The function getting transceiver parameters from DB
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
        Returns:
            True if there was a change in plugged transceivers.
            (changed | removed | added transceiver)
            False if there is no change.
        """
        is_settings_changed = False
        xcvr_list = []
        # Find all transceivers in DB
        for i in range(0, PORT_END):
            db_field_temp = None
            db_field_warn = None
            db_field_alarm = None
            try:
                db_key = Thermal.Thermals_db.get_all(Thermal.Thermals_db.STATE_DB, 'TRANSCEIVER_DOM_SENSOR|Ethernet{}'.format(i * 8))
                db_field = db_key.get("temperature", None)
                db_field_temp = float(db_field)
                db_field = db_key.get("temphighwarning", None)
                db_field_warn = float(db_field)
                db_field = db_key.get("temphighalarm", None)
                db_field_alarm = float(db_field)
            except ValueError as e:
                print("Error: {}".format(e))

            if Thermal.TRANSCEIVER_LIST[i][1] != db_field_warn or Thermal.TRANSCEIVER_LIST[i][2] != db_field_alarm:
                is_settings_changed = True

            xcvr_list.append([db_field_temp, db_field_warn, db_field_alarm])

        # Update temperatures
        if True == update_class_list:
            Thermal.TRANSCEIVER_LIST = xcvr_list
        indx = 0
        for i in range(0, len(Thermal.TRANSCEIVER_LIST)):
            if isinstance(Thermal.TRANSCEIVER_LIST[i][1], float):
                Thermal.TRANSCEIVER_TEMP_LIST[indx][0] = xcvr_list[i][0]
                indx += 1

        return is_settings_changed

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return (self.index + 1)

