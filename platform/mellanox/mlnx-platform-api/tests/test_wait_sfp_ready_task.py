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

import os
import sys

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform import wait_sfp_ready_task
from sonic_platform import utils


class TestWaitSfpReadyTask:
    def test_schedule(self):
        task = wait_sfp_ready_task.WaitSfpReadyTask()
        task.schedule_wait(0)
        assert not task.empty()
        task.cancel_wait(0)
        assert task.empty()

    def test_cancel_wait_unknown_index(self):
        task = wait_sfp_ready_task.WaitSfpReadyTask()
        # cancel_wait must be safe for an index that was never scheduled
        task.cancel_wait(42)
        assert task.empty()
        assert 42 not in task._ready_set

    def test_cancel_wait_removes_from_ready_set(self):
        task = wait_sfp_ready_task.WaitSfpReadyTask()
        # Pre-populate ready set to verify discard-style cleanup
        task._ready_set.add(7)
        task.cancel_wait(7)
        assert 7 not in task._ready_set
        
    def test_run(self):
        task = wait_sfp_ready_task.WaitSfpReadyTask()
        task.WAIT_TIME = 1 # Fast the test
        task.start()
        task.schedule_wait(0)
        assert utils.wait_until(lambda: 0 in task.get_ready_set(), 4, 0.5), 'sfp does not reach ready in 4 seconds'
        assert 0 not in task._wait_dict
        assert len(task._ready_set) == 0
        task.stop()
        task.join()
