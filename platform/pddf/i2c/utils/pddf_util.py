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
logger = logging.getLogger("pddf.util")
_handler = logging.StreamHandler()
_handler.setFormatter(logging.Formatter("%(levelname)s: [%(funcName)s:%(lineno)d] %(message)s"))
logger.addHandler(_handler)
logger.setLevel(logging.INFO)
logger.propagate = False
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
    logger.debug(sys.argv[0])
    logger.debug('ARGV      : %s', sys.argv[1:])

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
        logger.debug("options: %s", options)
        logger.debug("args: %s", args)
        logger.debug("argc: %d", len(sys.argv))
    
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
            logging.getLogger("pddf").setLevel(logging.DEBUG)
        elif opt in ('-f', '--force'): 
            FORCE = 1
        else:
            logger.info('no option')                          
    for arg in args:            
        if arg == 'install':
            try:
                status = do_install()
            except Exception:
                logger.exception("Driver initialization failed")
                sys.exit(1)
            if status:
                logger.error("do_install failed (rc=%s)", status)
                sys.exit(1)
        elif arg == 'clean':
            status = do_uninstall()
            if status:
                logger.error("do_uninstall failed (rc=%s)", status)
                sys.exit(1)
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
    logger.debug("[PDDF]%s", txt)
    return
    
def log_os_system(cmd):
    logger.info('Run :'+cmd)
    status, output = subprocess.getstatusoutput(cmd)
    logger.debug("%s with result: %s", cmd, status)
    logger.debug("      output: "+ output)
    if status:
        logger.error('Failed :'+cmd)
    return  status, output
            
