#!/usr/bin/env python
"""
Version: 1.0

Module contains an implementation of SONiC Platform Base API and
provides the 'Sensor' information which are available in the platform
"""

try:
    from sonic_platform_base.sensor_byted import SensorByted
    from vendor_sonic_platform.devcfg import Devcfg
    from collections import namedtuple
    import struct
except ImportError as error:
    raise ImportError(str(error) + "- required module not found")

SENSORS = ['ADM1166_0', 'ADM1166_1']


class Sensor(SensorByted):
    """Platform-specific Sensor class"""
    fault_mask = namedtuple('fault_mask', ['ch', 'name', 'uv', 'ov'])

    def __init__(self, index, parent=None):
        super(Sensor, self).__init__(index=index, parent=parent, name=SENSORS[index])
        self.sensor_dir = Devcfg.DEBUG_SENSOR_DIR + self.name.lower() + '/'

    def chip_rails_init(self):
        """
            Retrieves the sensor chip rails thresholds

            Returns:
            A list
        """
        chip_dict = {sensor: list() for sensor in SENSORS}
        for rail in Devcfg.sensor_chip_dict[self.name]:
            chip_dict[self.name].append(self.rail(*rail))
        chip_dict[self.name].sort(key=lambda x: x[0])
        return chip_dict[self.name]

    def get_rail_value(self, ch):
        """
        Retrieves current rail value of the sensor

        Returns:
            A float number of current rail value, round(value, 3)
        """
        vol_path = self.sensor_dir + Devcfg.adm1166_attr_sawp[self.name].get(self.rails_config[ch].name)
        vol_value = self.read_attr(vol_path)
        if vol_value == 'N/A':
            self.log_error("get {} vol failed".format(self.name))
            return float(0)
        return round(float(vol_value) / 1000, 3)

    def get_fault_records(self):
        """
        Retrieves the fault records of the sensor

        Returns:
            A list of fault records
            fault_records = namedtuple('records', ['ch', 'name', 'raw', 'info'])
        """
        fault_default_mask_list = [
            self.fault_mask(0, 'VP1', 1, 1 << 10),
            self.fault_mask(1, 'VP2', 1 << 1, 1 << 11),
            self.fault_mask(2, 'VP3', 1 << 2, 1 << 12),
            self.fault_mask(3, 'VP4', 1 << 3, 1 << 13),
            self.fault_mask(4, 'VH', 1 << 4, 1 << 14),
            self.fault_mask(5, 'VX1', 1 << 5, 1 << 15),
            self.fault_mask(6, 'VX2', 1 << 6, 1 << 16),
            self.fault_mask(7, 'VX3', 1 << 7, 1 << 17),
            self.fault_mask(8, 'VX4', 1 << 8, 1 << 18),
            self.fault_mask(9, 'VX5', 1 << 9, 1 << 19),
        ]
        fault_mask_list = []
        for channel_name, sysfs_name in Devcfg.adm1166_attr_sawp[self.name].items():
            for mask in fault_default_mask_list:
                if sysfs_name == mask.name:
                    fault_mask_list.append(mask._replace(name=channel_name))
                    break
        fault_path = self.sensor_dir + 'fault_record'
        fault_records = list()
        records_raw = []
        record_len = 8
        record_num = 16
        try:
            with open(fault_path, 'wb+') as fp:
                records_raw = fp.read(record_len * record_num)
                if all(value == '\xff' for value in set(records_raw)):
                    return []

                # clean records
                fp.seek(0)
                fp.write('0')

            for i in xrange(0, record_len * record_num, record_len):
                raw = records_raw[i:i + record_len]
                if all(value == '\xff' for value in set(raw)):
                    break
                raw_hex_list = ['0x{:02x}'.format(ord(r)) for r in raw]
                int_fault_record, = struct.unpack('I', raw[2:6])
                fault_info = ''
                for mask in fault_mask_list:
                    if int_fault_record & (mask.uv | mask.ov):
                        fault_info = 'VOUT_UV' if int_fault_record & mask.uv else 'VOUT_OV'
                        fault_records.append(self.records(mask.ch, mask.name, raw_hex_list, fault_info))
                if not fault_info:
                    fault_records.append(self.records(self.index, 'N/A', raw_hex_list, 'N/A'))

        except Exception as error:
            self.log_error("get fault record failed {}".format(str(error)))
            return []
        return fault_records
