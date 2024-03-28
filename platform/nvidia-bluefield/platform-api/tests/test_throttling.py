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
import sys

import tempfile
import pytest

from .utils import TTestData, TTestFSState, LogRecorderMock
from sonic_platform.throttling_checks.cpu_check import CPUThrottlingChecker
from sonic_platform.throttling_checks.ddr_check import DDRThrottlingChecker

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

test_first_run_ok = TTestData('First Run all OK', TTestFSState([1, 0, 1, 0], []), 'OK', [])
test_first_run_powert = TTestData('First Run power throttled', TTestFSState([2, 0, 1, 0], []), 'Throttled', [])
test_first_run_thermalt = TTestData('First Run thermal throttled', TTestFSState([1, 0, 2, 0], []), 'Throttled', [])
test_recovered_powert = TTestData('Power throttled recovery', TTestFSState([1, 1, 1, 0], [2, 0, 1, 0]), 'OK', 
                                  [('warning', 'CPU has recovered from power throttling')])
test_recovered_thermalt = TTestData('Thermal throttled recovery', TTestFSState([1, 0, 1, 1], [1, 0, 2, 0]), 'OK',
                                   [('warning', 'CPU has recovered from thermal throttling')])
test_in_between_powert = TTestData('Power throttled between checks', TTestFSState([1, 2, 1, 0], [1, 0, 1, 0]), 'OK',
                                  [('warning', 'CPU was power throttled')])
test_in_between_thermalt = TTestData('Power throttled between checks', TTestFSState([1, 0, 1, 2], [1, 0, 1, 0]), 'OK',
                                  [('warning', 'CPU was thermal throttled')])
test_no_sensors = TTestData('No sensors', TTestFSState([], []), 'OK',
                                   [('error', 'Failed to check CPU throttling: Could not read the')])

test_ddr_ok = TTestData('DDR OK', TTestFSState([0, 0, 0, 0, 60], []), 'OK', [])
test_ddr_throttled = TTestData('DDR throttled', TTestFSState([0, 0, 0, 0, 100], []), 'Throttled', [])
test_ddr_no_sensors = TTestData('DDR no sensor', TTestFSState([], []), 'OK',
                                   [('error', 'Failed to check DDR throttling: Could not read the')])

all_cpu_tests = [test_first_run_ok,
                 test_first_run_powert,
                 test_recovered_powert,
                 test_recovered_thermalt,
                 test_in_between_powert,
                 test_in_between_thermalt,
                 test_no_sensors]
first_run_tests = [test_first_run_ok, test_first_run_powert]
ddr_tests = [test_ddr_ok, test_ddr_throttled, test_ddr_no_sensors]

class Test:

    def prepare_fs(self, sensors_path, storage_path, fs_state: TTestFSState):
        sensors_data, storage_data = fs_state.get_dicts()
        if sensors_data:
            for sensor,value in sensors_data.items():
                with open(f'{sensors_path}/{sensor}', 'w') as f:
                    f.write(str(value))
        if storage_data:
            for sensor,value in storage_data.items():
                with open(f'{storage_path}/{sensor}', 'w') as f:
                    f.write(str(value))

    def verify_storage(self, storage_path, fs_state: TTestFSState):
        expected, _ = fs_state.get_dicts()
        for name, ev in expected.items():
            with open(f'{storage_path}/{name}') as f:
                v = int(f.readline().strip())
                assert v == ev

    def setup_and_run(self, cpu_ddr, checker, test_data: TTestData, sensors_path, storage_path=None):
        log_recorder = LogRecorderMock()
        from sonic_platform.throttling_checks import cpu_check
        from sonic_platform.throttling_checks import ddr_check
        cpu_check.logger = log_recorder
        ddr_check.logger = log_recorder

        self.prepare_fs(sensors_path, storage_path, test_data.fs_state)

        result = checker.check()
        test_data.validate(result, log_recorder, cpu_ddr)

        if storage_path:
            self.verify_storage(storage_path, test_data.fs_state)

    @pytest.mark.parametrize("test_data", all_cpu_tests, ids=list(map(lambda x: x.title, all_cpu_tests)))
    def test_cpu(self, test_data: TTestData):
        with tempfile.TemporaryDirectory() as sensors_path, tempfile.TemporaryDirectory() as storage_path:
            checker = CPUThrottlingChecker(sensors_path, storage_path)
            self.setup_and_run('CPU', checker, test_data, sensors_path, storage_path)

    @pytest.mark.parametrize("test_data", first_run_tests, ids=list(map(lambda x: x.title, first_run_tests)))
    def test_cpu_storage_subdir(self, test_data: TTestData):
        with tempfile.TemporaryDirectory() as sensors_path, tempfile.TemporaryDirectory() as storage_path:
            # test that CPUThrottlingChecker will work correctly if subdirectory for storage should be created
            storage_path = f'{storage_path}/storage'
            checker = CPUThrottlingChecker(sensors_path, storage_path)
            self.setup_and_run('CPU', checker, test_data, sensors_path, storage_path)

    @pytest.mark.parametrize("test_data", ddr_tests, ids=list(map(lambda x: x.title, ddr_tests)))
    def test_ddr(self, test_data: TTestData):
        with tempfile.TemporaryDirectory() as sensors_path:
            checker = DDRThrottlingChecker(sensors_path)
            self.setup_and_run('DDR', checker, test_data, sensors_path)
