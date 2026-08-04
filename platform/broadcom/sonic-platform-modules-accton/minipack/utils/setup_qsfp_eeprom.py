#!/usr/bin/env python
#
# Copyright (c) 2019 Edgecore Networks Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.

# ------------------------------------------------------------------
# HISTORY:
#    mm/dd/yyyy (A.D.)
#    7/22/2019:  Jostar create for minipack
# ------------------------------------------------------------------

try:
    import getopt
    import sys
    import subprocess
    import logging
    import logging.config
    import logging.handlers
    import types
    import time  # this is only being used as part of the example
    import traceback    
    from tabulate import tabulate    
    from minipack.pimutil import PimUtil
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

#2:idle state. 1:insert state, 0:remove state.
pim_state=[2,2,2,2,2,2,2,2]

# port_use_i2c_bus[idx], idx means port_idx. Idx start from 0 to 127 
# port_use_i2c_bus[0] means port0 use i2c-device number
# port_use_i2c_bus[1] means port1 use i2c-device number
#At default , port_use_i2c_bus are 0. When PIM insert, it will no be 0
port_use_i2c_bus= [0] * 128  

#pim_port_use_bus[idx] are for 8 channel use. At default, pim_port_use_bus[idx]=0
#pim_port_use_bus[idx] will save pim_idx(0 to 7).
#setup service will check when pim insert at first time. 
#It will find  
#  1. Does this pim card insert or not in the past. So it will check  pim_port_use_bus[idx]
#  2. If make sure that pim_port_use_bus[idx]==0. Assgin oom start i2c dev numer to pim_port_use_bus[idx].
#Never set pim_port_use_bus[idx] to 0 
#So if pim  next insert, it need to check whether pim_port_use_bus[idx] is 0 or not.
#if pim_port_use_bus[idx]!=0, means this pim card has setup oom sysfs
pim_port_use_bus=[0] * 8

oom_i2c_bus_table=1

pim_dev=PimUtil()

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/setup_qsfp_eeprom'
DEBUG = False
PIM_MIN=0
PIM_MAX=8

def my_log(txt):
    if DEBUG == True:
        print("[ACCTON DBG]: "+txt)
    return

def log_os_system(cmd):
    logging.info('Run :'+cmd)
    status = 1
    output = ""
    status, output = subprocess.getstatusoutput(cmd)
    if status:
        logging.info('Failed :'+cmd)
    return  status, output

def qsfp_map_bus(idx):
    port = idx + 1
    base = ((port-1)/8*8) + 10
    idx = (port - 1) % 8
    idx = 7 - idx
    if (idx%2):
        idx = idx -1
    else:
        idx = idx +1
    bus = base + idx
    return bus
    
def pca9548_sysfs(i2c_bus, create):
    if create==1:        
        cmdl = "echo pca9548 0x%x > /sys/bus/i2c/devices/i2c-%d/new_device"
    else:
        cmdl = "echo 0x%x > /sys/bus/i2c/devices/i2c-%d/delete_device"    
    
    cmdm = cmdl % (0x72, i2c_bus)
    status1, output =log_os_system(cmdm)    
    cmdm = cmdl % (0x71, i2c_bus)
    status2, output =log_os_system(cmdm)
    return (status1 | status2)


def qsfp_eeprom_sys(pim_idx, i2c_bus_order, create):   
    # initialize multiplexer for 8 PIMs
    global port_use_i2c_bus    
    start_port=pim_idx*16
    end_port = (pim_idx+1)*16
    start_bus=(i2c_bus_order-1)*16
    end_bus = i2c_bus_order*16
    k=start_port
    for i in range(start_bus, end_bus):
        bus = qsfp_map_bus(i)
        if create==1:
            status, output =log_os_system(
                "echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-"+str(bus)+"/new_device")
            if status:
                print(output)
                return 1
            status, output =log_os_system(
                "echo port"+str(k+1)+" > /sys/bus/i2c/devices/"+str(bus)+"-0050/port_name")
            
            status, output =log_os_system(
                "ln -s -f /sys/bus/i2c/devices/"+str(bus)+"-0050/eeprom" + " /usr/local/bin/minipack_qsfp/port" + str(k) + "_eeprom")
            if status:
                print(output)
                return 1
        else:        
            status, output =log_os_system(
                "echo 0x50 > /sys/bus/i2c/devices/i2c-"+str(bus)+"/delete_device")
            if status:
                print(output)
   
        k=k+1

    return 0

def check_pca_active( i2c_addr, bus):
    cmd = "i2cget -y -f %d 0x%x 0x0"
    cmd =  cmd %(bus, i2c_addr)
    status, output = subprocess.getstatusoutput(cmd)
    return status

def set_pim_port_use_bus(pim_idx):

    global pim_port_use_bus
    global oom_i2c_bus_table
    
    if pim_port_use_bus[pim_idx]!=0:
        return 0
    
    pim_port_use_bus[pim_idx]=oom_i2c_bus_table
    oom_i2c_bus_table=oom_i2c_bus_table+1
    
    return pim_port_use_bus[pim_idx]
   

