#!/usr/bin/env python
#
# Copyright (C) 2017 Inventec, Inc.
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

"""
Usage: %(scriptName)s [options] command object

options:
    -h | --help     : this help message
    -d | --debug    : run with debug mode
    -f | --force    : ignore error during installation or clean 
command:
    install     : install drivers and generate related sysfs nodes
    clean       : uninstall drivers and remove related sysfs nodes    
"""

import os
import commands
import sys, getopt
import logging
import re
import time
from collections import namedtuple

DEBUG = False
args = []
FORCE = 0
i2c_prefix = '/sys/bus/i2c/devices/'


if DEBUG == True:
    print sys.argv[0]
    print 'ARGV      :', sys.argv[1:]   


def main():
    global DEBUG
    global args
    global FORCE
        
    if len(sys.argv)<2:
        show_help()
         
    options, args = getopt.getopt(sys.argv[1:], 'hdf', ['help',
                                                       'debug',
                                                       'force',
                                                          ])
    if DEBUG == True:                                                           
        print options
        print args
        print len(sys.argv)
            
    for opt, arg in options:
        if opt in ('-h', '--help'):
            show_help()
        elif opt in ('-d', '--debug'):            
            DEBUG = True
            logging.basicConfig(level=logging.INFO)
        elif opt in ('-f', '--force'): 
            FORCE = 1
        else:
            logging.info('no option')                          
    for arg in args:            
        if arg == 'install':
           install()
        elif arg == 'clean':
           uninstall()
        else:
            show_help()
            
            
    return 0              
        
def show_help():
    print __doc__ % {'scriptName' : sys.argv[0].split("/")[-1]}
    sys.exit(0)
           
def show_log(txt):
    if DEBUG == True:
        print "[D7032]"+txt    
    return
    
def exec_cmd(cmd, show):
    logging.info('Run :'+cmd)  
    status, output = commands.getstatusoutput(cmd)    
    show_log (cmd +"with result:" + str(status))
    show_log ("      output:"+output)    
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print('Failed :'+cmd)
    return  status, output
        
instantiate =[ 
#'echo pca9548 0x71> /sys/bus/i2c/devices/i2c-0/new_device',
#'echo pca9548 0x72> /sys/bus/i2c/devices/i2c-0/i2c-2/new_device',
#'echo pca9548 0x72> /sys/bus/i2c/devices/i2c-0/i2c-3/new_device',
#'echo pca9548 0x72> /sys/bus/i2c/devices/i2c-0/i2c-4/new_device',
#'echo pca9548 0x72> /sys/bus/i2c/devices/i2c-0/i2c-5/new_device',
#'echo pca9548 0x72> /sys/bus/i2c/devices/i2c-0/i2c-6/new_device',
#'echo pca9548 0x72> /sys/bus/i2c/devices/i2c-0/i2c-7/new_device',
#'echo pca9548 0x72> /sys/bus/i2c/devices/i2c-0/i2c-8/new_device',
'echo inv_eeprom 0x53 > /sys/bus/i2c/devices/i2c-0/new_device']
#'echo inv_psoc 0x66> /sys/bus/i2c/devices/i2c-0/new_device',
#'echo inv_cpld 0x55> /sys/bus/i2c/devices/i2c-0/new_device']

drivers =[
'lpc_ich',
'i2c-i801',
'i2c-mux',
'i2c-mux-pca954x',
'i2c-dev',
'inv_eeprom',
'inv_platform',
'inv_psoc',
'inv_cpld',
'inv_pthread',
'swps']
 

                    
def system_install():
    global FORCE

    #install drivers
    for i in range(0,len(drivers)):
       status, output = exec_cmd("modprobe "+drivers[i], 1)
    if status:
	   print output
	   if FORCE == 0:                
	      return status             
    				 
    #instantiate devices
    for i in range(0,len(instantiate)):
       #time.sleep(1)
       status, output = exec_cmd(instantiate[i], 1)
    if status:
	   print output
	   if FORCE == 0:                
	      return status  
    
    for i in range(10,18):
        status, output =exec_cmd("echo sff8436 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-2/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:            
                return status   
    for i in range(18,26):
        status, output =exec_cmd("echo sff8436 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-3/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:            
                return status     
    for i in range(26,34):
        status, output =exec_cmd("echo sff8436 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-4/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:            
                return status     
    for i in range(34,42):
        status, output =exec_cmd("echo sff8436 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-5/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:            
                return status
    for i in range(42,50):
        status, output =exec_cmd("echo sff8436 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-6/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:            
                return status     				
    for i in range(50,58):
        status, output =exec_cmd("echo sff8436 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-7/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    for i in range(58,64):
        status, output =exec_cmd("echo sff8436 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-8/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    return 
     
        
def system_ready():
    if not device_found(): 
        return False
    return True
               
def install():                      
    if not device_found():
        print "No device, installing...."     
        status = system_install() 
        if status:
            if FORCE == 0:        
                return  status        
    else:
        print " D7054 devices detected...."           
    return

def uninstall():
    global FORCE
    #uninstall drivers
    for i in range(len(drivers)-1,-1,-1):
       status, output = exec_cmd("rmmod "+drivers[i], 1)
    if status:
	   print output
	   if FORCE == 0:                
	      return status             
    return

def device_found():
    ret1, log = exec_cmd("ls "+i2c_prefix+"*0072", 0)
    ret2, log = exec_cmd("ls "+i2c_prefix+"i2c-2", 0)
    return not(ret1 or ret2)				

if __name__ == "__main__":
    main()


