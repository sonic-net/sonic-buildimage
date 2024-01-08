#!/usr/bin/env python3
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import common

TOTAL_PORT_NUM = 56
SFP_MAX_NUM = 0
CPLDA_SFP_NUM = 0
CPLDB_SFP_NUM = 28
CPLDC_SFP_NUM = 28
FAN_NUM = 6
MOTOR_NUM_PER_FAN = 1
CPLD_NUM = 4
PSU_NUM = 2
TEMP_SENSOR_NUM_PER_PSU = 3

THERMAL_SENSOR_NUM = 8
NEED_WORK = 5
PATH_INDEX = 5
SYSSENSOR_PREFIX = '/sys/switch/sensor/'
SENSOR_PATH = SYSSENSOR_PREFIX+"temp"+'{}'+'/'

THERMAL_NODE_COMBINATION = (
    "temp_input",
    "temp_max_hyst",
    "temp_max",
    "temp_alias",
    "temp_type",
)

THERMAL_COMBINATION = {
    0: (
        "temp1_input",
        "temp1_max",
        "temp1_crit",
        "Ambient MAC side",
        "LM75BD / Ambient MAC side",
        "/sys/bus/i2c/devices/4-0070/hwmon/hwmon4/",
    ),
    1: (
        "temp2_input",
        "temp2_max",
        "temp2_crit",
        "Ambient MAC",
        "ASIC",
        "/sys/bus/i2c/devices/4-0070/hwmon/hwmon4/",
    ),
    2: (
        "temp3_input",
        "temp3_max",
        "temp3_crit",
        "Ambient FAN",
        "LM75BD / Ambient FAN",
        "/sys/bus/i2c/devices/4-0070/hwmon/hwmon4/",
    ),
    3: (
        "temp1_input",
        "temp1_max",
        "temp1_max_hyst",
        "Ambient NPU",
        "LM75BD / Ambient NPU",
        "/sys/bus/i2c/devices/1-004a/hwmon/hwmon1/",
    ),
    4: (
        "temp2_input",
        "temp2_max",
        "temp2_crit",
        "CPU",
        "Core 0",
        "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
    ),
    5: (
        "temp3_input",
        "temp3_max",
        "temp3_crit",
        "CPU",
        "Core 1",
        "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
    ),
    6: (
        "temp4_input",
        "temp4_max",
        "temp4_crit",
        "CPU",
        "Core 2",
        "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
    ),
    7: (
        "temp5_input",
        "temp5_max",
        "temp5_crit",
        "CPU",
        "Core 3",
        "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
    ),
}

DEVICE_BUS = {'cpld': ['6-0074', '7-0075', '8-0076'],
        'fan': ['4-0070'],
        'psu': ['2-0058', '3-0059'],
        'status_led': ['6-0074'],
        'fp_0_led': ['7-0075'],
        'fp_1_led': ['8-0076']
        }
DEVICE_BUS_FAN_NODE = ['4-0070/hwmon/hwmon4/']
DEVICE_BUS_PSU_NODE = ['2-0058/hwmon/hwmon2/', '3-0059/hwmon/hwmon3/']

class DeviceThread(common.threading.Thread):
    def __init__(self,threadname, q):
        common.threading.Thread.__init__(self,name = threadname)
        self.queue = q

    def run(self):
        while common.RUN:
            message = self.queue.get()
            self.onMessage(message)

    def onMessage(self, message):
        """
        Commands:
            led        : Led controls
            locate        : Blink locator LED
            sensors        : Read HW monitors
        """

        if len(message.command) < 1:
            result = self.onMessage.__doc__
        else:
            if message.command[0] == 'init':
                result = deviceInit()
            elif message.command[0] == 'led':
                result = ledControls(message.command[1:])
            elif message.command[0] == 'locate':
                locatethread = common.threading.Thread(target = locateDeviceLed)
                locatethread.start()
                result = 'Success'
            elif message.command[0] == 'sensors':
                result = getSensors()
            else:
                result = self.onMessage.__doc__

        if (message.callback is not None):
             message.callback(result)