def driver_check():
    ret, lsmod = log_os_system("lsmod| grep pddf")
    if ret:
        return False
    logger.info('mods:'+lsmod)
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
    status, output = log_os_system("pip3 show sonic-platform > /dev/null 2>&1")
    if status:
        if os.path.exists(SONIC_PLATFORM_PDDF_WHL_PKG):
            # Platform API 2.0 is supported
            if os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG):
                # bsp whl pkg is present but not installed on host <special case>
                if not os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG_BK):
                    log_os_system('mv '+SONIC_PLATFORM_BSP_WHL_PKG+' '+SONIC_PLATFORM_BSP_WHL_PKG_BK)
            # PDDF whl package exist ... this must be the whl package created from 
            # PDDF 2.0 ref API classes and some changes on top of it ... install it
            log_os_system('sync')
            shutil.copy(SONIC_PLATFORM_PDDF_WHL_PKG, SONIC_PLATFORM_BSP_WHL_PKG)
            log_os_system('sync')
            logger.info("Attempting to install the PDDF sonic_platform wheel package")
            if os.path.getsize(SONIC_PLATFORM_BSP_WHL_PKG) != 0:
                status, output = log_os_system("pip3 install "+ SONIC_PLATFORM_BSP_WHL_PKG)
                if status:
                    logger.error("Failed to install %s", SONIC_PLATFORM_BSP_WHL_PKG)
                    return status
                else:
                    logger.info("Successfully installed %s package", SONIC_PLATFORM_BSP_WHL_PKG)
            else:
                logger.error("Failed to copy %s properly. Exiting", SONIC_PLATFORM_PDDF_WHL_PKG)
                return -1
        else:
            # PDDF with platform APIs 1.5 must be supported
            device_plugin_path = "/".join([device_path, "plugins"])
            backup_path = "/".join([device_plugin_path, "orig"])
            logger.info("Loading PDDF generic plugins (1.0)")
            if os.path.exists(backup_path) is False:
                os.mkdir(backup_path)
                log_os_system("mv "+device_plugin_path+"/*.*"+" "+backup_path)
            
            for item in os.listdir(pddf_path):
                shutil.copy(pddf_path+"/"+item, device_plugin_path+"/"+item)
            
            shutil.copy('/usr/local/bin/pddfparse.py', device_plugin_path+"/pddfparse.py")

    else:
        # sonic_platform whl pkg is installed 2 possibilities, 1) bsp 2.0 classes
        # are installed, 2) system rebooted and either pddf/bsp 2.0 classes are already installed
        if os.path.exists(SONIC_PLATFORM_PDDF_WHL_PKG):
            if not os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG_BK):
                # bsp 2.0 classes are installed. Take a backup and copy pddf 2.0 whl pkg
                log_os_system('mv '+SONIC_PLATFORM_BSP_WHL_PKG+' '+SONIC_PLATFORM_BSP_WHL_PKG_BK)
                log_os_system('sync')
                shutil.copy(SONIC_PLATFORM_PDDF_WHL_PKG, SONIC_PLATFORM_BSP_WHL_PKG)
                log_os_system('sync')
                # uninstall the existing bsp whl pkg
                status, output = log_os_system("pip3 uninstall sonic-platform -y &> /dev/null")
                if status:
                    logger.error("Unable to uninstall BSP sonic-platform whl package")
                    return status
                logger.info("Attempting to install the PDDF sonic_platform wheel package")
                if os.path.getsize(SONIC_PLATFORM_BSP_WHL_PKG) != 0:
                    status, output = log_os_system("pip3 install "+ SONIC_PLATFORM_BSP_WHL_PKG)
                    if status:
                        logger.error("Failed to install %s", SONIC_PLATFORM_BSP_WHL_PKG)
                        return status
                    else:
                        logger.info("Successfully installed %s package", SONIC_PLATFORM_BSP_WHL_PKG)
                else:
                    logger.error("Failed to copy %s properly. Exiting", SONIC_PLATFORM_PDDF_WHL_PKG)
                    return -1
            else:
                # system rebooted in pddf mode 
                logger.info("System rebooted in PDDF mode, hence keeping the PDDF 2.0 classes")
        else:
            # pddf whl package doesnt exist
            logger.error("PDDF 2.0 classes don't exist. PDDF mode can not be enabled")
            sys.exit(1)

    # ##########################################################################
    # Take a backup of orig fancontrol
    if os.path.exists(device_path+"/fancontrol"):
        log_os_system("mv "+device_path+"/fancontrol"+" "+device_path+"/fancontrol.bak")
    
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
    status, output = log_os_system("pip3 show sonic-platform > /dev/null 2>&1")
    if status:
        # PDDF Platform API 2.0 is not supported but system is in PDDF mode, hence PDDF 1.0 plugins are present
        device_plugin_path = "/".join([device_path, "plugins"])
        backup_path = "/".join([device_plugin_path, "orig"])
        if os.path.exists(backup_path) is True:
            for item in os.listdir(device_plugin_path):
                if os.path.isdir(device_plugin_path+"/"+item) is False:
                    os.remove(device_plugin_path+"/"+item)

            log_os_system("mv "+backup_path+"/*"+" "+device_plugin_path)
            os.rmdir(backup_path)
        else:
            logger.error("Unable to locate original device files")

    else:
        # PDDF 2.0 apis are supported and PDDF whl package is installed
        if os.path.exists(SONIC_PLATFORM_PDDF_WHL_PKG):
            if os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG_BK):
                # platform is 2.0 compliant and original bsp 2.0 whl package exist
                log_os_system('mv '+SONIC_PLATFORM_BSP_WHL_PKG_BK+' '+SONIC_PLATFORM_BSP_WHL_PKG)
                status, output = log_os_system("pip3 uninstall sonic-platform -y &> /dev/null")
                if status:
                    logger.error("Unable to uninstall PDDF sonic-platform whl package")
                    return status
                logger.info("Attempting to install the BSP sonic_platform wheel package")
                status, output = log_os_system("pip3 install "+ SONIC_PLATFORM_BSP_WHL_PKG)
                if status:
                    logger.error("Failed to install %s", SONIC_PLATFORM_BSP_WHL_PKG)
                    return status
                else:
                    logger.info("Successfully installed %s package", SONIC_PLATFORM_BSP_WHL_PKG)
            else:
                # platform doesnt support 2.0 APIs but PDDF is 2.0 based
                # remove and uninstall the PDDF whl package
                if os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG):
                    os.remove(SONIC_PLATFORM_BSP_WHL_PKG)
                status, output = log_os_system("pip3 uninstall sonic-platform -y &> /dev/null")
                if status:
                    logger.error("Unable to uninstall PDDF sonic-platform whl package")
                    return status
        else:
            # something seriously wrong. System is in PDDF mode but pddf whl pkg is not present
            logger.error("Fatal error as the system is in PDDF mode but the pddf .whl original is not present")
    # ################################################################################################################

    if os.path.exists(device_path+"/fancontrol"):
        os.remove(device_path+"/fancontrol")

    if os.path.exists(device_path+"/fancontrol.bak"):
        log_os_system("mv "+device_path+"/fancontrol.bak"+" "+device_path+"/fancontrol")

    # BMC support
    f_sensors="/usr/bin/sensors"
    f_sensors_org="/usr/bin/sensors.org"
    if os.path.exists(f_sensors_org) is True:
        shutil.copy(f_sensors_org, f_sensors)

    return 0

