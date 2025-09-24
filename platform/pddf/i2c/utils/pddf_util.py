#!/usr/bin/env python


"""
Usage: %(scriptName)s [options] command object

options:
    -h | --help     : this help message
    -d | --debug    : run with debug mode
    -f | --force    : ignore error during installation or clean
command:
    install     : install drivers and generate related sysfs nodes
    clean       : uninstall drivers and remove related sysfs nodes
    switch-pddf     : switch to pddf mode, installing pddf drivers and generating sysfs nodes
    switch-nonpddf  : switch to per platform, non-pddf mode
"""

import logging
import getopt
import os
import shutil
import subprocess
import sys
from sonic_py_common import device_info
import pddfparse

PLATFORM_ROOT_PATH = '/usr/share/sonic/device'
SONIC_CFGGEN_PATH = '/usr/local/bin/sonic-cfggen'
HWSKU_KEY = 'DEVICE_METADATA.localhost.hwsku'
PLATFORM_KEY = 'DEVICE_METADATA.localhost.platform'

PROJECT_NAME = 'PDDF'
version = '1.1'
verbose = False
DEBUG = False
args = []
ALL_DEVICE = {}               
FORCE = 0
perm_kos = []
std_kos = []
custom_kos = []
devs = []

# Instantiate the class pddf_obj
try:
    pddf_obj = pddfparse.PddfParse()
except Exception as e:
    print("%s" % str(e))
    sys.exit()



if DEBUG == True:
    print(sys.argv[0])
    print('ARGV      :', sys.argv[1:])

def main():
    global DEBUG
    global args
    global FORCE
    global kos
        
    if len(sys.argv)<2:
        show_help()
         
    options, args = getopt.getopt(sys.argv[1:], 'hdf', ['help',
                                                       'debug',
                                                       'force',
                                                          ])
    if DEBUG == True:                                                           
        print(options)
        print(args)
        print(len(sys.argv))
    
    # generate the KOS list from pddf device JSON file
    if 'std_perm_kos' in pddf_obj.data['PLATFORM'].keys():
       perm_kos.extend(pddf_obj.data['PLATFORM']['std_perm_kos'])

    std_kos.extend(pddf_obj.data['PLATFORM']['std_kos'])
    std_kos.extend(pddf_obj.data['PLATFORM']['pddf_kos'])

    if 'custom_kos' in pddf_obj.data['PLATFORM']:
        custom_kos.extend(pddf_obj.data['PLATFORM']['custom_kos'])

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
            do_install()
        elif arg == 'clean':
           do_uninstall()
        elif arg == 'switch-pddf':
            do_switch_pddf()
        elif arg == 'switch-nonpddf':
            do_switch_nonpddf()
        else:
            show_help()
            
    return 0              
        
def show_help():
    print(__doc__ % {'scriptName' : sys.argv[0].split("/")[-1]})
    sys.exit(0)

def my_log(txt):
    if DEBUG == True:
        print("[PDDF]"+txt)
    return
    
def log_os_system(cmd, show):
    logging.info('Run :'+cmd)  
    status, output = subprocess.getstatusoutput(cmd)
    my_log (cmd +"with result:" + str(status))
    my_log ("      output:"+output)    
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print('Failed :'+cmd)
    return  status, output
            
def driver_check():
    ret, lsmod = log_os_system("lsmod| grep pddf", 0)
    if ret:
        return False
    logging.info('mods:'+lsmod)
    if len(lsmod) ==0:
        return False   
    return True

def get_path_to_device():
    # Get platform and hwsku
    (platform, hwsku) = device_info.get_platform_and_hwsku()

    # Load platform module from source
    platform_path = "/".join([PLATFORM_ROOT_PATH, platform])

    return platform_path

def get_path_to_pddf_plugin():
    pddf_path = "/".join([PLATFORM_ROOT_PATH, "pddf/plugins"])
    return pddf_path

