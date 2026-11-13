#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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
Unit tests for mellanox_bfb_installer.install_executor module.
"""

import os
import sys
import unittest
from unittest import mock

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))


class TestInstallExecutor(unittest.TestCase):
    """Tests for install_executor module."""

    def test_run_parallel_returns_zero_when_all_tasks_return_zero(self):
        from mellanox_bfb_installer import install_executor

        failed = install_executor.run_parallel(3, lambda _idx: 0)
        self.assertEqual(failed, 0)

    def test_run_parallel_returns_count_of_failing_tasks(self):
        from mellanox_bfb_installer import install_executor

        def task_fn(idx):
            return 1 if idx % 2 == 1 else 0

        failed = install_executor.run_parallel(4, task_fn)
        self.assertEqual(failed, 2)

    def test_run_parallel_invokes_task_fn_with_each_index(self):
        from mellanox_bfb_installer import install_executor

        seen = []

        def task_fn(idx):
            seen.append(idx)
            return 0

        install_executor.run_parallel(3, task_fn)
        self.assertEqual(sorted(seen), [0, 1, 2])

    def test_run_parallel_does_not_take_over_signal_handling(self):
        """Signals belong to the caller that supervises the installation transaction."""
        from mellanox_bfb_installer import install_executor
        import signal as sig

        with mock.patch.object(sig, "signal") as mock_signal_fn:
            install_executor.run_parallel(1, lambda _idx: 0)
        mock_signal_fn.assert_not_called()

    def test_run_parallel_counts_raised_exception_as_failure_and_logs(self):
        from mellanox_bfb_installer import install_executor

        def task_fn(idx):
            if idx == 1:
                raise RuntimeError("task failed")
            return 0

        mock_log = mock.MagicMock()
        with mock.patch.object(install_executor, "logger", mock_log):
            failed = install_executor.run_parallel(3, task_fn)
        self.assertEqual(failed, 1)
        mock_log.error.assert_called()
        self.assertIn("task failed", str(mock_log.error.call_args))


if __name__ == "__main__":
    unittest.main()
