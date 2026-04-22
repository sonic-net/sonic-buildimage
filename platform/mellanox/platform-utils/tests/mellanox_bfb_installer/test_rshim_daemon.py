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
Unit tests for mellanox_bfb_installer.rshim_daemon module.
"""

import os
import sys
import unittest
from unittest import mock

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))


class TestRshimDaemon(unittest.TestCase):
    """Tests for rshim_daemon module."""

    def test_start_rshim_daemon_success(self):
        from mellanox_bfb_installer import rshim_daemon

        with mock.patch.object(rshim_daemon.subprocess, "run") as mock_run:
            mock_run.return_value = mock.MagicMock(returncode=0)
            self.assertTrue(rshim_daemon.start_rshim_daemon("0", "0000:08:00.0"))
            mock_run.assert_called_once()
            # Check the subprocess.run call arguments strictly to make sure the io capturing isn't accidentally enabled.
            pos_args = mock_run.call_args[0]
            kwargs = mock_run.call_args[1]
            self.assertEqual(len(pos_args), 1, "subprocess.run should be called with exactly one positional argument")
            self.assertEqual(kwargs, {}, "subprocess.run should be called with no keyword arguments")
            call_args = pos_args[0]
            expected = [
                "start-stop-daemon",
                "--start",
                "--quiet",
                "--background",
                "--make-pidfile",
                "--pidfile",
                "/var/run/rshim_0.pid",
                "--exec",
                rshim_daemon.RSHIM_BINARY,
                "--",
                "-f",
                "-i",
                "0",
                "-d",
                "pcie-0000:08:00.0",
            ]
            self.assertEqual(call_args, expected, "subprocess.run args")

    def test_start_rshim_daemon_failure_logs_and_returns_false(self):
        from mellanox_bfb_installer import rshim_daemon

        mock_log = mock.MagicMock()
        with (
            mock.patch.object(rshim_daemon, "logger", mock_log),
            mock.patch.object(rshim_daemon.subprocess, "run") as mock_run,
        ):
            mock_run.return_value = mock.MagicMock(returncode=1)
            self.assertFalse(rshim_daemon.start_rshim_daemon("1", "0000:09:00.0"))
            mock_log.error.assert_called_once()
            # logger.error("Failed to start rshim for rshim%s", rid) -> args[0] is format string
            self.assertIn("Failed to start rshim for rshim", mock_log.error.call_args[0][0])
            self.assertEqual(mock_log.error.call_args[0][1], "1")

    def test_stop_rshim_daemon_returns_false_when_no_pidfile(self):
        from mellanox_bfb_installer import rshim_daemon

        mock_log = mock.MagicMock()
        with (
            mock.patch.object(rshim_daemon, "logger", mock_log),
            mock.patch.object(rshim_daemon.os.path, "isfile", return_value=False),
            mock.patch.object(rshim_daemon.subprocess, "run") as mock_run,
        ):
            self.assertFalse(rshim_daemon.stop_rshim_daemon("0"))
            mock_run.assert_not_called()
            mock_log.warning.assert_called_once()
            self.assertIn("missing pidfile", mock_log.warning.call_args[0][0])

    def test_stop_rshim_daemon_calls_start_stop_daemon_when_pidfile_exists(self):
        from mellanox_bfb_installer import rshim_daemon

        with (
            mock.patch.object(rshim_daemon.os.path, "isfile", return_value=True),
            mock.patch.object(rshim_daemon.subprocess, "run") as mock_run,
        ):
            mock_run.return_value = mock.MagicMock(returncode=0)
            self.assertTrue(rshim_daemon.stop_rshim_daemon("0"))
            mock_run.assert_called_once()
            # Check the subprocess.run call arguments strictly to make sure the io capturing isn't accidentally enabled.
            pos_args = mock_run.call_args[0]
            kwargs = mock_run.call_args[1]
            self.assertEqual(len(pos_args), 1, "subprocess.run should be called with exactly one positional argument")
            self.assertEqual(kwargs, {}, "subprocess.run should be called with no keyword arguments")
            call_args = pos_args[0]
            expected = [
                "start-stop-daemon",
                "--stop",
                "--quiet",
                "--pidfile",
                "/var/run/rshim_0.pid",
                "--remove-pidfile",
                "--retry",
                "TERM/15/KILL/5",
            ]
            self.assertEqual(len(call_args), len(expected), "subprocess.run args length")
            self.assertEqual(call_args, expected, "subprocess.run args")

    def test_stop_rshim_daemon_returns_false_on_nonzero_exit(self):
        from mellanox_bfb_installer import rshim_daemon

        mock_log = mock.MagicMock()
        with (
            mock.patch.object(rshim_daemon, "logger", mock_log),
            mock.patch.object(rshim_daemon.os.path, "isfile", return_value=True),
            mock.patch.object(rshim_daemon.subprocess, "run") as mock_run,
        ):
            mock_run.return_value = mock.MagicMock(returncode=1)
            self.assertFalse(rshim_daemon.stop_rshim_daemon("1"))
            mock_log.warning.assert_called_once()
            self.assertIn("exit code", mock_log.warning.call_args[0][0])

    def test_stop_rshim_daemon_returns_false_on_subprocess_exception(self):
        from mellanox_bfb_installer import rshim_daemon

        mock_log = mock.MagicMock()
        with (
            mock.patch.object(rshim_daemon, "logger", mock_log),
            mock.patch.object(rshim_daemon.os.path, "isfile", return_value=True),
            mock.patch.object(rshim_daemon.subprocess, "run", side_effect=OSError("boom")),
        ):
            self.assertFalse(rshim_daemon.stop_rshim_daemon("0"))
            mock_log.error.assert_called_once()
            self.assertIn("Failed to stop rshim", mock_log.error.call_args[0][0])

    def test_wait_for_rshim_boot_returns_true_when_boot_exists(self):
        from mellanox_bfb_installer import rshim_daemon

        with mock.patch.object(rshim_daemon.os.path, "exists", return_value=True):
            self.assertTrue(rshim_daemon.wait_for_rshim_boot("rshim0"))

    def test_wait_for_rshim_boot_returns_false_and_logs_on_timeout(self):
        from mellanox_bfb_installer import rshim_daemon

        mock_log = mock.MagicMock()
        with (
            mock.patch.object(rshim_daemon, "logger", mock_log),
            mock.patch.object(rshim_daemon.os.path, "exists", return_value=False),
            mock.patch.object(rshim_daemon.time, "sleep"),  # avoid 10s sleep
        ):
            self.assertFalse(rshim_daemon.wait_for_rshim_boot("rshim0"))
            mock_log.error.assert_called_once()
            self.assertIn("Boot file did not appear after 10 seconds", mock_log.error.call_args[0][0])


if __name__ == "__main__":
    unittest.main()