STATUS_ALERT = {'fan': ['wrongAirflow_alert', 'outerRPMOver_alert', 'outerRPMUnder_alert', 'outerRPMZero_alert',
            'innerRPMOver_alert', 'innerRPMUnder_alert', 'innerRPMZero_alert', 'notconnect_alert'],
        'psu': ['vout_over_voltage', 'iout_over_current_fault', 'iout_over_current_warning',
            'iput_over_current_warning', 'iput_insufficient', 'temp_over_temp_fault', 'temp_over_temp_warning']}
class PlatformStatusThread(common.threading.Thread):
    def __init__(self,threadname, timer):
        self.running = True
        common.threading.Thread.__init__(self,name = threadname)
        self.timer = timer
        self.fan_led_status = 'off'
        self.psu_led_status = 'off'

    def run(self):
        while common.RUN:
            self.checkPlatformStatus()
            common.time.sleep(self.timer)

    def checkPlatformStatus(self):
        total_result = common.PASS
        total_result += self.checkFanStatus()
        total_result += self.checkPsuStatus()

    def checkFanStatus(self):
        fan_result = common.PASS
        fan_bus = DEVICE_BUS['fan']
        fan_bus_nodes = DEVICE_BUS_FAN_NODE
        fan_alert = STATUS_ALERT['fan']
        fan_led = LED_COMMAND['fan_led']
        fan_normal = 'green'
        fan_abnormal = 'blink_amber'
        led_bus = DEVICE_BUS['status_led']
        led_path = common.I2C_PREFIX + led_bus[0] + '/' + LED_NODES[3]

        status, output = common.doBash("ls " + common.I2C_PREFIX)
        if output.find(fan_bus[0]) != -1:
            for num in range(0,FAN_NUM):
                for alert_type in fan_alert:
                    path = common.I2C_PREFIX + fan_bus_nodes[0] + "/fan" + str(num+1) + "_" + alert_type
                    result = common.readFile(path)
                    if result != 'Error':
                        fan_result += int(result)
        if fan_result != common.PASS:
            if self.fan_led_status != fan_abnormal:
                common.writeFile(led_path, fan_led[fan_abnormal])
                self.fan_led_status = fan_abnormal
                common.syslog.syslog(common.syslog.LOG_ERR, 'FAN Status Error !!!')
            return common.FAIL

        if self.fan_led_status != fan_normal:
            common.writeFile(led_path, fan_led[fan_normal])
            self.fan_led_status = fan_normal
            common.syslog.syslog(common.syslog.LOG_ERR, 'FAN Status Normal !!!')
        return common.PASS

    def checkPsuStatus(self):
        psu_result = common.PASS
        psu_bus = DEVICE_BUS['psu']
        psu_bus_nodes = DEVICE_BUS_PSU_NODE
        psu_alert = STATUS_ALERT['psu']
        psu_led = LED_COMMAND['pwr_led']
        psu_normal = 'green'
        psu_abnormal = 'blink_amber'
        led_bus = DEVICE_BUS['status_led']
        led_path = common.I2C_PREFIX + led_bus[0] + '/' + LED_NODES[1]

        status, output = common.doBash("ls " + common.I2C_PREFIX)
        if output.find(psu_bus[0]) != -1 and output.find(psu_bus[1]) != -1:
            for nodes in psu_bus_nodes:
                for alert_type in psu_alert:
                    path = common.I2C_PREFIX + nodes + "/" + alert_type
                    result = common.readFile(path)
                    if result != 'Error':
                        psu_result += int(result)
        if psu_result != common.PASS:
            if self.psu_led_status != psu_abnormal:
                common.writeFile(led_path, psu_led[psu_abnormal])
                self.psu_led_status = psu_abnormal
                common.syslog.syslog(common.syslog.LOG_ERR, 'PSU Status Error !!!')
            return common.FAIL

        if self.psu_led_status != psu_normal:
            common.writeFile(led_path, psu_led[psu_normal])
            self.psu_led_status = psu_normal
            common.syslog.syslog(common.syslog.LOG_ERR, 'PSU Status Normal !!!')
        return common.PASS

LED_COMMAND = {'sys_led': {'green':'1', 'amber':'3', 'off':'0', 'blink_green':'4', 'blink_amber':'6'},
        'pwr_led': {'green':'1', 'amber':'3', 'off':'0', 'blink_green':'4', 'blink_amber':'6'},
        'loc_led': {'on':'0', 'off':'1', 'blink':'2'},
        'fan_led': {'green':'1', 'amber':'3', 'off':'0', 'blink_green':'4', 'blink_amber':'6'},
        'cpld_allled_ctrl': {'off':'3', 'mix':'1', 'amber':'2', 'normal':'0'},
        'fp_0_led': {'disable':'0', 'enable':'1'},
        'fp_1_led': {'disable':'0', 'enable':'1'},
        'serial_led_enable': {'disable':'0', 'enable':'1'}}