def config_pddf_utils():
    device_path = get_path_to_device()
    pddf_path = get_path_to_pddf_plugin()

    # ##########################################################################
    SONIC_PLATFORM_BSP_WHL_PKG = "/".join([device_path, 'sonic_platform-1.0-py3-none-any.whl'])
    SONIC_PLATFORM_PDDF_WHL_PKG = "/".join([device_path, 'pddf', 'sonic_platform-1.0-py3-none-any.whl'])
    SONIC_PLATFORM_BSP_WHL_PKG_BK = "/".join([device_path, 'sonic_platform-1.0-py3-none-any.whl.orig'])
    status, output = log_os_system("pip3 show sonic-platform > /dev/null 2>&1", 1)
    if status:
        if os.path.exists(SONIC_PLATFORM_PDDF_WHL_PKG):
            # Platform API 2.0 is supported
            if os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG):
                # bsp whl pkg is present but not installed on host <special case>
                if not os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG_BK):
                    log_os_system('mv '+SONIC_PLATFORM_BSP_WHL_PKG+' '+SONIC_PLATFORM_BSP_WHL_PKG_BK, 1)
            # PDDF whl package exist ... this must be the whl package created from 
            # PDDF 2.0 ref API classes and some changes on top of it ... install it
            log_os_system('sync', 1)
            shutil.copy(SONIC_PLATFORM_PDDF_WHL_PKG, SONIC_PLATFORM_BSP_WHL_PKG)
            log_os_system('sync', 1)
            print("Attemting to install the PDDF sonic_platform wheel package ...")
            if os.path.getsize(SONIC_PLATFORM_BSP_WHL_PKG) != 0:
                status, output = log_os_system("pip3 install "+ SONIC_PLATFORM_BSP_WHL_PKG, 1)
                if status:
                    print("Error: Failed to install {}".format(SONIC_PLATFORM_BSP_WHL_PKG))
                    return status
                else:
                    print("Successfully installed {} package".format(SONIC_PLATFORM_BSP_WHL_PKG))
            else:
                print("Error: Failed to copy {} properly. Exiting ...".format(SONIC_PLATFORM_PDDF_WHL_PKG))
                return -1
        else:
            # PDDF with platform APIs 1.5 must be supported
            device_plugin_path = "/".join([device_path, "plugins"])
            backup_path = "/".join([device_plugin_path, "orig"])
            print("Loading PDDF generic plugins (1.0)")
            if os.path.exists(backup_path) is False:
                os.mkdir(backup_path)
                log_os_system("mv "+device_plugin_path+"/*.*"+" "+backup_path, 0)
            
            for item in os.listdir(pddf_path):
                shutil.copy(pddf_path+"/"+item, device_plugin_path+"/"+item)
            
            shutil.copy('/usr/local/bin/pddfparse.py', device_plugin_path+"/pddfparse.py")

    else:
        # sonic_platform whl pkg is installed 2 possibilities, 1) bsp 2.0 classes
        # are installed, 2) system rebooted and either pddf/bsp 2.0 classes are already installed
        if os.path.exists(SONIC_PLATFORM_PDDF_WHL_PKG):
            if not os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG_BK):
                # bsp 2.0 classes are installed. Take a backup and copy pddf 2.0 whl pkg
                log_os_system('mv '+SONIC_PLATFORM_BSP_WHL_PKG+' '+SONIC_PLATFORM_BSP_WHL_PKG_BK, 1)
                log_os_system('sync', 1)
                shutil.copy(SONIC_PLATFORM_PDDF_WHL_PKG, SONIC_PLATFORM_BSP_WHL_PKG)
                log_os_system('sync', 1)
                # uninstall the existing bsp whl pkg
                status, output = log_os_system("pip3 uninstall sonic-platform -y &> /dev/null", 1)
                if status:
                    print("Error: Unable to uninstall BSP sonic-platform whl package")
                    return status
                print("Attempting to install the PDDF sonic_platform wheel package ...")
                if os.path.getsize(SONIC_PLATFORM_BSP_WHL_PKG) != 0:
                    status, output = log_os_system("pip3 install "+ SONIC_PLATFORM_BSP_WHL_PKG, 1)
                    if status:
                        print("Error: Failed to install {}".format(SONIC_PLATFORM_BSP_WHL_PKG))
                        return status
                    else:
                        print("Successfully installed {} package".format(SONIC_PLATFORM_BSP_WHL_PKG))
                else:
                    print("Error: Failed to copy {} properly. Exiting ...".format(SONIC_PLATFORM_PDDF_WHL_PKG))
                    return -1
            else:
                # system rebooted in pddf mode 
                print("System rebooted in PDDF mode, hence keeping the PDDF 2.0 classes")
        else:
            # pddf whl package doesnt exist
            print("Error: PDDF 2.0 classes doesnt exist. PDDF mode can not be enabled")
            sys.exit(1)

    # ##########################################################################
    # Take a backup of orig fancontrol
    if os.path.exists(device_path+"/fancontrol"):
        log_os_system("mv "+device_path+"/fancontrol"+" "+device_path+"/fancontrol.bak", 0)
    
    # Create a link to fancontrol of PDDF
    if os.path.exists(device_path+"/pddf/fancontrol") and not os.path.exists(device_path+"/fancontrol"):
        shutil.copy(device_path+"/pddf/fancontrol",device_path+"/fancontrol")

    # BMC support
    f_sensors="/usr/bin/sensors"
    f_sensors_org="/usr/bin/sensors.org"
    f_pddf_sensors="/usr/local/bin/pddf_sensors"
    if os.path.exists(f_pddf_sensors) is True:
        if os.path.exists(f_sensors_org) is False:
            shutil.copy(f_sensors, f_sensors_org)
        shutil.copy(f_pddf_sensors, f_sensors)


    return 0

