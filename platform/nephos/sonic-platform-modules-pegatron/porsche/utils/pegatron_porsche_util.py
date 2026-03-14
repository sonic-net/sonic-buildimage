#!/usr/bin/env python
#
# Copyright (C) 2018 Pegatron, Inc.
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

import sys, getopt
import logging
import os
import commands
import threading

DEBUG = False

SFP_MAX_NUM = 48
CPLDA_SFP_NUM = 24
CPLDB_SFP_NUM = 12
CPLDC_SFP_NUM = 18

kernel_module = ['i2c_dev', 'i2c-mux-pca954x force_deselect_on_exit=1', 'at24', 'pegatron_porsche_cpld', 'pegatron_hwmon_mcu', 'pegatron_porsche_sfp']
moduleID = ['pca9544', 'pca9544', '24c02', 'porsche_hwmon_mcu', 'porsche_cpld', 'porsche_cpld', 'porsche_cpld', 'porsche_sfpA', 'porsche_sfpB', 'porsche_sfpC']
i2c_check_node = ['i2c-0', 'i2c-1']
device_address = ['0x72', '0x73', '0x54', '0x70', '0x74', '0x75', '0x76', '0x50', '0x50', '0x50']
device_node= ['i2c-2', 'i2c-6', 'i2c-4', 'i2c-5', 'i2c-6', 'i2c-7', 'i2c-8', 'i2c-6', 'i2c-7', 'i2c-8']

i2c_prefix = '/sys/bus/i2c/devices/'
cpld_bus = ['6-0074', '7-0075', '8-0076']
led_nodes = ['sys_led', 'pwr_led', 'loc_led', 'fan_led', "cpld_allled_ctrl", "serial_led_enable"]

def dbg_print(string):
	if DEBUG == True:
		print string    
	return

def do_cmd(cmd, show):
	logging.info('Run :' + cmd)  
	status, output = commands.getstatusoutput(cmd)    
	dbg_print(cmd + "with result:" + str(status))
	dbg_print("output:" + output)    
	if status:
		logging.info('Failed :' + cmd)
		if show:
			print('Failed :' + cmd)
	return  status, output

def check_device_position(num):  
	for i in range(0, len(i2c_check_node)):
		status, output = do_cmd("echo " + moduleID[num] + " " + device_address[num] + " > " + i2c_prefix + i2c_check_node[i] + "/new_device", 0)
		status, output = do_cmd("ls " + i2c_prefix + device_node[num], 0)
		device_node[num] = i2c_check_node[i]

		if status:
			status, output = do_cmd("echo " + device_address[num] + " > " + i2c_prefix + i2c_check_node[i] + "/delete_device", 0) 
		else:
			return

	return

def install_device():
	for i in range(0, len(moduleID)):
		if moduleID[i] == "pca9544":
			check_device_position(i)
		else:
			status, output = do_cmd("echo " + moduleID[i] + " " + device_address[i] + " > " + i2c_prefix + device_node[i] + "/new_device", 1)

	return 

def check_driver():
	for i in range(0, len(kernel_module)):
		status, output = do_cmd("lsmod | grep " + kernel_module[i], 0)
		if status:
			status, output = do_cmd("modprobe " + kernel_module[i], 1)

	return

def do_install():
	status, output = do_cmd("depmod -a", 1)

	check_driver()
	install_device()

	return

def do_uninstall():
	for i in range(0, len(kernel_module)):
		status, output = do_cmd("modprobe -r " + kernel_module[i], 1)

	for i in range(0, len(moduleID)):
		status, output = do_cmd("echo " + device_address[i] + " > " + i2c_prefix + i2c_check_node[i] + "/delete_device", 0)

	return

led_command = {'sys_led': {'green':'0', 'amber':'1', 'off':'2', 'blink_green':'3', 'blink_amber':'4'},
			   'pwr_led': {'green':'0', 'amber':'1', 'off':'2', 'blink_green':'3', 'blink_amber':'4'},
			   'loc_led': {'on':'0', 'off':'1', 'blink':'2'},
			   'fan_led': {'green':'0', 'amber':'1', 'off':'2', 'blink_green':'3', 'blink_amber':'4'},
			   'cpld_allled_ctrl': {'off':'0', 'mix':'1', 'amber':'2', 'normal':'3'},
			   'serial_led_enable': {'disable':'0', 'enable':'1'}}

def set_led(args):
	"""
	Usage: %(scriptName)s set led object command

	object:
		sys_led   : set SYS led      [command: off|green|amber|blink_green|blink_amber]
		pwr_led   : set PWR led      [command: off|green|amber|blink_green|blink_amber]
		loc_led   : set LOCATOR led  [command: off|on|blink]
		fan_led   : set FAN led      [command: off|green|amber|blink_green|blink_amber]
	"""
	if args[0] not in led_command:
		print set_led.__doc__
		sys.exit(0)

	for i in range(0,len(led_nodes)):
		if args[0] == led_nodes[i]:
			node = i2c_prefix + cpld_bus[1] + '/'+ led_nodes[i]

	command = led_command[args[0]]
	data = command[args[1]]

	status, output = do_cmd("echo "+ str(data) + " > "+ node, 1)

	return

def set_device(args):
	"""
	Usage: %(scriptName)s command object

	command:
		led     : set status led sys_led|pwr_led|loc_led|mst_led|fan_led|digit_led      
	"""

	if args[0] == 'led':
		set_led(args[1:])
		return
	else:
		print set_device.__doc__
																		   
	return

device_init = {'led': [['led', 'sys_led', 'green'], ['led', 'pwr_led', 'green'], ['led', 'fan_led', 'green'], ['led', 'cpld_allled_ctrl', 'normal'], ['led', 'serial_led_enable', 'enable']]}

def pega_init():
	#set led
	for i in range(0,len(device_init['led'])):
		set_device(device_init['led'][i])

	#set tx_disable
	for x in range(0, SFP_MAX_NUM-1):
		if x < CPLDB_SFP_NUM:
			bus = cpld_bus[1]
		elif x < CPLDB_SFP_NUM + CPLDA_SFP_NUM:
			bus = cpld_bus[0]
		else:
			bus = cpld_bus[2]

		nodes = i2c_prefix + bus + '/sfp' + str(x+1) + '_tx_disable'
		dbg_print("SFP_TX_DISABLE NODES: " + nodes)
		status, output = do_cmd("echo 0 > "+ nodes, 1)

	return

def main():
	"""
	Usage: %(scriptName)s command object

	command:
		install     : install drivers and generate related sysfs nodes
		clean       : uninstall drivers and remove related sysfs nodes  
		set         : change board setting [led]
		debug       : debug info [on/off]
	"""

	if len(sys.argv)<2:
		print main.__doc__

	for arg in sys.argv[1:]:           
		if arg == 'install':
			do_install()
			pega_init()
		elif arg == 'uninstall':
			do_uninstall()        
		elif arg == 'set':
			if len(sys.argv[2:])<1:
				print main.__doc__
			else:
				set_device(sys.argv[2:])                
			return
		elif arg == 'debug':
			if sys.argv[2] == 'on':
				DEBUG = True
			else:
				DEBUG = False
		else:
			print main.__doc__

if __name__ == "__main__":
	main()
