import json
import os

from sonic_py_common import device_info


class Config(object):
    """
    Manage configuration of system health.
    """

    # Default system health check interval
    DEFAULT_INTERVAL = 60

    # Default boot up timeout. When reboot system, system health will wait a few seconds before starting to work.
    DEFAULT_BOOTUP_TIMEOUT = 300

    # Default LED configuration. Different platform has different LED capability. This configuration allow vendor to
    # override the default behavior.
    DEFAULT_LED_CONFIG = {
        'fault': 'red',
        'normal': 'green',
        'booting': 'red'
    }

    # System health configuration file name
    CONFIG_FILE = 'system_health_monitoring_config.json'

    # Monit service configuration file path
    MONIT_CONFIG_FILE = '/etc/monit/monitrc'

    # Monit service start delay configuration entry
    MONIT_START_DELAY_CONFIG = 'with start delay'

    def __init__(self):
        """
        Constructor. Initialize all configuration entry to default value in case there is no configuration file.
        """
        self.platform_name = device_info.get_platform()
        self._config_file = os.path.join('/usr/share/sonic/device/', self.platform_name, Config.CONFIG_FILE)
        self._last_mtime = None
        self.config_data = None
        self.interval = Config.DEFAULT_INTERVAL
        self.ignore_services = None
        self.ignore_devices = None
        self.user_defined_checkers = None
        self.include_devices = None
    def config_file_exists(self):
        return os.path.exists(self._config_file)

    def load_config(self):
        """
        Load the configuration file from disk.
            1. If there is no configuration file, current config entries will reset to default value
            2. Only read the configuration file is last_mtime changes for better performance
            3. If there is any format issues in configuration file, current config entries will reset to default value
        :return:
        """
        if not self.config_file_exists():
            if self._last_mtime is not None:
                self._reset()
            return

        mtime = os.stat(self._config_file)
        if mtime != self._last_mtime:
            try:
                self._last_mtime = mtime
                with open(self._config_file, 'r') as f:
                    self.config_data = json.load(f)

                self.interval = self.config_data.get('polling_interval', Config.DEFAULT_INTERVAL)
                self.ignore_services = self._get_list_data('services_to_ignore')
                self.ignore_devices = self._get_list_data('devices_to_ignore')
                self.user_defined_checkers = self._get_list_data('user_defined_checkers')
                self.include_devices = self._get_list_data('include_devices')
            except Exception as e:
                self._reset()

    def _reset(self):
        """
        Reset current configuration entry to default value
        :return:
        """
        self._last_mtime = None
        self.config_data = None
        self.interval = Config.DEFAULT_INTERVAL
        self.ignore_services = None
        self.ignore_devices = None
        self.user_defined_checkers = None
        self.include_devices = None

    def get_led_color(self, status):
        """
        Get desired LED color according to the input status
        :param status: System health status
        :return: StringLED color
        """
        if self.config_data and 'led_color' in self.config_data:
            if status in self.config_data['led_color']:
                return self.config_data['led_color'][status]

        return self.DEFAULT_LED_CONFIG[status]

    def get_bootup_timeout(self):
        """
        Get boot up timeout from monit configuration file.
            1. If monit configuration file does not exist, return default value
            2. If there is any exception while parsing monit config, return default value
        :return: Integer timeout value
        """
        if not os.path.exists(Config.MONIT_CONFIG_FILE):
            return self.DEFAULT_BOOTUP_TIMEOUT

        try:
            with open(Config.MONIT_CONFIG_FILE) as f:
                lines = f.readlines()
                for line in lines:
                    if not line:
                        continue

                    line = line.strip()
                    if not line:
                        continue

                    pos = line.find('#')
                    if pos == 0:
                        continue

                    line = line[:pos]
                    pos = line.find(Config.MONIT_START_DELAY_CONFIG)
                    if pos != -1:
                        return int(line[pos + len(Config.MONIT_START_DELAY_CONFIG):].strip())
        except Exception:
            return self.DEFAULT_BOOTUP_TIMEOUT

    def _get_list_data(self, key):
        """
        Get list type configuration data by key and remove duplicate element.
        :param key: Key of the configuration entry
        :return: A set of configuration data if key exists
        """
        if key in self.config_data:
            data = self.config_data[key]
            if isinstance(data, list):
                return set(data)
        return None
