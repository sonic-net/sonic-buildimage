import re

class CPLD:
    def __init__(self, base_addr):
        self.base_path = '/sys/bus/i2c/devices/' + base_addr + '/'

    def read_register(self, reg):
        self.write_sysfs_file(self.base_path + 'reg_offset', reg)
        return self.read_sysfs_file(self.base_path + 'reg_value')
    
    def write_register(self, reg, value):
        self.write_sysfs_file(self.base_path + 'reg_offset', reg)
        self.write_sysfs_file(self.base_path + 'reg_value', value)
        
    def write_sysfs_file(self, sysfs_file, value):
        with open(sysfs_file, 'w') as fd:
            fd.write(str(hex(value)))

    def read_sysfs_file(self, sysfs_file) -> str:
        with open(sysfs_file, 'r') as fd:
            rv = fd.read()

        match = re.search(r"val:(\S+)", rv)
        if match:
            val = match.group(1)
            return int(val, 16)

        raise IOError("Unable to open %s file !" % sysfs_file)
