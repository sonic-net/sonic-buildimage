# Platform-specific LED control functionality for SONiC
#

# try:
#     from sonic_led.led_control_base import LedControlBase
#     from swsscommon import swsscommon
# except ImportError as e:
#     raise ImportError (str(e) + " - required module not found")

import time


class LedControlBase(object):
    #    __metaclass__ = abc.ABCMeta

    #    @abc.abstractmethod
    def port_link_state_change(self, port, state):
        """
        Called when port link state changes. Update port link state LED here.

        :param port: A string, SONiC port name (e.shg., "Ethernet0")
        :param state: A string, the port link state (either "up" or "down")
        """
        return


### Zion specified ###
read_fan_fault = 0
is_fan_all_OK = 0
read_power_status = 0
is_power_all_OK = 0
is_thermal_high = 0
is_reset_button_push = 0
##########################


def sysled_task():
    while True:
        system_led_check()
        time.sleep(5)

### Zion specified ###


def system_led_check():
    global read_fan_fault, read_power_status, is_fan_all_OK, is_power_all_OK, is_thermal_high, is_reset_button_push

    is_fan_all_OK = 1
    is_power_all_OK = 0
    is_thermal_high = 0
    is_reset_button_push = 0
    with open("/sys/bus/i2c/devices/1-005e/fan1_fault", "r") as f1:
        read_fan_fault = f1.read()
        with open("/sys/bus/i2c/devices/9-005f/fan1_led", "w") as f11:
            if str(read_fan_fault) == str("1\n"):
                is_fan_all_OK = 0
                f11.write("4")
            else:
                f11.write("1")
    with open("/sys/bus/i2c/devices/1-005e/fan2_fault", "r") as f1:
        read_fan_fault = f1.read()
        with open("/sys/bus/i2c/devices/9-005f/fan2_led", "w") as f11:
            if str(read_fan_fault) == str("1\n"):
                is_fan_all_OK = 0
                f11.write("4")
            else:
                f11.write("1")
    with open("/sys/bus/i2c/devices/1-005e/fan3_fault", "r") as f1:
        read_fan_fault = f1.read()
        with open("/sys/bus/i2c/devices/9-005f/fan3_led", "w") as f11:
            if str(read_fan_fault) == str("1\n"):
                is_fan_all_OK = 0
                f11.write("4")
            else:
                f11.write("1")
    with open("/sys/bus/i2c/devices/1-005e/fan4_fault", "r") as f1:
        read_fan_fault = f1.read()
        with open("/sys/bus/i2c/devices/9-005f/fan4_led", "w") as f11:
            if str(read_fan_fault) == str("1\n"):
                is_fan_all_OK = 0
                f11.write("4")
            else:
                f11.write("1")
    with open("/sys/bus/i2c/devices/1-005e/fan5_fault", "r") as f1:
        read_fan_fault = f1.read()
        with open("/sys/bus/i2c/devices/9-005f/fan5_led", "w") as f11:
            if str(read_fan_fault) == str("1\n"):
                is_fan_all_OK = 0
                f11.write("4")
            else:
                f11.write("1")
    with open("/sys/bus/i2c/devices/1-005e/fan6_fault", "r") as f1:
        read_fan_fault = f1.read()
        with open("/sys/bus/i2c/devices/9-005f/fan6_led", "w") as f11:
            if str(read_fan_fault) == str("1\n"):
                is_fan_all_OK = 0
                f11.write("4")
            else:
                f11.write("1")

    with open("/sys/bus/i2c/devices/1-005e/psu1_power_good", "r") as f1:
        read_power_status = f1.read()
        with open("/sys/bus/i2c/devices/9-005f/sys_pwr", "w") as f11:
            if str(read_power_status) == str("1\n"):
                f11.write("1")
            else:
                f11.write("4")
    with open("/sys/bus/i2c/devices/1-005e/psu1_present", "r") as f1:
        read_power_status = f1.read()
        with open("/sys/bus/i2c/devices/9-005f/sys_pwr", "w") as f11:
            if str(read_power_status) == str("1\n"):
                is_power_all_OK = is_power_all_OK + 1
                f11.write("1")
            else:
                f11.write("4")
    with open("/sys/bus/i2c/devices/1-005e/psu2_power_good", "r") as f1:
        read_power_status = f1.read()
        with open("/sys/bus/i2c/devices/9-005f/sys_pwr", "w") as f11:
            if str(read_power_status) == str("1\n"):
                f11.write("1")
            else:
                f11.write("4")
    with open("/sys/bus/i2c/devices/1-005e/psu2_present", "r") as f1:
        read_power_status = f1.read()
        with open("/sys/bus/i2c/devices/9-005f/sys_pwr", "w") as f11:
            if str(read_power_status) == str("1\n"):
                is_power_all_OK = is_power_all_OK + 1
                f11.write("1")
            else:
                f11.write("4")

    with open("/sys/bus/i2c/devices/9-005f/swi_ctrl", "r") as f5:
        is_reset_button_push = f5.read()
        if str(is_reset_button_push) == "1\n":
            is_reset_button_push = 1
        else:
            is_reset_button_push = 0

    with open("/sys/bus/i2c/devices/4-004d/hwmon/hwmon3/temp1_input", "r") as f3:
        is_thermal_high = f3.read()
        if int(is_thermal_high) >= 70000:
            is_thermal_high = 1
        else:
            is_thermal_high = 0

    with open("/sys/bus/i2c/devices/9-005f/sys_status", "w") as f2:
        if is_reset_button_push == 1:
            f2.write("3")
        elif is_fan_all_OK == 0 or is_power_all_OK == 0 or is_thermal_high == 1:
            f2.write("4")
        else:
            f2.write("1")

    return
