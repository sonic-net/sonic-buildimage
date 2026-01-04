#!/usr/bin/env python3
import re
import os
import subprocess
import time
from sonic_py_common.logger import Logger
from swsscommon.swsscommon import ConfigDBConnector

SYSLOG_IDENTIFIER = os.path.basename(__file__)
logger = Logger(SYSLOG_IDENTIFIER)
logger.log_info("Starting DHCP DoS logger...")

config_db = ConfigDBConnector()
config_db.connect()

ports = config_db.get_table('PORT').keys()
logger.log_info(f"Monitoring ports: {list(ports)}")
drop_pkts = {port: 0 for port in ports}

def interface_exists(ifname):
    return os.path.exists(f"/sys/class/net/{ifname}")

def handler():
    while True:
        for port in drop_pkts.keys():
            if not interface_exists(port):
                logger.log_warning(f"Skipping non-existent interface: {port}")
                continue
            try:
                output = subprocess.run(
                    ["tc", "-s", "qdisc", "show", "dev", str(port), "handle", "ffff:"],
                    capture_output=True,
                    text=True
                )
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
