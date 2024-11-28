#!/usr/bin/env python3

import base64
import binascii
import os
import re
import shutil
import subprocess
import time
from dateutil import parser
from sonic_py_common import logger

sonic_logger = logger.Logger()

MONITOR_INTERVAL = 3600 * 24
DSMS_CONF = "/var/opt/msft/client/dsms.conf"
ACMS_SEC_CONF = "/var/opt/msft/client/acms_secrets.ini"
ADHOC_SECRET_PATH = "/var/opt/msft/client/dsms/sonic-prod/adhocsecrets/"
DOWNLOAD_CERT_PATH = "/tmp/bootstrap.pfx"

class BootstrapMonitorError(Exception):
    def __init__(self, message):
        self.message = message
        super().__init__(self.message)

def execute_cmd(cmd, input=""):
    response = subprocess.run(cmd, input=input.encode(), shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if (response.returncode != 0):
        sonic_logger.log_error(response.stderr.decode())
        return False, ""
    return True, response.stdout.decode()

def check_bootstrap_status():
    sonic_logger.log_info("check_bootstrap_status: Checking bootstrap status")
    if os.path.isfile(DSMS_CONF):
        dsms_conf_file = open(DSMS_CONF, "r")
        text = dsms_conf_file.read()
        dsms_conf_file.close()
        if "HasBootstrapped=yes" in text:
            return
    else:
        sonic_logger.log_info("check_bootstrap_status: "+DSMS_CONF+" file not found!")
    raise BootstrapMonitorError("Bootstrap not ready")

def get_bootstrap_path():
    sonic_logger.log_info("get_bootstrap_path: Read bootstrap cert path")
    if os.path.isfile(ACMS_SEC_CONF):
        acms_fp = open(ACMS_SEC_CONF, "r")
        text = acms_fp.read()
        acms_fp.close()
        match = re.search(r'BootstrapCert=(.*pfx)', text)
        if match:
            return match.group(1).strip()
    else:
        sonic_logger.log_info("get_bootstrap_path: "+ACMS_SEC_CONF+" file not found!")
    raise BootstrapMonitorError("Failed to find bootstrap cert path")

def get_expire_date(cert_path):
    sonic_logger.log_info("get_expire_date: Read cert expiration date")
    rc, cert_text = execute_cmd(["openssl", "pkcs12", "-in", cert_path, "-nodes", "-out", "/dev/stdout", "-passin", "pass:"])
    if not rc:
        raise BootstrapMonitorError("Failed to decode " + cert_path)
    rc, date_text = execute_cmd(["openssl", "x509", "-in", "/dev/stdin", "-enddate", "-noout"], cert_text)
    if not rc:
        raise BootstrapMonitorError("Failed to read " + cert_path)
    try:
        poll_time = parser.parse(date_text, fuzzy=True)
    except parser._parser.ParserError:
        raise BootstrapMonitorError("Failed to parse " + date_text)
    return poll_time

def convert_new_cert(secret_notify_path, cert_path):
    sonic_logger.log_info("convert_new_cert: Convert secret to certificate")
    if os.path.exists(cert_path):
        os.remove(cert_path)
    if not os.path.isfile(secret_notify_path):
        raise BootstrapMonitorError("Notify file does not exist: " + secret_notify_path)
    with open(secret_notify_path, "r") as fp:
        secret_path = fp.read()
    if not os.path.isfile(secret_path):
        raise BootstrapMonitorError("Secret file does not exist: " + secret_path)
    with open(secret_path, "r") as fp:
        secret_data = fp.read()
    try:
        download_cert = base64.b64decode(secret_data)
    except binascii.Error as e:
        raise BootstrapMonitorError("Decode error: " + str(e))
    with open(cert_path, "wb") as fp:
        fp.write(download_cert)

def update_state_db(region, date):
    sonic_logger.log_info("update_state_db: Update region and date in STATE_DB")
    rc, _ = execute_cmd(["sonic-db-cli", "STATE_DB", "hset", "ACMS_BOOTSTRAP_CERT|localhost", "region", region])
    if not rc:
        raise BootstrapMonitorError("Failed to update region " + region)
    rc, _ = execute_cmd(["sonic-db-cli", "STATE_DB", "hset", "ACMS_BOOTSTRAP_CERT|localhost", "date", date])
    if not rc:
        raise BootstrapMonitorError("Failed to update date " + date)

def main():
    sonic_logger.set_min_log_priority_info()

    while True:
        # Update bootstrap certificate
        try:
            sonic_logger.log_info("check bootstrap certificate")
            # Check bootstrap status in dsms.conf
            check_bootstrap_status()
            # Retrieve the bootstrap certificate path from acms_secrets.ini
            cert_path = get_bootstrap_path()
            # Retrieve the expiration date of the bootstrap certificate
            bootstrap_date = get_expire_date(cert_path)
            # Update region and date in STATE_DB
            match = re.search(r'sonic_acms_bootstrap-(.*).pfx', cert_path)
            if not match:
                raise BootstrapMonitorError("Failed to find region: " + cert_path)
            region = match.group(1)
            update_state_db(region, str(bootstrap_date))
            # Check openssl version, use different secrets for openssl 3.0
            rc, version_text = execute_cmd(["openssl", "version"])
            if not rc:
                raise BootstrapMonitorError("Failed to run openssl version")
            if "OpenSSL 3.0" in version_text:
                secret_notify_path = ADHOC_SECRET_PATH + "bootstrap_3_0.notify"
            elif "OpenSSL 1.1" in version_text:
                secret_notify_path = ADHOC_SECRET_PATH + "bootstrap_1_1.notify"
            else:
                raise BootstrapMonitorError("Unexpected openssl version " + version_text)
            # Convert new cert from adhoc secrets, and retrieve the expiration date
            convert_new_cert(secret_notify_path, DOWNLOAD_CERT_PATH)
            # Replace the bootstrap certificate with the newly downloaded certificate if the expiration date is later
            new_date = get_expire_date(DOWNLOAD_CERT_PATH)
            if new_date > bootstrap_date:
                sonic_logger.log_info("replace " + cert_path)
                shutil.copy(DOWNLOAD_CERT_PATH, cert_path)
                update_state_db(region, str(new_date))
            else:
                sonic_logger.log_info("no need to replace")
        except BootstrapMonitorError as e:
            sonic_logger.log_info(str(e))
            time.sleep(MONITOR_INTERVAL)
        else:
            time.sleep(MONITOR_INTERVAL)


if __name__ == "__main__":
    main()
