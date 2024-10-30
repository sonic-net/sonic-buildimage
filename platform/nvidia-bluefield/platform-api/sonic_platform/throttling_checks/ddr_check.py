#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
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

import os
from sonic_py_common.logger import Logger
from .utils import MLXBF_SENSOR_PATH, DDR_THROTTLING_THRESHOLD, read_sensor

SYSLOG_IDENTIFIER = os.path.basename(__file__)
logger = Logger(SYSLOG_IDENTIFIER)


class DDRThrottlingChecker:

    def __init__(self, sensor_path = MLXBF_SENSOR_PATH):
        self._sensor_path = sensor_path

    def check(self):
        status = 'OK'
        try:
            temp = read_sensor(f'{self._sensor_path}/ddr_temp')
            if temp >= DDR_THROTTLING_THRESHOLD:
                status = 'Throttled'
        except Exception as e:
            logger.log_error(f'Failed to check DDR throttling: {e}')
        return f'Throttling\nDDR:{status}'


def main():
    checker = DDRThrottlingChecker()
    print(checker.check())


if __name__ == '__main__':
    main()

