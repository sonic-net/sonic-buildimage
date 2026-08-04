#############################################################################
# PDDF
# Module contains an implementation of SONiC pddfapi Base API and
# provides the pddfpai information
#
#############################################################################


try:
    from sonic_platform_pddf_base.pddfapi import PddfApi
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class newapi(PddfApi):
    """
    PDDF Platform-Specific pddfapi Class
    """

    def __init__(self):
        PddfApi.__init__(self)


    def show_attr_eeprom_device(self, dev, ops):
        ret = []
        attr_name = ops['attr']
        attr_list = dev['i2c']['attr_list']
        KEY = "eeprom"
        dsysfs_path = ""
    
        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []
    
        for attr in attr_list:
            if attr_name == attr['attr_name'] or attr_name == 'all':
                if 'drv_attr_name' in attr.keys():
                    real_name = attr['drv_attr_name']
                else:
                    real_name = attr['attr_name']
    
                dsysfs_path = self.show_device_sysfs(dev, ops) + \
                    "/%d-00%02x" % (int(dev['i2c']['topo_info']['parent_bus'], 0),
                                  int(dev['i2c']['topo_info']['dev_addr'], 0)) + \
                    "/%s" % real_name
                if dsysfs_path not in self.data_sysfs_obj[KEY]:
                    self.data_sysfs_obj[KEY].append(dsysfs_path)
                ret.append(dsysfs_path)
        return ret

    def show_attr_psu_i2c_device(self, dev, ops):
        target = ops['target']
        attr_name = ops['attr']
        ret = []
        KEY = "psu"
        dsysfs_path = ""

        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        if target == 'all' or target == dev['dev_info']['virt_parent']:
            attr_list = dev['i2c']['attr_list'] if 'i2c' in dev else []
            for attr in attr_list:
                if attr_name == attr['attr_name'] or attr_name == 'all':
                    if 'attr_devtype' in attr.keys() and attr['attr_devtype'] == "gpio":
                        # Check and enable the gpio from class
                        attr_path = self.get_gpio_attr_path(self.data[attr['attr_devname']], attr['attr_offset'])
                        if (os.path.exists(attr_path)):
                            if attr_path not in self.data_sysfs_obj[KEY]:
                                self.data_sysfs_obj[KEY].append(attr_path)
                            ret.append(attr_path)
                    else:
                        if 'drv_attr_name' in attr.keys():
                            real_name = attr['drv_attr_name']
                            real_dev = dev
                        elif 'attr_devattr' in attr.keys():
                            real_name = attr['attr_devattr']
                            real_devname = attr['attr_devname'] if 'attr_devname' in attr.keys() else ''
                            real_dev = self.data[real_devname]
                        else:
                            real_name = attr['attr_name']
                            real_dev = dev

                        dsysfs_path = self.show_device_sysfs(real_dev, ops) + \
                            "/%d-00%02x" % (int(real_dev['i2c']['topo_info']['parent_bus'], 0),
                                          int(real_dev['i2c']['topo_info']['dev_addr'], 0)) + \
                            "/%s" % real_name
                        if dsysfs_path not in self.data_sysfs_obj[KEY]:
                            self.data_sysfs_obj[KEY].append(dsysfs_path)
                            ret.append(dsysfs_path)
        return ret


    def show_attr_fan_device(self, dev, ops):
        ret = []
        attr_name = ops['attr']
        attr_list = dev['i2c']['attr_list'] if 'i2c' in dev else []
        KEY = "fan"
        dsysfs_path = ""

        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        for attr in attr_list:
            if attr_name == attr['attr_name'] or attr_name == 'all':
                if 'attr_devtype' in attr.keys() and attr['attr_devtype'] == "gpio":
                    # Check and enable the gpio from class
                    attr_path = self.get_gpio_attr_path(self.data[attr['attr_devname']], attr['attr_offset'])
                    if (os.path.exists(attr_path)):
                        if attr_path not in self.data_sysfs_obj[KEY]:
                            self.data_sysfs_obj[KEY].append(attr_path)
                        ret.append(attr_path)
                else:
                    if 'drv_attr_name' in attr.keys():
                        real_name = attr['drv_attr_name']
                        real_dev = dev
                    elif 'attr_devattr' in attr.keys():
                        real_name = attr['attr_devattr']
                        real_devname = attr['attr_devname'] if 'attr_devname' in attr.keys() else ''
                        real_dev = self.data[real_devname]
                    else:
                        real_name = attr['attr_name']
                        real_dev = dev

                    dsysfs_path = self.show_device_sysfs(real_dev, ops) + \
                        "/%d-00%02x" % (int(real_dev['i2c']['topo_info']['parent_bus'], 0),
                                      int(real_dev['i2c']['topo_info']['dev_addr'], 0)) + \
                        "/%s" % real_name
                    if dsysfs_path not in self.data_sysfs_obj[KEY]:
                        self.data_sysfs_obj[KEY].append(dsysfs_path)
                    ret.append(dsysfs_path)
        return ret

    # This is only valid for LM75
    def show_attr_temp_sensor_device(self, dev, ops):
        ret = []
        if 'i2c' not in dev.keys():
            return ret
        attr_name = ops['attr']
        attr_list = dev['i2c']['attr_list'] if 'i2c' in dev else []
        KEY = "temp-sensors"
        dsysfs_path = ""

        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        for attr in attr_list:
            if attr_name == attr['attr_name'] or attr_name == 'all':
                path = self.show_device_sysfs(dev, ops)+"/%d-00%02x/" % (int(dev['i2c']['topo_info']['parent_bus'], 0),
                                                                       int(dev['i2c']['topo_info']['dev_addr'], 0))
                if 'drv_attr_name' in attr.keys():
                    real_name = attr['drv_attr_name']
                else:
                    real_name = attr['attr_name']

                if (os.path.exists(path)):
                    full_path = glob.glob(path + 'hwmon/hwmon*/' + real_name)[0]
                    dsysfs_path = full_path
                    if dsysfs_path not in self.data_sysfs_obj[KEY]:
                        self.data_sysfs_obj[KEY].append(dsysfs_path)
                    ret.append(full_path)
        return ret

    def show_attr_xcvr_i2c_device(self, dev, ops):
        target = ops['target']
        attr_name = ops['attr']
        ret = []
        dsysfs_path = ""
        KEY = "xcvr"
        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        if target == 'all' or target == dev['dev_info']['virt_parent']:
            attr_list = dev['i2c']['attr_list']
            for attr in attr_list:
                if attr_name == attr['attr_name'] or attr_name == 'all':
                    if 'attr_devtype' in attr.keys() and attr['attr_devtype'] == "gpio":
                        # Check and enable the gpio from class
                        attr_path = self.get_gpio_attr_path(self.data[attr['attr_devname']], attr['attr_offset'])
                        if (os.path.exists(attr_path)):
                            if attr_path not in self.data_sysfs_obj[KEY]:
                                self.data_sysfs_obj[KEY].append(attr_path)
                            ret.append(attr_path)
                    else:
                        if 'drv_attr_name' in attr.keys():
                            real_name = attr['drv_attr_name']
                            real_dev = dev
                        elif 'attr_devattr' in attr.keys():
                            real_name = attr['attr_devattr']
                            real_devname = attr['attr_devname'] if 'attr_devname' in attr.keys() else ''
                            real_dev = self.data[real_devname]
                        else:
                            real_name = attr['attr_name']
                            real_dev = dev

                        dsysfs_path = self.show_device_sysfs(real_dev, ops) + \
                            "/%d-00%02x" % (int(real_dev['i2c']['topo_info']['parent_bus'], 0),
                                          int(real_dev['i2c']['topo_info']['dev_addr'], 0)) + \
                            "/%s" % real_name
                        if dsysfs_path not in self.data_sysfs_obj[KEY]:
                            self.data_sysfs_obj[KEY].append(dsysfs_path)
                            ret.append(dsysfs_path)
        return ret

