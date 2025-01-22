import os
import yaml
import subprocess
from sonic_py_common import device_info


class Common:

    DEVICE_PATH = '/usr/share/sonic/device/'
    PMON_PLATFORM_PATH = '/usr/share/sonic/platform/'
    CONFIG_DIR = 'sonic_platform_config'

    HOST_CHK_CMD = ["docker"]

    def __init__(self):
        (self.platform, self.hwsku) = device_info.get_platform_and_hwsku()

    def is_host(self):
        return subprocess.call(self.HOST_CHK_CMD) == 0

    def load_json_file(self, path):
        """
        Retrieves the json object from json file path

        Returns:
            A json object
        """
        with open(path, 'r') as f:
            json_data = yaml.safe_load(f)

        return json_data

    def get_config_path(self, config_name):
        """
        Retrieves the path to platform api config directory

        Args:
            config_name: A string containing the name of config file.

        Returns:
            A string containing the path to json file
        """
        return os.path.join(self.DEVICE_PATH, self.platform, self.CONFIG_DIR, config_name) if self.is_host() else os.path.join(self.PMON_PLATFORM_PATH, self.CONFIG_DIR, config_name)

