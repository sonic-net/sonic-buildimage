import os
from sonic_platform_base.sensor_base import VoltageSensorBase


class VoltageSensor(VoltageSensorBase):
    """Platform-specific VoltageSensor class for MAX11611 ADC with debug"""

    MAX_CHANNELS = 12
    IIO_PATH_BASE = "/sys/bus/iio/devices/"

    @classmethod
    def get_iio_device_path(cls):
        """Find the first iio:device* directory"""
        for entry in os.listdir(cls.IIO_PATH_BASE):
            full_path = os.path.join(cls.IIO_PATH_BASE, entry)
            if os.path.isdir(full_path) and entry.startswith("iio:device"):
                return full_path
        return None

    def __init__(self, voltage_index):
        if voltage_index < 0 or voltage_index >= self.MAX_CHANNELS:
            raise ValueError(f"Invalid voltage index: {voltage_index}")

        self.index = voltage_index

        # Auto-detect IIO device path
        self.voltage_path = self.get_iio_device_path()
        if not self.voltage_path:
            raise RuntimeError("No IIO device found for voltage sensors")

        # Raw voltage file
        self.voltage_raw_file = os.path.join(self.voltage_path, f"in_voltage{self.index}_raw")

        # Scale file
        self.voltage_scale_file = os.path.join(self.voltage_path, "in_voltage_scale")

        # Display name
        self.voltage_name = f"Voltage sensor {self.index}"

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, "r") as fd:
                data = fd.read().strip()
                return data
        except IOError:
            return None

    def __get_scale(self):
        scale_str = self.__read_txt_file(self.voltage_scale_file)
        if scale_str is None:
            return 1.0
        try:
            return float(scale_str)
        except ValueError:
            return 1.0

    def get_value(self):
        """Retrieve ADC measurement in millivolts with debug"""
        raw_str = self.__read_txt_file(self.voltage_raw_file)
        if raw_str is None:
            return None

        try:
            raw_val = int(raw_str)
        except ValueError:
            print(f"[DEBUG] Invalid raw value: {raw_str}")
            return None

        scale_mV = self.__get_scale()
        voltage_mV = raw_val * scale_mV
        return voltage_mV

    def get_name(self):
        return self.voltage_name

    def get_presence(self):
        exists = os.path.isfile(self.voltage_raw_file)
        return exists

    def get_status(self):
        return self.get_presence()
