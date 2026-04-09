#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from . import utils
from .device_data import DeviceDataManager
from sonic_py_common.logger import Logger

import atexit
import os
import sys
import threading
import fcntl

MODULE_READY_MAX_WAIT_TIME = 300
MODULE_READY_CHECK_INTERVAL = 5
ASIC_READY_DIR = '/tmp/asic_ready'
ASIC_READY_FILE_PREFIX = 'module_host_mgmt_asic_ready'
DEDICATE_INIT_DAEMON = 'xcvrd'


def get_asic_ready_file_path(asic_id):
    """Return path to the ready file for one ASIC. asic_id can be int (e.g. 0) or str (e.g. 'asic0')."""
    if isinstance(asic_id, str) and asic_id.startswith('asic'):
        suffix = asic_id
    else:
        suffix = f'asic{asic_id}'
    return os.path.join(ASIC_READY_DIR, f'{ASIC_READY_FILE_PREFIX}_{suffix}')


initialization_owner = False
logger = Logger()


class ModuleHostMgmtInitializer:
    """Responsible for initializing modules for host management mode.
    """
    def __init__(self):
        self.initialized = False
        self.lock = threading.Lock()
        self.asic_count = DeviceDataManager.get_asic_count()
        self.initialized_list = [False] * self.asic_count
        if ASIC_READY_DIR:
            os.makedirs(ASIC_READY_DIR, exist_ok=True)

    def initialize(self, chassis):
        """Initialize all modules. Only applicable for module host management mode.
        The real initialization job shall only be done in xcvrd. Only 1 owner is allowed
        to to the initialization. Other daemon/CLI shall wait for the initialization done.

        Args:
            chassis (object): chassis object
        """
        global initialization_owner
        not_initialized = []
        for i in range(len(self.initialized_list)):
            if not self.initialized_list[i]:
                not_initialized.append(i)

        if not not_initialized:
            return
        if utils.is_host():
            chassis.initialize_sfp()
        else:
            if self.is_initialization_owner():
                if not_initialized:
                    with self.lock:
                        logger.log_notice('Waiting for modules to be ready...')
                        sfp_count = chassis.get_num_sfps()
                        if not DeviceDataManager.wait_sysfs_ready(sfp_count):
                            logger.log_error('Modules are not ready')
                        else:
                            logger.log_notice('Modules are ready')

                        logger.log_notice('Starting module initialization for module host management...')
                        initialization_owner = True
                        self.remove_asics_from_ready_file(not_initialized)
                        chassis.initialize_sfp()
                        asic_ready_list = []
                        sfp_list = []
                        for asic_id in not_initialized:
                            asic_id_for_file = asic_id + 1
                            if utils.read_int_from_file(f'/var/run/hw-management/config/asic{asic_id_for_file}_ready') == 1:
                                asic_ready_list.append(asic_id)
                                asic_id = f'asic{asic_id}'
                                sfp_list.extend(chassis._asic_modules_dict[asic_id])
                        from .sfp import SFP
                        if sfp_list:
                            SFP.initialize_sfp_modules(sfp_list)
                            self.add_asics_to_ready_file(asic_ready_list)
                            for asic_index in asic_ready_list:
                                self.initialized_list[asic_index] = True
                            logger.log_notice('Module initialization for module host management done')
            else:
                chassis.initialize_sfp()

    def remove_asics_from_ready_file(self, asic_ids):
        """
        Remove Asic IDs from the asic ready files (one file per ASIC).
        Deletes the ready file for each given asic_id.
        Args:
            asic_ids (list): list of asic ids to remove (numbers)
        """
        for asic_id in asic_ids:
            path = get_asic_ready_file_path(asic_id)
            try:
                os.remove(path)
            except FileNotFoundError:
                pass

    def add_asics_to_ready_file(self, asic_ids):
        """
        Add Asic IDs to the asic ready files (one file per ASIC).
        Creates or overwrites the ready file for each given asic_id.
        Args:
            asic_ids (list): list of asic ids to add (numbers)
        """
        for asic_id in asic_ids:
            path = get_asic_ready_file_path(asic_id)
            with open(path, 'w') as file:
                file.write(f"asic{asic_id}\n")

    def is_initialization_owner(self):
        """Indicate whether current thread is the owner of doing module initialization

        Returns:
            bool: True if current thread is the owner
        """
        cmd = os.path.basename(sys.argv[0])
        return DEDICATE_INIT_DAEMON in cmd
    
    def set_asic_ready_value(self, asic_id, ready_bool):
        """
        Update self.initialized_list with ready_bool in case something has changed.
        """
        self.initialized_list[asic_id] = ready_bool
        if not ready_bool:
            # if the asic becomes not ready, remove it from the ready file
            self.remove_asics_from_ready_file([asic_id])
