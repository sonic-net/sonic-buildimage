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
from pathlib import Path
from enum import Enum, auto
from .utils import MLXBF_SENSOR_PATH, LAST_VALUES_STORAGE_PATH, read_sensor

SYSLOG_IDENTIFIER = os.path.basename(__file__)
logger = Logger(SYSLOG_IDENTIFIER)


class CheckResult(Enum):
    OK = auto()
    THROTTLED_NOW = auto()
    THROTTLED_BETWEEN_CHECKS = auto()
    RECOVERED = auto()


class CPUThrottlingChecker:
    fs_read_max_attempts = 3

    def __init__(self, sensor_path = MLXBF_SENSOR_PATH, storage_path = LAST_VALUES_STORAGE_PATH):
        self._sensor_path = sensor_path
        self._storage_path = storage_path

    def _read_count_and_state_fs(self, path, ttype, ok_if_not_found = False):
        count_path = f'{path}/{ttype}_throttling_event_count'
        state_path = f'{path}/{ttype}_throttling_state'

        # read count/state/count to avoid race condition
        for _ in range(CPUThrottlingChecker.fs_read_max_attempts):
            count = read_sensor(count_path, ok_if_not_found)
            state = read_sensor(state_path, ok_if_not_found)
            count_again = read_sensor(count_path, ok_if_not_found)
            if count == count_again:
                return count, state

        raise Exception(f'Failed to read {ttype} state - inconsistent  state during read attempt')

    def _read_count_and_state(self, ttype):
        return self._read_count_and_state_fs(self._sensor_path, ttype)

    def _read_last_count_and_state(self, ttype):
        return self._read_count_and_state_fs(self._storage_path, ttype, ok_if_not_found=True)

    def _save_count_and_state(self, ttype, count, state):
        count_path = f'{self._storage_path}/{ttype}_throttling_event_count'
        state_path = f'{self._storage_path}/{ttype}_throttling_state'

        Path(self._storage_path).mkdir(exist_ok=True)

        with open(count_path, 'w+') as f:
            f.write(str(count))

        with open(state_path, 'w+') as f:
            f.write(str(state))

    def _check_throttling_type(self, ttype) -> CheckResult:
        count, state = self._read_count_and_state(ttype)
        last_count, last_state = self._read_last_count_and_state(ttype)
        self._save_count_and_state(ttype, count, state)

        if state > 1:
            return CheckResult.THROTTLED_NOW

        # no previous state saved (first run)
        if last_state is None:
            return CheckResult.OK

        if last_state == 1:
            if count != last_count:
                logger.log_warning(f'CPU was {ttype} throttled')
                return CheckResult.THROTTLED_BETWEEN_CHECKS
        else:
            logger.log_warning(f'CPU has recovered from {ttype} throttling')
            return CheckResult.RECOVERED

    def check(self):
        status = 'OK'
        try:
            thermal_check = self._check_throttling_type('thermal')
            power_check = self._check_throttling_type('power')
            if thermal_check == CheckResult.THROTTLED_NOW or power_check == CheckResult.THROTTLED_NOW:
                status = 'Throttled'
        except Exception as e:
            logger.log_error(f'Failed to check CPU throttling: {e}')
        return f'Throttling\nCPU:{status}'


def main():
    checker = CPUThrottlingChecker()
    print(checker.check())


if __name__ == '__main__':
    main()
