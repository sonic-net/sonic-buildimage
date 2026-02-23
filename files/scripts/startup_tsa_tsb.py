#!/usr/bin/env python3

# Name: startup_tsa_tsb.py, version: 1.0
#
# Description: Module contains the definitions to the VOQ Startup TSA-TSB service
from sonic_py_common import multi_asic, device_info
from sonic_py_common.logger import Logger
import subprocess
import sys, getopt
from threading import Timer
import os

# Global Logger class instance
logger = Logger("startup_tsa_tsb")
logger.set_min_log_priority_info()

def get_tsb_timer_interval():
    platform = device_info.get_platform()
    conf_file = '/usr/share/sonic/device/{}/startup-tsa-tsb.conf'.format(platform)
    file = open(conf_file, 'r')
    Lines = file.readlines()
    for line in Lines:
        field = line.split('=')[0].strip()
        if field == "STARTUP_TSB_TIMER":
            return line.split('=')[1].strip()
    return 0

def get_sonic_config(ns, config_name):
    if ns == "":
        return subprocess.check_output(['sonic-cfggen', '-d', '-v', config_name.replace('"', "'"), ]).strip()
    else:
        return subprocess.check_output(['sonic-cfggen', '-d', '-v', config_name.replace('"', "'"), '-n', ns.replace('"', "'")]).strip()

def get_sub_role(asic_ns):
    sub_role_config = "DEVICE_METADATA['localhost']['sub_role']"
    sub_role = (get_sonic_config(asic_ns, sub_role_config)).decode()
    return sub_role

def get_tsa_config(asic_ns):
    tsa_config = 'BGP_DEVICE_GLOBAL.STATE.tsa_enabled'
    tsa_ena = (get_sonic_config(asic_ns, tsa_config)).decode()
    if asic_ns == "":
        logger.log_info('CONFIG_DB.{} : {}'.format(tsa_config, tsa_ena))
    else:
        logger.log_info('{} - CONFIG_DB.{} : {}'.format(asic_ns, tsa_config, tsa_ena))
    return tsa_ena

def get_tsa_status(num_asics):
    if num_asics > 1:
        counter = 0
        for asic_id in range(int(num_asics)):
            asic_ns = 'asic{}'.format(asic_id)
            sub_role = get_sub_role(asic_ns)
            if sub_role == 'FrontEnd':
                tsa_enabled = get_tsa_config(asic_ns)
                if tsa_enabled == 'false':
                    counter += 1
        if counter == int(num_asics):
            return True;
    else:
        tsa_enabled = get_tsa_config("")
        if tsa_enabled == 'false':
            return True;
    return False;

def config_tsa():
    num_asics = multi_asic.get_num_asics()
    tsa_ena = get_tsa_status(num_asics)
    if tsa_ena == True:
        logger.log_info("Setting TSA-TSB service field in STATE_DB")
        subprocess.check_output([
            'sonic-db-cli', 'STATE_DB', 'HSET', 'ALL_SERVICE_STATUS|tsa_tsb_service', 'running', 'OK'
        ]).strip()
        logger.log_info("Configuring TSA")
        subprocess.check_output(['TSA']).strip()
    else:
        #check if tsa_tsb service is already running, restart the timer
        try:
            startup_tsa_tsb_service_status = subprocess.check_output([
                'sonic-db-cli', 'STATE_DB', 'HGET', 'ALL_SERVICE_STATUS|tsa_tsb_service', 'running'
            ]).strip().decode('utf-8')  # Convert bytes to string
        except subprocess.CalledProcessError:
            startup_tsa_tsb_service_status = None  # Default if the field is missing

        if startup_tsa_tsb_service_status == 'OK':
            logger.log_info("TSA-TSB service is already running, just restart the timer")
            # execute TSA again: this is to overcome race condition where in its previous run, TSA configuration didnt complete on all asics
            subprocess.check_output(['TSA']).strip()
            return True
        else:
            if num_asics > 1:
                logger.log_info("Either TSA is already configured or switch sub_role is not Frontend - not configuring TSA")
            else:
                logger.log_info("Either TSA is already configured - not configuring TSA")
    return tsa_ena

def config_tsb():
    logger.log_info("Configuring TSB")
    subprocess.check_output(['TSB']).strip()

    logger.log_info("Removing the TSA-TSB service field from STATE_DB")
    subprocess.check_output([
        'sonic-db-cli', 'STATE_DB', 'HDEL', 'ALL_SERVICE_STATUS|tsa_tsb_service', 'running'
    ]).strip()

    tsb_issued = True
    return


def start_tsb_timer(interval):
    global timer
    logger.log_info("Starting timer with interval {} seconds to configure TSB".format(interval))
    timer = Timer(int(interval), config_tsb)
    timer.start()
    timer.join()
    return

def print_usage():
    logger.log_info("Usage: startup_tsa_tsb.py [options] command")
    logger.log_info("options:")
    logger.log_info("  -h | --help       : this help message")
    logger.log_info("command:")
    logger.log_info("start     : start the TSA/TSB")
    logger.log_info("stop      : stop the TSA/TSB")
    return

def reset_env_variables():
    logger.log_info("Resetting environment variable")
    os.environ.pop('STARTED_BY_TSA_TSB_SERVICE')
    return

def start_tsa_tsb(timer):
    #Configure TSA if it was not configured already in CONFIG_DB
    tsa_enabled = config_tsa()
    if tsa_enabled == True:
        #Start the timer to configure TSB
        start_tsb_timer(timer)
    return

def stop_tsa_tsb():
    reset_env_variables()
    return

def main():
    platform = device_info.get_platform()
    conf_file = '/usr/share/sonic/device/{}/startup-tsa-tsb.conf'.format(platform)
    #This check should be moved to service file or make this feature as configurable.
    #Adding it here for now.
    if not os.path.exists(conf_file):
        logger.log_info("{} does not exist, exiting the service".format(conf_file))
        return
    if len(sys.argv) <= 1:
        print_usage()
        return

    # parse command line options:
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'h:', ['help' ])
    except getopt.GetoptError:
        print_usage()
        return

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            print_usage()
            return

    for arg in args:
        if arg == 'start':
            tsb_timer = get_tsb_timer_interval()
            start_tsa_tsb(tsb_timer)
        elif arg == 'stop':
            stop_tsa_tsb()
        else:
            print_usage()
            return

    return

if __name__ == "__main__":
    main()
