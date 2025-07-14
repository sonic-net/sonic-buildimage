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

import common, device
#import fwupdates
import os

HOST = '127.0.0.1'
SOCKET_LIST = []
SOCKET_MAX_CLIENT = 10
QUEUES = []
THREADS = []
FUNCS = {}

class GlobalThread(common.threading.Thread):
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
             uninstall	: Uninstall platform drivers
        """
        if len(message.command) < 1:
            result = self.onMessage.__doc__
        else:
            if message.command[0] == 'uninstall':
                common.RUN = False
                doUninstall()
                result = 'Success'
            else:
                result = self.onMessage.__doc__
        if (message.callback is not None):
            message.callback(result)

class messageObject(object):
    def __init__(self, command, callback):
        super(messageObject, self).__init__()
        self.command = command
        self.callback = callback

def callback(sock, result):
	sock.sendall(result.encode(encoding='utf-8'))

def messageHandler():
    server_socket = common.socket.socket(common.socket.AF_INET, common.socket.SOCK_STREAM)
    server_socket.setsockopt(common.socket.SOL_SOCKET, common.socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, common.SOCKET_PORT))
    server_socket.listen(SOCKET_MAX_CLIENT)
    SOCKET_LIST.append(server_socket)

    while(common.RUN):
        ready_to_read,ready_to_write,in_error = common.select.select(SOCKET_LIST,[],[],0)
        for sock in ready_to_read:
            if sock == server_socket:
                sockfd, addr = server_socket.accept()
                SOCKET_LIST.append(sockfd)
            else:
                try:
                    data = sock.recv(common.SOCKET_RECV_BUFFER).decode(encoding='utf-8')
                    if data:
                        cb = common.partial(callback, sock)
                        cmdlist = data.split()

                        if cmdlist[0] not in common.CMD_TYPE:
                            callback(sock, 'Fail')
                            continue

                        msg = messageObject(cmdlist[1:], cb)
                        FUNCS[cmdlist[0]].put(msg)
                        continue
                    else:
                        if sock in SOCKET_LIST:
                            SOCKET_LIST.remove(sock)
                except:
                    raise
                continue
        common.time.sleep(0.2)

    server_socket.close()

KERNEL_MODULE = [
'clounix_sysfs_main',
'i2c_dev',
'i2c-mux',
'i2c-mux-pca9641',
'i2c-mux-pca954x force_deselect_on_exit=1',
'lm75',
'pegatron_fn8656_bnf_cpld loglevel=7 requirement_flag=1',
'pegatron_hwmon_mcu loglevel=7',
'pegatron_fn8656_bnf_psu loglevel=7',
'pegatron_fn8656_bnf_sfp loglevel=7',
'pegatron_fn8656_bnf_watchdog loglevel=7',
'clounix_watchdog']
#'pegatron_fn8656_bnf_ixgbe']

def checkDriver():
    for i in range(0, len(KERNEL_MODULE)):
        status, output = common.doBash("lsmod | grep " + KERNEL_MODULE[i])
        if status:
            common.doBash("modprobe " + KERNEL_MODULE[i])
    return

i2c_topology_dict=[
    {'bus': "i2c-0", 'driver' : "pca9641", 'address': "0x71"},
    {'bus': "i2c-1", 'driver' : "fn8656_bnf_cpld", 'address': "0x18"},
    {'bus': "i2c-1", 'driver' : "pca9545", 'address': "0x72"},
    {'bus': "i2c-1", 'driver' : "pca9548", 'address': "0x73"},
    {'bus': "i2c-1", 'driver' : "lm75b",   'address': "0x4a"},
    {'bus': "i2c-2", 'driver' : "fn8656_bnf_psu", 'address': "0x58"},
    {'bus': "i2c-3", 'driver' : "fn8656_bnf_psu", 'address': "0x59"},
    {'bus': "i2c-4", 'driver' : "pega_hwmon_mcu", 'address': "0x70"},
    {'bus': "i2c-6", 'driver' : "fn8656_bnf_cpld", 'address': "0x74"},
    {'bus': "i2c-7", 'driver' : "fn8656_bnf_cpld", 'address': "0x75"},
    {'bus': "i2c-8", 'driver' : "fn8656_bnf_cpld", 'address': "0x76"},
    {'bus': "i2c-9", 'driver' : "24c02", 'address': "0x54"}
]
SOFT_LINK_NODE_TYPE = ['sensor','transceiver','psu','fan','cpld','syseeprom','sysled','watchdog','fpga']
TRANSCIEVER_PREFIX = '/sys/switch/transceiver/'
PSU_PREFIX = '/sys/switch/psu/'
FAN_PREFIX = '/sys/switch/fan/'
CPLD_PREFIX = '/sys/switch/cpld/'
SYSEEPROM_PREFIX = '/sys/switch/syseeprom/'
SYSLED_PREFIX = '/sys/switch/sysled/'
WATCHDOG_PREFIX = '/sys/switch/watchdog/'
FPGA_PREFIX = '/sys/switch/fpga/'
SENSOR_PREFIX = '/sys/switch/sensor/'

FPGA_TOP_NODE_DICT = [
    {'src_node':'/sys/module/pegatron_fn8656_bnf_sfp/parameters/loglevel',         'dest_node':FPGA_PREFIX+'loglevel'},
    {'src_node':'/sys/module/pegatron_fn8656_bnf_sfp/parameters/fpga_debug',            'dest_node':FPGA_PREFIX+'debug'},
    {'src_node':'/sys/devices/pci0000:00/0000:00:1c.4/0000:0a:00.0/'+'fpga_chip_num',       'dest_node':FPGA_PREFIX+'num'},
    {'src_node':'/sys/devices/pci0000:00/0000:00:1c.4/0000:0a:00.0/'+'fpga_alias',          'dest_node':FPGA_PREFIX+'alias'},
    {'src_node':'/sys/devices/pci0000:00/0000:00:1c.4/0000:0a:00.0/'+'fpga_type',           'dest_node':FPGA_PREFIX+'type'},
    {'src_node':'/sys/devices/pci0000:00/0000:00:1c.4/0000:0a:00.0/'+'fpga_hw_version',     'dest_node':FPGA_PREFIX+'hw_version'},
    {'src_node':'/sys/devices/pci0000:00/0000:00:1c.4/0000:0a:00.0/'+'fpga_sw_version',     'dest_node':FPGA_PREFIX+'board_version'},
]
SYSEEPROM_TOP_NODE_DICT = [
    {'src_node':'/sys/module/m24c02/parameters/loglevel',         'dest_node':SYSEEPROM_PREFIX+'loglevel'},
    {'src_node':'/sys/module/m24c02/parameters/debug',            'dest_node':SYSEEPROM_PREFIX+'debug'},
    {'src_node':'/sys/module/m24c02/parameters/bsp_version',      'dest_node':SYSEEPROM_PREFIX+'bsp_version'},
    {'src_node':common.I2C_PREFIX+'9-0054/eeprom',                'dest_node':SYSEEPROM_PREFIX+'eeprom'},
]
WATCHDOG_TOP_NODE_DICT = [
    {'src_node':'/sys/module/pegatron_fn8656_bnf_watchdog/parameters/loglevel', 'dest_node':'/sys/clx/watchdog/'+'loglevel'},
    {'src_node':'/sys/clx/watchdog',         'dest_node':'/sys/switch/watchdog'},
]
'''
WATCHDOG_TOP_NODE_DICT = [
    {'src_node':'/sys/module/clounix_watchdog/parameters/loglevel',       'dest_node':WATCHDOG_PREFIX+'loglevel'},
    {'src_node':'/sys/module/clounix_watchdog/parameters/watchdog_debug', 'dest_node':WATCHDOG_PREFIX+'debug'},
    {'src_node':'/sys/bus/platform/devices/iTCO_wdt.0.auto/watchdog/watchdog0/identity',      'dest_node':WATCHDOG_PREFIX+'identity'},
    {'src_node':'/sys/bus/platform/devices/iTCO_wdt.0.auto/watchdog/watchdog0/state',         'dest_node':WATCHDOG_PREFIX+'state'},
    {'src_node':'/sys/bus/platform/devices/iTCO_wdt.0.auto/watchdog/watchdog0/timeleft',      'dest_node':WATCHDOG_PREFIX+'timeleft'},
    {'src_node':'/sys/devices/pci0000:00/0000:00:1f.0/clounix_ctrl',         'dest_node':WATCHDOG_PREFIX+'enable'},
    {'src_node':'/sys/devices/pci0000:00/0000:00:1f.0/clounix_timeout_ctrl', 'dest_node':WATCHDOG_PREFIX+'timeout'},
    {'src_node':'/sys/devices/pci0000:00/0000:00:1f.0/clounix_ping',         'dest_node':WATCHDOG_PREFIX+'reset'},
]
'''
SYSLED_TOP_NODE_DICT = [
    {'src_node':'/sys/module/pegatron_fn8656_bnf_cpld/parameters/loglevel',         'dest_node':SYSLED_PREFIX+'loglevel'},
    {'src_node':common.I2C_PREFIX+'6-0074/debug_sysled',        'dest_node':SYSLED_PREFIX+'debug'},
    {'src_node':common.I2C_PREFIX+'6-0074/sys_led',      'dest_node':SYSLED_PREFIX+'sys_led_status'},
    #not supported{'src_node':common.I2C_PREFIX+'6-0074/',      'dest_node':SYSLED_PREFIX+'bmc_led_status'},
    {'src_node':common.I2C_PREFIX+'6-0074/fan_led',      'dest_node':SYSLED_PREFIX+'fan_led_status'},
    {'src_node':common.I2C_PREFIX+'6-0074/pwr_led',      'dest_node':SYSLED_PREFIX+'psu_led_status'},
    {'src_node':common.I2C_PREFIX+'1-0018/loc_led',      'dest_node':SYSLED_PREFIX+'id_led_status'},
]

TRANSCIEVER_TOP_NODE_DICT = [
    {'src_node':'/sys/module/pegatron_fn8656_bnf_cpld/parameters/loglevel',         'dest_node':TRANSCIEVER_PREFIX+'loglevel'},
    {'src_node':common.I2C_PREFIX+'6-0074/debug_sfp',            'dest_node':TRANSCIEVER_PREFIX+'debug'},
    {'src_node':common.I2C_PREFIX+'7-0075/sfp_port_num',         'dest_node':TRANSCIEVER_PREFIX+'num'},
    {'src_node':common.I2C_PREFIX+'7-0075/sfp_all_present',      'dest_node':TRANSCIEVER_PREFIX+'present'},
    {'src_node':common.I2C_PREFIX+'7-0075/sfp_all_power_on',     'dest_node':TRANSCIEVER_PREFIX+'power_on'},
]
TRANSCIEVER_PORT_NODE_DICT = [
    {'src_node' : '/sys/devices/pci0000:00/0000:00:1c.4/0000:0a:00.0{}'+'sfp{}_eeprom',      'dest_node' : TRANSCIEVER_PREFIX+'eth{}/eeprom'},
    {'src_node' : common.I2C_PREFIX+'{}'+'/sfp{}_present',     'dest_node' : TRANSCIEVER_PREFIX+'eth{}/present'},
    {'src_node' : common.I2C_PREFIX+'{}'+'/sfp{}_reset',       'dest_node' : TRANSCIEVER_PREFIX+'eth{}/reset'},
    {'src_node' : common.I2C_PREFIX+'{}'+'/sfp{}_lowpower',    'dest_node' : TRANSCIEVER_PREFIX+'eth{}/lpmode'},
    {'src_node' : common.I2C_PREFIX+'{}'+'/sfp{}_irq_status',  'dest_node' : TRANSCIEVER_PREFIX+'eth{}/interrupt'},
    {'src_node' : common.I2C_PREFIX+'{}'+'/sfp{}_power_on',    'dest_node' : TRANSCIEVER_PREFIX+'eth{}/power_on'},
]

FAN_TOP_NODE_DICT = [
    {'src_node':'/sys/module/pegatron_hwmon_mcu/parameters/loglevel',         'dest_node':FAN_PREFIX+'loglevel'},
    {'src_node':'/sys/module/pegatron_hwmon_mcu/parameters/debug',            'dest_node':FAN_PREFIX+'debug'},
    {'src_node':common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan_num',              'dest_node':FAN_PREFIX+'num'}
]
FAN_ATTR_NODE_DICT = [
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_model_name',      'dest_node' : FAN_PREFIX+'fan{}/model_name'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_serial_num',      'dest_node' : FAN_PREFIX+'fan{}/serial_number'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_vendor',      'dest_node' : FAN_PREFIX+'fan{}/vendor'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_part_num',      'dest_node' : FAN_PREFIX+'fan{}/part_number'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_hard_version',      'dest_node' : FAN_PREFIX+'fan{}/hardware_version'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_motor_num',      'dest_node' : FAN_PREFIX+'fan{}/num_motors'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_present',        'dest_node' : FAN_PREFIX+'fan{}/status'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_led_status',     'dest_node' : FAN_PREFIX+'fan{}/led_status'}
]
FAN_MOTOR_NODE_DICT = [
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_inner_rpm',      'dest_node' : FAN_PREFIX+'fan{}/motor{}/speed'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_speed_tolerance',      'dest_node' : FAN_PREFIX+'fan{}/motor{}/speed_tolerance'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_speed_target',      'dest_node' : FAN_PREFIX+'fan{}/motor{}/speed_target'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan_pwm',              'dest_node' : FAN_PREFIX+'fan{}/motor{}/ratio'},
    {'src_node' : common.I2C_PREFIX+'4-0070/hwmon/hwmon4/fan{}_airflow_dir',    'dest_node' : FAN_PREFIX+'fan{}/motor{}/direction'}
]

PSU_TOP_NODE_DICT = [
    {'src_node':'/sys/module/pegatron_hwmon_mcu/parameters/loglevel',         'dest_node':PSU_PREFIX+'loglevel'},
    {'src_node':'/sys/module/pegatron_hwmon_mcu/parameters/debug',            'dest_node':PSU_PREFIX+'debug'},
    {'src_node':common.I2C_PREFIX+'2-0058/hwmon/hwmon2/psu_num',              'dest_node':PSU_PREFIX+'num'}
]
PSU_ATTR_NODE_DICT = [
    {'src_node' : common.I2C_PREFIX+'{}/manuafacture_model',      'dest_node' : PSU_PREFIX+'psu{}/model_name'},
    {'src_node' : common.I2C_PREFIX+'{}/manuafacture_serial',        'dest_node' : PSU_PREFIX+'psu{}/serial_number'},
    {'src_node' : common.I2C_PREFIX+'{}/manuafacture_id',      'dest_node' : PSU_PREFIX+'psu{}/vendor'},
    {'src_node' : common.I2C_PREFIX+'{}/manuafacture_date',      'dest_node' : PSU_PREFIX+'psu{}/date'},
    {'src_node' : common.I2C_PREFIX+'{}/manuafacture_revision',      'dest_node' : PSU_PREFIX+'psu{}/hardware_version'},
    {'src_node' : common.I2C_PREFIX+'{}/psu_alarm',          'dest_node' : PSU_PREFIX+'psu{}/alarm'},
    {'src_node' : common.I2C_PREFIX+'{}/curr_in_max',          'dest_node' : PSU_PREFIX+'psu{}/alarm_threshold_curr'},
    {'src_node' : common.I2C_PREFIX+'{}/vol_in_max',          'dest_node' : PSU_PREFIX+'psu{}/alarm_threshold_vol'},
    {'src_node' : common.I2C_PREFIX+'{}/manuafacture_part_num',          'dest_node' : PSU_PREFIX+'psu{}/part_number'},
    {'src_node' : common.I2C_PREFIX+'{}/power_out_max',          'dest_node' : PSU_PREFIX+'psu{}/max_output_power'},
    {'src_node' : common.I2C_PREFIX+'{}/curr_in',          'dest_node' : PSU_PREFIX+'psu{}/in_curr'},
    {'src_node' : common.I2C_PREFIX+'{}/vol_in',          'dest_node' : PSU_PREFIX+'psu{}/in_vol'},
    {'src_node' : common.I2C_PREFIX+'{}/power_in',          'dest_node' : PSU_PREFIX+'psu{}/in_power'},
    {'src_node' : common.I2C_PREFIX+'{}/curr_out',          'dest_node' : PSU_PREFIX+'psu{}/out_curr'},
    {'src_node' : common.I2C_PREFIX+'{}/vol_out',          'dest_node' : PSU_PREFIX+'psu{}/out_vol'},
    {'src_node' : common.I2C_PREFIX+'{}/power_out',          'dest_node' : PSU_PREFIX+'psu{}/out_power'},
    {'src_node' : common.I2C_PREFIX+'{}/psu_{}_status',          'dest_node' : PSU_PREFIX+'psu{}/status'},
    {'src_node' : common.I2C_PREFIX+'{}/psu_led_status',          'dest_node' : PSU_PREFIX+'psu{}/led_status'},
    {'src_node' : common.I2C_PREFIX+'{}/psu_type',          'dest_node' : PSU_PREFIX+'psu{}/type'},
    {'src_node' : common.I2C_PREFIX+'{}/sensor_num',          'dest_node' : PSU_PREFIX+'psu{}/num_temp_sensors'},
    {'src_node' : common.I2C_PREFIX+'{}/fan1_speed',          'dest_node' : PSU_PREFIX+'psu{}/fan_speed'},
    {'src_node' : common.I2C_PREFIX+'{}/vol_out_max',          'dest_node' : PSU_PREFIX+'psu{}/max_output_vol'},
    {'src_node' : common.I2C_PREFIX+'{}/vol_out_min',          'dest_node' : PSU_PREFIX+'psu{}/min_output_vol'}
]

SENSOR_TOP_NODE_DICT = [
    {'src_node':'/sys/module/pegatron_hwmon_mcu/parameters/loglevel',         'dest_node':SENSOR_PREFIX+'loglevel'},
    {'src_node':'/sys/module/pegatron_hwmon_mcu/parameters/debug',            'dest_node':SENSOR_PREFIX+'debug'},
]
PSU_SENSOR_NODE_DICT = [
    {'src_node' : common.I2C_PREFIX+'{}/temp{}_label',      'dest_node' : PSU_PREFIX+'psu{}/temp{}/temp_alias'},
    {'src_node' : common.I2C_PREFIX+'{}/psu_sensor_type',      'dest_node' : PSU_PREFIX+'psu{}/temp{}/temp_type'},
    {'src_node' : common.I2C_PREFIX+'{}/temp{}_max',      'dest_node' : PSU_PREFIX+'psu{}/temp{}/temp_max_hyst'},
    {'src_node' : common.I2C_PREFIX+'{}/temp{}_min',      'dest_node' : PSU_PREFIX+'psu{}/temp{}/temp_min'},
    {'src_node' : common.I2C_PREFIX+'{}/temp{}_crit',      'dest_node' : PSU_PREFIX+'psu{}/temp{}/temp_max'},
    {'src_node' : common.I2C_PREFIX+'{}/temp{}_input',      'dest_node' : PSU_PREFIX+'psu{}/temp{}/temp_input'},
]

CPLD_TOP_NODE_DICT = [
    {'src_node':'/sys/module/pegatron_fn8656_bnf_cpld/parameters/loglevel',         'dest_node':CPLD_PREFIX+'loglevel'},
    {'src_node':common.I2C_PREFIX+'6-0074/debug_cpld',          'dest_node':CPLD_PREFIX+'debug'},
    {'src_node':common.I2C_PREFIX+'6-0074/cpld_num',            'dest_node':CPLD_PREFIX+'num'},
    #not supported{'src_node':common.I2C_PREFIX+'',      'dest_node':CPLD_PREFIX+'reboot_cause'},
]
CPLD_DETAIL_NODE_DICT = [
    {'src_node' : common.I2C_PREFIX+'{}'+'/cpld_alias',      'dest_node' : CPLD_PREFIX+'cpld{}/alias'},
    {'src_node' : common.I2C_PREFIX+'{}'+'/cpld_type',       'dest_node' : CPLD_PREFIX+'cpld{}/type'},
    {'src_node' : common.I2C_PREFIX+'{}'+'/cpld_hw_version',       'dest_node' : CPLD_PREFIX+'cpld{}/hw_version'},
    {'src_node' : common.I2C_PREFIX+'{}'+'/cpld_board_version',    'dest_node' : CPLD_PREFIX+'cpld{}/board_version'}
]
def linkDeviceNode(node_type):

    if node_type == 'sensor':
        num = str(device.THERMAL_SENSOR_NUM)
        cmd = device.SYSSENSOR_PREFIX + "num_temp_sensors " + num
        common.doBash("echo add " + cmd + " > /sys/switch/clounix_cmd")
        for node in SENSOR_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

        for sensor_index in range(device.THERMAL_SENSOR_NUM):
            thermal_info = device.THERMAL_COMBINATION[sensor_index]
            for work_index in range(device.NEED_WORK):
                src_path = thermal_info[device.PATH_INDEX] + thermal_info[work_index]
                dst_path = device.SENSOR_PATH.format(sensor_index)
                dst_path = dst_path + device.THERMAL_NODE_COMBINATION[work_index]
                if work_index < 3 :
                    common.doBash("echo " + src_path + " " + dst_path + " > /sys/switch/clounix_cmd")
                else :
                    common.doBash("echo " + "add " + dst_path + " " + thermal_info[work_index] + " > /sys/switch/clounix_cmd")

    elif node_type == 'transceiver':
        #transceiver top level
        for node in TRANSCIEVER_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")
        #transceiver port node
        for port_num in range(device.SFP_MAX_NUM + 1, device.TOTAL_PORT_NUM + 1):
            for node in TRANSCIEVER_PORT_NODE_DICT:
                if port_num < device.CPLDB_SFP_NUM + 1:
                    if node['dest_node'].format(port_num).split('/')[-1] == 'eeprom':
                        bus = '/'
                    else:
                        bus = '7-0075'
                else:
                    if node['dest_node'].format(port_num).split('/')[-1] == 'eeprom':
                        bus = '/'
                    else:
                        bus = '8-0076'
                #check whether the link exists
                if not os.path.exists(node['dest_node'].format(port_num)) :
                    common.doBash("echo "+node['src_node'].format(bus,port_num)+' '+node['dest_node'].format(port_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'fan':
        for node in FAN_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

        for fan_num in range(1, device.FAN_NUM + 1):
            for node in FAN_ATTR_NODE_DICT:
                if not os.path.exists(node['dest_node'].format(fan_num)) :
                    common.doBash("echo "+node['src_node'].format(fan_num)+' '+node['dest_node'].format(fan_num)+" > /sys/switch/clounix_cmd")
                    #fan motor level
                    for motor_num in range(0, device.MOTOR_NUM_PER_FAN):
                        for motor_node in FAN_MOTOR_NODE_DICT:
                            if not os.path.exists(motor_node['dest_node'].format(fan_num,motor_num)) :
                                common.doBash("echo "+motor_node['src_node'].format(fan_num)+' '+motor_node['dest_node'].format(fan_num,motor_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'cpld':
        for node in CPLD_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

        cpld_bus = ['1-0018','6-0074','7-0075','8-0076']
        for cpld_num in range(1, device.CPLD_NUM + 1):
            for node in CPLD_DETAIL_NODE_DICT:
                #check whether the link exists
                if not os.path.exists(node['dest_node'].format(cpld_num)) :
                    common.doBash("echo "+node['src_node'].format(cpld_bus[cpld_num-1])+' '+node['dest_node'].format(cpld_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'psu':
        for node in PSU_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

        for psu_num in range(1, device.PSU_NUM + 1):
            for node in PSU_ATTR_NODE_DICT:
                if not os.path.exists(node['dest_node'].format(psu_num)) :
                    if node['dest_node'].format(psu_num).split('/')[-1] == 'status' :
                        bus = '6-0074'
                        common.doBash("echo "+node['src_node'].format(bus,psu_num)+' '+node['dest_node'].format(psu_num)+" > /sys/switch/clounix_cmd")
                        continue
                    if psu_num == 1:
                        bus = '2-0058/hwmon/hwmon2'
                    elif psu_num == 2:
                        bus = '3-0059/hwmon/hwmon3'
                    common.doBash("echo "+node['src_node'].format(bus)+' '+node['dest_node'].format(psu_num)+" > /sys/switch/clounix_cmd")

                    #psu temp sensor level
                    for temp_sensor_num in range(0, device.TEMP_SENSOR_NUM_PER_PSU):
                        for temp_sensor_node in PSU_SENSOR_NODE_DICT:
                            if not os.path.exists(temp_sensor_node['dest_node'].format(psu_num,temp_sensor_num)) :
                                common.doBash("echo "+temp_sensor_node['src_node'].format(bus,temp_sensor_num+1)+' '+temp_sensor_node['dest_node'].format(psu_num,temp_sensor_num)+" > /sys/switch/clounix_cmd")

    elif node_type == 'syseeprom':
        for node in SYSEEPROM_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

    elif node_type == 'fpga':
        for node in FPGA_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                fd = os.open(node['src_node'], os.O_RDONLY)
                common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")
                os.close(fd)                

    elif node_type == 'watchdog':
        for node in WATCHDOG_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

    elif node_type == 'sysled':
        for node in SYSLED_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")


def deleteLinkDeviceNode(node_type):
    if node_type == 'sensor':
        cmd = device.SYSSENSOR_PREFIX + "num_temp_sensors"
        common.doBash("echo del " + cmd + " > /sys/swtich/clounix_cmd")
        for sensor_index in range(device.THERMAL_SENSOR_NUM):
            for work_index in range(device.NEED_WORK):
                dst_path = device.SENSOR_PATH.format(sensor_index)
                dst_path = dst_path + device.THERMAL_NODE_COMBINATION[work_index]
                common.doBash("del " + dst_path + " > /sys/switch/clounix_cmd")

    elif node_type == 'transceiver':
        #transceiver
        for node in TRANSCIEVER_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

        for port_num in range(device.SFP_MAX_NUM + 1, device.TOTAL_PORT_NUM + 1):
            for node in TRANSCIEVER_PORT_NODE_DICT:
                #check whether the link exists
                if os.path.exists(node['dest_node'].format(port_num)) :
                    #del /sys/switch/transceiver/eth*/*
                    common.doBash("echo del "+node['dest_node'].format(port_num)+" > /sys/switch/clounix_cmd")
            #del /sys/switch/transceiver/eth*
            common.doBash("echo del /sys/switch/transceiver/eth{}".format(port_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'fan':
        #fan
        for node in FAN_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

        for fan_num in range(1, device.FAN_NUM + 1):
            for node in FAN_ATTR_NODE_DICT:
                for motor_num in range(0, device.MOTOR_NUM_PER_FAN):
                    for motor_node in FAN_MOTOR_NODE_DICT:
                        #del /sys/switch/fan/fan*/motor*/*
                        if os.path.exists(motor_node['dest_node'].format(fan_num,motor_num)) :
                            common.doBash("echo del "+motor_node['dest_node'].format(fan_num,motor_num)+" > /sys/switch/clounix_cmd")
                #del /sys/switch/fan/fan*/motor*
                if os.path.exists("/sys/switch/fan/fan{}/motor{}".format(fan_num,motor_num)) :
                    common.doBash("echo del /sys/switch/fan/fan{}/motor{}".format(fan_num,motor_num)+" > /sys/switch/clounix_cmd")
                #check whether the link exists
                if os.path.exists(node['dest_node'].format(fan_num)) :
                    #del /sys/switch/fan/fan*/*
                    common.doBash("echo del "+node['dest_node'].format(fan_num)+" > /sys/switch/clounix_cmd")
            #del /sys/switch/fan/fan*
            common.doBash("echo del /sys/switch/fan/fan{}".format(fan_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'cpld':
        #transceiver
        for node in CPLD_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

        for cpld_num in range(1, device.CPLD_NUM + 1):
            for node in CPLD_DETAIL_NODE_DICT:
                #check whether the link exists
                if os.path.exists(node['dest_node'].format(cpld_num)) :
                    #del /sys/switch/cpld/cpld*/*
                    common.doBash("echo del "+node['dest_node'].format(cpld_num)+" > /sys/switch/clounix_cmd")
            #del /sys/switch/cpld/cpld*
            common.doBash("echo del /sys/switch/cpld/cpld{}".format(cpld_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'psu':
        #psu
        for node in PSU_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

        for psu_num in range(1, device.PSU_NUM + 1):
            for node in PSU_ATTR_NODE_DICT:
                for temp_sensor_num in range(0, device.MOTOR_NUM_PER_FAN):
                    for motor_node in PSU_SENSOR_NODE_DICT:
                        #del /sys/switch/psu/psu*/temp*/*
                        if os.path.exists(motor_node['dest_node'].format(psu_num,temp_sensor_num)) :
                            common.doBash("echo del "+motor_node['dest_node'].format(psu_num,temp_sensor_num)+" > /sys/switch/clounix_cmd")
                #del /sys/switch/psu/psu*/temp*
                if os.path.exists("/sys/switch/psu/psu{}/temp{}".format(psu_num,temp_sensor_num)) :
                    common.doBash("echo del /sys/switch/psu/psu{}/temp{}".format(psu_num,temp_sensor_num)+" > /sys/switch/clounix_cmd")
                #check whether the link exists
                if os.path.exists(node['dest_node'].format(psu_num)) :
                    #del /sys/switch/psu/psu*/*
                    common.doBash("echo del "+node['dest_node'].format(psu_num)+" > /sys/switch/clounix_cmd")
            #del /sys/switch/psu/psu*
            common.doBash("echo del /sys/switch/psu/psu{}".format(psu_num)+" > /sys/switch/clounix_cmd")

    elif node_type == 'syseeprom':
        for node in SYSEEPROM_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

    elif node_type == 'fpga':
        for node in FPGA_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

    elif node_type == 'watchdog':
        for node in WATCHDOG_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

    elif node_type == 'sysled':
        for node in SYSLED_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")



def doInstall():
    common.doBash("depmod -a")
    common.doBash("rmmod hid_cp2112")
    common.doBash("modprobe -r iTCO_wdt")
    checkDriver()
    for o in i2c_topology_dict:
        common.doBash("echo " + o['driver'] + " " + o['address'] + " > " + common.I2C_PREFIX + o['bus'] + "/new_device")

    for node_type in SOFT_LINK_NODE_TYPE:
        linkDeviceNode(node_type)
    return

def setupThreads():
    global THREADS, QUEUES

    # Queues
    # Global
    queueGlobal = common.Queue.Queue()
    QUEUES.append(queueGlobal)
    FUNCS['global'] = queueGlobal

    # Device
    queueDevice = common.Queue.Queue()
    QUEUES.append(queueDevice)
    FUNCS['device'] = queueDevice

    # Threads
    # Global
    threadGlobal = GlobalThread('Global Handler', queueGlobal)
    THREADS.append(threadGlobal)

    # Device
    threadDevice = device.DeviceThread('Device Handler', queueDevice)
    THREADS.append(threadDevice)

    # Check platform status
    threadPlatformStatus = device.PlatformStatusThread('Platform Status Handler', 0.3)
    THREADS.append(threadPlatformStatus)

def functionInit():
    setupThreads()
    for thread in THREADS:
        thread.start()
    return

def deviceInit():
    msg = messageObject(['init'], None)
    FUNCS['device'].put(msg)
    return

def do_platformApiInit():
    print("Platform API Init....")
    common.doBash("/usr/local/bin/platform_api_mgnt.sh init")
    return

def do_platformApiInstall():
    print("Platform API Install....")
    common.doBash("/usr/local/bin/platform_api_mgnt.sh install")
    return

# Platform uninitialize
def doUninstall():
    for node_type in SOFT_LINK_NODE_TYPE:
        deleteLinkDeviceNode(node_type)
    for i in range(0, len(KERNEL_MODULE))[::-1]:
        common.doBash("modprobe -rq " + KERNEL_MODULE[i])
    for o in i2c_topology_dict:
        common.doBash("echo " + o['address'] + " > " + common.I2C_PREFIX + o['bus'] + "/delete_device")
    return

def main(): #start 20200219
    args = common.sys.argv[1:]

    if len(args[0:]) < 1:
        common.sys.exit(0)

    if args[0] == 'install':
        common.RUN = True
        doInstall()
        do_platformApiInit()
        do_platformApiInstall()
        #fwupdates.fwupdate()
        functionInit()
        deviceInit()
        messageHandler()


    common.sys.exit(0)

if __name__ == "__main__":
    main()
