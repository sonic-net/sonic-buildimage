#!/usr/bin/env python

import fcntl
import glob
import json
import os
import re
import subprocess
import time
import unicodedata
from sonic_py_common import device_info

bmc_cache = {}
cache = {}
SONIC_CFGGEN_PATH = '/usr/local/bin/sonic-cfggen'
LED_CTRL_LOCK_PATH = '/var/lock/pddf-locks/pddf-api-led.lock'
HWSKU_KEY = 'DEVICE_METADATA.localhost.hwsku'
PLATFORM_KEY = 'DEVICE_METADATA.localhost.platform'

dirname = os.path.dirname(os.path.realpath(__file__))

class PddfApi():
    def __init__(self):
        if not os.path.exists("/usr/share/sonic/platform"):
            self.platform, self.hwsku = device_info.get_platform_and_hwsku()
            os.symlink("/usr/share/sonic/device/"+self.platform, "/usr/share/sonic/platform")

        try:
            with open('/usr/share/sonic/platform/pddf/pddf-device.json') as f:
                self.data = json.load(f)
        except IOError:
            if os.path.exists('/usr/share/sonic/platform'):
                os.unlink("/usr/share/sonic/platform")

        self.data_sysfs_obj = {}
        self.sysfs_obj = {}

        os.makedirs(os.path.dirname(LED_CTRL_LOCK_PATH), exist_ok=True)

    def _acquire_led_ctrl_lock(self):
        self.lock_fd = os.open(LED_CTRL_LOCK_PATH, os.O_CREAT | os.O_RDWR)
        fcntl.flock(self.lock_fd, fcntl.LOCK_EX)

    def _release_led_ctrl_lock(self):
        if hasattr(self, 'lock_fd'):
            fcntl.flock(self.lock_fd, fcntl.LOCK_UN)
            os.close(self.lock_fd)

    #################################################################################################################
    #   GENERIC DEFS
    #################################################################################################################
    def runcmd(self, cmd):
        rc = os.system(cmd)
        if rc != 0:
            print("%s -- command failed" % cmd)
        return rc

    def get_cmd_output(self, cmd):
        result = subprocess.run(['/bin/bash', '-c', cmd], capture_output=True)
        if result.returncode != 0:
            print("%s -- command failed" % cmd)

        return result

    def get_dev_idx(self, dev, ops):
        parent = dev['dev_info']['virt_parent']
        pdev = self.data[parent]

        return pdev['dev_attr']['dev_idx']

    def get_paths(self, target, attr):
        aa = target + attr

        if aa in cache:
            return cache[aa]

        strings = []
        p = re.search(r'\d+$', target)
        if p is None:
            for bb in filter(re.compile(target).search, self.data.keys()):
                paths = self.dev_parse(self.data[bb], {"cmd": "show_attr", "target": bb, "attr": attr})
                if paths:
                    strings.extend(paths)
        else:
            if target in self.data.keys():
                paths = self.dev_parse(self.data[target], {"cmd": "show_attr", "target": target, "attr": attr})
                if paths:
                    strings.extend(paths)

        cache[aa] = strings
        return strings

    def get_path(self, target, attr):
        nodes = self.get_paths(target, attr)
        if nodes:
            if len(nodes)==1:
                return nodes[0]
            # CAREFULL!!! If more than one paths are expected, use get_paths
            else:
                return nodes[0]
        else:
            return None

    def get_device_type(self, key):
        if key not in self.data.keys():
            return None
        return self.data[key]['dev_info']['device_type']

    def get_platform(self):
        return self.data['PLATFORM']

    def get_num_psu_fans(self, dev):
        if dev not in self.data.keys():
            return 0

        if 'num_psu_fans' not in self.data[dev]['dev_attr']:
            return 0

        return self.data[dev]['dev_attr']['num_psu_fans']

    def get_num_psu_thermals(self, dev):
        if dev not in self.data.keys():
            return str(1)

        if 'num_psu_thermals' not in self.data[dev]['dev_attr']:
            return str(1)

        return self.data[dev]['dev_attr']['num_psu_thermals']

    def get_led_path(self):
        return ("pddf/devices/led")

    def get_led_cur_state_path(self):
        return ("pddf/devices/led/cur_state")

    def get_led_color(self):
        color_f = "/sys/kernel/pddf/devices/led/cur_state/color"
        try:
            with open(color_f, 'r') as f:
                color = f.read().strip("\r\n")
        except IOError:
            return ("Error")
        return color

    def get_led_color_devtype(self, key):
        if 'bmc' in self.data[key]:
            return 'bmc'

        attr_list = self.data[key]['i2c']['attr_list']
        for attr in attr_list:
            if 'attr_devtype' in attr:
                return attr['attr_devtype'].strip()
            else:
                return 'cpld'

    def get_led_color_from_gpio(self, led_device_name):
        attr_list = self.data[led_device_name]['i2c']['attr_list']
        attr = attr_list[0]
        if ':' in attr['bits']:
            bits_list = attr['bits'].split(':')
            bits_list.sort(reverse=True)
            max_bit = int(bits_list[0])
        else:
            max_bit = 0
        base_offset = int(attr['swpld_addr_offset'], 16)
        value = 0
        bit = 0
        while bit <= max_bit:
            offset = base_offset + bit
            if 'attr_devname' in attr:
                attr_path = self.get_gpio_attr_path(self.data[attr['attr_devname']], hex(offset))
            else:
                status = "[FAILED] attr_devname is not configured"
                return (status)
            if not os.path.exists(attr_path):
                status = "[FAILED] {} does not exist".format(attr_path)
                return (status)
            cmd = 'cat ' + attr_path
            gpio_value = subprocess.check_output(cmd, shell=True, universal_newlines=True)
            value |= int(gpio_value) << bit
            bit += 1

        for attr in attr_list:
            if int(attr['value'].strip(), 16) == value:
                return (attr['attr_name'])
        return ("off")

    def get_led_color_from_cpld(self, led_device_name):
        self._acquire_led_ctrl_lock()
        try:
            index = self.data[led_device_name]['dev_attr']['index']
            device_name = self.data[led_device_name]['dev_info']['device_name']
            self.create_attr('device_name', device_name,  self.get_led_path())
            self.create_attr('index', index, self.get_led_path())
            self.create_attr('dev_ops', 'get_status',  self.get_led_path())
            return self.get_led_color()
        finally:
            self._release_led_ctrl_lock()

    def get_led_color_from_bmc(self, led_device_name):
        for bmc_attr in self.data[led_device_name]['bmc']['ipmitool']['attr_list']:
            if (self.bmc_get_cmd(bmc_attr) == str(int(bmc_attr['value'], 16))):
                return (bmc_attr['attr_name'])
        return ("off")

    def set_led_color_from_gpio(self, led_device_name, color):
        attr_list = self.data[led_device_name]['i2c']['attr_list']
        for attr in attr_list:
            if attr['attr_name'].strip() == color.strip():
                base_offset = int(attr['swpld_addr_offset'], 16)
                if ':' in attr['bits']:
                    bits_list = attr['bits'].split(':')
                    bits_list.sort(reverse=True)
                    max_bit = int(bits_list[0])
                else:
                    max_bit = 0
                value = int(attr['value'], 16)
                i = 0
                while i <= max_bit:
                    _value = (value >> i) & 1
                    offset = base_offset + i
                    attr_path = self.get_gpio_attr_path(self.data[attr['attr_devname']], hex(offset))
                    i += 1
                    try:
                        cmd = "echo {} > {}".format(_value, attr_path)
                        self.runcmd(cmd)
                    except Exception as e:
                        msg = "Invalid gpio path : " + attr_path
                        return (False, msg)
        return (True, "Success")

    def set_led_color_from_cpld(self, led_device_name, color):
        self._acquire_led_ctrl_lock()
        try:
            index = self.data[led_device_name]['dev_attr']['index']
            device_name = self.data[led_device_name]['dev_info']['device_name']
            self.create_attr('device_name', device_name,  self.get_led_path())
            self.create_attr('index', index, self.get_led_path())
            self.create_attr('color', color, self.get_led_cur_state_path())
            self.create_attr('dev_ops', 'set_status',  self.get_led_path())
            return (True, "Success")
        finally:
            self._release_led_ctrl_lock()

    def get_system_led_color(self, led_device_name):
        if led_device_name not in self.data.keys():
            msg = led_device_name + " is not configured"
            return (False, msg)

        dtype = self.get_led_color_devtype(led_device_name)

        if dtype == 'gpio':
            color = self.get_led_color_from_gpio(led_device_name)
        elif dtype == 'bmc':
            color = self.get_led_color_from_bmc(led_device_name)
        else:
            # This case takes care of CPLD as well as I2CFPGA
            color = self.get_led_color_from_cpld(led_device_name)

        return (True, color)

    def set_system_led_color(self, led_device_name, color):
        # Check if the device is configured
        if led_device_name not in self.data.keys():
            msg = led_device_name + " is not configured"
            return (False, msg)

        # Check for the write permission
        if 'flag' in self.data[led_device_name]['dev_attr']:
            if self.data[led_device_name]['dev_attr']['flag'] == 'ro':
                return (False, "Set LED operation not supported or handled separately")

        found = False
        for attr in self.data[led_device_name]['i2c']['attr_list']:
            if attr['attr_name'] == color:
                found = True
        if not found:
            return (False, "Invalid color")

        dtype = self.get_led_color_devtype(led_device_name)

        if dtype == 'gpio':
            return (self.set_led_color_from_gpio(led_device_name, color))
        else:
            return (self.set_led_color_from_cpld(led_device_name, color))

    ###################################################################################################################
    #   SHOW ATTRIBIUTES DEFS
    ###################################################################################################################
    def is_led_device_configured(self, device_name, attr_name):
        if device_name in self.data.keys():
            attr_list = self.data[device_name]['i2c']['attr_list']
            for attr in attr_list:
                if attr['attr_name'].strip() == attr_name.strip():
                    return (True)
        return (False)

    def show_device_sysfs(self, dev, ops):
        parent = dev['dev_info']['device_parent']
        pdev = self.data[parent]
        if pdev['dev_info']['device_parent'] == 'SYSTEM':
            if 'topo_info' in pdev['i2c']:
                return "/sys/bus/i2c/devices/"+"i2c-%d"%int(pdev['i2c']['topo_info']['dev_addr'], 0)
            else:
                return "/sys/bus/i2c/devices"
        if 'topo_info' in dev['i2c'] and 'parent_bus' in dev['i2c']['topo_info']:
            return self.show_device_sysfs(pdev, ops) + "/" + "i2c-%d" % int(dev['i2c']['topo_info']['parent_bus'], 0)
        else:
            return self.show_device_sysfs(pdev, ops)

    def get_gpio_attr_path(self, dev, offset):
        base = int(dev['i2c']['dev_attr']['gpio_base'], 16)
        port_num = base + int(offset, 16)
        gpio_name = 'gpio'+str(port_num)
        path = '/sys/class/gpio/'+gpio_name+'/value'
        return path

    # This is alid for 'at24' type of EEPROM devices. Only one attribtue 'eeprom'
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

    def show_attr_gpio_device(self, dev, ops):
        ret = []
        KEY = "gpio"
        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        return ret

    def show_attr_mux_device(self, dev, ops):
        ret = []
        KEY = "mux"
        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        return ret

    def show_attr_hwmon_device(self, dev, ops, data_sysfs_key):
        def _path_expand(*path):
            full_path = glob.glob(os.path.join(*path))
            if not full_path:
                return None
            return full_path[0]

        ret = []
        if 'i2c' not in dev.keys():
            return ret
        attr_name = ops['attr']
        attr_list = dev['i2c']['attr_list'] if 'i2c' in dev else []
        KEY = data_sysfs_key
        dsysfs_path = ""

        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        # Current/Voltage sensors are oftentimes rails that are part of a DPM/DCDC
        if "virt_parent" in dev['dev_info']:
            i2c_dev = self.data[dev['dev_info']['virt_parent']]
        else:
            i2c_dev = dev

        for attr in attr_list:
            if attr_name == attr['attr_name'] or attr_name == 'all':
                if 'drv_attr_name' in attr.keys():
                    real_name = attr['drv_attr_name']
                else:
                    real_name = attr['attr_name']

                if 'topo_info' in i2c_dev['i2c']:
                    path = self.show_device_sysfs(i2c_dev, ops)+"/%d-00%02x/"%(int(i2c_dev['i2c']['topo_info']['parent_bus'], 0),
                            int(i2c_dev['i2c']['topo_info']['dev_addr'], 0))
                    full_path = _path_expand(path, 'hwmon', 'hwmon*', real_name)
                elif 'path_info' in i2c_dev['i2c']:
                    path = i2c_dev['i2c']['path_info']['sysfs_base_path']
                    full_path = _path_expand(path, real_name)

                if full_path is None:
                    return []

                dsysfs_path = full_path
                if dsysfs_path not in self.data_sysfs_obj[KEY]:
                    self.data_sysfs_obj[KEY].append(dsysfs_path)
                ret.append(full_path)

        return ret

    def show_attr_voltage_sensor_device(self, dev, ops):
        return self.show_attr_hwmon_device(dev, ops, "voltage-sensors")

    def show_attr_current_sensor_device(self, dev, ops):
        return self.show_attr_hwmon_device(dev, ops, "current-sensors")

    def show_attr_temp_sensor_device(self, dev, ops):
        return self.show_attr_hwmon_device(dev, ops, "temp-sensors")

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

    def show_attr_psu_device(self, dev, ops):
        return self.show_attr_psu_i2c_device(dev, ops)

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

    def show_attr_sysstatus_device(self, dev, ops):
        ret = []
        attr_name = ops['attr']
        attr_list = dev['attr_list']
        KEY = "sys-status"
        dsysfs_path = ""

        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        for attr in attr_list:
            if attr_name == attr['attr_name'] or attr_name == 'all':
                dsysfs_path = "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/" + attr['attr_name']
                if dsysfs_path not in self.data_sysfs_obj[KEY]:
                    self.data_sysfs_obj[KEY].append(dsysfs_path)
                ret.append(dsysfs_path)
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

    def show_attr_xcvr_device(self, dev, ops):
        return self.show_attr_xcvr_i2c_device(dev, ops)

    def show_attr_cpld_device(self, dev, ops):
        ret = []
        KEY = "cpld"
        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        return ret

    def show_attr_multifpgapci_device(self, dev, ops):
        ret = []
        KEY="multifpgapci"
        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        return ret

    ###################################################################################################################
    #  SPYTEST
    ###################################################################################################################
    def verify_attr(self, key, attr, path):
        node = "/sys/kernel/%s/%s" % (path, key)
        try:
            with open(node, 'r') as f:
                status = f.read()
        except IOError:
            print("PDDF_VERIFY_ERR: IOError: node:%s key:%s" % (node, key))
            return

        status = status.rstrip("\n\r")
        if attr[key] != status:
            print("PDDF_VERIFY_ERR: node: %s switch:%s" % (node, status))

    def verify_device(self, attr, path, ops):
        for key in attr.keys():
            self.verify_attr(key, attr, path)

    def get_led_device(self, device_name):
        self._acquire_led_ctrl_lock()
        try:
            self.create_attr('device_name', self.data[device_name]['dev_info']['device_name'], "pddf/devices/led")
            self.create_attr('index', self.data[device_name]['dev_attr']['index'], "pddf/devices/led")
            cmd = "echo 'verify'  > /sys/kernel/pddf/devices/led/dev_ops"
            self.runcmd(cmd)
        finally:
            self._release_led_ctrl_lock()

    def validate_sysfs_creation(self, obj, validate_type):
        dir = '/sys/kernel/pddf/devices/'+validate_type
        if (os.path.exists(dir) or validate_type == 'client'):
            for sysfs in obj[validate_type]:
                if not os.path.exists(sysfs):
                    print("[SYSFS FILE] " + sysfs + ": does not exist")
        else:
            print("[SYSFS DIR] " + dir + ": does not exist")

    def validate_dsysfs_creation(self, obj, validate_type):
        if validate_type in obj.keys():
            # There is a possibility that some components dont have any device-self.data attr
            if not obj[validate_type]:
                print("[SYSFS ATTR] for " + validate_type + ": empty")
            else:
                for sysfs in obj[validate_type]:
                    if not os.path.exists(sysfs):
                        print("[SYSFS FILE] " + sysfs + ": does not exist")
        else:
            print("[SYSFS DIR] " + dir + ": not configured")

    def verify_sysfs_data(self, verify_type):
        if (verify_type == 'LED'):
            for key in self.data.keys():
                if key != 'PLATFORM':
                    attr = self.data[key]['dev_info']
                    if attr['device_type'] == 'LED':
                        self.get_led_device(key)
                        self.verify_attr('device_name', self.data[key]['dev_info'], "pddf/devices/led")
                        self.verify_attr('index', self.data[key]['dev_attr'], "pddf/devices/led")
                        for attr in self.data[key]['i2c']['attr_list']:
                            path = "pddf/devices/led/" + attr['attr_name']
                            for entry in attr.keys():
                                if (entry != 'attr_name' and entry != 'swpld_addr' and entry != 'swpld_addr_offset'):
                                    self.verify_attr(entry, attr, path)
                                if (entry == 'swpld_addr' or entry == 'swpld_addr_offset'):
                                    self.verify_attr(entry, attr, 'pddf/devices/led')

    def modules_validation(self, validate_type):
        kos = []
        supported_type = False
        module_validation_status = []

        if validate_type == "bmc":
            kos = ['ipmi_devintf', 'ipmi_si', 'ipmi_msghandler']
            validate_type = 'ipmi'
        else:
            # generate the KOS list from pddf device JSON file
            kos.extend(self.data['PLATFORM']['pddf_kos'])

            if 'custom_kos' in self.data['PLATFORM']:
                kos.extend(self.data['PLATFORM']['custom_kos'])

        for mod in kos:
            if validate_type in mod or validate_type == "pddf":
                supported_type = True
                cmd = "lsmod | grep " + mod
                try:
                    subprocess.check_output(cmd, shell=True, universal_newlines=True)
                except Exception as e:
                    module_validation_status.append(mod)
        if supported_type:
            if module_validation_status:
                module_validation_status.append(":ERROR not loaded")
                print(str(module_validation_status)[1:-1])
            else:
                print("Loaded")
        else:
            print(validate_type + " not configured")

    ###################################################################################################################
    #   PARSE DEFS
    ###################################################################################################################

    def psu_parse(self, dev, ops):
        ret = []
        for ifce in (dev['i2c']['interface'] if 'i2c' in dev else []):
            val = getattr(self, ops['cmd']+"_psu_device")(self.data[ifce['dev']], ops)
            if val:
                if str(val[0]).isdigit():
                    if val[0] != 0:
                        # in case if 'create' functions
                        print("{}_psu_device failed for {}".format(ops['cmd'], ifce['dev']))
                        return val
                else:
                    # in case of 'show_attr' functions
                    ret.extend(val)
        return ret

    def fan_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_fan_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_fan_device failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

        return ret

    def temp_sensor_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_temp_sensor_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_temp_sensor_device failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

        return ret

    def voltage_sensor_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_voltage_sensor_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_voltage_sensor_device failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

        return ret

    def current_sensor_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_current_sensor_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_current_sensor_device failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

        return ret

    def cpld_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_cpld_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_cpld_device failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

        return ret

    def sysstatus_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_sysstatus_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_sysstatus_device failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

        return ret

    def gpio_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_gpio_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_gpio_device failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

        return ret

    def mux_parse(self, dev, ops):
        val = []
        ret = getattr(self, ops['cmd']+"_mux_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_mux_device() cmd failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))
                    return ret
            else:
                val.extend(ret)

        for ch in dev['i2c']['channel']:
            ret = self.dev_parse(self.data[ch['dev']], ops)
            if ret:
                if str(ret[0]).isdigit():
                    if ret[0] != 0:
                        # in case if 'create' functions
                        return ret
                else:
                    val.extend(ret)
        return val

    def mux_parse_reverse(self, dev, ops):
        val = []
        for ch in reversed(dev['i2c']['channel']):
            ret = self.dev_parse(self.data[ch['dev']], ops)
            if ret:
                if str(ret[0]).isdigit():
                    if ret[0] != 0:
                        # in case if 'create' functions
                        return ret
                else:
                    val.extend(ret)

        ret = getattr(self, ops['cmd']+"_mux_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_mux_device() cmd failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))
                    return ret
            else:
                val.extend(ret)

        return val

    def eeprom_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_eeprom_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_eeprom_device() cmd failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

        return ret

    def optic_parse(self, dev, ops):
        val = []
        for ifce in dev['i2c']['interface']:
            ret = getattr(self, ops['cmd']+"_xcvr_device")(self.data[ifce['dev']], ops)
            if ret:
                if str(ret[0]).isdigit():
                    if ret[0] != 0:
                        # in case if 'create' functions
                        print("{}_xcvr_device() cmd failed for {}".format(ops['cmd'], ifce['dev']))
                        return ret
                else:
                    val.extend(ret)
        return val

    def cpu_parse(self, bus, ops):
        val = []
        for dev in bus['i2c']['CONTROLLERS']:
            dev1 = self.data[dev['dev']]
            for d in dev1['i2c']['DEVICES']:
                ret = self.dev_parse(self.data[d['dev']], ops)
                if ret:
                    if str(ret[0]).isdigit():
                        if ret[0] != 0:
                            # in case if 'create' functions
                            return ret
                    else:
                        val.extend(ret)
        return val

    def cpu_parse_reverse(self, bus, ops):
        val = []
        for dev in reversed(bus['i2c']['CONTROLLERS']):
            dev1 = self.data[dev['dev']]
            for d in dev1['i2c']['DEVICES']:
                ret = self.dev_parse(self.data[d['dev']], ops)
                if ret:
                    if str(ret[0]).isdigit():
                        if ret[0] != 0:
                            # in case if 'create' functions
                            return ret
                    else:
                        val.extend(ret)
        return val

    def multifpgapci_parse(self, dev, ops):
        val = []
        ret = getattr(self, ops['cmd']+"_multifpgapci_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_multifpgapci_device() cmd failed".format(ops['cmd']))
                    return ret
            else:
                val.extend(ret)

        for bus in dev['i2c']['channel']:
            ret = self.dev_parse(self.data[bus['dev']], ops)
            if ret:
                 if str(ret[0]).isdigit():
                      if ret[0] != 0:
                            # in case if 'create' functions
                           return ret
                 else:
                      val.extend(ret)
        return val

    def dev_parse(self, dev, ops):
        attr = dev['dev_info']
        if attr['device_type'] == 'CPU':
            if ops['cmd'] == 'delete':
                return self.cpu_parse_reverse(dev, ops)
            else:
                return self.cpu_parse(dev, ops)

        if attr['device_type'] == 'EEPROM':
            return self.eeprom_parse(dev, ops)

        if attr['device_type'] == 'MUX':
            if ops['cmd'] == 'delete':
                return self.mux_parse_reverse(dev, ops)
            else:
                return self.mux_parse(dev, ops)

        if attr['device_type'] == 'GPIO':
            return self.gpio_parse(dev, ops)

        if attr['device_type'] == 'PSU':
            return self.psu_parse(dev, ops)

        if attr['device_type'] == 'FAN':
            return self.fan_parse(dev, ops)

        if attr['device_type'] == 'TEMP_SENSOR':
            return self.temp_sensor_parse(dev, ops)

        if attr['device_type'] == 'VOLTAGE_SENSOR':
            return self.voltage_sensor_parse(dev, ops)

        if attr['device_type'] == 'CURRENT_SENSOR':
            return self.current_sensor_parse(dev, ops)

        if attr['device_type'] in ['SFP', 'SFP+', 'SFP28', 'QSFP', 'QSFP+', 'QSFP28', 'QSFP-DD', 'OSFP']:
            return self.optic_parse(dev, ops)

        if attr['device_type'] == 'CPLD':
            return self.cpld_parse(dev, ops)

        if attr['device_type'] == 'SYSSTAT':
            return self.sysstatus_parse(dev, ops)

        if attr['device_type'] == 'MULTIFPGAPCIE':
            return self.multifpgapci_parse(dev, ops)

    def create_attr(self, key, value, path):
        cmd = "echo '%s' > /sys/kernel/%s/%s" % (value,  path, key)
        self.runcmd(cmd)

    def led_parse(self, ops):
        getattr(self, ops['cmd']+"_led_platform_device")("PLATFORM", ops)
        for key in self.data.keys():
            if key != 'PLATFORM' and 'dev_info' in self.data[key]:
                attr = self.data[key]['dev_info']
                if attr['device_type'] == 'LED':
                    getattr(self, ops['cmd']+"_led_device")(key, ops)

    def get_device_list(self, list, type):
        for key in self.data.keys():
            if key != 'PLATFORM' and 'dev_info' in self.data[key]:
                attr = self.data[key]['dev_info']
                if attr['device_type'] == type:
                    list.append(self.data[key])

    def cli_dump_dsysfs(self, component):
        self.dev_parse(self.data['SYSTEM'], {"cmd": "show_attr", "target": "all", "attr": "all"})
        if 'SYSSTATUS' in self.data:
            self.dev_parse(self.data['SYSSTATUS'], {"cmd": "show_attr", "target": "all", "attr": "all"})
        if component in self.data_sysfs_obj:
            return self.data_sysfs_obj[component]
        else:
            return None

    ###################################################################################################################
    #   BMC APIs
    ###################################################################################################################
    def populate_bmc_cache_db(self, bmc_attr):
        bmc_cmd = str(bmc_attr['bmc_cmd']).strip()

        sdr_dump_file = "/usr/local/sdr_dump"
        __bmc_cmd = bmc_cmd
        if 'ipmitool' in bmc_cmd:
            if not os.path.isfile(sdr_dump_file):
                sdr_dump_cmd = "ipmitool sdr dump " + sdr_dump_file
                subprocess.check_output(sdr_dump_cmd, shell=True, universal_newlines=True)
            dump_cmd = "ipmitool -S " + sdr_dump_file
            __bmc_cmd = __bmc_cmd.replace("ipmitool", dump_cmd, 1)
        o_list = subprocess.check_output(__bmc_cmd, shell=True, universal_newlines=True).strip().split('\n')
        bmc_cache[bmc_cmd]={}
        bmc_cache[bmc_cmd]['time']=time.time()
        for entry in o_list:
            if 'separator' in bmc_attr.keys():
                name = str(entry.split(bmc_attr['separator'])[0]).strip()
            else:
                name = str(entry.split()[0]).strip()

            bmc_cache[bmc_cmd][name]=entry

    def non_raw_ipmi_get_request(self, bmc_attr):
        bmc_db_update_time = 1
        value = 'N/A'
        bmc_cmd = str(bmc_attr['bmc_cmd']).strip()
        field_name = str(bmc_attr['field_name']).strip()
        field_pos = int(bmc_attr['field_pos'])-1

        if bmc_cmd not in bmc_cache:
            self.populate_bmc_cache_db(bmc_attr)
        else:
            now = time.time()
            if (int(now - bmc_cache[bmc_cmd]['time']) > bmc_db_update_time):
                self.populate_bmc_cache_db(bmc_attr)

        try:
            data=bmc_cache[bmc_cmd][field_name]
            if 'separator' in bmc_attr:
                value = data.split(bmc_attr['separator'])[field_pos].strip()
            else:
                value = data.split()[field_pos].strip()
        except Exception as e:
           pass

        if 'mult' in bmc_attr.keys():
            if value.replace('.','',1).strip().isdigit():
                value = float(value) * float(bmc_attr['mult'])
            else:
                value = 0.0
        return str(value)

    def raw_ipmi_get_request(self, bmc_attr):
        value = 'N/A'
        cmd = bmc_attr['bmc_cmd'] + " 2>/dev/null"
        if bmc_attr['type'] == 'raw':
            try:
                value = subprocess.check_output(cmd, shell=True, universal_newlines=True).strip()
            except Exception as e:
                pass

            if value != 'N/A':
                value = str(int(value, 16))
            return value

        if bmc_attr['type'] == 'mask':
            mask = int(bmc_attr['mask'].encode('utf-8'), 16)
            try:
                value = subprocess.check_output(cmd, shell=True, universal_newlines=True).strip()
            except Exception as e:
                pass

            if value != 'N/A':
                value = str(int(value, 16) & mask)

            return value

        if bmc_attr['type'] == 'ascii':
            try:
                value = subprocess.check_output(cmd, shell=True, universal_newlines=True).strip()
            except Exception as e:
                pass

            if value != 'N/A':
                tmp = ''.join(chr(int(i, 16)) for i in value.split())
                tmp = "".join(i for i in str(tmp) if unicodedata.category(i)[0] != "C")
                value = str(tmp)

            return (value)

        return value

    def bmc_get_cmd(self, bmc_attr):
        if int(bmc_attr['raw']) == 1:
            value = self.raw_ipmi_get_request(bmc_attr)
        else:
            value = self.non_raw_ipmi_get_request(bmc_attr)
        return (value)

    def non_raw_ipmi_set_request(self, bmc_attr, val):
        value = 'N/A'
        # TODO: Implement it
        return value

    def raw_ipmi_set_request(self, bmc_attr, val):
        value = 'N/A'
        # TODO: Implement this
        return value

    def bmc_set_cmd(self, bmc_attr, val):
        if int(bmc_attr['raw']) == 1:
            value = self.raw_ipmi_set_request(bmc_attr, val)
        else:
            value = self.non_raw_ipmi_set_request(bmc_attr, val)
        return (value)

    # bmc-based attr: return attr obj
    # non-bmc-based attr: return empty obj
    def check_bmc_based_attr(self, device_name, attr_name):
        if device_name in self.data.keys():
            if "bmc" in self.data[device_name].keys() and 'ipmitool' in self.data[device_name]['bmc'].keys():
                attr_list = self.data[device_name]['bmc']['ipmitool']['attr_list']
                for attr in attr_list:
                    if attr['attr_name'].strip() == attr_name.strip():
                        return attr
                # Required attr_name is not supported in BMC object
                return {}
        return None

    def get_attr_name_output(self, device_name, attr_name):
        bmc_attr = self.check_bmc_based_attr(device_name, attr_name)
        output = {"mode": "", "status": ""}

        if bmc_attr is not None and bmc_attr!={}:
            output['mode']="bmc"
            output['status']=self.bmc_get_cmd(bmc_attr)
        else:
            # bmc_attr is either None or {}. In both the cases, its highly likely that the attribute
            # is i2c based
            output['mode']="i2c"
            node = self.get_path(device_name, attr_name)
            if node is None:
                return {}
            try:
                # Seen some errors in case of unencodable characters hence ignoring them in python3
                with open(node, 'r', errors='ignore') as f:
                    output['status'] = f.read()
            except IOError:
                return {}
        return output

    def set_attr_name_output(self, device_name, attr_name, val):
        bmc_attr = self.check_bmc_based_attr(device_name, attr_name)
        output = {"mode": "", "status": ""}

        if bmc_attr is not None:
            if bmc_attr == {}:
                return {}
            output['mode'] = "bmc"
            output['status'] = False  # No set operation allowed for BMC attributes as they are handled by BMC itself
        else:
            output['mode'] = "i2c"
            node = self.get_path(device_name, attr_name)
            if node is None:
                return {}
            try:
                with open(node, 'w') as f:
                    f.write(str(val))
            except IOError:
                return {}

            output['status'] = True

        return output
