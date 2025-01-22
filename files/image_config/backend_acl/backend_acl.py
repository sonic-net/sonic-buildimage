#!/usr/bin/env python

import os
import subprocess
import syslog
import time

from swsscommon.swsscommon import SonicV2Connector

SYSLOG_IDENTIFIER = os.path.basename(__file__)

SONIC_CFGGEN_PATH = '/usr/local/bin/sonic-cfggen'

def log_info(msg):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_INFO, msg)
    syslog.closelog()

def run_command(cmd, return_cmd=False):
   log_info("executing cmd =  {}".format(cmd))
   proc = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE)
   out, err = proc.communicate()
   if return_cmd:
       if err:
           return "Unknown"

       if len(out) > 0:
           return out.strip().decode('utf-8')

def _get_device_type():
    """
    Get device type
    """
    device_type = run_command([SONIC_CFGGEN_PATH, '-m', '-v', 'DEVICE_METADATA.localhost.type'], return_cmd=True)
    return device_type

def _is_storage_device():
    """
    Check if the device is a storage device or not
    """
    storage_device = run_command([SONIC_CFGGEN_PATH, '-d', '-v', 'DEVICE_METADATA.localhost.storage_device'], return_cmd=True)
    return storage_device == "true"

def _is_acl_table_present():
    """
    Check if acl table exists
    """
    acl_table = run_command([SONIC_CFGGEN_PATH, '-d', '-v', 'ACL_TABLE.DATAACL'], return_cmd=True)
    return (acl_table != "Unknown" and bool(acl_table))

def _is_switch_table_present():
    state_db = SonicV2Connector(host='127.0.0.1')
    state_db.connect(state_db.STATE_DB, False)
    table_present = False
    wait_time = 0
    TIMEOUT = 120
    STEP = 10

    while wait_time < TIMEOUT:
        if state_db.exists(state_db.STATE_DB, 'SWITCH_CAPABILITY|switch'):
            table_present = True
            break
        time.sleep(STEP)
        wait_time += STEP
    if not table_present:
        log_info("Switch table not present")
    return table_present

def load_backend_acl(device_type):
    """
    Load acl on backend storage device
    """
    BACKEND_ACL_TEMPLATE_FILE = os.path.join('/', "usr", "share", "sonic", "templates", "backend_acl.j2")
    BACKEND_ACL_FILE = os.path.join('/', "etc", "sonic", "backend_acl.json")

    # this acl needs to be loaded only on a storage backend ToR. acl load will fail if the switch table isn't present
    if _is_storage_device() and _is_acl_table_present() and _is_switch_table_present():
        if os.path.isfile(BACKEND_ACL_TEMPLATE_FILE):
            run_command(['sudo', SONIC_CFGGEN_PATH, '-d', '-t', '{},{}'.format(BACKEND_ACL_TEMPLATE_FILE, BACKEND_ACL_FILE)])
        if os.path.isfile(BACKEND_ACL_FILE):
            run_command(['acl-loader', 'update', 'full', BACKEND_ACL_FILE, '--table_name', 'DATAACL'])
    else:
        log_info("Skipping backend acl load - conditions not met")

def main():
    device_type = _get_device_type()
    if device_type != "BackEndToRRouter":
        log_info("Skipping backend acl load on unsupported device type: {}".format(device_type))
        return

    load_backend_acl(device_type)

if __name__ == "__main__":
    main()
