import sys
sys.path.append('/usr/share/sonic/platform/plugins')
import pddfparse


class LedUtil:
    color_map = {
        "STATUS_LED_COLOR_GREEN": "on",
        "STATUS_LED_COLOR_RED": "faulty",
        "STATUS_LED_COLOR_OFF": "off"
    }

    def __init__(self):
        global pddf_obj
        pddf_obj = pddfparse.PddfParse()
        self.path = "pddf/devices/led"
        self.cur_state_path = "pddf/devices/led/cur_state"

    def set_status_led(self, led_device_name, color, color_state="SOLID"):
        if (not led_device_name in list(pddf_obj.data.keys())):
            status = "ERROR: " + led_device_name + " is not configured"
            return (status)

        if (not color in list(self.color_map.keys())):
            status = "ERROR: Invalid color"
            return (status)

        index = pddf_obj.data[led_device_name]['dev_attr']['index']
        pddf_obj.create_attr('device_name', led_device_name,  self.path)
        pddf_obj.create_attr('index', index, self.path)
        pddf_obj.create_attr(
            'color', self.color_map[color], self.cur_state_path)
        pddf_obj.create_attr('color_state', color_state, self.cur_state_path)
        pddf_obj.create_attr('dev_ops', 'set_status',  self.path)
        return ("Executed")

    def get_status_led(self, led_device_name):
        if (not led_device_name in list(pddf_obj.data.keys())):
            status = "ERROR: " + led_device_name + " is not configured"
            return (status)

        index = pddf_obj.data[led_device_name]['dev_attr']['index']
        pddf_obj.create_attr('device_name', led_device_name,  self.path)
        pddf_obj.create_attr('index', index, self.path)
        pddf_obj.create_attr('dev_ops', 'get_status',  self.path)
        color_f = "/sys/kernel/" + self.cur_state_path + "/color"
        color_state_f = "/sys/kernel/" + self.cur_state_path + "/color_state"

        try:
            with open(color_f, 'r') as f:
                color = f.read().strip("\r\n")
            with open(color_state_f, 'r') as f:
                color_state = f.read().strip("\r\n")
        except IOError:
            status = "ERROR :" + color_f + " open failed"
            return (status)
        status = "%s-%s:\t%s %s\n" % (led_device_name,
                                      index, color, color_state)
        return (status)
