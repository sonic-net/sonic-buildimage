#
# Sample pddf_psuutil file
#
# All the supported PSU SysFS aattributes are
#- psu_present
#- psu_model_name
#- psu_power_good
#- psu_mfr_id
#- psu_serial_num
#- psu_fan_dir
#- psu_v_out
#- psu_i_out
#- psu_p_out
#- psu_fan1_speed_rpm
#

import os.path
import sys
sys.path.append('/usr/share/sonic/platform/plugins')
import pddfparse
import json

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """PDDF generic PSU util class"""

    def __init__(self):
        PsuBase.__init__(self)
        global pddf_obj
        global plugin_data
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)) + '/../pddf/pd-plugin.json')) as pd:
            plugin_data = json.load(pd)

        pddf_obj = pddfparse.PddfParse()
        self.platform = pddf_obj.get_platform()

    def get_num_psus(self):
        return int(self.platform['num_psus'])

    def get_psu_status(self, index):
        if index is None:
            return False

        device = "PSU" + "%d" % index
        output = pddf_obj.get_attr_name_output(device, "psu_power_good")
        if not output:
            return False

        mode = output['mode']
        val = output['status']

        val = val.rstrip()
        vmap = plugin_data['PSU']['psu_power_good'][mode]['valmap']

        if val in vmap:
            return vmap[val]
        else:
            return False

    def get_psu_presence(self, index):
        if index is None:
            return False

        status = 0
        device = "PSU" + "%d" % index
        output = pddf_obj.get_attr_name_output(device, "psu_present")
        if not output:
            return False

        mode = output['mode']
        status = output['status']

        vmap = plugin_data['PSU']['psu_present'][mode]['valmap']

        if status.rstrip('\n') in vmap:
            return vmap[status.rstrip('\n')]
        else:
            return False

    def get_powergood_status(self, idx):
        if idx is None:
            return False

        if idx < 1 or idx > self.platform['num_psus']:
            print("Invalid index %d\n" % idx)
            return False

        device = "PSU"+"%d" % (idx)
        output = pddf_obj.get_attr_name_output(device, "psu_power_good")
        if not output:
            return False

        mode = output['mode']
        status = output['status']

        vmap = plugin_data['PSU']['psu_power_good'][mode]['valmap']

        if status.rstrip('\n') in vmap:
            return vmap[status.rstrip('\n')]
        else:
            return False

    def get_model(self, idx):
        if idx is None:
            return None

        if idx < 1 or idx > self.platform['num_psus']:
            print("Invalid index %d\n" % idx)
            return None

        device = "PSU"+"%d" % (idx)
        output = pddf_obj.get_attr_name_output(device, "psu_model_name")
        if not output:
            return None

        model = output['status']

        # strip_non_ascii
        stripped = (c for c in model if 0 < ord(c) < 127)
        model = ''.join(stripped)

        return model.rstrip('\n')

    def get_mfr_id(self, idx):
        if idx is None:
            return None

        if idx < 1 or idx > self.platform['num_psus']:
            print("Invalid index %d\n" % idx)
            return None

        device = "PSU"+"%d" % (idx)
        output = pddf_obj.get_attr_name_output(device, "psu_mfr_id")
        if not output:
            return None

        mfr = output['status']

        return mfr.rstrip('\n')

    def get_serial(self, idx):
        if idx is None:
            return None

        if idx < 1 or idx > self.platform['num_psus']:
            print("Invalid index %d\n" % idx)
            return None

        device = "PSU"+"%d" % (idx)
        output = pddf_obj.get_attr_name_output(device, "psu_serial_num")
        if not output:
            return None

        serial = output['status']

        return serial.rstrip('\n')

    def get_direction(self, idx):
        if idx is None:
            return None

        if idx < 1 or idx > self.platform['num_psus']:
            print("Invalid index %d\n" % idx)
            return None

        device = "PSU"+"%d" % (idx)
        output = pddf_obj.get_attr_name_output(device, "psu_fan_dir")
        if not output:
            return None

        mode = output['mode']
        direction = output['status'].rstrip('\n')

        vmap = plugin_data['PSU']['psu_fan_dir'][mode]['valmap']
        if direction in vmap:
            airflow_dir_real = vmap[direction]
        else:
            airflow_dir_real = direction

        return airflow_dir_real

    def get_output_voltage(self, idx):
        if idx is None:
            return 0.0

        if idx < 1 or idx > self.platform['num_psus']:
            print("Invalid index %d\n" % idx)
            return 0.0

        device = "PSU"+"%d" % (idx)
        output = pddf_obj.get_attr_name_output(device, "psu_v_out")
        if not output:
            return 0.0

        v_out = output['status']

        # value returned by the psu driver is in mV
        return float(v_out)/1000

    def get_output_current(self, idx):
        if idx is None:
            return 0.0

        if idx < 1 or idx > self.platform['num_psus']:
            print("Invalid index %d\n" % idx)
            return 0.0

        device = "PSU"+"%d" % (idx)
        output = pddf_obj.get_attr_name_output(device, "psu_i_out")
        if not output:
            return 0.0

        i_out = output['status']

        # current in mA
        return float(i_out)/1000

    def get_output_power(self, idx):
        if idx is None:
            return 0.0

        if idx < 1 or idx > self.platform['num_psus']:
            print("Invalid index %d\n" % idx)
            return 0.0

        device = "PSU"+"%d" % (idx)
        output = pddf_obj.get_attr_name_output(device, "psu_p_out")
        if not output:
            return 0.0

        p_out = output['status']

        # power is returned in micro watts
        return float(p_out)/1000000

    def get_fan_rpm(self, idx, fan_idx):
        if idx is None or fan_idx is None:
            return 0

        if idx < 1 or idx > self.platform['num_psus']:
            print("Invalid index %d\n" % idx)
            return 0

        device = "PSU"+"%d" % (idx)
        num_fans = pddf_obj.get_num_psu_fans(device)

        if fan_idx < 1 or fan_idx > num_fans:
            print("Invalid PSU-fan index %d\n" % fan_idx)
            return 0

        output = pddf_obj.get_attr_name_output(device, "psu_fan"+str(fan_idx)+"_speed_rpm")
        if not output:
            return 0

        #mode = output['mode']
        output['status'] = output['status'].rstrip()
        if output['status'].isalpha():
            return 0
        else:
            speed = int(output['status'])

        return speed

    def dump_sysfs(self):
        return pddf_obj.cli_dump_dsysfs('psu')
