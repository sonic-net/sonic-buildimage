from natsort import natsorted
from swsscommon.swsscommon import SonicV2Connector

from .health_checker import HealthChecker


class HardwareChecker(HealthChecker):
    """
    Check system hardware status. For now, it checks ASIC, PSU and fan status.
    """

    ASIC_TEMPERATURE_KEY = 'TEMPERATURE_INFO|ASIC'
    FAN_TABLE_NAME = 'FAN_INFO'
    PSU_TABLE_NAME = 'PSU_INFO'

    def __init__(self):
        HealthChecker.__init__(self)
        self._db = SonicV2Connector(use_unix_socket_path=True)
        self._db.connect(self._db.STATE_DB)

    def get_category(self):
        return 'Hardware'

    def check(self, config):
        self.reset()
        self._check_asic_status(config)
        self._check_fan_status(config)
        self._check_psu_status(config)

    def _check_asic_status(self, config):
        """
        Check if ASIC temperature is in valid range.
        :param config: Health checker configuration
        :return:
        """
        if config.ignore_devices and 'asic' in config.ignore_devices:
            return

        ASIC_TEMPERATURE_KEY_LIST = self._db.keys(self._db.STATE_DB,
                                                  HardwareChecker.ASIC_TEMPERATURE_KEY + '*')
        for asic_key in ASIC_TEMPERATURE_KEY_LIST:
            temperature = self._db.get(self._db.STATE_DB, asic_key,
                                                          'temperature')
            temperature_threshold = self._db.get(self._db.STATE_DB, asic_key,
                                                          'high_threshold')
            asic_name = asic_key.split('|')[1]
            if not temperature:
                self.set_object_not_ok('ASIC', asic_name,
                        'Failed to get {} temperature'.format(asic_name))
            elif not temperature_threshold:
                self.set_object_not_ok('ASIC', asic_name,
                        'Failed to get {} temperature threshold'.format(asic_name))
            else:
                try:
                    temperature = float(temperature)
                    temperature_threshold = float(temperature_threshold)
                    if temperature > temperature_threshold:
                        self.set_object_not_ok('ASIC', asic_name,
                                               '{} temperature is too hot, temperature={}, threshold={}'.format(
                                                asic_name, temperature, temperature_threshold))
                    else:
                        self.set_object_ok('ASIC', asic_name)
                except ValueError as e:
                    self.set_object_not_ok('ASIC', asic_name,
                                           'Invalid {} temperature data, temperature={}, threshold={}'.format(
                                            asic_name, temperature, temperature_threshold))

    def _check_fan_status(self, config):
        """
        Check fan status including:
            1. Check all fans are present
            2. Check all fans are in good state
            3. Check fan speed is in valid range
            4. Check all fans direction are the same
        :param config: Health checker configuration
        :return:
        """
        if config.ignore_devices and 'fan' in config.ignore_devices:
            return

        keys = self._db.keys(self._db.STATE_DB, HardwareChecker.FAN_TABLE_NAME + '*')
        if not keys:
            self.set_object_not_ok('Fan', 'Fan', 'Failed to get fan information')
            return

        expect_fan_direction = None
        for key in natsorted(keys):
            key_list = key.split('|')
            if len(key_list) != 2:  # error data in DB, log it and ignore
                self.set_object_not_ok('Fan', key, 'Invalid key for FAN_INFO: {}'.format(key))
                continue

            name = key_list[1]
            if config.ignore_devices and name in config.ignore_devices:
                continue
            data_dict = self._db.get_all(self._db.STATE_DB, key)
            presence = data_dict.get('presence', 'false')
            if presence.lower() != 'true':
                self.set_object_not_ok('Fan', name, '{} is missing'.format(name))
                continue

            if not self._ignore_check(config.ignore_devices, 'fan', name, 'speed'):
                speed = data_dict.get('speed', None)
                speed_target = data_dict.get('speed_target', None)
                speed_tolerance = data_dict.get('speed_tolerance', None)
                if not speed:
                    self.set_object_not_ok('Fan', name, 'Failed to get actual speed data for {}'.format(name))
                    continue
                elif not speed_target:
                    self.set_object_not_ok('Fan', name, 'Failed to get target speed date for {}'.format(name))
                    continue
                elif not speed_tolerance:
                    self.set_object_not_ok('Fan', name, 'Failed to get speed tolerance for {}'.format(name))
                    continue
                else:
                    try:
                        speed = float(speed)
                        speed_target = float(speed_target)
                        speed_tolerance = float(speed_tolerance)
                        speed_min_th = speed_target * (1 - float(speed_tolerance) / 100)
                        speed_max_th = speed_target * (1 + float(speed_tolerance) / 100)
                        if speed < speed_min_th or speed > speed_max_th:
                            self.set_object_not_ok('Fan', name,
                                                   '{} speed is out of range, speed={}, range=[{},{}]'.format(name,
                                                                                                              speed,
                                                                                                              speed_min_th,
                                                                                                              speed_max_th))
                            continue
                    except ValueError:
                        self.set_object_not_ok('Fan', name,
                                               'Invalid fan speed data for {}, speed={}, target={}, tolerance={}'.format(
                                                   name,
                                                   speed,
                                                   speed_target,
                                                   speed_tolerance))
                        continue

            if not self._ignore_check(config.ignore_devices, 'fan', name, 'direction'):
                direction = data_dict.get('direction', 'N/A')
                # ignore fan whose direction is not available to avoid too many false alarms
                if direction != 'N/A':
                    if not expect_fan_direction:
                        # initialize the expect fan direction
                        expect_fan_direction = (name, direction)
                    elif direction != expect_fan_direction[1]:
                        self.set_object_not_ok('Fan', name,
                                               f'{name} direction {direction} is not aligned with {expect_fan_direction[0]} direction {expect_fan_direction[1]}')
                        continue

            status = data_dict.get('status', 'false')
            if status.lower() != 'true':
                self.set_object_not_ok('Fan', name, '{} is broken'.format(name))
                continue

            self.set_object_ok('Fan', name)

    def _check_psu_status(self, config):
        """
        Check PSU status including:
            1. Check all PSUs are present
            2. Check all PSUs are power on
            3. Check PSU temperature is in valid range
            4. Check PSU voltage is in valid range
        :param config: Health checker configuration
        :return:
        """
        if config.ignore_devices and 'psu' in config.ignore_devices:
            return

        keys = self._db.keys(self._db.STATE_DB, HardwareChecker.PSU_TABLE_NAME + '*')
        if not keys:
            self.set_object_not_ok('PSU', 'PSU', 'Failed to get PSU information')
            return

        for key in natsorted(keys):
            key_list = key.split('|')
            if len(key_list) != 2:  # error data in DB, log it and ignore
                self.set_object_not_ok('PSU', key, 'Invalid key for PSU_INFO: {}'.format(key))
                continue

            name = key_list[1]
            if config.ignore_devices and name in config.ignore_devices:
                continue

            data_dict = self._db.get_all(self._db.STATE_DB, key)
            presence = data_dict.get('presence', 'false')
            if presence.lower() != 'true':
                self.set_object_not_ok('PSU', name, '{} is missing or not available'.format(name))
                continue

            status = data_dict.get('status', 'false')
            if status.lower() != 'true':
                self.set_object_not_ok('PSU', name, '{} is out of power'.format(name))
                continue

            if not self._ignore_check(config.ignore_devices, 'psu', name, 'temperature'):
                temperature = data_dict.get('temp', None)
                temperature_threshold = data_dict.get('temp_threshold', None)
                if temperature is None:
                    self.set_object_not_ok('PSU', name, 'Failed to get temperature data for {}'.format(name))
                    continue
                elif temperature_threshold is None:
                    self.set_object_not_ok('PSU', name, 'Failed to get temperature threshold data for {}'.format(name))
                    continue
                else:
                    try:
                        temperature = float(temperature)
                        temperature_threshold = float(temperature_threshold)
                        if temperature > temperature_threshold:
                            self.set_object_not_ok('PSU', name,
                                                   '{} temperature is too hot, temperature={}, threshold={}'.format(
                                                       name, temperature,
                                                       temperature_threshold))
                            continue
                    except ValueError:
                        self.set_object_not_ok('PSU', name,
                                               'Invalid temperature data for {}, temperature={}, threshold={}'.format(
                                                   name, temperature,
                                                   temperature_threshold))
                        continue

            if not self._ignore_check(config.ignore_devices, 'psu', name, 'voltage'):
                voltage = data_dict.get('voltage', None)
                voltage_min_th = data_dict.get('voltage_min_threshold', None)
                voltage_max_th = data_dict.get('voltage_max_threshold', None)
                if voltage is None:
                    self.set_object_not_ok('PSU', name, 'Failed to get voltage data for {}'.format(name))
                    continue
                elif voltage_min_th is None:
                    self.set_object_not_ok('PSU', name,
                                           'Failed to get voltage minimum threshold data for {}'.format(name))
                    continue
                elif voltage_max_th is None:
                    self.set_object_not_ok('PSU', name,
                                           'Failed to get voltage maximum threshold data for {}'.format(name))
                    continue
                else:
                    try:
                        voltage = float(voltage)
                        voltage_min_th = float(voltage_min_th)
                        voltage_max_th = float(voltage_max_th)
                        if voltage < voltage_min_th or voltage > voltage_max_th:
                            self.set_object_not_ok('PSU', name,
                                                   '{} voltage is out of range, voltage={}, range=[{},{}]'.format(name,
                                                                                                                  voltage,
                                                                                                                  voltage_min_th,
                                                                                                                  voltage_max_th))
                            continue
                    except ValueError:
                        self.set_object_not_ok('PSU', name,
                                               'Invalid voltage data for {}, voltage={}, range=[{},{}]'.format(name,
                                                                                                               voltage,
                                                                                                               voltage_min_th,
                                                                                                               voltage_max_th))
                        continue

            if not self._ignore_check(config.ignore_devices, 'psu', name, 'power_threshold'):
                power_overload = data_dict.get('power_overload', None)
                if power_overload == 'True':

                    try:
                        power = data_dict['power']
                        power_critical_threshold = data_dict['power_critical_threshold']
                        self.set_object_not_ok('PSU', name, 'System power exceeds threshold ({}w)'.format(power_critical_threshold))
                    except KeyError:
                        self.set_object_not_ok('PSU', name, 'System power exceeds threshold but power_critical_threshold is invalid')
                    continue

            self.set_object_ok('PSU', name)

    def reset(self):
        self._info = {}

    @classmethod
    def _ignore_check(cls, ignore_set, category, object_name, check_point):
        if not ignore_set:
            return False

        if '{}.{}'.format(category, check_point) in ignore_set:
            return True
        elif '{}.{}'.format(object_name, check_point) in ignore_set:
            return True
        return False
