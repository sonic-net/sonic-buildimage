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

import json
import common
import os
from sonic_py_common import device_info
from sonic_platform_pddf_base import pddfapi

PLATFORM_CFG_MODULE = "clounix_platform"

ADDITIONAL_FAULT_CAUSE_FILE = "/host/reboot-cause/platform/additional_fault_cause"
ADM1166_FAULT_HISTORY_FILE = "/host/reboot-cause/platform/adm1166_{}_fault_position"
ADM1166_FAULT_POSITION = "/sys/bus/i2c/devices/i2c-{}/{}-00{}/adm1166_fault_log_addr"
ADM1166_CFG_POSITION = "/sys/bus/i2c/devices/i2c-{}/{}-00{}/{}"
ADM1166_CAUSE_MSG = "User issued 'adm1166_fault' command [User:{}, Time: {}]"

KERNEL_MODULE = [
    'clounix_fpga_register_init'
]

# Instantiate the class pddf_api
try:
    pddf_api = pddfapi.PddfApi()
except Exception as e:
    print("%s" % str(e))
    common.sys.exit(0)


def doInstall():
    status, output = common.doBash("depmod -a")
    for i in range(0, len(KERNEL_MODULE)):
        status, output = common.doBash("lsmod | grep " + KERNEL_MODULE[i])
        if status:
            status, output = common.doBash("modprobe " + KERNEL_MODULE[i])
    return

def doUninstall():
      for mod in KERNEL_MODULE:
        status, output = common.doBash("modprobe -rq " + mod)

def process_adm1166_factors():
    adm1166_dev = [k for k in pddf_api.data.keys() if 'ADM1166_' in k]
    print(adm1166_dev)
    for dev in adm1166_dev:
        bus = int(pddf_api.data[dev]['i2c']['topo_info']['parent_bus'],16)
        addr = pddf_api.data[dev]['i2c']['topo_info']['dev_addr']
        addr = addr[2:]
        for node in pddf_api.data[dev]['factors'].keys():
            val = pddf_api.data[dev]['factors'][node]
            cfg_position = ADM1166_CFG_POSITION.format(bus, bus, addr, node)
            os.system("echo " + val + " > " + cfg_position)

def process_adm1166_fault():
    indx = 1
    adm1166_dev = [k for k in pddf_api.data.keys() if 'ADM1166_' in k]
    for dev in adm1166_dev:
        fault_indx = 0
        bus = int(pddf_api.data[dev]['i2c']['topo_info']['parent_bus'],16)
        addr = pddf_api.data[dev]['i2c']['topo_info']['dev_addr']
        addr = addr[2:]
        fault_position = ADM1166_FAULT_POSITION.format(bus, bus, addr)
        fault_history = ADM1166_FAULT_HISTORY_FILE.format(indx)

        fd = os.popen("cat " + fault_position)
        adm1166_fault = fd.read()
        adm1166_fault = adm1166_fault.strip('\n')

        if os.path.exists(fault_history):
            fd = os.popen("cat " + fault_history)
            adm1166_history_fault = fd.read()
            adm1166_history_fault = adm1166_history_fault.strip('\n')
            if adm1166_fault[0:2] != adm1166_history_fault[0:2]:
                fault_indx = indx

        os.system("echo " + adm1166_fault + " > " + fault_history)
        indx = indx+1
		
        if fault_indx != 0:
            fd = os.popen("date")
            time = fd.read()
            time = time.strip('\x0a')
            usr = "adm1166_" + str(fault_indx)
            msg = ADM1166_CAUSE_MSG.format(usr, time)
            os.system("echo " + msg + " > " +  ADDITIONAL_FAULT_CAUSE_FILE)
            return

def deviceInit():

    #disalbe eeprom write-protect
    path = "/sys_switch/fan/eepromwp"
    common.writeFile(path, "1")

    SFP_PATH = "/sys_switch/transceiver"
    sfp_num = int(common.readFile(SFP_PATH + "/number"))

    # Set SFP& QSFP reset to normal
    for x in range(0, sfp_num):
        path = SFP_PATH  + '/eth' + str(x+1) + '/reset'
        result = common.writeFile(path, "0")

    # Set QSFP power enable  and high power mode  the present signal
    for x in range(0, sfp_num):
        path = SFP_PATH  + '/eth' + str(x+1) + '/present'
        result  = common.readFile(path)
        if result == '1' and os.path.exists(SFP_PATH  + '/eth' + str(x+1) + '/power_on'):
            path = SFP_PATH  + '/eth' + str(x+1) + '/power_on'
            result = common.writeFile(path, "1")

    # Set SFP && QSFP  high power mode  according to the present signal
    for x in range(0, sfp_num):
        path = SFP_PATH  + '/eth' + str(x+1) + '/low_power_mode'
        result = common.writeFile(path, "0")
    return

def do_platformApiInit():
    print("Platform API Init....")
    status, output = common.doBash("/usr/local/bin/platform_api_mgnt.sh init")
    return

def do_platformApiInstall():
    print("Platform API Install....")
    status, output = common.doBash("/usr/local/bin/platform_api_mgnt.sh install")
    return

def main():
    args = common.sys.argv[1:]

    if len(args[0:]) < 1:
        common.sys.exit(0)

    if args[0] == 'install':
        common.RUN = True
        do_platformApiInit()
        doInstall()
        do_platformApiInstall()
        
        deviceInit()
        process_adm1166_fault()
        process_adm1166_factors()

    if args[0] == 'uninstall':
        common.RUN = False
        doUninstall()
    common.sys.exit(0)

if __name__ == "__main__":
    main()