def cleanup_pddf_utils():
    device_path = get_path_to_device()
    SONIC_PLATFORM_BSP_WHL_PKG = "/".join([device_path, 'sonic_platform-1.0-py3-none-any.whl'])
    SONIC_PLATFORM_PDDF_WHL_PKG = "/".join([device_path, 'pddf', 'sonic_platform-1.0-py3-none-any.whl'])
    SONIC_PLATFORM_BSP_WHL_PKG_BK = "/".join([device_path, 'sonic_platform-1.0-py3-none-any.whl.orig'])
    # ##########################################################################
    status, output = log_os_system("pip3 show sonic-platform > /dev/null 2>&1", 1)
    if status:
        # PDDF Platform API 2.0 is not supported but system is in PDDF mode, hence PDDF 1.0 plugins are present
        device_plugin_path = "/".join([device_path, "plugins"])
        backup_path = "/".join([device_plugin_path, "orig"])
        if os.path.exists(backup_path) is True:
            for item in os.listdir(device_plugin_path):
                if os.path.isdir(device_plugin_path+"/"+item) is False:
                    os.remove(device_plugin_path+"/"+item)

            log_os_system("mv "+backup_path+"/*"+" "+device_plugin_path, 1)
            os.rmdir(backup_path)
        else:
            print("\nERR: Unable to locate original device files...\n")

    else:
        # PDDF 2.0 apis are supported and PDDF whl package is installed
        if os.path.exists(SONIC_PLATFORM_PDDF_WHL_PKG):
            if os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG_BK):
                # platform is 2.0 compliant and original bsp 2.0 whl package exist
                log_os_system('mv '+SONIC_PLATFORM_BSP_WHL_PKG_BK+' '+SONIC_PLATFORM_BSP_WHL_PKG, 1)
                status, output = log_os_system("pip3 uninstall sonic-platform -y &> /dev/null", 1)
                if status:
                    print("Error: Unable to uninstall PDDF sonic-platform whl package")
                    return status
                print("Attemting to install the BSP sonic_platform wheel package ...")
                status, output = log_os_system("pip3 install "+ SONIC_PLATFORM_BSP_WHL_PKG, 1)
                if status:
                    print("Error: Failed to install {}".format(SONIC_PLATFORM_BSP_WHL_PKG))
                    return status
                else:
                    print("Successfully installed {} package".format(SONIC_PLATFORM_BSP_WHL_PKG))
            else:
                # platform doesnt support 2.0 APIs but PDDF is 2.0 based
                # remove and uninstall the PDDF whl package
                if os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG):
                    os.remove(SONIC_PLATFORM_BSP_WHL_PKG)
                status, output = log_os_system("pip3 uninstall sonic-platform -y &> /dev/null", 1)
                if status:
                    print("Error: Unable to uninstall PDDF sonic-platform whl package")
                    return status
        else:
            # something seriously wrong. System is in PDDF mode but pddf whl pkg is not present
            print("Error: Fatal error as the system is in PDDF mode but the pddf .whl original is not present")
    # ################################################################################################################

    if os.path.exists(device_path+"/fancontrol"):
        os.remove(device_path+"/fancontrol")

    if os.path.exists(device_path+"/fancontrol.bak"):
        log_os_system("mv "+device_path+"/fancontrol.bak"+" "+device_path+"/fancontrol", 0)

    # BMC support
    f_sensors="/usr/bin/sensors"
    f_sensors_org="/usr/bin/sensors.org"
    if os.path.exists(f_sensors_org) is True:
        shutil.copy(f_sensors_org, f_sensors)

    return 0

