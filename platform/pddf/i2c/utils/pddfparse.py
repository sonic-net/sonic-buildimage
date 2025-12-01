#!/usr/bin/env python
import argparse
import glob
import json
import os
import re
import subprocess
import sys
import time
import unicodedata
from sonic_py_common import device_info

bmc_cache = {}
cache = {}
SONIC_CFGGEN_PATH = '/usr/local/bin/sonic-cfggen'
HWSKU_KEY = 'DEVICE_METADATA.localhost.hwsku'
PLATFORM_KEY = 'DEVICE_METADATA.localhost.platform'

dirname = os.path.dirname(os.path.realpath(__file__))

class PddfParse():
    def __init__(self):
        if not os.path.exists("/usr/share/sonic/platform"):
            platform, hwsku = device_info.get_platform_and_hwsku()
            os.symlink("/usr/share/sonic/device/"+platform, "/usr/share/sonic/platform")

        try:
            with open('/usr/share/sonic/platform/pddf/pddf-device.json') as f:
                self.data = json.load(f)
        except IOError:
            if os.path.exists('/usr/share/sonic/platform'):
                os.unlink("/usr/share/sonic/platform")
            raise Exception('PDDF JSON file not found. PDDF is not supported on this platform')

        self.data_sysfs_obj = {}
        self.sysfs_obj = {}


    ###################################################################################################################
    #   GENERIC DEFS
    ###################################################################################################################
    def runcmd(self, cmd):
        rc = os.system(cmd)
        if rc != 0:
            print("%s -- command failed" % cmd)
        return rc

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

        return (color)

    ###################################################################################################################
    #   CREATE DEFS
    ###################################################################################################################
    def create_device(self, attr, path, ops):
        ret = 0
        for key in attr.keys():
            if type(attr[key]) is list:
                val = " ".join(attr[key])
            else:
                val = attr[key]

            cmd = "echo '%s' > /sys/kernel/%s/%s" % (val, path, key)
            ret = self.runcmd(cmd)
            if ret != 0:
                return ret
        return ret


    def create_psu_i2c_device(self, dev, ops):
        create_ret = []
        ret = 0
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['PSU']:
            ret = self.create_device(dev['i2c']['topo_info'], "pddf/devices/psu/i2c", ops)
            if ret != 0:
                return create_ret.append(ret)
            cmd = "echo '%s' > /sys/kernel/pddf/devices/psu/i2c/i2c_name" % (dev['dev_info']['device_name'])
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
            cmd = "echo '%s'  > /sys/kernel/pddf/devices/psu/i2c/psu_idx" % (self.get_dev_idx(dev, ops))
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
            cmd = "echo '%s' > /sys/kernel/pddf/devices/psu/i2c/psu_thermals" % (self.get_num_psu_thermals(dev['dev_info']['virt_parent']))
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
            cmd = "echo '%s' > /sys/kernel/pddf/devices/psu/i2c/psu_temp_high_thresh_bitmap" % (self.get_psu_temp_high_thresh_bitmap(dev['dev_info']['device_name'],
                                                                                                    int(self.get_num_psu_thermals(dev['dev_info']['virt_parent']))))
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
            for attr in dev['i2c']['attr_list']:
                ret = self.create_device(attr, "pddf/devices/psu/i2c", ops)
                if ret != 0:
                    return create_ret.append(ret)
                cmd = "echo 'add' > /sys/kernel/pddf/devices/psu/i2c/attr_ops"
                ret = self.runcmd(cmd)
                if ret != 0:
                    return create_ret.append(ret)

            cmd = "echo 'add' > /sys/kernel/pddf/devices/psu/i2c/dev_ops"
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
        else:
            cmd = "echo %s 0x%x > /sys/bus/i2c/devices/i2c-%d/new_device" % (dev['i2c']['topo_info']['dev_type'],
                    int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)

        return create_ret.append(ret)

    def create_psu_bmc_device(self, dev, ops):
        return [0]

    def create_psu_device(self, dev, ops):
        return self.create_psu_i2c_device(dev, ops)

    def create_fan_device(self, dev, ops):
        create_ret = []
        ret = 0
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['FAN']:
            ret = self.create_device(dev['i2c']['topo_info'], "pddf/devices/fan/i2c", ops)
            if ret != 0:
                return create_ret.append(ret)
            cmd = "echo '%s' > /sys/kernel/pddf/devices/fan/i2c/i2c_name" % (dev['dev_info']['device_name'])
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
            ret = self.create_device(dev['i2c']['dev_attr'], "pddf/devices/fan/i2c", ops)
            if ret != 0:
                return create_ret.append(ret)
            for attr in dev['i2c']['attr_list']:
                ret = self.create_device(attr, "pddf/devices/fan/i2c", ops)
                if ret != 0:
                    return create_ret.append(ret)
                cmd = "echo 'add' > /sys/kernel/pddf/devices/fan/i2c/attr_ops"
                ret = self.runcmd(cmd)
                if ret != 0:
                    return create_ret.append(ret)

            cmd = "echo 'add' > /sys/kernel/pddf/devices/fan/i2c/dev_ops"
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
        else:
            cmd = "echo %s 0x%x > /sys/bus/i2c/devices/i2c-%d/new_device" % (dev['i2c']['topo_info']['dev_type'],
                    int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)

        return create_ret.append(ret)

    def create_non_pddf_i2c_device(self, dev, ops):
        # Create i2c devices for which a PDDF specific driver is not needed
        create_ret = []
        ret = 0
        cmd = "echo %s 0x%x > /sys/bus/i2c/devices/i2c-%d/new_device" % (dev['i2c']['topo_info']['dev_type'],
                int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
        ret = self.runcmd(cmd)
        return create_ret.append(ret)

    def create_temp_sensor_device(self, dev, ops):
        return self.create_non_pddf_i2c_device(dev, ops)

    def create_asic_temp_sensor_device(self, dev, ops):
        # NO-OP
        return [0]

    def create_dpm_device(self, dev, ops):
        return self.create_non_pddf_i2c_device(dev, ops)

    def create_dcdc_device(self, dev, ops):
        return self.create_non_pddf_i2c_device(dev, ops)

    def create_cpld_device(self, dev, ops):
        create_ret = []
        ret = 0
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['CPLD']:
            ret = self.create_device(dev['i2c']['topo_info'], "pddf/devices/cpld", ops)
            if ret != 0:
                return create_ret.append(ret)

            cmd = "echo '%s' > /sys/kernel/pddf/devices/cpld/i2c_name" % (dev['dev_info']['device_name'])
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
            # TODO: If attributes are provided then, use 'self.create_device' for them too
            cmd = "echo 'add' > /sys/kernel/pddf/devices/cpld/dev_ops"
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
        else:
            cmd = "echo %s 0x%x > /sys/bus/i2c/devices/i2c-%d/new_device" % (dev['i2c']['topo_info']['dev_type'],
                    int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)

        return create_ret.append(ret)

    def create_fpgai2c_device(self, dev, ops):
        create_ret = []
        ret = 0
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['FPGAI2C']:
            ret = self.create_device(dev['i2c']['topo_info'], "pddf/devices/fpgai2c", ops)
            if ret!=0:
                return create_ret.append(ret)

            cmd= "echo '%s' > /sys/kernel/pddf/devices/fpgai2c/i2c_name"%(dev['dev_info']['device_name'])
            ret = self.runcmd(cmd)
            if ret!=0:
                return create_ret.append(ret)
            cmd= "echo 'add' > /sys/kernel/pddf/devices/fpgai2c/dev_ops"
            ret = self.runcmd(cmd)
            if ret!=0:
                return create_ret.append(ret)
        else:
            cmd= "echo %s 0x%x > /sys/bus/i2c/devices/i2c-%d/new_device" % (dev['i2c']['topo_info']['dev_type'], int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            ret = self.runcmd(cmd)
            if ret!=0:
                return create_ret.append(ret)

        return create_ret.append(ret)

    def create_cpldmux_device(self, dev, ops):
        create_ret = []
        ret = 0
        ret = self.create_device(dev['i2c']['topo_info'], "pddf/devices/cpldmux", ops)
        if ret != 0:
            return create_ret.append(ret)
        cmd = "echo '%s' > /sys/kernel/pddf/devices/cpldmux/i2c_name" % (dev['dev_info']['device_name'])
        ret = self.runcmd(cmd)
        if ret != 0:
            return create_ret.append(ret)
        self.create_device(dev['i2c']['dev_attr'], "pddf/devices/cpldmux", ops)
        # Parse channel info
        for chan in dev['i2c']['channel']:
            self.create_device(chan, "pddf/devices/cpldmux", ops)
            cmd = "echo 'add' > /sys/kernel/pddf/devices/cpldmux/chan_ops"
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)

        cmd = "echo 'add' > /sys/kernel/pddf/devices/cpldmux/dev_ops"
        ret = self.runcmd(cmd)
        return create_ret.append(ret)

    def create_gpio_device(self, dev, ops):
        create_ret = []
        ret = 0
        ret = self.create_device(dev['i2c']['topo_info'], "pddf/devices/gpio", ops)
        if ret != 0:
            return create_ret.append(ret)
        cmd = "echo '%s' > /sys/kernel/pddf/devices/gpio/i2c_name" % (dev['dev_info']['device_name'])
        ret = self.runcmd(cmd)
        if ret != 0:
            return create_ret.append(ret)
        ret = self.create_device(dev['i2c']['dev_attr'], "pddf/devices/gpio", ops)
        if ret != 0:
            return create_ret.append(ret)
        cmd = "echo 'add' > /sys/kernel/pddf/devices/gpio/dev_ops"
        ret = self.runcmd(cmd)
        if ret != 0:
            return create_ret.append(ret)

        time.sleep(2)
        base = dev['i2c']['dev_attr']['gpio_base']
        for inst in dev['i2c']['ports']:
            if inst['port_num'] != "":
                port_no = int(base, 16) + int(inst['port_num'])
                cmd = "echo %d > /sys/class/gpio/export" % port_no
                ret = self.runcmd(cmd)
                if ret != 0:
                    return create_ret.append(ret)
                if inst['direction'] != "":
                    cmd = "echo %s >/sys/class/gpio/gpio%d/direction" % (inst['direction'], port_no)
                    ret = self.runcmd(cmd)
                    if ret != 0:
                        return create_ret.append(ret)
                    if inst['active_low'] == "1" :
                        cmd = "echo %s >/sys/class/gpio/gpio%d/active_low" % (inst['active_low'], port_no)
                        ret = self.runcmd(cmd)
                        if ret != 0:
                            return create_ret.append(ret)
                    if inst['value'] != "":
                        for i in inst['value'].split(','):
                            cmd = "echo %s >/sys/class/gpio/gpio%d/value" % (i.rstrip(), port_no)
                            ret = self.runcmd(cmd)
                            if ret != 0:
                                return create_ret.append(ret)

        return create_ret.append(ret)

    def create_mux_device(self, dev, ops):
        create_ret = []
        ret = 0
        ret = self.create_device(dev['i2c']['topo_info'], "pddf/devices/mux", ops)
        if ret != 0:
            return create_ret.append(ret)
        cmd = "echo '%s' > /sys/kernel/pddf/devices/mux/i2c_name" % (dev['dev_info']['device_name'])
        ret = self.runcmd(cmd)
        if ret != 0:
            return create_ret.append(ret)
        cmd = "echo %s > /sys/kernel/pddf/devices/mux/virt_bus" % (dev['i2c']['dev_attr']['virt_bus'])
        ret = self.runcmd(cmd)
        if ret != 0:
            return create_ret.append(ret)
        cmd = "echo 'add' > /sys/kernel/pddf/devices/mux/dev_ops"
        ret = self.runcmd(cmd)
        # Check if the dev_attr array contain idle_state
        if 'idle_state' in dev['i2c']['dev_attr']:
            cmd = "echo {} > /sys/bus/i2c/devices/{}-00{:02x}/idle_state".format(dev['i2c']['dev_attr']['idle_state'],
                    int(dev['i2c']['topo_info']['parent_bus'],0), int(dev['i2c']['topo_info']['dev_addr'],0))
            ret = self.runcmd(cmd)

        return create_ret.append(ret)

    def create_xcvr_i2c_device(self, dev, ops):
        create_ret = []
        ret = 0
        # Check if topo_info is present otherwise return from here
        if "topo_info" not in dev['i2c']:
            return create_ret.append(ret)
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['PORT_MODULE']:
            self.create_device(dev['i2c']['topo_info'], "pddf/devices/xcvr/i2c", ops)
            cmd = "echo '%s' > /sys/kernel/pddf/devices/xcvr/i2c/i2c_name" % (dev['dev_info']['device_name'])
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
            cmd = "echo '%s'  > /sys/kernel/pddf/devices/xcvr/i2c/dev_idx" % (self.get_dev_idx(dev, ops))
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
            for attr in dev['i2c']['attr_list']:
                self.create_device(attr, "pddf/devices/xcvr/i2c", ops)
                cmd = "echo 'add' > /sys/kernel/pddf/devices/xcvr/i2c/attr_ops"
                ret = self.runcmd(cmd)
                if ret != 0:
                    return create_ret.append(ret)

            cmd = "echo 'add' > /sys/kernel/pddf/devices/xcvr/i2c/dev_ops"
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
        else:
            cmd = "echo %s 0x%x > /sys/bus/i2c/devices/i2c-%d/new_device" % (dev['i2c']['topo_info']['dev_type'],
                    int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            ret = self.runcmd(cmd)
            # print("\n")
            if ret != 0:
                return create_ret.append(ret)
            # Add port name
            port_name_sysfs = '/sys/bus/i2c/devices/{}-00{:02x}/port_name'.format(
                int(dev['i2c']['topo_info']['parent_bus'], 0), int(dev['i2c']['topo_info']['dev_addr'], 0))

            if os.path.exists(port_name_sysfs):
                cmd = "echo {} > /sys/bus/i2c/devices/{}-00{:02x}/port_name".format(
                    dev['dev_info']['virt_parent'].lower(), int(dev['i2c']['topo_info']['parent_bus'], 0),
                    int(dev['i2c']['topo_info']['dev_addr'], 0))
                ret = self.runcmd(cmd)
                if ret != 0:
                    return create_ret.append(ret)

        return create_ret.append(ret)

    def create_xcvr_bmc_device(self, dev, ops):
        return [0]

    def create_xcvr_device(self, dev, ops):
        return self.create_xcvr_i2c_device(dev, ops)

    def create_sysstatus_device(self, dev, ops):
        create_ret = []
        ret = 0
        for attr in dev['attr_list']:
            self.create_device(attr, "pddf/devices/sysstatus", ops)
            cmd = "echo 'add' > /sys/kernel/pddf/devices/sysstatus/attr_ops"
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)

    def create_eeprom_device(self, dev, ops):
        create_ret = []
        ret = 0
        if "EEPROM" in self.data['PLATFORM']['pddf_dev_types'] and \
                dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['EEPROM']:
            self.create_device(dev['i2c']['topo_info'], "pddf/devices/eeprom/i2c", ops)
            cmd = "echo '%s' > /sys/kernel/pddf/devices/eeprom/i2c/i2c_name" % (dev['dev_info']['device_name'])
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)
            self.create_device(dev['i2c']['dev_attr'], "pddf/devices/eeprom/i2c", ops)
            cmd = "echo 'add' > /sys/kernel/pddf/devices/eeprom/i2c/dev_ops"
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)

        else:
            cmd = "echo %s 0x%x > /sys/bus/i2c/devices/i2c-%d/new_device" % (dev['i2c']['topo_info']['dev_type'],
                    int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)

        return create_ret.append(ret)

    def create_fpgapci_device(self, dev, ops):
        create_ret = []
        ret = 0
        ret = self.create_device(dev['i2c']['dev_attr'], "pddf/devices/fpgapci", ops)
        if ret!=0:
            return create_ret.append(ret)

        cmd = "echo 'fpgapci_init' > /sys/kernel/pddf/devices/fpgapci/dev_ops"
        ret = self.runcmd(cmd)
        return create_ret.append(ret)

    def create_multifpgapcisystem_device(self, dev, ops):
        create_ret = []
        ret = 0
        for i in dev['dev_attr']['PCI_DEVICE_IDS']:
            cmd = "echo '{} {}' > /sys/kernel/pddf/devices/multifpgapci/register_pci_device_id".format(i['vendor'], i['device'])
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)

        cmd = "echo 'multifpgapci_init' > /sys/kernel/pddf/devices/multifpgapci/dev_ops"
        ret = self.runcmd(cmd)
        return create_ret.append(ret)

    def create_multifpgapci_device(self, dev, ops):
        create_ret = []
        ret = 0
        bdf = dev['dev_info']['device_bdf']
        
        # Top level data store
        ret = self.create_device(dev['dev_info']['dev_attr'], "pddf/devices/multifpgapci/{}".format(bdf), ops)
        if ret != 0:
            return create_ret.append(ret)

        # PDDF client data store
        cmd = "echo '{}' > /sys/kernel/pddf/devices/multifpgapci/{}/i2c_name".format(dev['dev_info']['device_name'], bdf)
        ret = self.runcmd(cmd)
        if ret != 0:
            return create_ret.append(ret)

        # I2C specific data store
        ret = self.create_device(dev['i2c']['dev_attr'], "pddf/devices/multifpgapci/{}/i2c".format(bdf), ops)
        if ret != 0:
            return create_ret.append(ret)

        # MDIO specific data store
        if 'mdio' in dev:
            ret = self.create_device(dev['mdio']['dev_attr'], "pddf/devices/multifpgapci/{}/mdio".format(bdf), ops)
            if ret != 0:
                return create_ret.append(ret)

        # TODO: add GPIO & SPI specific data stores

        cmd = "echo 'fpgapci_init' > /sys/kernel/pddf/devices/multifpgapci/{}/dev_ops".format(bdf)
        ret = self.runcmd(cmd)
        if ret != 0:
            return create_ret.append(ret)

        for bus in range(int(dev['i2c']['dev_attr']['num_virt_ch'], 16)):
            cmd = "echo {} > /sys/kernel/pddf/devices/multifpgapci/{}/i2c/new_i2c_adapter".format(bus, bdf)
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)

        if 'mdio' in dev:
            ret = self.create_mdio_bus(bdf, dev['mdio'], ops)
            if ret != 0:
                return create_ret.append(ret)

        if 'gpio' in dev:
            ret = self.create_multifpgapci_gpio_device(bdf, dev['gpio'], ops)
            if ret != 0:
                return create_ret.append(ret)

        return create_ret.append(ret)

    def create_mdio_bus(self, bdf, mdio_dev, ops):
        for bus in range(int(mdio_dev['dev_attr']['num_virt_ch'], 16)):
            cmd = "echo {} > /sys/kernel/pddf/devices/multifpgapci/{}/mdio/new_mdio_bus".format(bus, bdf)
            ret = self.runcmd(cmd)
            if ret != 0:
                return ret

        return 0

    def create_multifpgapci_gpio_device(self, bdf, gpio_dev, ops):
        create_ret = []
        ret = 0

        for line in gpio_dev.keys():
            for attr in gpio_dev[line]['attr_list']:
                ret = self.create_device(attr, "pddf/devices/multifpgapci/{}/gpio/line".format(bdf), ops)
                if ret != 0:
                    return create_ret.append(ret)

            cmd = "echo 'init' > /sys/kernel/pddf/devices/multifpgapci/{}/gpio/line/create_line".format(bdf)
            ret = self.runcmd(cmd)
            if ret != 0:
                return create_ret.append(ret)

        cmd = "echo 'init' > /sys/kernel/pddf/devices/multifpgapci/{}/gpio/create_chip".format(bdf)
        ret = self.runcmd(cmd)
        if ret != 0:
            return create_ret.append(ret)

        return create_ret.append(ret)

    #################################################################################################################################
    #   DELETE DEFS
    ###################################################################################################################
    def delete_eeprom_device(self, dev, ops):
        if "EEPROM" in self.data['PLATFORM']['pddf_dev_types'] and \
                dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['EEPROM']:
            cmd = "echo '%s' > /sys/kernel/pddf/devices/eeprom/i2c/i2c_name" % (dev['dev_info']['device_name'])
            self.runcmd(cmd)
            cmd = "echo 'delete' > /sys/kernel/pddf/devices/eeprom/i2c/dev_ops"
            self.runcmd(cmd)
        else:
            cmd = "echo 0x%x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (
                    int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            self.runcmd(cmd)

    def delete_sysstatus_device(self, dev, ops):
        # NOT A PHYSICAL DEVICE.... rmmod on module would remove all the artifacts
        pass


    def delete_xcvr_i2c_device(self, dev, ops):
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['PORT_MODULE']:
            cmd = "echo '%s' > /sys/kernel/pddf/devices/xcvr/i2c/i2c_name" % (dev['dev_info']['device_name'])
            self.runcmd(cmd)
            cmd = "echo 'delete' > /sys/kernel/pddf/devices/xcvr/i2c/dev_ops"
            self.runcmd(cmd)
        else:
            cmd = "echo 0x%x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (
                    int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            self.runcmd(cmd)

    def delete_xcvr_device(self, dev, ops):
        self.delete_xcvr_i2c_device(dev, ops)
        return

    def delete_gpio_device(self, dev, ops):
        cmd = "echo '%s' > /sys/kernel/pddf/devices/gpio/i2c_name" % (dev['dev_info']['device_name'])
        self.runcmd(cmd)
        cmd = "echo 'delete' > /sys/kernel/pddf/devices/gpio/dev_ops"
        self.runcmd(cmd)

    def delete_mux_device(self, dev, ops):
        cmd = "echo '%s' > /sys/kernel/pddf/devices/mux/i2c_name" % (dev['dev_info']['device_name'])
        self.runcmd(cmd)
        cmd = "echo 'delete' > /sys/kernel/pddf/devices/mux/dev_ops"
        self.runcmd(cmd)

    def delete_cpld_device(self, dev, ops):
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['CPLD']:
            cmd = "echo '%s' > /sys/kernel/pddf/devices/cpld/i2c_name" % (dev['dev_info']['device_name'])
            self.runcmd(cmd)
            cmd = "echo 'delete' > /sys/kernel/pddf/devices/cpld/dev_ops"
            self.runcmd(cmd)
        else:
            cmd = "echo 0x%x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (
                    int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            self.runcmd(cmd)

    def delete_fpgai2c_device(self, dev, ops):
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['FPGAI2C']:
            cmd= "echo '%s' > /sys/kernel/pddf/devices/fpgai2c/i2c_name"%(dev['dev_info']['device_name'])
            self.runcmd(cmd)
            cmd= "echo 'delete' > /sys/kernel/pddf/devices/fpgai2c/dev_ops"
            self.runcmd(cmd)
        else:
            cmd= "echo 0x%x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            self.runcmd(cmd)

    def delete_cpldmux_device(self, dev, ops):
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['CPLDMUX']:
            cmd = "echo '%s' > /sys/kernel/pddf/devices/cpldmux/i2c_name" % (dev['dev_info']['device_name'])
            self.runcmd(cmd)
            cmd = "echo 'delete' > /sys/kernel/pddf/devices/cpldmux/dev_ops"
            self.runcmd(cmd)

    def delete_non_pddf_i2c_device(self, dev, ops):
        # Delete i2c devices for which a PDDF specific driver is not needed
        cmd = "echo 0x%x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (
                int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
        self.runcmd(cmd)

    def delete_temp_sensor_device(self, dev, ops):
        return self.delete_non_pddf_i2c_device(dev, ops)

    def delete_asic_temp_sensor_device(self, dev, ops):
        # NO-OP
        return

    def delete_dpm_device(self, dev, ops):
        return self.delete_non_pddf_i2c_device(dev, ops)

    def delete_dcdc_device(self, dev, ops):
        return self.delete_non_pddf_i2c_device(dev, ops)

    def delete_fan_device(self, dev, ops):
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['FAN']:
            cmd = "echo '%s' > /sys/kernel/pddf/devices/fan/i2c/i2c_name" % (dev['dev_info']['device_name'])
            self.runcmd(cmd)
            cmd = "echo 'delete' > /sys/kernel/pddf/devices/fan/i2c/dev_ops"
            self.runcmd(cmd)
        else:
            cmd = "echo 0x%x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (
                    int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            self.runcmd(cmd)


    def delete_psu_i2c_device(self, dev, ops):
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['PSU']:
            cmd = "echo '%s' > /sys/kernel/pddf/devices/psu/i2c/i2c_name" % (dev['dev_info']['device_name'])
            self.runcmd(cmd)
            cmd = "echo 'delete' > /sys/kernel/pddf/devices/psu/i2c/dev_ops"
            self.runcmd(cmd)
        else:
            cmd = "echo 0x%x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (
                    int(dev['i2c']['topo_info']['dev_addr'], 0), int(dev['i2c']['topo_info']['parent_bus'], 0))
            self.runcmd(cmd)

    def delete_psu_device(self, dev, ops):
        self.delete_psu_i2c_device(dev, ops)
        return

    def delete_fpgapci_device(self, dev, ops):
        return

    def delete_multifpgapci_device(self, dev, ops):
        bdf = dev['dev_info']['device_bdf']

        cmd = "echo '{}' > /sys/kernel/pddf/devices/multifpgapci/{}/i2c_name".format(dev['dev_info']['device_name'], bdf)
        self.runcmd(cmd)
        cmd = "echo 'fpgapci_deinit' > /sys/kernel/pddf/devices/multifpgapci/{}/dev_ops".format(bdf)
        self.runcmd(cmd)

    #################################################################################################################################
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

    # 'at24' type of EEPROM devices have only one attribtue 'eeprom'
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

    # This is only valid for LM75
    def show_attr_temp_sensor_device(self, dev, ops):
        ret = []
        attr_name = ops['attr']
        attr_list = dev['i2c']['attr_list'] if 'i2c' in dev else []
        KEY = "temp-sensors"
        dsysfs_path = ""

        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        for attr in attr_list:
            if attr_name == attr['attr_name'] or attr_name == 'all':
                path = self.show_device_sysfs(dev, ops) + \
                    "/%d-00%02x/" % (int(dev['i2c']['topo_info']['parent_bus'], 0),
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

    def show_attr_asic_temp_sensor_device(self, dev, ops):
        # NO-OP
        return []

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

    def show_attr_fpgai2c_device(self, dev, ops):
        ret = []
        KEY="fpgai2c"
        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY]=[]

        return ret

    def show_attr_cpldmux_device(self, dev, ops):
        ret = []
        KEY = "cpldmux"
        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        return ret


    def show_attr_fpgapci_device(self, dev, ops):
        ret = []
        KEY="fpgapci"
        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY]=[]

        return ret

    def show_attr_multifpgapci_device(self, dev, ops):
        ret = []
        KEY="multifpgapci"
        if KEY not in self.data_sysfs_obj:
            self.data_sysfs_obj[KEY] = []

        return ret


    ###################################################################################################################
    #   SHOW DEFS
    ###################################################################################################################
    def check_led_cmds(self, key, ops):
        name = ops['target']+'_LED'
        if (ops['target'] == 'config' or ops['attr'] == 'all') or \
                (name == self.data[key]['dev_info']['device_name'] and
                 ops['attr'] == self.data[key]['dev_attr']['index']):
            return (True)
        else:
            return (False)

    def dump_sysfs_obj(self, obj, key_type):
        if (key_type == 'keys'):
            for key in obj.keys():
                print(key)
            return

        for key in obj:
            if (key == key_type or key_type == 'all'):
                print(key+":")
                for entry in obj[key]:
                    print("\t"+entry)

    def add_list_sysfs_obj(self, obj, KEY, extra_list):
        for sysfs in extra_list:
            if not sysfs in obj[KEY]:
                obj[KEY].append(sysfs)

    def sysfs_attr(self, key, value, path, obj, obj_key):
        sysfs_path = "/sys/kernel/%s/%s" % (path, key)
        if not sysfs_path in obj[obj_key]:
            obj[obj_key].append(sysfs_path)

    def sysfs_device(self, attr, path, obj, obj_key, exceptions=[]):
        for key in attr.keys():
            if key not in exceptions:
                sysfs_path = "/sys/kernel/%s/%s" % (path, key)
                if not sysfs_path in obj[obj_key]:
                    obj[obj_key].append(sysfs_path)

    def show_eeprom_device(self, dev, ops):
        return

    def show_mux_device(self, dev, ops):
        KEY = 'mux'
        if not KEY in self.sysfs_obj:
            self.sysfs_obj[KEY] = []
            self.sysfs_device(dev['i2c']['topo_info'], "pddf/devices/mux", self.sysfs_obj, KEY)
            self.sysfs_device(dev['i2c']['dev_attr'], "pddf/devices/mux", self.sysfs_obj, KEY, ['idle_state'])
            sysfs_path = "/sys/kernel/pddf/devices/mux/dev_ops"
            if not sysfs_path in self.sysfs_obj[KEY]:
                self.sysfs_obj[KEY].append(sysfs_path)
            extra_list = ['/sys/kernel/pddf/devices/mux/i2c_type',
                    '/sys/kernel/pddf/devices/mux/i2c_name',
                    '/sys/kernel/pddf/devices/mux/error']
            self.add_list_sysfs_obj(self.sysfs_obj, KEY, extra_list)

    def show_gpio_device(self, dev, ops):
        KEY = 'gpio'
        if not KEY in self.sysfs_obj:
            self.sysfs_obj[KEY] = []
            self.sysfs_device(dev['i2c']['topo_info'], "pddf/devices/gpio", self.sysfs_obj, KEY)
            self.sysfs_device(dev['i2c']['dev_attr'], "pddf/devices/gpio", self.sysfs_obj, KEY)
            sysfs_path = "/sys/kernel/pddf/devices/gpio/dev_ops"
            if not sysfs_path in self.sysfs_obj[KEY]:
                self.sysfs_obj[KEY].append(sysfs_path)
            extra_list = ['/sys/kernel/pddf/devices/gpio/i2c_type',
                    '/sys/kernel/pddf/devices/gpio/i2c_name',
                    '/sys/kernel/pddf/devices/gpio/error']
            self.add_list_sysfs_obj(self.sysfs_obj, KEY, extra_list)

    def show_psu_i2c_device(self, dev, ops):
        KEY = 'psu'
        path = 'pddf/devices/psu/i2c'
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['PSU']:
            if not KEY in self.sysfs_obj:
                self.sysfs_obj[KEY] = []
                self.sysfs_device(dev['i2c']['topo_info'], path, self.sysfs_obj, KEY)
                sysfs_path = "/sys/kernel/pddf/devices/psu/i2c/psu_idx"
                self.sysfs_obj[KEY].append(sysfs_path)

                for attr in dev['i2c']['attr_list']:
                    self.sysfs_device(attr, "pddf/devices/psu/i2c", self.sysfs_obj, KEY)
                    sysfs_path = "/sys/kernel/pddf/devices/psu/i2c/dev_ops"
                    if not sysfs_path in self.sysfs_obj[KEY]:
                        self.sysfs_obj[KEY].append(sysfs_path)
                extra_list = ['/sys/kernel/pddf/devices/psu/i2c/i2c_type',
                        '/sys/kernel/pddf/devices/fan/i2c/i2c_name',
                        '/sys/kernel/pddf/devices/psu/i2c/error',
                        '/sys/kernel/pddf/devices/psu/i2c/attr_ops']
                self.add_list_sysfs_obj(self.sysfs_obj, KEY, extra_list)

    def show_psu_device(self, dev, ops):
        self.show_psu_i2c_device(dev, ops)
        return

    def show_client_device(self):
        KEY = 'client'
        if not KEY in self.sysfs_obj:
            self.sysfs_obj[KEY] = []
            extra_list = ['/sys/kernel/pddf/devices/showall']
            self.add_list_sysfs_obj(self.sysfs_obj, KEY, extra_list)

    def show_fan_device(self, dev, ops):
        KEY = 'fan'
        path = 'pddf/devices/fan/i2c'
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['FAN']:
            if not KEY in self.sysfs_obj:
                self.sysfs_obj[KEY] = []

                self.sysfs_device(dev['i2c']['topo_info'], path, self.sysfs_obj, KEY, ['client_type'])
                self.sysfs_device(dev['i2c']['dev_attr'], path, self.sysfs_obj, KEY)
                for attr in dev['i2c']['attr_list']:
                    self.sysfs_device(attr, path, self.sysfs_obj, KEY)
                extra_list = ['/sys/kernel/pddf/devices/fan/i2c/i2c_type',
                        '/sys/kernel/pddf/devices/fan/i2c/i2c_name',
                        '/sys/kernel/pddf/devices/fan/i2c/error',
                        '/sys/kernel/pddf/devices/fan/i2c/attr_ops',
                        '/sys/kernel/pddf/devices/fan/i2c/dev_ops']
                self.add_list_sysfs_obj(self.sysfs_obj, KEY, extra_list)

    def show_temp_sensor_device(self, dev, ops):
        return

    def show_asic_temp_sensor_device(self, dev, ops):
        return

    def show_sysstatus_device(self, dev, ops):
        KEY = 'sysstatus'
        if not KEY in self.sysfs_obj:
            self.sysfs_obj[KEY] = []
            for attr in dev['attr_list']:
                self.sysfs_device(attr, "pddf/devices/sysstatus", self.sysfs_obj, KEY)
                sysfs_path = "/sys/kernel/pddf/devices/sysstatus/attr_ops"
                if not sysfs_path in self.sysfs_obj[KEY]:
                    self.sysfs_obj[KEY].append(sysfs_path)

    def show_xcvr_i2c_device(self, dev, ops):
        KEY = 'xcvr'
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['PORT_MODULE']:
            if not KEY in self.sysfs_obj:
                self.sysfs_obj[KEY] = []
                self.sysfs_device(dev['i2c']['topo_info'], "pddf/devices/xcvr/i2c", self.sysfs_obj, KEY, ['client_type'])

                for attr in dev['i2c']['attr_list']:
                    self.sysfs_device(attr, "pddf/devices/xcvr/i2c", self.sysfs_obj, KEY)
                    sysfs_path = "/sys/kernel/pddf/devices/xcvr/i2c/dev_ops"
                    if not sysfs_path in self.sysfs_obj[KEY]:
                        self.sysfs_obj[KEY].append(sysfs_path)
                extra_list = ['/sys/kernel/pddf/devices/xcvr/i2c/i2c_type',
                        '/sys/kernel/pddf/devices/xcvr/i2c/i2c_name',
                        '/sys/kernel/pddf/devices/xcvr/i2c/error',
                        '/sys/kernel/pddf/devices/xcvr/i2c/attr_ops']
                self.add_list_sysfs_obj(self.sysfs_obj, KEY, extra_list)

    def show_xcvr_device(self, dev, ops):
        self.show_xcvr_i2c_device(dev, ops)
        return

    def show_cpld_device(self, dev, ops):
        KEY = 'cpld'
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['CPLD']:
            if not KEY in self.sysfs_obj:
                self.sysfs_obj[KEY] = []
                self.sysfs_device(dev['i2c']['topo_info'], "pddf/devices/cpld", self.sysfs_obj, KEY)
                sysfs_path = "/sys/kernel/pddf/devices/cpld/dev_ops"
                if not sysfs_path in self.sysfs_obj[KEY]:
                    self.sysfs_obj[KEY].append(sysfs_path)
                extra_list = ['/sys/kernel/pddf/devices/cpld/i2c_type',
                        '/sys/kernel/pddf/devices/cpld/i2c_name',
                        '/sys/kernel/pddf/devices/cpld/error']
                self.add_list_sysfs_obj(self.sysfs_obj, KEY, extra_list)

    def show_fpgai2c_device(self, dev, ops):
        KEY ='fpgai2c'
        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['FPGAI2C']:
            if not KEY in self.sysfs_obj:
                self.sysfs_obj[KEY] = []
                self.sysfs_device(dev['i2c']['topo_info'], "pddf/devices/fpgai2c", self.sysfs_obj, KEY)
                sysfs_path = "/sys/kernel/pddf/devices/fpgai2c/dev_ops"
                if not sysfs_path in self.sysfs_obj[KEY]:
                    self.sysfs_obj[KEY].append(sysfs_path)
                extra_list = ['/sys/kernel/pddf/devices/fpgai2c/i2c_type',
                              '/sys/kernel/pddf/devices/fpgai2c/i2c_name',
                              '/sys/kernel/pddf/devices/fpgai2c/error']
                self.add_list_sysfs_obj(self.sysfs_obj, KEY, extra_list)


    def show_led_platform_device(self, key, ops):
        if ops['attr'] == 'all' or ops['attr'] == 'PLATFORM':
            KEY = 'platform'
            if not KEY in self.sysfs_obj:
                self.sysfs_obj[KEY] = []
                path = 'pddf/devices/platform'
                self.sysfs_attr('num_psus', self.data['PLATFORM']['num_psus'], path, self.sysfs_obj, KEY)
                self.sysfs_attr('num_fantrays', self.data['PLATFORM']['num_fantrays'], path, self.sysfs_obj, KEY)

    def show_led_device(self, key, ops):
        if self.check_led_cmds(key, ops):
            KEY='led'
            if not KEY in self.sysfs_obj:
                self.sysfs_obj[KEY] = []
                path="pddf/devices/led"
                self.sysfs_attr('device_name', self.data[key]['dev_info']['device_name'], path, self.sysfs_obj, KEY)
                self.sysfs_attr('swpld_addr', self.data[key]['dev_info']['device_name'], path, self.sysfs_obj, KEY)
                self.sysfs_attr('swpld_addr_offset', self.data[key]['dev_info']['device_name'], path, self.sysfs_obj, KEY)
                state_attr_path="pddf/devices/led/state_attr"
                self.sysfs_attr('bits', self.data[key]['dev_info']['device_name'], state_attr_path, self.sysfs_obj, KEY)
                self.sysfs_attr('value', self.data[key]['dev_info']['device_name'], state_attr_path, self.sysfs_obj, KEY)
                sysfs_path="/sys/kernel/pddf/devices/led/dev_ops"
                if not sysfs_path in self.sysfs_obj[KEY]:
                    self.sysfs_obj[KEY].append(sysfs_path)
                extra_list=['/sys/kernel/pddf/devices/led/cur_state/color']
                self.add_list_sysfs_obj(self.sysfs_obj, KEY, extra_list)


    def show_fpgapci_device(self, dev, ops):
        KEY ='fpgapci'
        if dev['dev_info']['device_type'] in self.data['PLATFORM']['pddf_dev_types']['FPGAPCIE']:
            if not KEY in self.sysfs_obj:
                self.sysfs_obj[KEY] = []
                sysfs_path = "/sys/kernel/pddf/devices/fpgapci"
                if not sysfs_path in self.sysfs_obj[KEY]:
                    self.sysfs_obj[KEY].append(sysfs_path)
                extra_list = ['/sys/kernel/pddf/devices/fpgapci/vendor_id',
                              '/sys/kernel/pddf/devices/fpgapci/virt_bus',
                              '/sys/kernel/pddf/devices/fpgapci/device_id',
                              '/sys/kernel/pddf/devices/fpgapci/data_base_offset',
                              '/sys/kernel/pddf/devices/fpgapci/data_size',
                              '/sys/kernel/pddf/devices/fpgapci/i2c_ch_base_offset',
                              '/sys/kernel/pddf/devices/fpgapci/i2c_ch_size',
                              '/sys/kernel/pddf/devices/fpgapci/virt_i2c_ch']
                self.add_list_sysfs_obj(self.sysfs_obj, KEY, extra_list)

    def show_multifpgapci_device(self, dev, ops):
        # TODO: Implement this once we've finalized the sysfs interface (i.e. i2c, spi directories)
        return

    def validate_xcvr_device(self, dev, ops):
        devtype_list = ['optoe1', 'optoe2', 'optoe3']
        dev_attribs = ['xcvr_present', 'xcvr_reset', 'xcvr_intr_status', 'xcvr_lpmode']
        ret_val = "xcvr validation failed"

        if dev['i2c']['topo_info']['dev_type'] in devtype_list:
            for attr in dev['i2c']['attr_list']:
                if 'attr_name' in attr.keys() and 'eeprom' in attr.values():
                    ret_val = "xcvr validation success"
                else:
                    print("xcvr validation Failed")
                    return

        elif dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['PORT_MODULE']:
            for attr in dev['i2c']['attr_list']:
                if attr.get("attr_name") in dev_attribs:
                    ret_val = "Success"
                else:
                    print("xcvr validation Failed")
                    return
        print(ret_val)

    def validate_eeprom_device(self, dev, ops):
        devtype_list = ['24c02']
        dev_access_mode = ['BLOCK', 'BYTE']
        dev_attribs = ['eeprom']
        ret_val = "eeprom failed"

        if dev['i2c']['topo_info']['dev_type'] in devtype_list:
            if dev['i2c']['dev_attr']['access_mode'] in dev_access_mode:
                for attr in dev['i2c']['attr_list']:
                    if attr.get("attr_name") in dev_attribs:
                        ret_val = "eeprom success"
        print(ret_val)

    def validate_mux_device(self, dev, ops):
        devtype_list = ['pca9548', 'pca9545', 'pca9546', 'pca954x']
        dev_channels = ["0", "1", "2", "3", "4", "5", "6", "7"]
        ret_val = "mux failed"

        if dev['i2c']['topo_info']['dev_type'] in devtype_list:
            for attr in dev['i2c']['channel']:
                if attr.get("chn") in dev_channels:
                    ret_val = "Mux success"
        print(ret_val)

    def validate_cpld_device(self, dev, ops):
        devtype_list = ['i2c_cpld']
        ret_val = "cpld failed"

        if dev['i2c']['topo_info']['dev_type'] in devtype_list:
            ret_val = "cpld success"
        print(ret_val)

    def validate_fpgai2c_device(self, dev, ops):
        devtype_list = ['i2c_fpga']
        ret_val = "fpgai2c failed"

        if dev['i2c']['topo_info']['dev_type'] in devtype_list:
            ret_val = "fpgai2c success"
        print(ret_val)

    def validate_sysstatus_device(self, dev, ops):
        dev_attribs = ['board_info', 'cpld1_version', 'power_module_status', 'system_reset5',
                       'system_reset6', 'system_reset7', 'misc1', 'cpld2_version', 'cpld3_version'
                       ]
        ret_val = "sysstatus failed"

        if dev['dev_info']['device_type'] == "SYSSTAT":
            for attr in dev['attr_list']:
                if attr.get("attr_name") in dev_attribs:
                    ret_val = "sysstatus success"
        print(ret_val)

    def validate_temp_sensor_device(self, dev, ops):
        devtype_list = ['lm75']
        dev_attribs = ['temp1_max', 'temp1_max_hyst', 'temp1_input']
        ret_val = "temp sensor failed"

        if dev['dev_info']['device_type'] == "TEMP_SENSOR":
            if dev['i2c']['topo_info']['dev_type'] in devtype_list:
                for attr in dev['i2c']['attr_list']:
                    if attr.get("attr_name") in dev_attribs:
                        ret_val = "tempsensor success"
        print(ret_val)

    def validate_asic_temp_sensor_device(self, dev, ops):
        dev_attribs = ['display_name', 'temp1_high_threshold', 'temp1_high_crit_threshold']
        ret_val = "asic temp sensor failed"

        if dev['dev_info']['device_type'] == "ASIC_TEMP_SENSOR":
            for attr in dev['dev_attr'].keys():
                if attr in dev_attribs:
                    ret_val = "asic temp sensor success"
        print(ret_val)

    def validate_fan_device(self, dev, ops):
        ret_val = "fan failed"

        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['FAN']:
            if dev['i2c']['dev_attr']['num_fantrays'] is not None:
                ret_val = "fan success"

        print(ret_val)

    def validate_psu_device(self, dev, ops):
        dev_attribs = ['psu_present', 'psu_model_name', 'psu_power_good', 'psu_mfr_id', 'psu_serial_num',
                       'psu_fan_dir', 'psu_v_out', 'psu_i_out', 'psu_p_out', 'psu_fan1_speed_rpm'
                       ]
        ret_val = "psu failed"

        if dev['i2c']['topo_info']['dev_type'] in self.data['PLATFORM']['pddf_dev_types']['PSU']:
            for attr in dev['i2c']['attr_list']:
                if attr.get("attr_name") in dev_attribs:
                    if attr.get("attr_devaddr") is not None:
                        if attr.get("attr_offset") is not None:
                            if attr.get("attr_mask") is not None:
                                if attr.get("attr_len") is not None:
                                    ret_val = "psu success"
                else:
                    ret_val = "psu failed"

        print(ret_val)

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
        self.create_attr('device_name', self.data[device_name]['dev_info']['device_name'], "pddf/devices/led")
        self.create_attr('index', self.data[device_name]['dev_attr']['index'], "pddf/devices/led")
        cmd = "echo 'verify'  > /sys/kernel/pddf/devices/led/dev_ops"
        self.runcmd(cmd)

    def validate_sysfs_creation(self, obj, validate_type):
        dir = '/sys/kernel/pddf/devices/'+validate_type
        if (os.path.exists(dir) or validate_type == 'client'):
            for sysfs in obj[validate_type]:
                if(not os.path.exists(sysfs)):
                    print("[SYSFS FILE] " + sysfs + ": does not exist")
        else:
            print("[SYSFS DIR] " + dir + ": does not exist")

    def validate_dsysfs_creation(self, obj, validate_type):
        if validate_type in obj.keys():
            # There is a possibility that some components dont have any device-self.data attr
            if not obj[validate_type]:
                print("[SYSFS ATTR] for " + validate_type + ": empty ")
            else:
                for sysfs in obj[validate_type]:
                    if(not os.path.exists(sysfs)):
                        print("[SYSFS FILE] " + sysfs + ": does not exist")
        else:
            print("[SYSFS KEY] for " + validate_type + ": not configured")

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



    def schema_validation(self, validate_type):
        process_validate_type = 0
        for key in self.data.keys():
            if (key != 'PLATFORM'):
                temp_obj = {}
                schema_list = []
                try:
                    device_type = self.data[key]["dev_info"]["device_type"]
                except Exception as e:
                    print("dev_info or device_type ERROR: " + key)
                    print(e)

                if validate_type == 'mismatch':
                    process_validate_type = 1
                    device_type = "PSU"
                    schema_file = "/usr/local/bin/schema/FAN.schema"
                    schema_list.append(schema_file)
                elif validate_type == 'missing':
                    process_validate_type = 1
                    schema_file = "/usr/local/bin/schema/PLATFORM.schema"
                    schema_list.append(schema_file)

                elif validate_type == 'empty':
                    process_validate_type = 1
                    if not device_type:
                        print("Empty device_type for " + key)
                        continue
                elif (validate_type == 'all' or validate_type == device_type):
                    process_validate_type = 1
                    if "bmc" in self.data[key].keys():
                        schema_file = "/usr/local/bin/schema/"+device_type + "_BMC.schema"
                        schema_list.append(schema_file)

                    if "i2c" in self.data[key].keys():
                        schema_file = "/usr/local/bin/schema/"+device_type + ".schema"
                        schema_list.append(schema_file)
                if device_type:
                    temp_obj[device_type] = self.data[key]
                    for schema_file in schema_list:
                        if (os.path.exists(schema_file)):
                            print("Validate " + schema_file + ";" + key)
                            json_data = json.dumps(temp_obj)
                            with open(schema_file, 'r') as f:
                                schema = json.load(f)
                            try:
                                from jsonschema import validate
                                validate(temp_obj, schema)
                            except Exception as e:
                                print("Validation ERROR: " + schema_file + ";" + key)
                                if validate_type == 'mismatch':
                                    return
                                else:
                                    print(e)
                        else:
                            print("ERROR Missing File: " + schema_file)
        if not process_validate_type:
            print("device_type: " + validate_type + " not configured")

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
                    subprocess.check_output(cmd, shell=True)
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

    def asic_temp_sensor_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_asic_temp_sensor_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_asic_temp_sensor_device failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

        return ret

    def dpm_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_dpm_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_dpm_device failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

        return ret

    def dcdc_parse(self, dev, ops):
        ret = []
        ret = getattr(self, ops['cmd']+"_dcdc_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_dcdc_device failed for {}".format(ops['cmd'], dev['dev_info']['device_name']))

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

    def fpgai2c_parse(self, dev, ops):
        val = []
        ret = getattr(self, ops['cmd']+"_fpgai2c_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0]!=0:
                    # in case if 'create' functions
                    print("{}_fpgai2c_device failed".format(ops['cmd']))
                    return ret
            else:
                # in case of 'show_attr' functions
                val.extend(ret)

        return val

    def cpldmux_parse(self, dev, ops):
        val = []
        ret = getattr(self, ops['cmd']+"_cpldmux_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_cpldmux_device() cmd failed".format(ops['cmd']))
                    return ret
            else:
                val.extend(ret)

        for chan in dev['i2c']['channel']:
            for device in chan['dev']:
                ret = self.dev_parse(self.data[device], ops)
                if ret:
                    if str(ret[0]).isdigit():
                        if ret[0] != 0:
                            # in case if 'create' functions
                            return ret
                    else:
                        val.extend(ret)

        return val

    def cpldmux_parse_reverse(self, dev, ops):
        val = []
        for chan in reversed(dev['i2c']['channel']):
            for device in reversed(chan['dev']):
                ret = self.dev_parse(self.data[device], ops)
                if ret:
                    if str(ret[0]).isdigit():
                        if ret[0] != 0:
                            # in case if 'create' functions
                            return ret
                    else:
                        val.extend(ret)

        ret = getattr(self, ops['cmd']+"_cpldmux_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    # in case if 'create' functions
                    print("{}_cpldmux_device() cmd failed".format(ops['cmd']))
                    return ret
            else:
                val.extend(ret)

        return val

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
            for d in reversed(dev1['i2c']['DEVICES']):
                ret = self.dev_parse(self.data[d['dev']], ops)
                if ret:
                    if str(ret[0]).isdigit():
                        if ret[0] != 0:
                            # in case if 'create' functions
                            return ret
                    else:
                        val.extend(ret)
        return val

    def fpgapci_parse(self, dev, ops):
        val = []
        ret = getattr(self, ops['cmd']+"_fpgapci_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0]!=0:
                    # in case if 'create' functions
                    print("{}_fpgapci_device() cmd failed".format(ops['cmd']))
                    return ret
            else:
                val.extend(ret)

        for bus in dev['i2c']['channel']:
            ret = self.dev_parse(self.data[bus['dev']], ops)
            if ret:
                 if str(ret[0]).isdigit():
                      if ret[0]!=0:
                            # in case if 'create' functions
                           return ret
                 else:
                      val.extend(ret)
        return val

    def multifpgapcisystem_parse(self, ops):
        val = []

        dev = self.data.get("MULTIFPGAPCIESYSTEM0")
        if not dev:
            return []

        if "dev_info" not in dev:
            return []

        if dev["dev_info"].get("device_type") != "MULTIFPGAPCIESYSTEM":
            return []

        ret = getattr(self, ops['cmd']+"_multifpgapcisystem_device")(dev, ops)
        if ret:
            if str(ret[0]).isdigit():
                if ret[0] != 0:
                    print("{}_multifpgapcisystem_device() cmd failed".format(ops['cmd']))
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

    # 'create' and 'show_attr' ops returns an array
    # 'delete', 'show' and 'validate' ops return None
    def dev_parse(self, dev, ops):
        attr=dev['dev_info']
        if attr['device_type'] == 'CPU':
            if ops['cmd']=='delete':
                return self.cpu_parse_reverse(dev, ops)
            else:
                return self.cpu_parse(dev, ops)
        
        if attr['device_type'] == 'FPGAPCIE':
            return self.fpgapci_parse(dev, ops)

        if attr['device_type'] == 'EEPROM':
            return self.eeprom_parse(dev, ops)

        if attr['device_type'] == 'MUX':
            if ops['cmd']=='delete':
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

        if attr['device_type'] == 'ASIC_TEMP_SENSOR':
            return self.asic_temp_sensor_parse(dev, ops)

        if attr['device_type'] in ['SFP', 'SFP+', 'SFP28', 'QSFP', 'QSFP+', 'QSFP28', 'QSFP-DD', 'OSFP']:
            return self.optic_parse(dev, ops)

        if attr['device_type'] == 'FPGAI2C':
                return self.fpgai2c_parse(dev, ops)

        if attr['device_type'] == 'CPLD':
            return self.cpld_parse(dev, ops)

        if attr['device_type'] == 'CPLDMUX':
            if ops['cmd'] == 'delete':
                return self.cpldmux_parse_reverse(dev, ops)
            else:
                return self.cpldmux_parse(dev, ops)

        if attr['device_type'] == 'SYSSTAT':
            return self.sysstatus_parse(dev, ops)

        if attr['device_type'] == 'MULTIFPGAPCIE':
            return self.multifpgapci_parse(dev, ops)

        if attr['device_type'] == 'DCDC':
            return self.dcdc_parse(dev, ops)

        if attr['device_type'] == 'DPM':
            return self.dpm_parse(dev, ops)

    def is_supported_sysled_state(self, sysled_name, sysled_state):
        if not sysled_name in self.data.keys():
            return False, "[FAILED] " + sysled_name + " is not configured"
        for attr in self.data[sysled_name]['i2c']['attr_list']:
            if attr['attr_name'] == sysled_state:
                return True, "[PASS] supported"
        return False,  "[FAILED]: Invalid color"


    def create_attr(self, key, value, path, exceptions=[]):
        if key not in exceptions:
            cmd = "echo '%s' > /sys/kernel/%s/%s" % (value,  path, key)
            self.runcmd(cmd)

    def create_led_platform_device(self, key, ops):
        if ops['attr'] == 'all' or ops['attr'] == 'PLATFORM':
            path = 'pddf/devices/platform'
            self.create_attr('num_psus', self.data['PLATFORM']['num_psus'], path)
            self.create_attr('num_fantrays', self.data['PLATFORM']['num_fantrays'], path)

    def create_led_device(self, key, ops):
        if ops['attr']=='all' or ops['attr']==self.data[key]['dev_info']['device_name']:
            path = "pddf/devices/led"
            ops_state = ""
            if 'bmc' in self.data[key]:
                return
            for attr in self.data[key]['i2c']['attr_list']:
                self.create_attr('device_name', self.data[key]['dev_info']['device_name'], path)
                self.create_attr('index', self.data[key]['dev_attr']['index'], path)
                #attr_devtype and attr_devname are optional in json file.
                #if attr_devtype is not defined, it means it is "cpld"
                if 'attr_devtype' not in attr.keys():
                    self.create_attr('attr_devtype', 'cpld', path)
                for attr_key in attr.keys():
                    if attr_key in ['swpld_addr_offset', 'swpld_addr', 'attr_devtype', 'attr_devname', 'attr_bdf']:
                        self.create_attr(attr_key, attr[attr_key], path)
                    elif attr_key not in ['attr_name', 'descr', 'state']:
                        state_path = path+'/state_attr'
                        self.create_attr(attr_key, attr[attr_key],state_path)
                cmd="echo '"  + attr['attr_name']+"' > /sys/kernel/pddf/devices/led/dev_ops"
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


    def create_pddf_devices(self):
        ret = self.multifpgapcisystem_parse({"cmd": "create", "target": "all", "attr": "all"})
        if ret:
            if ret[0] != 0:
                return ret[0]
        self.led_parse({"cmd": "create", "target": "all", "attr": "all"})
        create_ret = 0
        ret = self.dev_parse(self.data['SYSTEM'], {"cmd": "create", "target": "all", "attr": "all"})
        if ret:
            if ret[0] != 0:
                return ret[0]
        if 'SYSSTATUS' in self.data:
            ret = self.dev_parse(self.data['SYSSTATUS'], {"cmd": "create", "target": "all", "attr": "all"})
            if ret:
                if ret[0] != 0:
                    return ret[0]
        return create_ret

    def create_subtree(self, device_name):
        subtree = self.data.get(device_name)
        if not subtree:
            print(f"Invalid device_name {device_name}")
            return 1

        ret = self.dev_parse(subtree, {"cmd": "create", "target": "all", "attr": "all"})
        if ret:
            if ret[0] != 0:
                return ret[0]

        return 0

    def delete_subtree(self, device_name):
        subtree = self.data.get(device_name)
        if not subtree:
            print(f"Invalid device_name {device_name}")
            return 1

        ret = self.dev_parse(self.data[device_name], {"cmd": "delete", "target": "all", "attr": "all"})
        if ret:
            if ret[0] != 0:
                return ret[0]

        return 0

    def delete_pddf_devices(self):
        self.dev_parse(self.data['SYSTEM'], {"cmd": "delete", "target": "all", "attr": "all"})
        if 'SYSSTATUS' in self.data:
            self.dev_parse(self.data['SYSSTATUS'], {"cmd": "delete", "target": "all", "attr": "all"})

    def populate_pddf_sysfsobj(self):
        self.dev_parse(self.data['SYSTEM'], {"cmd": "show", "target": "all", "attr": "all"})
        if 'SYSSTATUS' in self.data:
            self.dev_parse(self.data['SYSSTATUS'], {"cmd": "show", "target": "all", "attr": "all"})
        self.led_parse({"cmd": "show", "target": "all", "attr": "all"})
        self.show_client_device()

    def cli_dump_dsysfs(self, component):
        self.dev_parse(self.data['SYSTEM'], {"cmd": "show_attr", "target": "all", "attr": "all"})
        if 'SYSSTATUS' in self.data:
            self.dev_parse(self.data['SYSSTATUS'], {"cmd": "show_attr", "target": "all", "attr": "all"})
        if component in self.data_sysfs_obj:
            return self.data_sysfs_obj[component]
        else:
            return None

    def validate_pddf_devices(self, *args):
        self.populate_pddf_sysfsobj()
        v_ops = {'cmd': 'validate', 'target': 'all', 'attr': 'all'}
        self.dev_parse(self.data['SYSTEM'], v_ops)

    def get_psu_temp_high_thresh_bitmap(self, psu_pmbus_device, num_psu_thermals):
        """
        Check which PSU thermal sensors have high threshold support

        Args:
            psu_pmbus_device (str): PSU PMBUS device name (e.g. "PSU1-PMBUS")
            num_psu_thermals (int): Number of thermal sensors in the PSU

        Returns:
            int: Bitmap where each bit represents whether a thermal sensor supports high threshold
                 Bit 0 (LSB) corresponds to thermal sensor 1, bit 1 to sensor 2, etc.
                 A bit value of 1 means the sensor supports high threshold, 0 means it doesn't
        """
        bitmap = 0

        # Get the set of attribute names for this device (if it exists)
        attr_names = set()
        if psu_pmbus_device in self.data:
            device_data = self.data[psu_pmbus_device]
            if "i2c" in device_data and "attr_list" in device_data["i2c"]:
                attr_names = {attr.get("attr_name") for attr in device_data["i2c"]["attr_list"]}

        # Check each thermal sensor's high threshold attribute and set corresponding bit
        for thermal_idx in range(1, num_psu_thermals + 1):
            attr_name = f"psu_temp{thermal_idx}_high_threshold"
            if attr_name in attr_names:
                # Set the bit corresponding to this thermal sensor (0-indexed)
                bitmap |= (1 << (thermal_idx - 1))

        return bitmap

    ##################################################################################################################
    #   BMC APIs 
    ##################################################################################################################
    def populate_bmc_cache_db(self, bmc_attr):
        bmc_cmd = str(bmc_attr['bmc_cmd']).strip()
        
        if 'delimiter' in bmc_attr.keys():
            delim = str(bmc_attr['delimiter']).strip()
        else:
            delim = None

        o_list = subprocess.check_output(bmc_cmd, shell=True).strip().split('\n')
        bmc_cache[bmc_cmd] = {}
        bmc_cache[bmc_cmd]['time'] = time.time()
        for entry in o_list:
            name = entry.split(delim)[0].strip()
            bmc_cache[bmc_cmd][name] = entry

    def non_raw_ipmi_get_request(self, bmc_attr):
        bmc_db_update_time = 1
        value = 'N/A'
        bmc_cmd = str(bmc_attr['bmc_cmd']).strip()
        field_name = str(bmc_attr['field_name']).strip()
        field_pos = int(bmc_attr['field_pos'])-1

        if 'delimiter' in bmc_attr.keys():
            delim = str(bmc_attr['delimiter']).strip()
        else:
            delim = None

        if not bmc_cmd in bmc_cache:
            self.populate_bmc_cache_db(bmc_attr)
        else:
            now = time.time()
            if (int(now - bmc_cache[bmc_cmd]['time']) > bmc_db_update_time):
                self.populate_bmc_cache_db(bmc_attr)

        try:
            data = bmc_cache[bmc_cmd][field_name]
            value = data.split(delim)[field_pos].strip()
        except Exception as e:
            pass

        if 'mult' in bmc_attr.keys() and not value.isalpha():
            if value.isalpha():
                value = 0.0
            value = float(value) * float(bmc_attr['mult'])

        return str(value)

    def raw_ipmi_get_request(self, bmc_attr):
        value = 'N/A'
        cmd = bmc_attr['bmc_cmd'] + " 2>/dev/null"
        if bmc_attr['type'] == 'raw':
            try:
                value = subprocess.check_output(cmd, shell=True).strip()
            except Exception as e:
                pass

            if value != 'N/A':
                value = str(int(value, 16))
            return value

        if bmc_attr['type'] == 'mask':
            mask = int(bmc_attr['mask'].encode('utf-8'), 16)
            try:
                value = subprocess.check_output(cmd, shell=True).strip()
            except Exception as e:
                pass

            # value should either be '1' or '0'
            if value != 'N/A':
                value = '1' if bool(int(value, 16) & mask) else '0'

            return value

        if bmc_attr['type'] == 'ascii':
            try:
                value = subprocess.check_output(cmd, shell=True)
            except Exception as e:
                pass

            if value != 'N/A':
                tmp = ''.join(chr(int(i, 16)) for i in value.split())
                tmp = "".join(i for i in unicode(tmp) if unicodedata.category(i)[0] != "C")
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

        if bmc_attr is not None:
            if bmc_attr == {}:
                return {}
            output['mode'] = "bmc"
            output['status'] = self.bmc_get_cmd(bmc_attr)
        else:
            output['mode'] = "i2c"
            node = self.get_path(device_name, attr_name)
            if node is None:
                return {}
            try:
                with open(node, 'r') as f:
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

    ###################################################################################################################

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--create", action='store_true', help="create the I2C topology")
    parser.add_argument("--sysfs", action='store', nargs="+",  help="show access-attributes sysfs for the I2C topology")
    parser.add_argument("--dsysfs", action='store', nargs="+",  help="show data-attributes sysfs for the I2C topology")
    parser.add_argument("--delete", action='store_true', help="Remove all the created I2C clients from topology")
    parser.add_argument("--validate", action='store', help="Validate the device specific attribute data elements")
    parser.add_argument("--schema", action='store', nargs="+",  help="Schema Validation")
    parser.add_argument("--modules", action='store', nargs="+", help="Loaded modules validation")
    parser.add_argument("--create-subtree", type=str,
        help="Create a specified node and all its descendant nodes in the I2C topology.")
    parser.add_argument("--delete-subtree", type=str,
        help="Remove a specified node and all its descendant nodes from the I2C topology.")

    args = parser.parse_args()

    # Create the object
    try:
        pddf_obj = PddfParse()
    except Exception as e:
        print("%s" % str(e))
        sys.exit()

    if args.create:
        pddf_obj.create_pddf_devices()

    if args.sysfs:
        if args.sysfs[0] == 'all':
            pddf_obj.populate_pddf_sysfsobj()
        if args.sysfs[0] == 'print':
            pddf_obj.populate_pddf_sysfsobj()
            pddf_obj.dump_sysfs_obj(pddf_obj.sysfs_obj, args.sysfs[1])
        if args.sysfs[0] == 'validate':
            pddf_obj.populate_pddf_sysfsobj()
            pddf_obj.validate_sysfs_creation(pddf_obj.sysfs_obj, args.sysfs[1])
        if args.sysfs[0] == 'verify':
            pddf_obj.verify_sysfs_data(args.sysfs[1])

    if args.dsysfs:
        if args.dsysfs[0] == 'validate':
            pddf_obj.dev_parse(pddf_obj.data['SYSTEM'], {"cmd": "show_attr", "target": "all", "attr": "all"})
            if 'SYSSTATUS' in pddf_obj.data:
                pddf_obj.dev_parse(pddf_obj.data['SYSSTATUS'], {"cmd": "show_attr", "target": "all", "attr": "all"})
            pddf_obj.validate_dsysfs_creation(pddf_obj.data_sysfs_obj, args.dsysfs[1])

        elif args.dsysfs[0] == 'print':
            pddf_obj.dev_parse(pddf_obj.data['SYSTEM'], {"cmd": "show_attr", "target": "all", "attr": "all"})
            if 'SYSSTATUS' in pddf_obj.data:
                pddf_obj.dev_parse(pddf_obj.data['SYSSTATUS'], {"cmd": "show_attr", "target": "all", "attr": "all"})
            pddf_obj.dump_sysfs_obj(pddf_obj.data_sysfs_obj, args.dsysfs[1])

        elif args.dsysfs[0] == 'all':
            ret = pddf_obj.dev_parse(pddf_obj.data['SYSTEM'], {"cmd": "show_attr", "target": "all", "attr": "all"})
            if 'SYSSTATUS' in pddf_obj.data:
                ret += pddf_obj.dev_parse(pddf_obj.data['SYSSTATUS'], {"cmd": "show_attr", "target": "all",
                                                                       "attr": "all"})
            pddf_obj.dump_sysfs_obj(pddf_obj.data_sysfs_obj, 'all')
        else:
            val = pddf_obj.dev_parse(pddf_obj.data[args.dsysfs[0]], {"cmd": "show_attr", "target": args.dsysfs[0],
                                                               "attr": args.dsysfs[1]})
            for i in val:
                print(i)

    if args.delete:
        pddf_obj.delete_pddf_devices()

    if args.validate:
        if args.validate[0] == 'all':
            pddf_obj.validate_pddf_devices(args.validate[1:])
        else:
            pass

    if args.schema:
        pddf_obj.schema_validation(args.schema[0])

    if args.modules:
        pddf_obj.modules_validation(args.modules[0])

    if args.create_subtree:
        sys.exit(pddf_obj.create_subtree(args.create_subtree))

    if args.delete_subtree:
        sys.exit(pddf_obj.delete_subtree(args.delete_subtree))

if __name__ == "__main__":
    main()