def driver_install():
    global FORCE
    logger.info("PDDF driver_install: starting")

    # check for pre_driver_install script
    if os.path.exists('/usr/local/bin/pddf_pre_driver_install.sh'):
        logger.info("PDDF driver_install: running pre_driver_install script")
        status, output = log_os_system('/usr/local/bin/pddf_pre_driver_install.sh')
        if status:
            logger.error("PDDF driver_install: pre_driver_install script failed (rc=%d)", status)
            return status
        logger.debug("pre_driver_install output: %s", output)

    # Removes the perm_kos first, then reload them in a proper sequence
    for mod in perm_kos:
        cmd = "modprobe -rq " + mod
        status, output = log_os_system(cmd)
        if status:
            logger.warning("PDDF driver_install: unable to unload module %s", mod)
            # Don't exit but continue

    log_os_system("depmod")

    # Load "normal" kos first
    logger.info("PDDF driver_install: loading %d standard kernel modules", len(perm_kos + std_kos))
    for mod in perm_kos + std_kos:
        status, output = log_os_system("modprobe " + mod)
        if status:
            logger.error("PDDF driver_install: failed to load module %s (rc=%d)", mod, status)
            if FORCE == 0:
                return status

    # Load "custom" kos now.  On failure, retry with force flag.  Force flag is not
    # allowed to be used on signed modules so do not try that first.
    if custom_kos:
        logger.info("PDDF driver_install: loading %d custom kernel modules", len(custom_kos))
    for mod in custom_kos:
        status, output = log_os_system("modprobe " + mod)
        if not status:
            continue

        logger.warning("PDDF driver_install: module %s failed, retrying with force", mod)
        status, output = log_os_system("modprobe -f " + mod)
        if status:
            logger.error("PDDF driver_install: module %s force load also failed (rc=%d)", mod, status)
            if FORCE == 0:
                return status

    logger.info("PDDF driver_install: configuring PDDF utilities")
    output = config_pddf_utils()
    if output:
        logger.error("PDDF driver_install: config_pddf_utils failed (rc=%d)", output)
    # check for post_driver_install script
    if os.path.exists('/usr/local/bin/pddf_post_driver_install.sh'):
        logger.info("PDDF driver_install: running post_driver_install script")
        status, output = log_os_system('/usr/local/bin/pddf_post_driver_install.sh')
        if status:
            logger.error("PDDF driver_install: post_driver_install script failed (rc=%d)", status)
            return status
        logger.debug("post_driver_install output: %s", output)

    logger.info("PDDF driver_install: completed successfully")
    return 0
    
def driver_uninstall():
    global FORCE

    status = cleanup_pddf_utils()
    if status:
        logger.error("cleanup_pddf_utils failed (rc=%d)", status)

    for mod in std_kos + custom_kos:
        # do not remove i2c-i801 modules
        if "i2c-i801" in mod:
            continue

        status, output = log_os_system("modprobe -rq " + mod)
        if status:
            logger.error("module unload failed (rc=%d)", status)
            if FORCE == 0:        
                return status              
    return 0

