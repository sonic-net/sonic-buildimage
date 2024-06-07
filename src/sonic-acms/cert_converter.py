#!/usr/bin/env python3

'''
This script converts every downloaded cert from dSMS in .pfx format to .crt and .key format.
It watches for changes to the uber notify file by dSMS and triggers the conversion process
'''

import os, subprocess, time
import random, string
from sonic_py_common import logger
from swsscommon import swsscommon

# Location where this script finally puts the certs after format conversion
certs_path = "/etc/sonic/credentials/"
# Location where ACMS downloads certs from dSMS
acms_certs_path = "/var/opt/msft/client/dsms/sonic-prod/certificates/chained/"
# Location of the uber notify file
uber_notify_file_path = "/var/opt/msft/client/anysecret.notify"
# Length of password for private key encryption
password_length = 64
# Duration between polling for cert changes in seconds
polling_frequency = 3600
# Redis DB information
REDIS_TIMEOUT_MS = 0

sonic_logger = logger.Logger()
sonic_logger.set_min_log_priority_info()

def set_acms_certs_path_from_db():
    global acms_certs_path
    try:
        config_db = swsscommon.DBConnector("CONFIG_DB", REDIS_TIMEOUT_MS, True)
        device_metadata = swsscommon.Table(config_db, swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)
    except RuntimeError as e:
        # TODO: Use specific exception swsscommon.RedisError
        sonic_logger.log_error("cert_converter : Unable to get dsms_cert_path " + str(e))
        return
    (_, tuples) = device_metadata.get("localhost")
    localhost = dict(tuples)
    postfix = localhost.get('dsms_cert_path', None)
    acms_certs_path = f"/var/opt/msft/client/dsms/{postfix}/" if postfix else acms_certs_path
    sonic_logger.log_info("cert_converter : acms_certs_path = " + acms_certs_path)

