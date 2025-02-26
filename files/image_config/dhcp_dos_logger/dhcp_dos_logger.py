#!/usr/bin/env python3

import re
import os
import subprocess
import time
from sonic_py_common.logger import Logger
from swsscommon.swsscommon import ConfigDBConnector

SYSLOG_IDENTIFIER = os.path.basename(__file__)

# Global logger instance
logger = Logger(SYSLOG_IDENTIFIER)
logger.log_info("Starting DHCP DoS logger...")

# Connect to config db
config_db = ConfigDBConnector()
config_db.connect()

# Initialize
drop_pkts = {}

# Get list of ports
ports = config_db.get_table('PORT').keys()

# Initialize the ports with zero initial packet drops
drop_pkts = {port: 0 for port in ports}

# Main handler function
def handler():
    """
    Continuously monitors ports for dropped DHCP packets and logs them.
    """
    while True:
        for port in drop_pkts.keys():
            try:
                output = subprocess.run(["tc", "-s", "qdisc", "show", "dev", str(port), "handle", "ffff:"], capture_output=True)
                if output.returncode == 0:  # Check for successful execution
                    match = re.search(r'dropped (\d+)', output.stdout)
                    if match:
                        dropped_count = int(match.group(1))
                        if dropped_count > drop_pkts[port]:
                            logger.log_warning(f"Port {port}: Current DHCP drop counter is {dropped_count}")
                            drop_pkts[port] = dropped_count
                        else:
                            pass
                    else:
                        logger.log_warning(f"Failed to get dropped packet information for port {port}")
            except subprocess.CalledProcessError as e:
                logger.log_error(f"Error executing 'tc' command: {e}")

        time.sleep(10)


# Entry point function
def main():
    """
    Entry point for the daemon.
  """
    handler()


if __name__ == "__main__":
    main()