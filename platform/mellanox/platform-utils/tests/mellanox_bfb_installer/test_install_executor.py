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

        task_fn = lambda idx, child_pids: 0
        with mock.patch.object(install_executor, "signal") as mock_signal:
            failed = install_executor.run_parallel(3, task_fn)
        self.assertEqual(failed, 0)
        mock_signal.signal.assert_called()

    def test_run_parallel_returns_count_of_failing_tasks(self):
        from mellanox_bfb_installer import install_executor

        def task_fn(idx, child_pids):
            return 1 if idx % 2 == 1 else 0

        with mock.patch.object(install_executor, "signal"):
            failed = install_executor.run_parallel(4, task_fn)
        self.assertEqual(failed, 2)

    def test_run_parallel_invokes_task_fn_with_each_index(self):
        from mellanox_bfb_installer import install_executor

        seen = []

        def task_fn(idx, child_pids):
            seen.append(idx)
            return 0

        with mock.patch.object(install_executor, "signal"):
            install_executor.run_parallel(3, task_fn)
        self.assertEqual(sorted(seen), [0, 1, 2])

    def test_run_parallel_registers_signal_handler_for_sigint_sigterm_sighup(self):
        from mellanox_bfb_installer import install_executor
        import signal as sig

        # Patch only signal.signal (the function) so SIGINT/SIGTERM/SIGHUP stay real
        with mock.patch.object(install_executor.signal, "signal") as mock_signal_fn:
            install_executor.run_parallel(1, lambda idx, child_pids: 0)
        self.assertEqual(mock_signal_fn.call_count, 3)
        calls = [c[0] for c in mock_signal_fn.call_args_list]
        self.assertIn((sig.SIGINT, mock.ANY), calls)
        self.assertIn((sig.SIGTERM, mock.ANY), calls)
        self.assertIn((sig.SIGHUP, mock.ANY), calls)

    def test_run_parallel_counts_raised_exception_as_failure_and_logs(self):
        from mellanox_bfb_installer import install_executor

        def task_fn(idx, child_pids):
            if idx == 1:
                raise RuntimeError("task failed")
            return 0

        mock_log = mock.MagicMock()
        with (
            mock.patch.object(install_executor, "signal"),
            mock.patch.object(install_executor, "logger", mock_log),
        ):
            failed = install_executor.run_parallel(3, task_fn)
        self.assertEqual(failed, 1)
        mock_log.error.assert_called()
        self.assertIn("task failed", str(mock_log.error.call_args))

    def test_kill_handler_kills_all_child_pids(self):
        from mellanox_bfb_installer import install_executor
        import signal as sig

        # Task appends pids to the collection run_parallel passes; handler kills them
        def task_fn(idx, child_pids):
            child_pids.append(100)
            child_pids.append(200)
            return 0

        with mock.patch.object(install_executor.signal, "signal") as mock_signal_fn:
            install_executor.run_parallel(1, task_fn)
        handler = mock_signal_fn.call_args_list[0][0][1]
        with mock.patch.object(install_executor.os, "kill") as mock_kill:
            with self.assertRaises(SystemExit):
                handler()
        self.assertEqual(mock_kill.call_count, 2)
        killed = {c[0][0] for c in mock_kill.call_args_list}
        self.assertEqual(killed, {100, 200})
        for c in mock_kill.call_args_list:
            self.assertEqual(c[0][1], sig.SIGKILL)


if __name__ == "__main__":
    unittest.main()
