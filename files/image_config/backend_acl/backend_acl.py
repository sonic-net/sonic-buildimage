#!/usr/bin/env python

import os
import subprocess
import sys
import syslog
import time

from swsscommon.swsscommon import SonicV2Connector

SYSLOG_IDENTIFIER = os.path.basename(__file__)

SONIC_CFGGEN_PATH = '/usr/local/bin/sonic-cfggen'

def log_info(msg):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_INFO, msg)
    syslog.closelog()


def run_command(cmd):
    log_info("executing cmd =  {}".format(cmd))
    try:
        proc = subprocess.run(
            cmd,
            shell=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
    except OSError as e:
        log_info("command failed to start: {}".format(str(e)))
        return False, ""

    if proc.returncode != 0:
        log_info(
            "command failed with exit code {}: {}".format(
                proc.returncode,
                proc.stderr.strip()
            )
        )
        return False, proc.stdout.strip()

    if proc.stderr:
        log_info("command wrote to stderr: {}".format(proc.stderr.strip()))

    return True, proc.stdout.strip()

def _get_device_type():
    """
    Get device type
    """
    success, device_type = run_command(
        [SONIC_CFGGEN_PATH, '-d', '-v', 'DEVICE_METADATA.localhost.type']
    )
    if not success or not device_type:
        log_info("Unable to determine device type from CONFIG_DB")
        return None

    return device_type

def _is_storage_device():
    """
    Check if the device is a storage device or not
    """
    success, storage_device = run_command(
        [SONIC_CFGGEN_PATH, '-d', '-v', 'DEVICE_METADATA.localhost.storage_device']
    )
    if not success:
        log_info("Unable to determine storage-device status from CONFIG_DB")
        return None

    if storage_device not in ("true", "false"):
        log_info("Storage-device metadata is missing or invalid in CONFIG_DB")
        return None

    return storage_device == "true"

def _is_acl_table_present():
    """
    Check if acl table exists
    """
    success, acl_table = run_command(
        [SONIC_CFGGEN_PATH, '-d', '-v', 'ACL_TABLE.DATAACL']
    )
    if not success:
        log_info("Unable to determine DATAACL status from CONFIG_DB")
        return None

    return bool(acl_table)

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
    is_storage_device = _is_storage_device()
    if is_storage_device is None:
        return False
    if not is_storage_device:
        log_info("Skipping backend acl load - device is not a storage device")
        return True

    is_acl_table_present = _is_acl_table_present()
    if is_acl_table_present is None:
        return False
    if not is_acl_table_present:
        log_info("Skipping backend acl load - DATAACL is not configured")
        return True

    if not _is_switch_table_present():
        log_info("Unable to load backend acl - switch capability table is unavailable")
        return False

    if not os.path.isfile(BACKEND_ACL_TEMPLATE_FILE):
        log_info("Unable to load backend acl - template file is missing")
        return False

    success, _ = run_command(
        ['sudo', SONIC_CFGGEN_PATH, '-d', '-t',
         '{},{}'.format(BACKEND_ACL_TEMPLATE_FILE, BACKEND_ACL_FILE)]
    )
    if not success:
        return False

    if not os.path.isfile(BACKEND_ACL_FILE):
        log_info("Unable to load backend acl - generated ACL file is missing")
        return False

    success, _ = run_command(
        ['acl-loader', 'update', 'full', BACKEND_ACL_FILE, '--table_name', 'DATAACL']
    )
    return success

def main():
    device_type = _get_device_type()
    if device_type is None:
        return 1

    if device_type != "BackEndToRRouter":
        log_info("Skipping backend acl load on unsupported device type: {}".format(device_type))
        return 0

    return 0 if load_backend_acl(device_type) else 1

if __name__ == "__main__":
    sys.exit(main())
