#!/usr/bin/env python3
import re
import os
import subprocess
import time
from sonic_py_common.logger import Logger
from swsscommon.swsscommon import ConfigDBConnector, SonicDBConfig
from sonic_py_common import multi_asic

SYSLOG_IDENTIFIER = os.path.basename(__file__)
logger = Logger(SYSLOG_IDENTIFIER)
logger.log_info("Starting DHCP DoS logger...")

if multi_asic.is_multi_asic():
    SonicDBConfig.initializeGlobalConfig()
    ports_table = multi_asic.get_table('PORT')
    
else:
    config_db = ConfigDBConnector()
    config_db.connect()
    ports_table = config_db.get_table('PORT')

logger.log_info(f"Monitoring ports: {list(ports_table.keys())}")
drop_pkts = {port: 0 for port in ports_table}

#Get Linux network namespace for a port
def get_port_namespace(port):
    try:
        return multi_asic.get_namespace_for_port(port)
    except Exception:
        return None

#Check if interface exists for a port in the namespace
def interface_exists(ifname, namespace=None):
    if multi_asic.is_multi_asic() and namespace is not None:
        output = subprocess.run(["ip", "netns", "exec", namespace, "test", "-e", f"/sys/class/net/{ifname}"], capture_output=True)
        return output.returncode == 0

    return os.path.exists(f"/sys/class/net/{ifname}")

def handler():
    while True:
        for port in drop_pkts.keys():
            if multi_asic.is_multi_asic():
                namespace = get_port_namespace(port)
            else:
                namespace = None

            #Check if the interface exist for the port in that namespace
            if not interface_exists(port, namespace):
                logger.log_warning(f"Skipping non-existent interface: {port}")
                continue
            try:
                cmd = ["tc", "-s", "qdisc", "show", "dev", str(port), "handle", "ffff:"]
                if multi_asic.is_multi_asic() and namespace is not None:
                    cmd = ["ip", "netns", "exec", namespace] + cmd

                output = subprocess.run(cmd, capture_output=True, text=True)
                logger.log_debug(f"TC output for {port}: {output.stdout}")

                if output.returncode == 0:
                    match = re.search(r'dropped (\d+)', output.stdout)
                    if match:
                        dropped_count = int(match.group(1))
                        if dropped_count > drop_pkts[port]:
                            msg = f"Port {port}: DHCP drop counter increased to {dropped_count}"
                            logger.log_warning(msg)
                            drop_pkts[port] = dropped_count
                    else:
                        logger.log_debug(f"No drops found for port {port}")
                else:
                    logger.log_error(f"TC command failed for {port}: {output.stderr}")
            except Exception as e:
                logger.log_error(f"Error on port {port}: {str(e)}")
        time.sleep(10)

if __name__ == "__main__":
    handler()