def device_install():
    global FORCE
    logger.info("PDDF device_install: starting device creation")

    # check for pre_device_creation script
    if os.path.exists('/usr/local/bin/pddf_pre_device_create.sh'):
        logger.info("PDDF device_install: running pre_device_create script")
        status, output = log_os_system('/usr/local/bin/pddf_pre_device_create.sh')
        if status:
            logger.error("PDDF device_install: pre_device_create script failed (rc=%d)", status)
            return status

    # trigger the pddf_obj script for FAN, PSU, CPLD, MUX, etc
    logger.info("PDDF device_install: creating PDDF devices (FAN, PSU, CPLD, MUX, etc.)")
    status = pddf_obj.create_pddf_devices()
    if status:
        logger.error("PDDF device_install: create_pddf_devices failed (rc=%d)", status)
        if FORCE == 0:
            return status

    # check for post_device_create script
    if os.path.exists('/usr/local/bin/pddf_post_device_create.sh'):
        logger.info("PDDF device_install: running post_device_create script")
        status, output = log_os_system('/usr/local/bin/pddf_post_device_create.sh')
        if status:
            logger.error("PDDF device_install: post_device_create script failed (rc=%d)", status)
            return status
        logger.debug("post_device_create output: %s", output)

    logger.info("PDDF device_install: device creation completed successfully")
    return

def device_uninstall():
    global FORCE
    # Trigger the paloparse script for deletion of FAN, PSU, OPTICS, CPLD clients
    status = pddf_obj.delete_pddf_devices()
    if status:
        logger.error("delete_pddf_devices failed (rc=%d)", status)
        if FORCE == 0:
            return status
    return 
        
def do_install():
    logger.info("PDDF install: starting system check")
    if not os.path.exists('/usr/share/sonic/platform/pddf_support'):
        logger.warning("PDDF install: pddf_support file not found, PDDF mode is not enabled")
        return

    if driver_check()== False :
        logger.info("%s has no PDDF driver installed", PROJECT_NAME.upper())
        logger.info("PDDF install: installing drivers")
        status = driver_install()
        if status:
            logger.error("PDDF install: driver installation failed (rc=%d)", status)
            return  status
        logger.info("PDDF install: driver installation completed successfully")
    else:
        logger.info("PDDF install: drivers already loaded, skipping driver install")

    logger.info("PDDF install: creating devices")
    status = device_install()
    if status:
        logger.error("PDDF install: device creation failed (rc=%d)", status)
        return status

    # Check if S3IP support is enabled, if yes, start the service in no block mode
    if 'enable_s3ip' in pddf_obj.data['PLATFORM'].keys() and pddf_obj.data['PLATFORM']['enable_s3ip'] == 'yes':
        logger.info("PDDF install: enabling S3IP service")
        log_os_system('systemctl enable pddf-s3ip-init.service')
        log_os_system('systemctl start --no-block pddf-s3ip-init.service')

    logger.info("PDDF install: completed successfully")
    return
    
def do_uninstall():
    logger.info("PDDF uninstall: starting")
    if not os.path.exists('/usr/share/sonic/platform/pddf_support'):
        logger.warning("PDDF uninstall: pddf_support file not found, PDDF mode is not enabled")
        return


    if os.path.exists('/var/log/pddf'):
        logger.info("PDDF uninstall: removing pddf log files")
        log_os_system("sudo rm -rf /var/log/pddf")

    logger.info("PDDF uninstall: removing devices")
    status = device_uninstall()
    if status:
        logger.error("PDDF uninstall: device removal failed (rc=%d)", status)
        return status


    if driver_check()== False :
        logger.info("%s has no PDDF driver installed", PROJECT_NAME.upper())
    else:
        logger.info("PDDF uninstall: removing drivers")
        status = driver_uninstall()
        if status:
            logger.error("PDDF uninstall: driver removal failed (rc=%d)", status)
            if FORCE == 0:        
                return  status                          
    logger.info("PDDF uninstall: completed")
    return       

