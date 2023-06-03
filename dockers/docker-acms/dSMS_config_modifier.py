#!/usr/bin/env python3
'''
This script will determine the correct dSMS region to configure ACMS and update the config file
It will also update the ACMS config file with the bootstrap cert path
'''
import os, re, sys
from sonic_py_common import logger
from swsscommon import swsscommon

# ACMS config file
acms_conf = "/var/opt/msft/client/acms_secrets.ini"

# Redis DB information
REDIS_TIMEOUT_MS = 0

sonic_logger = logger.Logger()
sonic_logger.set_min_log_priority_info()

def get_device_cloudtype():
    config_db = swsscommon.DBConnector("CONFIG_DB", REDIS_TIMEOUT_MS, True)
    device_metadata = swsscommon.Table(config_db, swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)
    (status, tuples) = device_metadata.get("localhost")
    localhost = dict(tuples)
    return localhost.get('cloudtype', '')

def get_device_region_from_bootstrap_cert(path_to_bootstrap_cert):
    # /etc/sonic/credentials/sonic_acms_bootstrap-uswestcentral.pfx
    cert = os.path.basename(path_to_bootstrap_cert)
    region = cert.split('-')[1].split('.')[0]
    return region

def update_dsms_url(conf_file, new_url):
    '''
    Update the dSMS URL in the given config file by looking for line starting with FullHttpsDsmsUrl pattern
    '''
    with open(conf_file, "r+") as conf_file_t:
        contents = conf_file_t.read()
        if new_url not in contents:
            new_contents = re.sub("FullHttpsDsmsUrl=.*", "FullHttpsDsmsUrl="+new_url, contents)
            conf_file_t.seek(0)
            conf_file_t.write(new_contents)

def update_acms_dns(ip):
    '''
    Update the ACMS DNS in /etc/resolv.conf
    '''
    with open('/etc/resolv.conf', "w") as dns_file:
        new_contents = "nameserver " + ip + "\n"
        dns_file.write(new_contents)
    return

def fix_endpoint_for_cloud(cloud):
    '''
    Identify the correct URL pattern for dSMS endpoints in national clouds 
    '''
    # Convert cloudtype to lowercase for case-insensitive string comparison
    if cloud.lower() == "FairFax".lower():
        url_pattern = "https://region-dsms.dsms.core.usgovcloudapi.net"
        update_acms_dns("10.111.39.6")
    elif cloud.lower() == "BlackForest".lower():
        url_pattern = "https://region-dsms.dsms.core.cloudapi.de"
    elif cloud.lower() == "Mooncake".lower():
        url_pattern = "https://region-dsms.dsms.core.chinacloudapi.cn"
        update_acms_dns("10.41.21.20")
    elif cloud.lower() == "USnat".lower():
        url_pattern = "https://region-dsms.dsms.core.eaglex.ic.gov"
        update_acms_dns("10.1.12.0")
    elif cloud.lower() == "USSec".lower():
        url_pattern = "https://region-dsms.dsms.core.microsoft.scloud"
        update_acms_dns("10.1.48.0")
    else:
        sonic_logger.log_error("CloudType "+cloud+" not listed!")
        return False
    sonic_logger.log_info("dSMS_config_modifier: fix_endpoint_for_cloud: dSMS endpoint for "+cloud+" is "+url_pattern)
    # Change URL pattern in the ACMS config file
    update_dsms_url(acms_conf, url_pattern)
    return True

def update_config(path_to_bootstrap_cert):
    '''
    Determine the correct dSMS endpoint and update the ACMS config file
    '''
    region = get_device_region_from_bootstrap_cert(path_to_bootstrap_cert)
    with open(acms_conf, "r+") as acms_conf_file:
        contents = acms_conf_file.read()
        new_contents = re.sub("region", region, contents)
        new_contents = re.sub("BootstrapCert=.*\n*", "BootstrapCert="+path_to_bootstrap_cert+"\n", new_contents)
        sonic_logger.log_info("dSMS_config_modifier: update_config: Updated config: "+new_contents)
        acms_conf_file.seek(0)
        acms_conf_file.write(new_contents)
        sonic_logger.log_debug("dSMS_config_modifier: update_config: dSMS endpoint set to "+region)