def create_pddf_log_files():
    if not os.path.exists('/var/log/pddf'):
        log_os_system("sudo mkdir /var/log/pddf", 1)

    log_os_system("sudo touch /var/log/pddf/led.txt", 1)
    log_os_system("sudo touch /var/log/pddf/psu.txt", 1)
    log_os_system("sudo touch /var/log/pddf/fan.txt", 1)
    log_os_system("sudo touch /var/log/pddf/xcvr.txt", 1)
    log_os_system("sudo touch /var/log/pddf/sysstatus.txt", 1)
    log_os_system("sudo touch /var/log/pddf/cpld.txt", 1)
    log_os_system("sudo touch /var/log/pddf/cpldmux.txt", 1)
    log_os_system("sudo touch /var/log/pddf/client.txt", 1)
    log_os_system("sudo touch /var/log/pddf/mux.txt", 1)
    log_os_system("sudo touch /var/log/pddf/fpgapci.txt", 1)

def driver_install():
    global FORCE

    # check for pre_driver_install script
    if os.path.exists('/usr/local/bin/pddf_pre_driver_install.sh'):
        status, output = log_os_system('/usr/local/bin/pddf_pre_driver_install.sh', 1)
        if status:
            print("Error: pddf_pre_driver_install script failed with error %d"%status)
            return status
        # For debug
        print(output)

    # Removes the perm_kos first, then reload them in a proper sequence
    for mod in perm_kos:
        cmd = "modprobe -rq " + mod
        status, output = log_os_system(cmd, 1)
        if status:
            print("driver_install: Unable to unload {}".format(mod))
            # Don't exit but continue

    log_os_system("depmod", 1)

    # Load "normal" kos first
    for mod in perm_kos + std_kos:
        status, output = log_os_system("modprobe " + mod, 1)
        if status:
            print("driver_install() failed with error %d"%status)
            if FORCE == 0:
                return status

    # Load "custom" kos now.  On failure, retry with force flag.  Force flag is not
    # allowed to be used on signed modules so do not try that first.
    for mod in custom_kos:
        status, output = log_os_system("modprobe " + mod, 1)
        if not status:
            continue

        print("driver_install() failed with error %d retrying with force"%status)
        status, output = log_os_system("modprobe -f " + mod, 1)
        if status:
            print("driver_install(force) failed with error %d"%status)
            if FORCE == 0:
                return status

    output = config_pddf_utils()
    if output:
        print("config_pddf_utils() failed with error %d"%output)
    # check for post_driver_install script
    if os.path.exists('/usr/local/bin/pddf_post_driver_install.sh'):
        status, output = log_os_system('/usr/local/bin/pddf_post_driver_install.sh', 1)
        if status:
            print("Error: pddf_post_driver_install script failed with error %d"%status)
            return status
        # Useful for debugging
        print(output)


    return 0
    