def do_switch_pddf():
    try:
        import pddf_switch_svc
    except ImportError:
        logger.error("Unable to find pddf_switch_svc.py. PDDF might not be supported on this platform")
        sys.exit()
    logger.info("Checking PDDF support")
    status = pddf_switch_svc.check_pddf_support()
    if not status:
        logger.warning("PDDF is not supported on this platform")
        return status


    logger.info("Checking system")
    if os.path.exists('/usr/share/sonic/platform/pddf_support'):
        logger.info("%s system is already in pddf mode", PROJECT_NAME.upper())
    else:
        logger.info("Checking if native sonic-platform whl package is installed in pmon docker")
        status, output = log_os_system("docker exec -it pmon pip3 show sonic-platform")
        if not status:
            # Need to remove this whl module
            status, output = log_os_system("docker exec -it pmon pip3 uninstall sonic-platform -y")
            if not status:
                logger.info("Successfully uninstalled the native sonic-platform whl pkg from pmon container")
            else:
                logger.error("Unable to uninstall the sonic-platform whl pkg from pmon container."
                        " Do it manually before moving to nonpddf mode")
                return status
        logger.info("Stopping the pmon service")
        status, output = log_os_system("systemctl stop pmon.service")
        if status:
            logger.error("Pmon stop failed")
            if FORCE==0:
                return status

        logger.info("Stopping the platform services")
        status = pddf_switch_svc.stop_platform_svc()
        if not status:
            if FORCE==0:
                return status

        logger.info("Creating the pddf_support file")
        if os.path.exists('/usr/share/sonic/platform'):
            log_os_system("touch /usr/share/sonic/platform/pddf_support")
        else:
            logger.error("/usr/share/sonic/platform path doesn't exist. Unable to set pddf mode")
            return -1

        logger.info("Starting the PDDF platform service")
        status = pddf_switch_svc.start_platform_pddf()
        if not status:
            if FORCE==0:
                return status

        logger.info("Restarting the pmon service")
        status, output = log_os_system("systemctl start pmon.service")
        if status:
            logger.error("Pmon restart failed")
            if FORCE==0:
                return status

        return

def do_switch_nonpddf():
    try:
        import pddf_switch_svc
    except ImportError:
        logger.error("Unable to find pddf_switch_svc.py. PDDF might not be supported on this platform")
        sys.exit()
    logger.info("Checking system")
    if not os.path.exists('/usr/share/sonic/platform/pddf_support'):
        logger.info("%s system is already in non-pddf mode", PROJECT_NAME.upper())
    else:
        logger.info("Checking if sonic-platform whl package is installed in pmon docker")
        status, output = log_os_system("docker exec -it pmon pip3 show sonic-platform")
        if not status:
            # Need to remove this whl module
            status, output = log_os_system("docker exec -it pmon pip3 uninstall sonic-platform -y")
            if not status:
                logger.info("Successfully uninstalled the sonic-platform whl pkg from pmon container")
            else:
                logger.error("Unable to uninstall the sonic-platform whl pkg from pmon container."
                        " Do it manually before moving to nonpddf mode")
                return status
        logger.info("Stopping the pmon service")
        status, output = log_os_system("systemctl stop pmon.service")
        if status:
            logger.error("Stopping pmon service failed")
            if FORCE==0:
                return status

        logger.info("Stopping the PDDF platform service")
        status = pddf_switch_svc.stop_platform_pddf()
        if not status:
            if FORCE==0:
                return status

        logger.info("Removing the pddf_support file")
        if os.path.exists('/usr/share/sonic/platform'):
            log_os_system("rm -f /usr/share/sonic/platform/pddf_support")
        else:
            logger.error("/usr/share/sonic/platform path doesnt exist. Unable to set non-pddf mode")
            return -1

        logger.info("Starting the platform services")
        status = pddf_switch_svc.start_platform_svc()
        if not status:
            if FORCE==0:
                return status

        logger.info("Restarting the pmon service")
        status, output = log_os_system("systemctl start pmon.service")
        if status:
            logger.error("Restarting pmon service failed")
            if FORCE==0:
                return status

        return

if __name__ == "__main__":
    main()