LED_NODES = ['sys_led', 'pwr_led', 'loc_led', 'fan_led', "cpld_allled_ctrl", "serial_led_enable"]
def ledControls(args):
    """
    Commands:
        set        : Set led config
        get        : Get led status
    """
    COMMAND_TYPE = ['set', 'get']
    if len(args) < 1 or args[0] not in COMMAND_TYPE:
        return ledControls.__doc__

    result = setGetLed(args[0:])

    return result

def setGetLed(args):
    """
    Commands:
        sys_led        : System status led [green/amber/off/blink_green/blink_amber]
        pwr_led        : Power status led [green/amber/off/blink_green/blink_amber]
        loc_led        : Locator led [on/off/blink]
        fan_led        : Fan led [green/amber/off/blink_green/blink_amber]
    """
    if len(args) < 3 or args[1] not in LED_COMMAND:
        return setGetLed.__doc__

    for i in range(0,len(LED_NODES)):
        if args[1] == LED_NODES[i]:
            led_bus = DEVICE_BUS['status_led']
            path = common.I2C_PREFIX + led_bus[0] + '/' + LED_NODES[i]
        if args[1] == 'fp_0_led':
            led_bus = DEVICE_BUS['fp_0_led']
            path = common.I2C_PREFIX + led_bus[0] + '/' + 'serial_led_enable'
        if args[1] == 'fp_1_led':
            led_bus = DEVICE_BUS['fp_1_led']
            path = common.I2C_PREFIX + led_bus[0] + '/' + 'serial_led_enable'
        command = LED_COMMAND[args[1]]

    if args[0] == 'set':
        if args[2] in command:
            data = command[args[2]]
            result = common.writeFile(path, data)
        else:
            result = setGetLed.__doc__
    else:
        result = common.readFile(node)
        if result != "Error":
            result = list (command.keys()) [list (command.values()).index (result)]

    return result

def locateDeviceLed():
    setGetLed(['set', 'loc_led', 'blink'])
    common.time.sleep(20)
    setGetLed(['set', 'loc_led', 'off'])

SENSORS_PATH = common.I2C_PREFIX + '4-0070/hwmon/hwmon4/' #maurice arbiter
SENSORS_NODES = {'fan_rpm': ['_inner_rpm', '_outer_rpm'],
        'fan_vol': ['ADC1_vol', 'ADC2_vol','ADC3_vol', 'ADC4_vol','ADC5_vol', 'ADC6_vol', 'ADC7_vol', 'ADC8_vol'],
        'temp':['temp1_input', 'temp2_input', 'temp3_input'],
        'fan_alert':['_status_alert', '_wrongAirflow_alert', '_outerRPMOver_alert', '_outerRPMUnder_alert',
                '_outerRPMZero_alert', '_innerRPMOver_alert', '_innerRPMUnder_alert', '_innerRPMZero_alert', '_notconnect_alert'],
        'vol_alert':['_under_alert', '_over_alert'],
        'temp_alert':['lm75_48_temp_alert', 'lm75_49_temp_alert', 'lm75_4a_temp_alert', 'sa56004x_Ltemp_alert', 'sa56004x_Rtemp_alert']}
SENSORS_TYPE = {'fan_rpm': ['Inner RPM', 'Outer RPM'],
        'fan_vol': ['P0.1', 'P0.2','P0.6', 'P0.7','P1.0', 'P1.1', 'P1.2', 'P1.6']}
def getSensors():
    string = ''
    # Firmware version
    val = common.readFile(SENSORS_PATH + 'mb_fw_version')
    string = '\n' + "MB-SW Version: " + val

    val = common.readFile(SENSORS_PATH + 'fb_fw_version')
    string += '\n' + "FB-SW Version: " + val

    # Fan
    string += getFan()

    # HW Monitor
    #string += '\n' + getHWMonitor()

    # Voltage
    string += '\n' + getVoltage()

    return string

