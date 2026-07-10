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
from sonic_py_common.logger import Logger

import atexit
import os
import sys
import threading

MODULE_READY_MAX_WAIT_TIME = 300
MODULE_READY_CHECK_INTERVAL = 5
MODULE_READY_CONTAINER_FILE = '/tmp/module_host_mgmt_ready'
MODULE_READY_HOST_FILE = '/tmp/nv-syncd-shared/module_host_mgmt_ready'
DEDICATE_INIT_DAEMON = 'xcvrd'
initialization_owner = False

logger = Logger()


class ModuleHostMgmtInitializer:
    """Responsible for initializing modules for host management mode.
    """
    def __init__(self):
        self.initialized = False
        self.lock = threading.Lock()

    def initialize(self, chassis):
        """Initialize all modules. Only applicable for module host management mode.
        The real initialization job shall only be done in xcvrd. Only 1 owner is allowed
        to to the initialization. Other daemon/CLI shall wait for the initialization done.

        Args:
            chassis (object): chassis object
        """
        global initialization_owner
        if self.initialized:
            return
        
        if utils.is_host():
            self.wait_module_ready()
            chassis.initialize_sfp()
        else:
            if self.is_initialization_owner():
                if not self.initialized:
                    with self.lock:
                        if not self.initialized:
                            from sonic_platform.device_data import DeviceDataManager
                            logger.log_notice('Waiting for modules to be ready...')
                            sfp_count = chassis.get_num_sfps()
                            if not DeviceDataManager.wait_sysfs_ready(sfp_count):
                                logger.log_error('Modules are not ready')
                            else:
                                logger.log_notice('Modules are ready')

                            logger.log_notice('Starting module initialization for module host management...')
                            initialization_owner = True
                            self.remove_module_ready_file()
                            
                            chassis.initialize_sfp()
                            
                            from .sfp import SFP
                            SFP.initialize_sfp_modules(chassis._sfp_list)
                            
                            self.create_module_ready_file()    
                            self.initialized = True
                            logger.log_notice('Module initialization for module host management done')
            else:
                self.wait_module_ready()
                chassis.initialize_sfp()
                  
    @classmethod
    def create_module_ready_file(cls):
        """Create module ready file
        """
        with open(MODULE_READY_CONTAINER_FILE, 'w'):
            pass

    @classmethod
    def remove_module_ready_file(cls):
        """Remove module ready file
        """
        if os.path.exists(MODULE_READY_CONTAINER_FILE):
            os.remove(MODULE_READY_CONTAINER_FILE)
                
    def wait_module_ready(self):
        """Wait up to MODULE_READY_MAX_WAIT_TIME seconds for all modules to be ready
        """
        if utils.is_host():
            module_ready_file = MODULE_READY_HOST_FILE
        else:
            module_ready_file = MODULE_READY_CONTAINER_FILE

        if os.path.exists(module_ready_file):
            self.initialized = True
            return
        else:
            print('Waiting module to be initialized...')
        
        if utils.wait_until(os.path.exists, MODULE_READY_MAX_WAIT_TIME, MODULE_READY_CHECK_INTERVAL, module_ready_file):
            self.initialized = True
        else:
            logger.log_error('Module initialization timeout', True)
            
    def is_initialization_owner(self):
        """Indicate whether current thread is the owner of doing module initialization

        Returns:
            bool: True if current thread is the owner
        """
        cmd = os.path.basename(sys.argv[0])
        return DEDICATE_INIT_DAEMON in cmd

@atexit.register
def clean_up():
    """Remove module ready file when program exits.
    When module host management is enabled, xcvrd is the dependency for all other
    daemon/CLI who potentially uses SFP API.
    """
    if initialization_owner:
        ModuleHostMgmtInitializer.remove_module_ready_file()
