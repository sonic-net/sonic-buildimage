#!/usr/bin/env python3

import os, subprocess, time, shutil, sys, re
import dSMS_config_modifier
from sonic_py_common import logger

sonic_logger = logger.Logger()

# Default ACMS directory
ACMS_DIR_PATH = "/var/opt/msft/acms"
# Default dSMS client directory
CLIENT_DIR_PATH = "/var/opt/msft/client"
# Original ACMS config
ACMS_CONFIG_PATH = "/acms/acms_secrets.ini"
# Original ACMS.ini
ACMS_INI_PATH = "/acms/acms.ini"
# Bootstrap status file
DSMS_CONF = "/var/opt/msft/client/dsms.conf"
# SONiC credentials directory
SONIC_CREDS_PATH = "/etc/sonic/credentials/"
# SONiC ACMS bootstrap certificate naming pattern
SONIC_ACMS_BOOTSTRAP_PATTERN = "sonic_acms_bootstrap-.*\.pfx"

WAIT_TIME_FOR_BOOTSTRAP_CERT = 60

def exec_cmd(cmd):
    process = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdoutdata, stderrdata = process.communicate()
    return process.returncode, stdoutdata, stderrdata


def get_bootstrap_cert_paths():
    # Returns list of bootstrap certs sorted based on last modified time
    files = [f for f in os.listdir(SONIC_CREDS_PATH) if re.match(SONIC_ACMS_BOOTSTRAP_PATTERN, f)]
    bootstrap_certs = []
    for f in files:
        path = os.path.join(SONIC_CREDS_PATH, f)
        tup = (path, os.path.getmtime(path))
        bootstrap_certs.append(tup)

    bootstrap_certs.sort(key = lambda x: x[1], reverse = True)
    return bootstrap_certs


def get_bootstrap_status():
    sonic_logger.log_info("start: check_bootstrap_status: Checking bootstrap status")
    if os.path.isfile(DSMS_CONF):
        dsms_conf_file = open(DSMS_CONF, "r")
        text = dsms_conf_file.read()
        dsms_conf_file.close()
        if "HasBootstrapped=yes" in text:
            return True
    else:
        sonic_logger.log_info("start: check_bootstrap_status: "+DSMS_CONF+" file not found!")
    return False


def update_acms_config(bootstrap_cert):
    cloud = dSMS_config_modifier.get_device_cloudtype()
    if cloud in ('None', ''):
        sonic_logger.log_error("start: update_acms_config: Fail to get cloud type, need retry")
        return False
    if cloud.lower() != "Public".lower():
        if not dSMS_config_modifier.fix_endpoint_for_cloud(cloud):
            sonic_logger.log_error("start: update_acms_config: Fixing endpoint for cloudtype "+cloud+" failed!")
            sys.exit(1)
    dSMS_config_modifier.update_config(bootstrap_cert)
    sonic_logger.log_info("start: update_acms_config: ACMS Config update complete")
    return True


def main():
    sonic_logger.set_min_log_priority_info()

    if os.path.exists('/var/run/rsyslogd.pid'):
        os.remove('/var/run/rsyslogd.pid')
    rc, stdoutdata, stderrdata = exec_cmd("supervisorctl start rsyslogd")
    if rc != 0:
        sonic_logger.log_error("start: main: rsyslogd start failed!")

    if os.path.isdir(ACMS_DIR_PATH):
        sonic_logger.log_info("start: main: "+ACMS_DIR_PATH+" exists. Deleting it")
        shutil.rmtree(ACMS_DIR_PATH)
    os.makedirs(ACMS_DIR_PATH)
    sonic_logger.log_info("start: main: "+ACMS_DIR_PATH+" has been created")
    shutil.copy2(ACMS_INI_PATH, ACMS_DIR_PATH)
    dest = os.path.join(ACMS_DIR_PATH, "acms.ini")
    if os.path.isfile(dest):
        sonic_logger.log_info("start: main: acms.ini copied to "+ACMS_DIR_PATH)  
    else:
        sonic_logger.log_error("start: main: acms.ini could not be copied to "+ACMS_DIR_PATH)
        sys.exit(1)

    if os.path.isdir(CLIENT_DIR_PATH):
        sonic_logger.log_info("start: main: "+CLIENT_DIR_PATH+" exists. Deleting it")
        shutil.rmtree(CLIENT_DIR_PATH)
    os.makedirs(CLIENT_DIR_PATH)
    sonic_logger.log_info("start: main: "+CLIENT_DIR_PATH+" has been created")

    ctr = 0
    # Bootstrap ACMS
    while(get_bootstrap_status() == False):
        # Fresh copy of acms_secrets.ini
        shutil.copy2(ACMS_CONFIG_PATH, CLIENT_DIR_PATH)
        dest = os.path.join(CLIENT_DIR_PATH, "acms_secrets.ini")
        if os.path.isfile(dest):
            sonic_logger.log_info("start: main: acms_secrets.ini copied to "+CLIENT_DIR_PATH)
        else:
            sonic_logger.log_error("start: main: acms_secrets.ini could not be copied to "+CLIENT_DIR_PATH)
            sys.exit(1)

        # Remove stale dsms.conf    
        if os.path.isfile(DSMS_CONF):
            os.remove(DSMS_CONF)
        boostrap_certs = get_bootstrap_cert_paths()
        if len(boostrap_certs) > 0:
            if ctr < len(boostrap_certs):
                curr_bootstrap_cert = boostrap_certs[ctr][0]
            else:
                curr_bootstrap_cert = boostrap_certs[0][0]
                ctr = 0
            sonic_logger.log_info("start: main: Trying to bootstrap with "+curr_bootstrap_cert)
            if (update_acms_config(curr_bootstrap_cert) == True):
                exec_cmd("/usr/bin/acms -Bootstrap -Dependant client -BaseDirPath /var/opt/msft/")
                # Choose a different bootstrap cert for next bootstrap attempt
                ctr += 1
                ctr = ctr % len(boostrap_certs)
        else:
            sonic_logger.log_info("start: main: Waiting for bootstrap cert")
        time.sleep(WAIT_TIME_FOR_BOOTSTRAP_CERT)

    sonic_logger.log_info("start: main: ACMS bootstrapping complete")

    # Start ACMS
    rc, stdoutdata, stderrdata = exec_cmd("supervisorctl start acms")
    if rc == 0:
        sonic_logger.log_info("start: main: ACMS has been started")
    else:
        sonic_logger.log_error("start: main: Starting ACMS failed!")   


if __name__ == "__main__":
    main()