def getFan():
    string = ''
    for i in range(0,FAN_NUM):
        # Status
        result = getFanStatus(i)
        string += '\n\n' + "FAN " + str(i+1) + ": " + result

        if result == 'Disconnect':
            continue

        # Alert
        result = getFanAlert(i)
        string += '\n' + "     Status: " + result

        # Inner RPM
        result = getFanInnerRPM(i)
        string += '\n' + "  Inner RPM: " + result.rjust(10) + " RPM"

        # Outer RPM
        result = getFanOuterRPM(i)
        string += '\n' + "  Outer RPM: " + result.rjust(10) + " RPM"

    return string

def getFanStatus(num):
    val = common.readFile(SENSORS_PATH + 'fan' + str(num+1) + '_present')
    if val != 'Error':
        if (int(val, 16) & 0x1) == 0x1:
            result = 'Connect'
        else:
            result = 'Disconnect'
    else:
        result = val
    return result

def getFanAlert(num):
    alert = 0
    alert_types = SENSORS_NODES['fan_alert']
    for alert_type in alert_types:
        val = common.readFile(SENSORS_PATH + 'fan' + str(num+1) + alert_type)
        if val != 'Error':
            alert += int(val, 16)
        else:
            return val

    if alert > 0:
        result = 'Warning'
    else:
        result = 'Normal'

    return result

def getFanInnerRPM(num):
    return common.readFile(SENSORS_PATH + 'fan' + str(num+1) + '_inner_rpm')

def getFanOuterRPM(num):
    return common.readFile(SENSORS_PATH + 'fan' + str(num+1) + '_outer_rpm')

def getHWMonitor():
    string = ''
    try:
        temp_type = SENSORS_TYPE['temp']
    except Exception:
        return string

    for types in temp_type:
        val = common.readFile(SENSORS_PATH + types)
        val_alert = common.readFile(SENSORS_PATH + types + '_alert')
        if val_alert != 'Error':
            if int(val_alert, 16) == 1:
                alert = 'Warning'
            else:
                alert = 'Normal'
        else:
            alert = val_alert
        string += '\n' + types + ": " + val + " C" + " ( " + alert + " )"

    return string

def getVoltage():
    string = ''
    nodes = SENSORS_NODES['fan_vol']
    types = SENSORS_TYPE['fan_vol']
    for i in range(0,len(nodes)):
        val = common.readFile(SENSORS_PATH + nodes[i])
        alert = getVoltageAlert(i)
        string += '\n' + types[i] + ": " + val + " V ( " + alert + " )"

    return string

def getVoltageAlert(num):
    alert = 0
    nodes = SENSORS_NODES['vol_alert']
    for node in nodes:
        val = common.readFile(SENSORS_PATH + 'ADC' + str(num+1) + node)
        if val != 'Error':
            alert += int(val, 16)
        else:
            return val

    if alert > 0:
        result = 'Warning'
    else:
        result = 'Normal'

    return result

DEVICE_INIT = {'led': [['set', 'sys_led', 'green'], ['set', 'pwr_led', 'green'], ['set', 'fan_led', 'green'], ['set', 'cpld_allled_ctrl', 'normal'], ['set', 'serial_led_enable', 'enable'], ['set', 'fp_0_led', 'enable'], ['set', 'fp_1_led', 'enable']]}
def deviceInit():
    # Set led
    for i in range(0,len(DEVICE_INIT['led'])):
        setGetLed(DEVICE_INIT['led'][i])

        cpld_bus = DEVICE_BUS['cpld']
    # Set QSFP reset to normal
    for x in range(SFP_MAX_NUM, TOTAL_PORT_NUM):
        if x < CPLDB_SFP_NUM:
            bus = cpld_bus[1]
        else:
            bus = cpld_bus[2]

        path = common.I2C_PREFIX + bus + '/sfp' + str(x+1) + '_reset'
        common.writeFile(path, "0")

    # Set QSFP lpmode to high
    for x in range(SFP_MAX_NUM, TOTAL_PORT_NUM):
        if x < CPLDB_SFP_NUM:
            bus = cpld_bus[1]
        else:
            bus = cpld_bus[2]

        path = common.I2C_PREFIX + bus + '/sfp' + str(x+1) + '_lowpower'
        common.writeFile(path, "0")

    return
