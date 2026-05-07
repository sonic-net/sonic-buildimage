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
Unit tests for mellanox_bfb_installer.reset_dpu module.
"""

import io
import os
import sys
import unittest
from unittest import mock

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))


class TestIsChassisModuleTablePresent(unittest.TestCase):
    """Tests for function _is_chassis_module_table_present."""

    def test_true_when_keys_returned(self):
        """Test if returns True when STATE_DB has the key (SonicV2Connector)."""
        from mellanox_bfb_installer import reset_dpu

        mock_db = mock.MagicMock()
        mock_db.STATE_DB = "STATE_DB"
        mock_db.keys.return_value = ["CHASSIS_MODULE_TABLE|DPU0"]
        with mock.patch.object(reset_dpu, "SonicV2Connector", return_value=mock_db):
            self.assertTrue(reset_dpu._is_chassis_module_table_present("dpu0"))
        mock_db.connect.assert_called_once_with("STATE_DB")
        mock_db.keys.assert_called_once_with(mock_db.STATE_DB, "CHASSIS_MODULE_TABLE|DPU0")

    def test_false_when_empty(self):
        """Test if returns False when STATE_DB has no matching key."""
        from mellanox_bfb_installer import reset_dpu

        mock_db = mock.MagicMock()
        mock_db.STATE_DB = "STATE_DB"
        mock_db.keys.return_value = []
        with mock.patch.object(reset_dpu, "SonicV2Connector", return_value=mock_db):
            self.assertFalse(reset_dpu._is_chassis_module_table_present("DPU1"))


class TestChassisModuleTableDpuGetField(unittest.TestCase):
    """Tests for function _chassis_module_table_dpu_get_field."""

    def test_returns_str_when_present(self):
        """Test if returns the field value as str when STATE_DB has it."""
        from mellanox_bfb_installer import reset_dpu

        mock_db = mock.MagicMock()
        mock_db.STATE_DB = "STATE_DB"
        mock_db.get.return_value = "True"
        with mock.patch.object(reset_dpu, "SonicV2Connector", return_value=mock_db):
            self.assertEqual(
                reset_dpu._chassis_module_table_dpu_get_field("DPU0", "transition_in_progress"),
                "True",
            )
        mock_db.connect.assert_called_once_with("STATE_DB")
        mock_db.get.assert_called_once_with(mock_db.STATE_DB, "CHASSIS_MODULE_TABLE|DPU0", "transition_in_progress")

    def test_returns_none_when_missing_or_empty(self):
        """Test if returns None when get returns None or empty string."""
        from mellanox_bfb_installer import reset_dpu

        mock_db = mock.MagicMock()
        mock_db.STATE_DB = "STATE_DB"
        for ret in (None, ""):
            mock_db.get.return_value = ret
            with mock.patch.object(reset_dpu, "SonicV2Connector", return_value=mock_db):
                self.assertIsNone(reset_dpu._chassis_module_table_dpu_get_field("DPU1", "some_field"))

    def test_coerces_non_str_value(self):
        """Test if non-string values from get are returned as str."""
        from mellanox_bfb_installer import reset_dpu

        mock_db = mock.MagicMock()
        mock_db.STATE_DB = "STATE_DB"
        mock_db.get.return_value = 42
        with mock.patch.object(reset_dpu, "SonicV2Connector", return_value=mock_db):
            self.assertEqual(reset_dpu._chassis_module_table_dpu_get_field("DPU2", "f"), "42")

    def test_returns_none_on_exception(self):
        """Test if returns None when SonicV2Connector or DB access raises."""
        from mellanox_bfb_installer import reset_dpu

        mock_db = mock.MagicMock()
        mock_db.STATE_DB = "STATE_DB"
        mock_db.connect.side_effect = RuntimeError("no db")
        with mock.patch.object(reset_dpu, "SonicV2Connector", return_value=mock_db):
            self.assertIsNone(reset_dpu._chassis_module_table_dpu_get_field("DPU3", "f"))


class TestWaitForModuleTransitionToComplete(unittest.TestCase):
    """Tests for function wait_for_module_transition_to_complete."""

    def test_no_op_when_flag_not_true(self):
        """Test if returns immediately when not True."""
        from mellanox_bfb_installer import reset_dpu

        with (
            mock.patch.object(reset_dpu, "_chassis_module_table_dpu_get_field", return_value="false") as mock_get_field,
            mock.patch.object(reset_dpu.time, "sleep") as mock_sleep,
        ):
            reset_dpu.wait_for_module_transition_to_complete("dpu0")
        mock_get_field.assert_called_once_with("DPU0", "transition_in_progress")
        mock_sleep.assert_not_called()

    def test_loops_until_cleared(self):
        """Test if loops until transition_in_progress is no longer True."""
        from mellanox_bfb_installer import reset_dpu

        t0 = 1000
        get_field_calls = [
            ("transition_in_progress", "True"),
            ("transition_start_time", str(t0)),
            ("transition_in_progress", "True"),
            ("transition_in_progress", "False"),
        ]

        def fake_get_field(_dpu_upper: str, field: str):
            expected_field, val = get_field_calls.pop(0)
            self.assertEqual(field, expected_field)
            return val

        with (
            mock.patch.object(reset_dpu, "_chassis_module_table_dpu_get_field", side_effect=fake_get_field),
            mock.patch.object(reset_dpu.time, "sleep") as mock_sleep,
            mock.patch.object(reset_dpu.time, "time", return_value=t0 + 10),
        ):
            reset_dpu.wait_for_module_transition_to_complete("dpu1")

        # Assert called twice, each for 2 seconds.
        self.assertEqual(mock_sleep.call_count, 2)
        self.assertEqual(mock_sleep.call_args_list, [mock.call(2), mock.call(2)])

        self.assertEqual(get_field_calls, [])

    def test_timeout_proceeds(self):
        """Test if proceeds after timeout."""
        from mellanox_bfb_installer import reset_dpu

        self.assertEqual(reset_dpu.DPU_TRANSITION_WAIT_TIMEOUT_SECS, 13 * 60)

        start = 1_000_000
        get_field_calls = [
            ("transition_in_progress", "True"),
            ("transition_start_time", str(start)),
            ("transition_in_progress", "True"),
        ]

        def fake_get_field(_dpu_upper: str, field: str):
            expected_field, val = get_field_calls.pop(0)
            self.assertEqual(field, expected_field)
            return val

        timeout_secs = reset_dpu.DPU_TRANSITION_WAIT_TIMEOUT_SECS
        with (
            mock.patch.object(reset_dpu, "_chassis_module_table_dpu_get_field", side_effect=fake_get_field),
            mock.patch.object(reset_dpu.time, "sleep"),
            mock.patch.object(
                reset_dpu.time,
                "time",
                return_value=start + timeout_secs,
            ),
        ):
            reset_dpu.wait_for_module_transition_to_complete("dpu2")

        self.assertEqual(get_field_calls, [])


class TestRebootWithProgress(unittest.TestCase):
    """Tests for _reboot_with_progress (sonic-bfb-installer.sh run_dpuctl_reset behavior)."""

    def test_prints_elapsed_and_total(self):
        """Stdout shows reboot progress line ending with total seconds (matches shell script)."""
        from mellanox_bfb_installer import reset_dpu

        buf = io.StringIO()
        with mock.patch.object(reset_dpu.sys, "stdout", buf):
            reset_dpu._reboot_with_progress("dpu0", lambda: None)
        out = buf.getvalue()
        self.assertIn("dpu0: Reboot:", out)
        self.assertIn("seconds elapsed in total", out)

    def test_logs_error_when_reboot_raises(self):
        """Exception from reboot callback is logged and does not propagate."""
        from mellanox_bfb_installer import reset_dpu

        mock_log = mock.MagicMock()

        def boom() -> None:
            raise RuntimeError("reboot failed")

        buf = io.StringIO()
        with (
            mock.patch.object(reset_dpu, "logger", mock_log),
            mock.patch.object(reset_dpu.sys, "stdout", buf),
        ):
            reset_dpu._reboot_with_progress("dpu0", boom)
        mock_log.error.assert_called_once()
        self.assertIn("reboot failed", str(mock_log.error.call_args))


class TestResetDpu(unittest.TestCase):
    """Tests for function reset_dpu."""

    def test_uses_dpuctlplat_when_chassis_entry_absent(self):
        """Test if uses DpuCtlPlat when CHASSIS_MODULE_TABLE has no entry."""
        from mellanox_bfb_installer import reset_dpu

        mock_log = mock.MagicMock()
        mock_dpu_ctl = mock.MagicMock()
        with (
            mock.patch.object(reset_dpu, "logger", mock_log),
            mock.patch.object(reset_dpu, "_is_chassis_module_table_present", return_value=False),
            mock.patch.object(reset_dpu, "DpuCtlPlat", return_value=mock_dpu_ctl),
        ):
            reset_dpu.reset_dpu("dpu0", False)
        mock_log.info.assert_any_call("Using DpuCtlPlat to reset %s", "dpu0")
        mock_dpu_ctl.dpu_reboot.assert_called_once_with(forced=True, skip_pre_post=False)

    def test_sets_verbosity_on_dpu_ctl(self):
        """Test if sets verbosity on DpuCtlPlat when use_verbose is True."""
        from mellanox_bfb_installer import reset_dpu

        mock_dpu_ctl = mock.MagicMock()
        with (
            mock.patch.object(reset_dpu, "_is_chassis_module_table_present", return_value=False),
            mock.patch.object(reset_dpu, "DpuCtlPlat", return_value=mock_dpu_ctl),
        ):
            reset_dpu.reset_dpu("dpu0", True)
        self.assertEqual(mock_dpu_ctl.verbosity, True)

    def test_uses_module_helper_when_chassis_entry_exists(self):
        """Test if uses ModuleHelper when CHASSIS_MODULE_TABLE has entry."""
        from mellanox_bfb_installer import reset_dpu

        mock_log = mock.MagicMock()
        mock_dpu_ctl = mock.MagicMock()
        mock_helper = mock.MagicMock()
        with (
            mock.patch.object(reset_dpu, "logger", mock_log),
            mock.patch.object(reset_dpu, "_is_chassis_module_table_present", return_value=True),
            mock.patch.object(reset_dpu, "DpuCtlPlat", return_value=mock_dpu_ctl),
            mock.patch.object(reset_dpu, "ModuleHelper", return_value=mock_helper),
        ):
            reset_dpu.reset_dpu("dpu0", False)
        mock_log.info.assert_any_call("Using ModuleHelper to reset %s", "dpu0")
        mock_helper.module_pre_shutdown.assert_called_once_with("dpu0")
        mock_dpu_ctl.dpu_reboot.assert_called_once_with(forced=True, skip_pre_post=True)
        mock_helper.module_post_startup.assert_called_once_with("dpu0")


if __name__ == "__main__":
    unittest.main()
