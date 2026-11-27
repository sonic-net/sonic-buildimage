#!/usr/bin/env python3
# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

import os
import yaml
import logging
import shutil
import subprocess
from cisco.pacific.xr_sysctl import xr_sysctl

PLATFORM_DATA_MIGRATION_LOG = '/var/log/platform_data_migration.log'
PLATFORM_DATA_CACHE_PATH = '/etc/sonic/old_config/platform'
SONIC_DEVICE = '/usr/share/sonic/device'

def main():

    migration_info = []

    logging.basicConfig(filename=PLATFORM_DATA_MIGRATION_LOG,
                        level=logging.DEBUG,
                        format='%(asctime)s, - %(levelname)s, -%(message)s')

    # Find the platform path
    plat = xr_sysctl("bootstrap.onie.platform").str_get_value()
    path = SONIC_DEVICE
    plat_path = os.path.join(path, plat, "")

    dir_list = os.listdir(PLATFORM_DATA_CACHE_PATH)
    for dir in dir_list:

        #Copy the data file to the destination path
        dst_file = ''
        from_path = os.path.join(PLATFORM_DATA_CACHE_PATH, dir)
        data_file = os.path.join(PLATFORM_DATA_CACHE_PATH, dir, 'data_file')
        tmp_file = os.path.join(from_path, 'migrate.txt')
        if not os.path.isdir(from_path):
            continue

        try:
            with open(tmp_file, 'r') as f:
                dst_file = f.read()
                shutil.copy(data_file, dst_file)
        except Exception as e:
            logging.error(f"Error: {e}")
            continue

        #Process the file
        mig_script = os.path.join(from_path, 'postmigration_script')
        if os.path.exists(mig_script):
            process = subprocess.Popen([mig_script],
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE,
                                       text=True)
            stdout, stderr = process.communicate()
            logging.info(f'stdout: {stdout} stderr: {stderr}')

if __name__ == '__main__':
    main()