def driver_uninstall():
    global FORCE

    status = cleanup_pddf_utils()
    if status:
        print("cleanup_pddf_utils() failed with error %d"%status)

    for mod in std_kos + custom_kos:
        # do not remove i2c-i801 modules
        if "i2c-i801" in mod:
            continue

        status, output = log_os_system("modprobe -rq " + mod, 1)
        if status:
            print("driver_uninstall() failed with error %d"%status)
            if FORCE == 0:        
                return status              
    return 0

def device_install():
    global FORCE

    # check for pre_device_creation script
    if os.path.exists('/usr/local/bin/pddf_pre_device_create.sh'):
        status, output = log_os_system('/usr/local/bin/pddf_pre_device_create.sh', 1)
        if status:
            print("Error: pddf_pre_device_create script failed with error %d"%status)
            return status

    # trigger the pddf_obj script for FAN, PSU, CPLD, MUX, etc
    status = pddf_obj.create_pddf_devices()
    if status:
        print("Error: create_pddf_devices() failed with error %d"%status)
        if FORCE == 0:
            return status

    # check for post_device_create script
    if os.path.exists('/usr/local/bin/pddf_post_device_create.sh'):
        status, output = log_os_system('/usr/local/bin/pddf_post_device_create.sh', 1)
        if status:
            print("Error: pddf_post_device_create script failed with error %d"%status)
            return status
        # Useful for debugging
        print(output)

    return

def device_uninstall():
    global FORCE
    # Trigger the paloparse script for deletion of FAN, PSU, OPTICS, CPLD clients
    status = pddf_obj.delete_pddf_devices()
    if status:
        print("Error: delete_pddf_devices() failed with error %d"%status)
        if FORCE == 0:
            return status
    return 
        
def do_install():
    print("Checking system....")
    if not os.path.exists('/usr/share/sonic/platform/pddf_support'):
        print(PROJECT_NAME.upper() +" mode is not enabled")
        return

    if driver_check()== False :
        print(PROJECT_NAME.upper() +" has no PDDF driver installed....")
        create_pddf_log_files()
        print("Installing ...")
        status = driver_install()
        if status:
            return  status
    else:
        print(PROJECT_NAME.upper() +" drivers detected....")

    print("Creating devices ...")
    status = device_install()
    if status:
        return status

    # Check if S3IP support is enabled, if yes, start the service in no block mode
    if 'enable_s3ip' in pddf_obj.data['PLATFORM'].keys() and pddf_obj.data['PLATFORM']['enable_s3ip'] == 'yes':
        log_os_system('systemctl enable pddf-s3ip-init.service', 1)
        log_os_system('systemctl start --no-block pddf-s3ip-init.service', 1)

    return
    
def do_uninstall():
    print("Checking system....")
    if not os.path.exists('/usr/share/sonic/platform/pddf_support'):
        print(PROJECT_NAME.upper() +" mode is not enabled")
        return


    if os.path.exists('/var/log/pddf'):
        print("Remove pddf log files.....")
        log_os_system("sudo rm -rf /var/log/pddf", 1)

    print("Remove all the devices...")
    status = device_uninstall()
    if status:
        return status


    if driver_check()== False :
        print(PROJECT_NAME.upper() +" has no PDDF driver installed....")
    else:
        print("Removing installed driver....")
        status = driver_uninstall()
        if status:
            if FORCE == 0:        
                return  status                          
    return       

