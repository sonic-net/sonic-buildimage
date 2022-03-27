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
import signal
import sys
import threading
import time
import os
import common
import subprocess
from datetime import datetime
import sonic_platform

fw_path = '/usr/local/fw_bin/'

def run_command_with_timeout(command, timeout):
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=timeout, shell=True)
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, None, "Command timed out"

def firmware_bin_scan():
	fw_files = os.listdir(fw_path)
	fw_dict = {}
	for file in fw_files:
		if not file.endswith('.bin'):
			continue
		str1 = file.split('_')
		name = ''
		ver = ''
		if len(str1) >= 2:
			name = str1[0]
			str2 = str1[1].split('.')
			if len(str2) >= 2:
				ver = str2[0]
		if name and ver:
			fw_dict[name] = ver
	return fw_dict


def do_upgrade(upgrade_dict):
	print('Following fpga/cpld will be automatically upgraded to the following versions')
	print('Reboot may be occured during the upgrade process')
	for name,ver in upgrade_dict.items():
		print(name,' -> '+ver)
	
	for name,ver in upgrade_dict.items():
		fw_file = fw_path + name + '_' + ver + '.bin'
		print(fw_file)
		if not os.path.exists(fw_file):
			continue
		if name == 'CPLD-1':
			upg_cmd = 'clx_fpga -f {} -b 5 -r 1'.format(fw_file)
			print('run_command: ' + upg_cmd)
			status, output, error = run_command_with_timeout(upg_cmd, 300)
			if status:
				print('upgrade failed:{}->{} err:{}'.format(name,ver,error))
			else:
				print('upgrade success:{}->{}'.format(name,ver))
				find_cmd = "find /sys/devices/ -name fpga_cpld_reset"
				status, retstr = common.doBash(find_cmd)
				if status == 0:
					cpld_reset_cmd = "echo 1 > " + retstr
					status, output = common.doBash(cpld_reset_cmd)
					print('Do:' + cpld_reset_cmd)
		if name == 'CPLD-2':
			upg_cmd = 'clx_fpga -f {} -b 5 -r 2'.format(fw_file)
			print('run_command: ' + upg_cmd)
			status, output, error = run_command_with_timeout(upg_cmd, 300)
			if status:
				print('upgrade failed:{}->{} err:{}'.format(name,ver,error))
			else:
				print('upgrade success:{}->{}'.format(name,ver))
				find_cmd = "find /sys/devices/ -name fpga_cpld_reset"
				status, retstr = common.doBash(find_cmd)
				if status == 0:
					cpld_reset_cmd = "echo 1 > " + retstr
					status, output = common.doBash(cpld_reset_cmd)
					print('Do:' + cpld_reset_cmd)
		if name == 'CPLD-4':
			upg_cmd = 'clx_fpga -f {} -b 5 -r 4'.format(fw_file)
			print('run_command: ' + upg_cmd)
			status, output, error = run_command_with_timeout(upg_cmd, 300)
			if status:
				print('upgrade failed:{}->{} err:{}'.format(name,ver,error))
			else:
				print('upgrade success:{}->{}'.format(name,ver))
		if name == 'CPLD-3':
			upg_cmd = 'clx_fpga -f {} -b 5 -r 3'.format(fw_file)
			print('run_command: ' + upg_cmd)
			status, output, error = run_command_with_timeout(upg_cmd, 300)
			if status:
				print('upgrade failed:{}->{} err:{}'.format(name,ver,error))
			else:
				print('upgrade success:{}->{}'.format(name,ver))
		if name == 'FPGA':
			upg_cmd = 'clx_fpga -f {} -b 3 -r 0'.format(fw_file)
			print('run_command: ' + upg_cmd)
			status, output, error = run_command_with_timeout(upg_cmd, 300)
			if status:
				print('upgrade failed:{}->{} err:{}'.format(name,ver,error))
			else:
				print('upgrade success:{}->{}'.format(name,ver))
				find_cmd = "find /sys/devices/ -name power_cycle"
				status, retstr = common.doBash(find_cmd)
				if status == 0:
					fpga_refresh_cmd = "echo 0x1 > " + retstr
					print('Do:' + fpga_refresh_cmd)
					status, output = common.doBash(fpga_refresh_cmd)

	return True

def firmware_inspection():
	print('Firmware inspection start...')
	bin_dict = firmware_bin_scan()
	if not bin_dict:
		return False,"No valid firmware found in {}".format(fw_path)

	chassis = sonic_platform.platform.Platform().get_chassis()
	components = chassis.get_all_components()

	fw_dict = {}
	for i in range(0,chassis.get_num_components()):
		fw_name = components[i].get_name()
		fw_ver = components[i].get_firmware_version()
		if fw_name and fw_ver and fw_ver != 'N/A':
			fw_dict[fw_name] = fw_ver
	
	upgrade_dict = {}
	try:
		for bin_name,bin_ver in bin_dict.items():
			for fw_name,fw_ver in fw_dict.items():
				if bin_name == fw_name:
					if int(bin_ver, 0) > int(fw_ver, 0):
						upgrade_dict[bin_name] = bin_ver
	except Exception as e:
		print(e)

	if upgrade_dict:
		do_upgrade(upgrade_dict)

	print('Firmware inspection complete')
	return True