def execute_cmd(cmd, input=""):
    response = subprocess.run(cmd, input=input.encode(), shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if (response.returncode != 0):
        sonic_logger.log_error(response.stderr.decode())
        return False, ""
    return True, response.stdout.decode()

def get_list_of_certs(path):
    if not os.path.exists(path):
        return []
    # Make list of the certs at a given location
    files = [file_t for file_t in os.listdir(path) if (os.path.isfile(os.path.join(path, file_t)) and not os.path.islink(os.path.join(path, file_t)))]
    cert_list = []
    supported_cert_ext = ['pfx', 'crt', 'key']
    # Expected file names are restapiserver.crt.x and restapiserver.key.x, and other files will be ignored
    for file_t in files:
        if ("notify" not in file_t) and ("metadata" not in file_t):
            file_data = file_t.split(".")
            if len(file_data) < 3:
                sonic_logger.log_warning("cert_converter : ignore " + file_t)
                continue
            file_ext = file_t.split(".")[1]
            file_name = file_t.split(".")[0]
            if (file_ext in supported_cert_ext) and ("sonic_acms_bootstrap" not in file_name) and ("temp" not in file_name) and ("test" not in file_name):
                cert_name = file_name
                cert_ver = file_data[2]
                cert_list.append(cert_name+"."+cert_ver)
    cert_list = list(set(cert_list))
    return cert_list

def link_to_latest_cert(acms_certs_path, certs_path):
    # Create symlink to latest cert file
    link_files = [f for f in os.listdir(certs_path) if (os.path.isfile(os.path.join(certs_path, f)) and os.path.islink(os.path.join(certs_path, f)))]
    targets = []
    for link in link_files:
        targets.append(os.readlink(os.path.join(certs_path, link)))
    notify_files = [f for f in os.listdir(acms_certs_path) if (os.path.isfile(os.path.join(acms_certs_path, f)) and (".notify" in f))]
    for n_file in notify_files:
        cert_name = n_file.split(".")[0]
        sonic_logger.log_info("cert_converter : link_to_latest_cert : Linking cert "+cert_name+".pfx")
        cert_ver = open(acms_certs_path+n_file, "r").readline().split(acms_certs_path)[1].split(".")[2]
        crt_file_name = certs_path+cert_name+".crt"
        crt_versioned_file_name = crt_file_name+"."+cert_ver
        if crt_versioned_file_name not in targets:
            sonic_logger.log_notice("cert_converter : link_to_latest_cert : linking " + crt_versioned_file_name + " to " + crt_file_name, True)
            if os.path.exists(crt_file_name):
                os.remove(crt_file_name)
            os.symlink(crt_versioned_file_name, crt_file_name)
        key_file_name = certs_path+cert_name+".key"
        key_versioned_file_name = key_file_name+"."+cert_ver
        if key_versioned_file_name not in targets:
            sonic_logger.log_notice("cert_converter : link_to_latest_cert : linking " + key_versioned_file_name + " to " + key_file_name, True)
            if os.path.exists(key_file_name):
                os.remove(key_file_name)
            os.symlink(key_versioned_file_name, key_file_name)
        sonic_logger.log_info("cert_converter : link_to_latest_cert : Finished linking cert "+cert_name+".pfx")
    return True

def convert_certs(acms_certs_path, certs_path, password_length):
    existing_cert_names = get_list_of_certs(certs_path)
    downloaded_cert_names = get_list_of_certs(acms_certs_path)
    new_cert_flag = False

    if len(downloaded_cert_names):
        for cert_name in downloaded_cert_names:
            # Start converting those certs which have not been converted
            if cert_name not in existing_cert_names:
                sonic_logger.log_info("cert_converter : convert_certs : Start converting "+cert_name)
                name = cert_name.split(".")[0]
                ver = cert_name.split(".")[1]
                # Extract the certificate from the pfx file
                cmd = ["openssl", "pkcs12", "-clcerts", "-nokeys", "-in", acms_certs_path+name+".pfx."+ver, "-out", certs_path+name+".crt."+ver, "-password", "pass:", "-passin", "pass:"]
                sonic_logger.log_info("cert_converter : convert_certs : "+" ".join(cmd))
                ret, _ = execute_cmd(cmd)
                if not ret:
                    sonic_logger.log_error("cert_converter : convert_certs : Extracting crt from pfx failed!", True)
                    return False                
                # Generate a random password for encrypting the private key
                string_choice = string.ascii_uppercase + string.ascii_lowercase + string.digits
                random_password = ''.join(random.choice(string_choice) for _ in range(password_length))
                # Extract the private key from the pfx file
                cmd = ["openssl", "pkcs12", "-nocerts", "-in", acms_certs_path+name+".pfx."+ver, "-out", "/dev/stdout", "-password", "pass:", "-passin", "pass:", "-passout", "pass:"+random_password]
                ret, private_key = execute_cmd(cmd)
                if not ret:
                    sonic_logger.log_error("cert_converter : convert_certs : Creating private key from pfx failed!", True)
                    return False
                # Decrypt the private key
                cmd = ["openssl", "rsa", "-in", "/dev/stdin", "-out", certs_path+name+".key."+ver, "-passin", "pass:"+random_password]
                ret, _ = execute_cmd(cmd, input=private_key)
                if not ret:
                    sonic_logger.log_error("cert_converter : convert_certs : Extracting key from pfx failed!", True)
                    return False
                new_cert_flag = True
                sonic_logger.log_info("cert_converter : convert_certs : Finished converting "+cert_name)
            else:
                sonic_logger.log_info("cert_converter : convert_certs : "+cert_name+" already converted")
        if new_cert_flag:
            if not (link_to_latest_cert(acms_certs_path, certs_path)):
                sonic_logger.log_error("cert_converter : convert_certs : linking certs failed!", True)
                return False
    else:
        sonic_logger.log_info("cert_converter : convert_certs : no certs downloaded")
    return True


def clean_current_certs(certs_path):
    if not os.path.exists(certs_path):
        return
    # Remove chained crt, chained key and link file
    files = [f for f in os.listdir(certs_path)]
    supported_cert_ext = ['crt', 'key']
    for file_t in files:
        file_data = file_t.split(".")
        if len(file_data) < 2:
            continue
        file_ext = file_data[1]
        file_name = file_data[0]
        if (file_ext in supported_cert_ext) and ("sonic_acms_bootstrap" not in file_name) and ("temp" not in file_name) and ("test" not in file_name):
            os.remove(os.path.join(certs_path, file_t))


def main():
    set_acms_certs_path_from_db()
    while True:
        sonic_logger.log_info("cert_converter : main : Check if uber_notify_file is present")
        if os.path.isfile(uber_notify_file_path):
            sonic_logger.log_info("cert_converter : main : uber_notify_file found, clean old certs...")
            clean_current_certs(certs_path)
            sonic_logger.log_info("cert_converter : main : uber_notify_file found, converting all certs...")
            if not convert_certs(acms_certs_path, certs_path, password_length):
                sonic_logger.log_error("cert_converter : main : Cert conversion failed!")
            break
        else:
            time.sleep(60)

    sonic_logger.log_info("cert_converter : main : Start polling every 1hr...")
    while True:
        sonic_logger.log_notice("cert_converter : main : Checking for cert changes...")
        if not convert_certs(acms_certs_path, certs_path, password_length):
            sonic_logger.log_error("cert_converter : main : Cert conversion failed!")
        time.sleep(polling_frequency)

if __name__ == "__main__":
    main()