def do_switch_pddf():
    try:
        import pddf_switch_svc
    except ImportError:
        print("Unable to find pddf_switch_svc.py. PDDF might not be supported on this platform")
        sys.exit()
    print("Check the pddf support...")
    status = pddf_switch_svc.check_pddf_support()
    if not status:
        print("PDDF is not supported on this platform")
        return status


    print("Checking system....")
    if os.path.exists('/usr/share/sonic/platform/pddf_support'):
        print(PROJECT_NAME.upper() +" system is already in pddf mode....")
    else:
        print("Check if the native sonic-platform whl package is installed in the pmon docker")
        status, output = log_os_system("docker exec -it pmon pip3 show sonic-platform", 1)
        if not status:
            # Need to remove this whl module
            status, output = log_os_system("docker exec -it pmon pip3 uninstall sonic-platform -y", 1)
            if not status:
                print("Successfully uninstalled the native sonic-platform whl pkg from pmon container")
            else:
                print("Error: Unable to uninstall the sonic-platform whl pkg from pmon container."
                        " Do it manually before moving to nonpddf mode")
                return status
        print("Stopping the pmon service ...")
        status, output = log_os_system("systemctl stop pmon.service", 1)
        if status:
            print("Pmon stop failed")
            if FORCE==0:
                return status

        print("Stopping the platform services..")
        status = pddf_switch_svc.stop_platform_svc()
        if not status:
            if FORCE==0:
                return status

        print("Creating the pddf_support file...")
        if os.path.exists('/usr/share/sonic/platform'):
            log_os_system("touch /usr/share/sonic/platform/pddf_support", 1)
        else:
            print("/usr/share/sonic/platform path doesn't exist. Unable to set pddf mode")
            return -1

        print("Starting the PDDF platform service...")
        status = pddf_switch_svc.start_platform_pddf()
        if not status:
            if FORCE==0:
                return status

        print("Restart the pmon service ...")
        status, output = log_os_system("systemctl start pmon.service", 1)
        if status:
            print("Pmon restart failed")
            if FORCE==0:
                return status

        return

def do_switch_nonpddf():
    try:
        import pddf_switch_svc
    except ImportError:
        print("Unable to find pddf_switch_svc.py. PDDF might not be supported on this platform")
        sys.exit()
    print("Checking system....")
    if not os.path.exists('/usr/share/sonic/platform/pddf_support'):
        print(PROJECT_NAME.upper() +" system is already in non-pddf mode....")
    else:
        print("Check if the sonic-platform whl package is installed in the pmon docker")
        status, output = log_os_system("docker exec -it pmon pip3 show sonic-platform", 1)
        if not status:
            # Need to remove this whl module
            status, output = log_os_system("docker exec -it pmon pip3 uninstall sonic-platform -y", 1)
            if not status:
                print("Successfully uninstalled the sonic-platform whl pkg from pmon container")
            else:
                print("Error: Unable to uninstall the sonic-platform whl pkg from pmon container."
                        " Do it manually before moving to nonpddf mode")
                return status
        print("Stopping the pmon service ...")
        status, output = log_os_system("systemctl stop pmon.service", 1)
        if status:
            print("Stopping pmon service failed")
            if FORCE==0:
                return status

        print("Stopping the PDDF platform service...")
        status = pddf_switch_svc.stop_platform_pddf()
        if not status:
            if FORCE==0:
                return status

        print("Removing the pddf_support file...")
        if os.path.exists('/usr/share/sonic/platform'):
            log_os_system("rm -f /usr/share/sonic/platform/pddf_support", 1)
        else:
            print("/usr/share/sonic/platform path doesnt exist. Unable to set non-pddf mode")
            return -1

        print("Starting the platform services...")
        status = pddf_switch_svc.start_platform_svc()
        if not status:
            if FORCE==0:
                return status

        print("Restart the pmon service ...")
        status, output = log_os_system("systemctl start pmon.service", 1)
        if status:
            print("Restarting pmon service failed")
            if FORCE==0:
                return status

        return

if __name__ == "__main__":
    main()