def del_pim_port_use_bus(pim_idx):
   global oom_i2c_bus_table
   
   oom_i2c_bus_table=oom_i2c_bus_table-1
   pim_port_use_bus[pim_idx]=0
      

def device_remove():    
    cmd1 = "echo 0x%x > /sys/bus/i2c/devices/i2c-%d/delete_device"    
    
    for bus in range(2, 10):        
        #ret=check_pca_active(0x72, bus)
        #if ret==0:
        
        cmdm= cmd1 % (0x72, bus)        
        status, output = subprocess.getstatusoutput(cmdm)
        print("Remove %d-0072 i2c device"%bus)
        cmdm= cmd1 % (0x71, bus)
        status, output = subprocess.getstatusoutput(cmdm)
        print("Remove %d-0071 i2c device"%bus)

    cmd="rm -f /usr/local/bin/minipack_qsfp/port*"
    status, output=log_os_system(cmd)
    return status
    
class device_monitor(object):
   
    PIM_STATE_REMOVE = 0
    PIM_STATE_INSERT = 1
    PIM_STATE_IDLE=2   
    
    def __init__(self, log_file, log_level):
        """Needs a logger and a logger level."""
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='w',
            level=log_level,
            format= '[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )
        # set up logging to console
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        sys_handler = handler = logging.handlers.SysLogHandler(address = '/dev/log')
        sys_handler.setLevel(logging.WARNING)       
        logging.getLogger('').addHandler(sys_handler)

        #logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)

    def manage_pim(self):
        global pim_dev
        global pim_state
        
        for pim_idx in range(PIM_MIN, PIM_MAX):
            presence=pim_dev.get_pim_presence(pim_idx)
            if presence==1:
                if pim_state[pim_idx]!=self.PIM_STATE_INSERT:
                    #find which i2c_device can use. It start from 2
                    i2c_bus_order=set_pim_port_use_bus(pim_idx)
                    if i2c_bus_order==0:
                        logging.info("pim_state[%d] PIM_STATE_INSERT", pim_idx);
                        pim_state[pim_idx]=self.PIM_STATE_INSERT
                        continue
                    
                    logging.info ("pim_idx=%d oom use i2c_bus_order=%d", pim_idx, i2c_bus_order)
                    ready=0
                    retry_limit=100
                    retry_count=0
                    while retry_count < retry_limit:                    
                        ret=check_pca_active(0x72, pim_idx+2)
                        if ret==0:
                            ready=1
                            break                        
                        retry_count=retry_count+1
                        time.sleep(0.2)
                        
                    if ready==1:
                        status=pca9548_sysfs(pim_idx+2, 1)
                        if status:
                            status=pca9548_sysfs(pim_idx+2, 0) #del pca i2c device, give up set oom at this time.
                            del_pim_port_use_bus(pim_idx)
                            continue
                        status=qsfp_eeprom_sys(pim_idx,i2c_bus_order,  1)
                        if status:
                            status=pca9548_sysfs(pim_idx+2, 0) #del pca i2c device, give up set oom at this time.
                            del_pim_port_use_bus(pim_idx)                            
                            continue
                        #ret_2=check_pca_active(0x72, pim_idx+2)
                        #ret_1=check_pca_active(0x71, pim_idx+2)
                        
                        pim_state[pim_idx]=self.PIM_STATE_INSERT
                        logging.info("pim_state[%d] PIM_STATE_INSERT", pim_idx);
                    else:
                        print("retry check 100 times for check pca addr")
                        del_pim_port_use_bus(pim_idx)
            else:
                if pim_state[pim_idx]==self.PIM_STATE_INSERT:                    
                    #pca9548_sysfs(pim_idx+2, 0)
                    pim_state[pim_idx]=self.PIM_STATE_REMOVE
                    logging.info("pim_state[%d] PIM_STATE_REMOVE", pim_idx);
            
def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    remove_dev=0
    cpu_pca_i2c_ready=0
    
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv,'hdlr',['lfile='])
        except getopt.GetoptError:
            print('A:Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
            return 0
        for opt, arg in opts:
            if opt == '-h':
                print('B:Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
                return 0
            elif opt in ('-d', '--debug'):
                log_level = logging.DEBUG
            elif opt in ('-l', '--lfile'):
                log_file = arg
            elif opt in ('-r', '--remove'):
                remove_dev=1
            
    if remove_dev==1:
        device_remove()
        return 0
    monitor = device_monitor(log_file, log_level)
    global pim_dev
    pim_dev.init_pim_fpga()
                    
    while cpu_pca_i2c_ready==0:
        status=check_pca_active(0x70, 1)
        time.sleep(0.5)
        if status==0:
            cpu_pca_i2c_ready=1
            print("Make sure CPU pca i2c device is ready")
            break
    
    while True:
        monitor.manage_pim()
        time.sleep(2)


if __name__ == "__main__":
    main(sys.argv[1:])
