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
Unit tests for mellanox_bfb_installer.bfb_install_core module.
"""

import os
import shutil
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))

from mellanox_bfb_installer import install_executor  # noqa: E402


class TestBfBInstallCore(unittest.TestCase):
    """Tests for bfb_install_core module."""

    def test_run_bfb_install_success_logs_and_returns_zero(self):
        from mellanox_bfb_installer import bfb_install_core

        mock_log = mock.MagicMock()
        with tempfile.NamedTemporaryFile(mode="w", suffix=".result", delete=False) as f:
            result_path = f.name
        try:
            mock_proc = mock.MagicMock()
            mock_proc.stdout = iter(["line1\n", "line2\n"])
            mock_proc.poll = mock.MagicMock(side_effect=[None, None, 0])
            mock_proc.returncode = 0
            with (
                mock.patch.object(bfb_install_core, "logger", mock_log),
                mock.patch.object(bfb_install_core.subprocess, "Popen", return_value=mock_proc),
                mock.patch.object(bfb_install_core.time, "sleep"),
                mock.patch("sys.stdout"),
            ):
                status = bfb_install_core._run_bfb_install_image_delivery(
                    rshim="rshim0",
                    rshim_id="0",
                    bfb_path="/path/to.bfb",
                    result_file_path=result_path,
                    child_pids=install_executor.PidCollection(),
                    verbose=False,
                )
            self.assertEqual(status, 0)
            mock_log.info.assert_any_call(
                "Installing bfb image on DPU connected to %s using %s",
                "rshim0",
                "timeout 1200s bfb-install -b /path/to.bfb -r rshim0",
            )
            mock_log.info.assert_any_call("%s: Installation Successful", "0")
            with open(result_path) as rf:
                lines = rf.read().splitlines()
            self.assertEqual(lines, ["0: line1", "0: line2"])
        finally:
            os.unlink(result_path)

    def test_run_bfb_install_failure_logs_error_and_returns_nonzero(self):
        from mellanox_bfb_installer import bfb_install_core

        mock_log = mock.MagicMock()
        with tempfile.NamedTemporaryFile(mode="w", suffix=".result", delete=False) as f:
            result_path = f.name
        try:
            mock_proc = mock.MagicMock()
            mock_proc.stdout = iter(["error line\n"])
            mock_proc.poll = mock.MagicMock(side_effect=[None, 1])
            mock_proc.returncode = 1
            with (
                mock.patch.object(bfb_install_core, "logger", mock_log),
                mock.patch.object(bfb_install_core.subprocess, "Popen", return_value=mock_proc),
                mock.patch.object(bfb_install_core.time, "sleep"),
                mock.patch("sys.stdout"),
            ):
                status = bfb_install_core._run_bfb_install_image_delivery(
                    rshim="rshim1",
                    rshim_id="1",
                    bfb_path="/bfb.bfb",
                    child_pids=install_executor.PidCollection(),
                    result_file_path=result_path,
                    verbose=False,
                )
            self.assertEqual(status, 1)
            mock_log.error.assert_called_once_with(
                "%s: Error: Installation failed on connected DPU! Exit code: %s", "1", 1
            )
        finally:
            os.unlink(result_path)

    def test_run_bfb_install_builds_correct_command(self):
        from mellanox_bfb_installer import bfb_install_core

        with tempfile.NamedTemporaryFile(mode="w", suffix=".result", delete=False) as f:
            result_path = f.name
        try:
            mock_proc = mock.MagicMock()
            mock_proc.stdout = iter([])
            mock_proc.poll = mock.MagicMock(return_value=0)
            mock_proc.returncode = 0
            with (
                mock.patch.object(bfb_install_core.subprocess, "Popen", return_value=mock_proc) as mock_popen,
                mock.patch.object(bfb_install_core.time, "sleep"),
                mock.patch("sys.stdout"),
            ):
                bfb_install_core._run_bfb_install_image_delivery(
                    rshim="rshim0",
                    rshim_id="0",
                    bfb_path="/b.bfb",
                    result_file_path=result_path,
                    child_pids=install_executor.PidCollection(),
                    config_path="/c.yaml",
                    verbose=False,
                )
            pos_args = mock_popen.call_args[0]
            kwargs = mock_popen.call_args[1]
            self.assertEqual(len(pos_args), 1)
            expected_cmd = [
                "timeout",
                "1200s",
                "bfb-install",
                "-b",
                "/b.bfb",
                "-r",
                "rshim0",
                "-c",
                "/c.yaml",
            ]
            self.assertEqual(pos_args[0], expected_cmd)
            expected_kwargs = {
                "stdout": bfb_install_core.subprocess.PIPE,
                "stderr": bfb_install_core.subprocess.STDOUT,
                "text": True,
            }
            self.assertEqual(kwargs, expected_kwargs)
        finally:
            os.unlink(result_path)

    def test_full_install_bfb_on_device_returns_one_when_no_pci_bus(self):
        """full_install_bfb_on_device returns 1 and logs when rshim_pci_bus_id is falsy."""
        from mellanox_bfb_installer import bfb_install_core

        mock_log = mock.MagicMock()
        work_dir = tempfile.mkdtemp()
        try:
            with (
                mock.patch.object(bfb_install_core, "logger", mock_log),
                mock.patch.object(bfb_install_core.reset_dpu, "wait_for_module_transition_to_complete"),
            ):
                status = bfb_install_core.full_install_bfb_on_device(
                    rshim_name="rshim0",
                    rshim_id="0",
                    dpu_name="dpu0",
                    rshim_pci_bus_id=None,
                    dpu_pci_bus_id=None,
                    config_path=None,
                    bfb_path="/x.bfb",
                    work_dir=work_dir,
                    verbose=False,
                    child_pids=install_executor.PidCollection(),
                )
            self.assertEqual(status, 1)
            mock_log.error.assert_called_once()
            self.assertIn("Could not find rshim PCI bus ID", mock_log.error.call_args[0][0])
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)

    def test_full_install_bfb_on_device_returns_one_when_start_rshim_daemon_fails(self):
        """full_install_bfb_on_device returns 1 when start_rshim_daemon returns False."""
        from mellanox_bfb_installer import bfb_install_core

        work_dir = tempfile.mkdtemp()
        try:
            with (
                mock.patch.object(bfb_install_core.reset_dpu, "wait_for_module_transition_to_complete"),
                mock.patch.object(bfb_install_core.rshim_daemon, "start_rshim_daemon", return_value=False),
            ):
                status = bfb_install_core.full_install_bfb_on_device(
                    rshim_name="rshim0",
                    rshim_id="0",
                    dpu_name="dpu0",
                    rshim_pci_bus_id="0000:08:00.0",
                    dpu_pci_bus_id=None,
                    config_path=None,
                    bfb_path="/x.bfb",
                    work_dir=work_dir,
                    verbose=False,
                    child_pids=install_executor.PidCollection(),
                )
            self.assertEqual(status, 1)
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)

    def test_full_install_bfb_on_device_returns_one_when_wait_for_rshim_boot_fails(self):
        """full_install_bfb_on_device returns 1 when wait_for_rshim_boot returns False."""
        from mellanox_bfb_installer import bfb_install_core

        work_dir = tempfile.mkdtemp()
        try:
            with (
                mock.patch.object(bfb_install_core.reset_dpu, "wait_for_module_transition_to_complete"),
                mock.patch.object(bfb_install_core.rshim_daemon, "start_rshim_daemon", return_value=True),
                mock.patch.object(bfb_install_core.rshim_daemon, "wait_for_rshim_boot", return_value=False),
                mock.patch.object(bfb_install_core.rshim_daemon, "stop_rshim_daemon"),
                mock.patch.object(bfb_install_core.reset_dpu, "reset_dpu"),
            ):
                status = bfb_install_core.full_install_bfb_on_device(
                    rshim_name="rshim0",
                    rshim_id="0",
                    dpu_name="dpu0",
                    rshim_pci_bus_id="0000:08:00.0",
                    dpu_pci_bus_id=None,
                    config_path=None,
                    bfb_path="/x.bfb",
                    work_dir=work_dir,
                    verbose=False,
                    child_pids=install_executor.PidCollection(),
                )
            self.assertEqual(status, 1)
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)

    def test_full_install_bfb_on_device_waits_for_module_transition_before_start_rshim(self):
        """Transition wait runs for dpu_name before start_rshim_daemon."""
        from mellanox_bfb_installer import bfb_install_core

        work_dir = tempfile.mkdtemp()
        try:
            mock_wait = mock.MagicMock()
            mock_start = mock.MagicMock(return_value=True)
            with (
                mock.patch.object(bfb_install_core.reset_dpu, "wait_for_module_transition_to_complete", mock_wait),
                mock.patch.object(bfb_install_core.rshim_daemon, "start_rshim_daemon", mock_start),
                mock.patch.object(bfb_install_core.rshim_daemon, "wait_for_rshim_boot", return_value=False),
                mock.patch.object(bfb_install_core.rshim_daemon, "stop_rshim_daemon"),
                mock.patch.object(bfb_install_core.reset_dpu, "reset_dpu"),
            ):
                bfb_install_core.full_install_bfb_on_device(
                    rshim_name="rshim0",
                    rshim_id="0",
                    dpu_name="dpu7",
                    rshim_pci_bus_id="0000:08:00.0",
                    dpu_pci_bus_id=None,
                    config_path=None,
                    bfb_path="/x.bfb",
                    work_dir=work_dir,
                    verbose=False,
                    child_pids=install_executor.PidCollection(),
                )
            mock_wait.assert_called_once_with("dpu7")
            mock_start.assert_called_once()
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)

    def test_full_install_bfb_on_device_success_returns_zero_and_calls_reset(self):
        """full_install_bfb_on_device returns 0 on success and calls stop_rshim_daemon and reset_dpu."""
        from mellanox_bfb_installer import bfb_install_core

        work_dir = tempfile.mkdtemp()
        try:
            with (
                mock.patch.object(bfb_install_core.reset_dpu, "wait_for_module_transition_to_complete"),
                mock.patch.object(bfb_install_core.platform_dpu, "remove_cx7_pci_device"),
                mock.patch.object(bfb_install_core.rshim_daemon, "start_rshim_daemon", return_value=True),
                mock.patch.object(bfb_install_core.rshim_daemon, "wait_for_rshim_boot", return_value=True),
                mock.patch.object(bfb_install_core.rshim_daemon, "stop_rshim_daemon") as mock_stop,
                mock.patch.object(bfb_install_core, "_run_bfb_install_image_delivery", return_value=0),
                mock.patch.object(bfb_install_core.reset_dpu, "reset_dpu") as mock_reset,
            ):
                status = bfb_install_core.full_install_bfb_on_device(
                    rshim_name="rshim0",
                    rshim_id="0",
                    dpu_name="dpu0",
                    rshim_pci_bus_id="0000:08:00.0",
                    dpu_pci_bus_id=None,
                    config_path=None,
                    bfb_path="/x.bfb",
                    work_dir=work_dir,
                    verbose=False,
                    child_pids=install_executor.PidCollection(),
                )
            self.assertEqual(status, 0)
            mock_stop.assert_called_once_with("0")
            mock_reset.assert_called_once()
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)

    def test_full_install_bfb_on_device_passes_correct_args_to_run_bfb_install_image_delivery(self):
        """full_install_bfb_on_device calls _run_bfb_install_image_delivery with correct kwargs."""
        from mellanox_bfb_installer import bfb_install_core

        work_dir = tempfile.mkdtemp()
        try:
            child_pids = install_executor.PidCollection()
            with (
                mock.patch.object(bfb_install_core.reset_dpu, "wait_for_module_transition_to_complete"),
                mock.patch.object(bfb_install_core.platform_dpu, "remove_cx7_pci_device"),
                mock.patch.object(bfb_install_core.rshim_daemon, "start_rshim_daemon", return_value=True),
                mock.patch.object(bfb_install_core.rshim_daemon, "wait_for_rshim_boot", return_value=True),
                mock.patch.object(bfb_install_core.rshim_daemon, "stop_rshim_daemon"),
                mock.patch.object(bfb_install_core, "_run_bfb_install_image_delivery", return_value=0) as mock_run,
                mock.patch.object(bfb_install_core.reset_dpu, "reset_dpu"),
            ):
                bfb_install_core.full_install_bfb_on_device(
                    rshim_name="rshim1",
                    rshim_id="1",
                    dpu_name="dpu1",
                    rshim_pci_bus_id="0000:08:00.0",
                    dpu_pci_bus_id=None,
                    config_path="/cfg.yaml",
                    bfb_path="/img.bfb",
                    work_dir=work_dir,
                    verbose=True,
                    child_pids=child_pids,
                )
            mock_run.assert_called_once()
            call_kwargs = mock_run.call_args[1]
            self.assertEqual(call_kwargs["rshim"], "rshim1")
            self.assertEqual(call_kwargs["rshim_id"], "1")
            self.assertEqual(call_kwargs["bfb_path"], "/img.bfb")
            self.assertEqual(call_kwargs["config_path"], "/cfg.yaml")
            self.assertEqual(call_kwargs["verbose"], True)
            self.assertIs(call_kwargs["child_pids"], child_pids)
            self.assertIn("result_file.", call_kwargs["result_file_path"])
            self.assertTrue(call_kwargs["result_file_path"].startswith(work_dir))
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)

    def test_full_install_bfb_on_device_calls_remove_cx7_pci_device_with_dpu_bus_id_and_rshim_id_prefix(self):
        """full_install_bfb_on_device calls remove_cx7_pci_device with dpu_pci_bus_id and log prefix when present."""
        from mellanox_bfb_installer import bfb_install_core

        work_dir = tempfile.mkdtemp()
        try:
            with (
                mock.patch.object(bfb_install_core.reset_dpu, "wait_for_module_transition_to_complete"),
                mock.patch.object(bfb_install_core.platform_dpu, "remove_cx7_pci_device") as mock_remove,
                mock.patch.object(bfb_install_core.rshim_daemon, "start_rshim_daemon", return_value=True),
                mock.patch.object(bfb_install_core.rshim_daemon, "wait_for_rshim_boot", return_value=True),
                mock.patch.object(bfb_install_core.rshim_daemon, "stop_rshim_daemon"),
                mock.patch.object(bfb_install_core, "_run_bfb_install_image_delivery", return_value=0),
                mock.patch.object(bfb_install_core.reset_dpu, "reset_dpu"),
            ):
                bfb_install_core.full_install_bfb_on_device(
                    rshim_name="rshim2",
                    rshim_id="2",
                    dpu_name="dpu2",
                    rshim_pci_bus_id="0000:08:00.1",
                    dpu_pci_bus_id="0000:08:00.0",
                    config_path=None,
                    bfb_path="/x.bfb",
                    work_dir=work_dir,
                    verbose=False,
                    child_pids=install_executor.PidCollection(),
                )
            mock_remove.assert_called_once_with("0000:08:00.0", "2: ")
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)

    def test_full_install_bfb_on_device_returns_one_when_image_delivery_fails(self):
        """full_install_bfb_on_device returns 1 when _run_bfb_install_image_delivery returns non-zero."""
        from mellanox_bfb_installer import bfb_install_core

        work_dir = tempfile.mkdtemp()
        try:
            with (
                mock.patch.object(bfb_install_core.reset_dpu, "wait_for_module_transition_to_complete"),
                mock.patch.object(bfb_install_core.platform_dpu, "remove_cx7_pci_device"),
                mock.patch.object(bfb_install_core.rshim_daemon, "start_rshim_daemon", return_value=True),
                mock.patch.object(bfb_install_core.rshim_daemon, "wait_for_rshim_boot", return_value=True),
                mock.patch.object(bfb_install_core.rshim_daemon, "stop_rshim_daemon") as mock_stop,
                mock.patch.object(bfb_install_core, "_run_bfb_install_image_delivery", return_value=1),
                mock.patch.object(bfb_install_core.reset_dpu, "reset_dpu") as mock_reset,
            ):
                status = bfb_install_core.full_install_bfb_on_device(
                    rshim_name="rshim0",
                    rshim_id="0",
                    dpu_name="dpu0",
                    rshim_pci_bus_id="0000:08:00.0",
                    dpu_pci_bus_id=None,
                    config_path=None,
                    bfb_path="/x.bfb",
                    work_dir=work_dir,
                    verbose=False,
                    child_pids=install_executor.PidCollection(),
                )
            self.assertEqual(status, 1)
            mock_stop.assert_called_once_with("0")
            mock_reset.assert_called_once()
        finally:
            shutil.rmtree(work_dir, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
