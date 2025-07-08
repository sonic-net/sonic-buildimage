#!/usr/bin/env python3
#
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES.
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

"""
BMC firmware update utility
Handles BMC firmware update process
"""

import sys

def main():
    try:
        from sonic_platform.bmc import BMC
        from sonic_py_common.logger import Logger
        
        logger = Logger('mlnx-bmc-fw-update')

        if len(sys.argv) != 2:
            logger.log_error("Missing firmware image path argument")
            sys.exit(1)
        image_path = sys.argv[1]
    
        bmc = BMC.get_instance()
        if bmc is None:
            logger.log_error("Failed to get BMC instance")
            sys.exit(1)
        
        logger.log_notice(f"Starting BMC firmware update with {image_path}")
        ret, error_msg = bmc.update_firmware(image_path)
        if ret != 0:
            logger.log_error(f'Failed to update BMC firmware. Error {ret}: {error_msg}')
            sys.exit(1)
        
        logger.log_notice("BMC firmware updated successfully, restarting BMC...")
        ret, error_msg = bmc.request_bmc_reset()
        if ret != 0:
            logger.log_error(f'Failed to restart BMC. Error {ret}: {error_msg}')
            sys.exit(1)
        
        logger.log_notice("BMC firmware update completed successfully")
        
    except Exception as e:
        logger.log_error(f'BMC firmware update exception: {e}')
        sys.exit(1)

if __name__ == "__main__":
    main()
