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
logger.log_info("Starting kdump config monitor...")

# Connect to config db
config_db = ConfigDBConnector()
config_db.connect()

# Path to the kdump-tools file
kdump_tools_file = '/etc/default/kdump-tools'

def read_kdump_config():
    """
    Read the kdump configuration from config_db.
    """
    kdump_config = config_db.get_table('KDUMP')
    logger.log_info(f"Current kdump configuration: {kdump_config}")  # Add this line for debugging
    return kdump_config


def update_kdump_tools_file(ssh_string, ssh_path):
    """
    Update the /etc/default/kdump-tools file with new ssh_string and ssh_path values.
    If ssh_string or ssh_path is None, replace the value with the initial value obtained from the file.
    """
    # Read the contents of the file and store initial values
    with open(kdump_tools_file, 'r') as file:
        lines = file.readlines()
    # Modify the desired lines
    for i, line in enumerate(lines):
        
        if line.startswith('SSH='):
            if ssh_string:
                lines[i] = f'SSH="{ssh_string}"\n'
            else:
                lines[i] = f'SSH=""\n'

        elif line.startswith('SSH_KEY='):

            if ssh_path:
                lines[i] = f'SSH_KEY="{ssh_path}"\n'
            else:
                lines[i] = f'SSH_KEY=""\n'
    try:
        with open(kdump_tools_file, 'r') as file:
            lines = file.readlines()

        updated = False
        for i, line in enumerate(lines):
            if line.startswith('SSH='):
                lines[i] = f'SSH="{ssh_string if ssh_string else ""}"\n'
                updated = True
            elif line.startswith('SSH_KEY='):
                lines[i] = f'SSH_KEY="{ssh_path if ssh_path else ""}"\n'
                updated = True

        if updated:
            with open(kdump_tools_file, 'w') as file:
                file.writelines(lines)
            logger.log_info(f"Updated kdump-tools file with SSH: {ssh_string} and SSH_KEY: {ssh_path}")
        else:
            logger.log_info("No updates needed for kdump-tools file.")

    except Exception as e:
        logger.log_error(f"Error updating kdump-tools file: {e}")
          (daemon path setting and added some logs why service is not updating in kdump-tools file)


# Main handler function
def handler():
    """
    Continuously monitors the kdump_table for changes and updates the kdump-tools file.
    """
    
    previous_config = read_kdump_config()

    while True:
        current_config = read_kdump_config()

        ssh_string = current_config.get('ssh_string', None)
        ssh_path = current_config.get('ssh_path', None)

        # Check if the configuration has changed
        if current_config != previous_config:
            logger.log_info("Detected change in kdump configuration")

            # Update the kdump-tools file
            update_kdump_tools_file(ssh_string, ssh_path)
            logger.log_info(f"Updated kdump-tools file with SSH: {ssh_string} and SSH_KEY: {ssh_path}")

            # Update the previous configuration
            previous_config = current_config

        time.sleep(10)

# Entry point function
def main():
    """
    Entry point for the daemon.
    """
    handler()

if __name__ == "__main__":
    main()