##########


class LedControl(LedControlBase):
    """Platform specific LED control class"""
    PORT_TABLE_PREFIX = "PORT_TABLE:"

    SONIC_PORT_NAME_PREFIX = "Ethernet"

    LED_SYSFS_PATH_BREAKOUT_CAPABLE = "/sys/class/leds/qsfp{0}_{1}/brightness"
    LED_SYSFS_PATH_NO_BREAKOUT = "/sys/class/leds/qsfp{0}/brightness"

    QSFP_BREAKOUT_START_IDX = 1
    QSFP_BREAKOUT_END_IDX = 24
    QSFP_NO_BREAKOUT_START_IDX = 25
    QSFP_NO_BREAKOUT_END_IDX = 32

    LED_COLOR_OFF = 0
    LED_COLOR_GREEN = 1
    LED_COLOR_YELLOW = 2

    # Helper method to map SONiC port name to Arista QSFP index
    def _port_name_to_qsfp_index(self, port_name):
        # Strip "Ethernet" off port name
        if not port_name.startswith(self.SONIC_PORT_NAME_PREFIX):
            return -1

        sonic_port_num = int(port_name[len(self.SONIC_PORT_NAME_PREFIX):])

        swss = swsscommon.SonicV2Connector()
        swss.connect(swss.APPL_DB)

        lanes = swss.get(
            swss.APPL_DB, self.PORT_TABLE_PREFIX + port_name, 'lanes')

        # SonicV2Connector.get() will return None when key does not exist.
        if lanes:
            lanes_len = len(lanes.split(','))
        else:
            lanes_len = 0

        # SONiC port nums are 0-based and increment by 4
        # Arista QSFP indices are 1-based and increment by 1
        return (((sonic_port_num/4) + 1), sonic_port_num % 4, lanes_len)

    # Concrete implementation of port_link_state_change() method

    def port_link_state_change_bk(self, port, state):
        qsfp_index, lane_index, lanes = self._port_name_to_qsfp_index(port)

        # Ignore invalid QSFP indices
        if qsfp_index <= 0 or lanes <= 0 or lanes > 4:
            return

        # QSFP indices 1-24 are breakout-capable and have four LEDs, and each LED indicate one lane.
        # whereas indices 25-32 are not breakout-capable, and only have one
        if qsfp_index <= self.QSFP_BREAKOUT_END_IDX:
            # assuming 40G, then we need to control four lanes
            led_sysfs_paths = [self.LED_SYSFS_PATH_BREAKOUT_CAPABLE.format(
                qsfp_index, i) for i in range(lane_index + 1, lane_index + 1 + lanes)]
        else:
            led_sysfs_paths = [
                self.LED_SYSFS_PATH_NO_BREAKOUT.format(qsfp_index)]

        for led_sysfs_path in led_sysfs_paths:
            led_file = open(led_sysfs_path, "w")

            if state == "up":
                led_file.write("%d" % self.LED_COLOR_GREEN)
            else:
                led_file.write("%d" % self.LED_COLOR_OFF)

            led_file.close()

    # Constructor
    def __init__(self):
        # Initialize all front-panel status LEDs to green
        with open("/sys/bus/i2c/devices/9-005f/sys_locator", "w") as f:
            f.write("0")
        with open("/sys/bus/i2c/devices/9-005f/sys_pwr", "w") as f:
            f.write("1")
        with open("/sys/bus/i2c/devices/9-005f/sys_status", "w") as f:
            f.write("1")

        # Initialize all fan LEDs to green
        with open("/sys/bus/i2c/devices/9-005f/fan1_led", "w") as f:
            f.write("1")
        with open("/sys/bus/i2c/devices/9-005f/fan2_led", "w") as f:
            f.write("1")
        with open("/sys/bus/i2c/devices/9-005f/fan3_led", "w") as f:
            f.write("1")
        with open("/sys/bus/i2c/devices/9-005f/fan4_led", "w") as f:
            f.write("1")
        with open("/sys/bus/i2c/devices/9-005f/fan5_led", "w") as f:
            f.write("1")
        with open("/sys/bus/i2c/devices/9-005f/fan6_led", "w") as f:
            f.write("1")
        sysled_task()

        # Initialize: Turn all front panel QSFP LEDs off
        # # for qsfp_index in range(self.QSFP_BREAKOUT_START_IDX, self.QSFP_BREAKOUT_END_IDX + 1):
        # #     for lane in range(1, 5):
        # #         led_sysfs_path = self.LED_SYSFS_PATH_BREAKOUT_CAPABLE.format(qsfp_index, lane)
        # #         with open(led_sysfs_path, 'w') as led_file:
        # #             led_file.write("%d" % self.LED_COLOR_OFF)

        # # for qsfp_index in range(self.QSFP_NO_BREAKOUT_START_IDX, self.QSFP_NO_BREAKOUT_END_IDX + 1):
        # #     led_sysfs_path = self.LED_SYSFS_PATH_NO_BREAKOUT.format(qsfp_index)
        # #     with open(led_sysfs_path, 'w') as led_file:
        # #         led_file.write("%d" % self.LED_COLOR_OFF)
