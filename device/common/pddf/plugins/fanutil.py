# Sample pddf_fanutil file
# All the supported FAN SysFS aattributes are
#- fan<idx>_present
#- fan<idx>_direction
#- fan<idx>_input
#- fan<idx>_pwm
#- fan<idx>_fault
# where idx is in the range [1-12]
#


import os.path
import sys
import ast
sys.path.append('/usr/share/sonic/platform/plugins')
import pddfparse
import json

try:
    from sonic_fan.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanUtil(FanBase):
    """PDDF generic FAN util class"""

    def __init__(self):
        FanBase.__init__(self)
        global pddf_obj
        global plugin_data
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)) + '/../pddf/pd-plugin.json')) as pd:
            plugin_data = json.load(pd)

        pddf_obj = pddfparse.PddfParse()
        self.platform = pddf_obj.get_platform()

        self.num_fans = (self.platform['num_fantrays'] * self.platform['num_fans_pertray'])

    def get_num_fans(self):
        return self.num_fans

    def get_presence(self, idx):
        # 1 based fan index
        if idx < 1 or idx > self.num_fans:
            print("Invalid fan index %d\n" % idx)
            return False

        attr_name = "fan" + str(idx) + "_present"
        output = pddf_obj.get_attr_name_output("FAN-CTRL", attr_name)
        if not output:
            return False

        mode = output['mode']
        presence = output['status'].rstrip()

        vmap = plugin_data['FAN']['present'][mode]['valmap']

        if presence in vmap:
            status = vmap[presence]
        else:
            status = False

        return status

    def get_status(self, idx):
        # 1 based fan index
        if idx < 1 or idx > self.num_fans:
            print("Invalid fan index %d\n" % idx)
            return False

        speed = self.get_speed(idx)
        status = True if (speed != 0) else False
        return status

    def get_direction(self, idx):
        # 1 based fan index
        if idx < 1 or idx > self.num_fans:
            print("Invalid fan index %d\n" % idx)
            return None

        attr = "fan" + str(idx) + "_direction"
        output = pddf_obj.get_attr_name_output("FAN-CTRL", attr)
        if not output:
            return None

        mode = output['mode']
        val = output['status']

        val = val.rstrip()
        vmap = plugin_data['FAN']['direction'][mode]['valmap']

        if val in vmap:
            direction = vmap[val]
        else:
            direction = val

        return direction

    def get_directions(self):
        num_fan = self.get_num_fan()

        for i in range(1, num_fan+1):
            attr = "fan" + str(i) + "_direction"
            output = pddf_obj.get_attr_name_output("FAN-CTRL", attr)
            if not output:
                return None

            mode = output['mode']
            val = output['status']

            val = val.rstrip()
            vmap = plugin_data['FAN']['direction'][mode]['valmap']

            direction = vmap[str(val)]

            print("FAN-%d direction is %s" % (i, direction))

        return 0

    def get_speed(self, idx):
        # 1 based fan index
        if idx < 1 or idx > self.num_fans:
            print("Invalid fan index %d\n" % idx)
            return 0

        attr = "fan" + str(idx) + "_input"
        output = pddf_obj.get_attr_name_output("FAN-CTRL", attr)
        if not output:
            return 0

        #mode = output['mode']
        val = output['status'].rstrip()

        if val.isalpha():
            return 0
        else:
            rpm_speed = int(float(val))

        return rpm_speed

    def get_speeds(self):
        num_fan = self.get_num_fan()
        ret = "FAN_INDEX\t\tRPM\n"

        for i in range(1, num_fan+1):
            attr1 = "fan" + str(i) + "_input"
            output = pddf_obj.get_attr_name_output("FAN-CTRL", attr1)
            if not output:
                return ""

            #mode = output['mode']
            val = output['status'].rstrip()

            if val.isalpha():
                frpm = 0
            else:
                frpm = int(val)

            ret += "FAN-%d\t\t\t%d\n" % (i, frpm)

        return ret

    def set_speed(self, val):
        if val < 0 or val > 100:
            print("Error: Invalid speed %d. Please provide a valid speed percentage" % val)
            return False

        num_fan = self.num_fans
        if 'duty_cycle_to_pwm' not in plugin_data['FAN']:
            print("Setting fan speed is not allowed !")
            return False
        else:
            duty_cycle_to_pwm = ast.literal_eval(plugin_data['FAN']['duty_cycle_to_pwm'])
            pwm = duty_cycle_to_pwm(val)
            print("New Speed: %d%% - PWM value to be set is %d\n" % (val, pwm))

            for i in range(1, num_fan+1):
                attr = "fan" + str(i) + "_pwm"
                node = pddf_obj.get_path("FAN-CTRL", attr)
                if node is None:
                    return False
                try:
                    with open(node, 'w') as f:
                        f.write(str(pwm))
                except IOError:
                    return False

        return True

    def dump_sysfs(self):
        return pddf_obj.cli_dump_dsysfs('fan')

    def get_change_event(self):
        """
        TODO: This function need to be implemented
        when decide to support monitoring FAN(fand)
        on this platform.
        """
        raise NotImplementedError
