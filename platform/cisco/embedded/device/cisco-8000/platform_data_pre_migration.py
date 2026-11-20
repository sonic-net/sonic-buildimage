#!/usr/bin/env python3
# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

import os
import yaml
import shutil
import logging
import subprocess
from cisco.pacific.xr_sysctl import xr_sysctl

PLATFORM_DATA_MIGRATION_LOG = '/var/log/platform_data_migration.log'
PLATFORM_DATA_MIGRATION_FILE = 'platform_data_migration.yaml'
PLATFORM_DATA_CACHE_PATH = '/host/old_config/platform'
SONIC_DEVICE = '/usr/share/sonic/device'
CISCO_8000_DEVICE='/usr/share/sonic/device/cisco-8000'

def main():

    migration_info = []

    logging.basicConfig(filename=PLATFORM_DATA_MIGRATION_LOG,
                        level=logging.DEBUG,
                        format='%(asctime)s, - %(levelname)s, -%(message)s')

    # Find the platform path
    plat = xr_sysctl("bootstrap.onie.platform").str_get_value()
    path = SONIC_DEVICE
    plat_path = os.path.join(path, plat, "")

    #Check for platform_data_migration.yaml presence
    cfg_file = os.path.join(path, plat, PLATFORM_DATA_MIGRATION_FILE)
    if os.path.isfile(cfg_file) is False:
        return

    #Read the migration list
    try:
        with open(cfg_file, 'r') as f:
            cfg = yaml.safe_load(f)

            if 'migration_cfg' not in cfg:
                return

            for data in cfg['migration_cfg']:
                tmp = {}
                if 'source_file' in data:
                    src_dir = (data['source_file'].split(':'))[0]
                    if src_dir == 'platform':
                        tmp['source_file_dir'] = plat_path
                    else:
                        tmp['source_file_dir'] = (data['source_file'].split(':'))[0]
                    tmp['source_file'] = (data['source_file'].split(':'))[1]
                else:
                    #Ignore the entries without source file
                    continue
                if 'destination_file' in data:
                    dst_dir = (data['destination_file'].split(':'))[0]
                    if dst_dir == 'platform':
                        tmp['destination_file_dir'] = plat_path
                    else:
                        tmp['destination_file_dir'] = (data['destination_file'].split(':'))[0]
                    tmp['destination_file'] = (data['destination_file'].split(':'))[1]
                else:
                    tmp['destination_file_dir'] = (data['source_file'].split(':'))[0]
                    tmp['destination_file'] = (data['source_file'].split(':'))[1]
                if 'premigration_script' in data:
                    if data['premigration_script'] is not None:
                        tmp['premigration_script_dir'] = (data['premigration_script'].split(':'))[0]
                        tmp['premigration_script'] = (data['premigration_script'].split(':'))[1]
                if 'postmigration_script' in data:
                    if data['postmigration_script'] is not None:
                        tmp['postmigration_script_dir'] = (data['postmigration_script'].split(':'))[0]
                        tmp['postmigration_script'] = (data['postmigration_script'].split(':'))[1]
                migration_info.append(tmp)
    except FileNotFoundError:
        logging.info(f'Platform data migration data not needed: {cfg_file}')
        sys.exit(0)

    try:
        os.makedirs(PLATFORM_DATA_CACHE_PATH, exist_ok=True)
    except OSError as e:
        logging.error(f"Error creating directory '{PLATFORM_DATA_CACHE_PATH}': {e}")
        sys.exit(1)

    post_mig_script = os.path.join(CISCO_8000_DEVICE, 'platform_data_post_migration.py')
    shutil.copy(post_mig_script, PLATFORM_DATA_CACHE_PATH)

    for i, d in enumerate(migration_info, start=1):
        try:
            sub_dir = os.path.join(PLATFORM_DATA_CACHE_PATH, str(i), "")
            os.makedirs(sub_dir, exist_ok=True)
        except OSError as error:
            logging.error(f"Error creating directory '{sub_dir}': {error}")
            sys.exit(1)

        #Process the source file
        if 'premigration_script' in d:
            script = os.path.join(d['premigration_script_dir'], d['premigration_script'])
            process = subprocess.Popen([script],
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE,
                                       text=True)
            stdout, stderr = process.communicate()
            logging.info(f'stdout: {stdout} stderr: {stderr}')

        #Cache the source file
        src_file = os.path.join(d['source_file_dir'], d['source_file'])
        dst_file = os.path.join(sub_dir, 'data_file')
        shutil.copy(src_file, dst_file)

        #Cache the postmigration script
        if 'postmigration_script' in d:
            src_script = os.path.join(d['postmigration_script_dir'], d['postmigration_script'])
            dst_script = os.path.join(sub_dir, 'postmigration_script')
            shutil.copy(src_script, dst_script)

        #Save the destination path
        try:
            with open(sub_dir+'migrate.txt', 'w') as f:
                data_file = os.path.join(d['destination_file_dir'], d['destination_file'])
                f.write(data_file)
        except Exception as e:
            logging.error(f"Error: {e}")



if __name__ == '__main__':
    main()
