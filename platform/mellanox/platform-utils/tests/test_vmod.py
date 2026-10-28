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

"""Unit tests for the OE firmware version read and compare."""

import os
import sys
from unittest import mock

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from mellanox_fw_manager import vmod
from mellanox_fw_manager.spectrum_manager import SpectrumFirmwareManager

# flint -d <pci> --vmod --downstream_device_ids 0 q, MFT 4.37.0-1010, FW 39_2020_0770-00x
VMOD_QUERY = (
    "Host : 0000:02:00.0\n"
    " Device index 0\n"
    " Component Index 5\n"
    " Component Status PRESENT\n"
    " Component Update State {state}\n"
    " Running state is :  {running_state} \n"
    "Information block is :  FW image A is present  FW image B is present \n"
    "FW A Version : {a}\n"
    "FW B Version : {b}\n"
    "FW Factory Version : 00.00.0000\n"
    "SupportedProtocol: Does not support either one of the FW update procedures\n"
    "Serial number is 0\n"
)


def vmod_query(a, b, running, boot=None):
    boot = boot or running
    if boot == running:
        state = "IDLE"
        running_state = f"Image {running} is running  Image {boot} is committed, module boots from image {boot}"
    else:  # burned, waiting for the ASIC reset
        state = "ACTIVE_PENDING_RESET"
        running_state = f"Image {boot} is committed, module boots from image {boot}  Image {running} is running"
    return VMOD_QUERY.format(state=state, running_state=running_state, a=a, b=b)


DEVICE_QUERY = vmod_query(a='95.10.0750', b='95.10.0752', running='B')


class TestParseVersions:

    def test_running_bank(self):
        assert vmod.parse_hw_versions(DEVICE_QUERY) == ("95.10.0752", "95.10.0752")

    def test_burned_image_waiting_for_reset(self):
        output = vmod_query(a='95.10.0768', b='95.10.0770', running='B', boot='A')
        assert vmod.parse_hw_versions(output) == vmod.HwVersions(running="95.10.0770", boot="95.10.0768")

    def test_old_image_in_the_other_bank_is_ignored(self):
        output = vmod_query(a='95.10.0768', b='95.10.0770', running='A')
        assert vmod.parse_hw_versions(output) == ("95.10.0768", "95.10.0768")

    def test_no_boot_bank_means_the_running_bank(self):
        output = "Running state is :  Image B is running\nFW A Version : 95.10.0750\nFW B Version : 95.10.0752\n"
        assert vmod.parse_hw_versions(output) == ("95.10.0752", "95.10.0752")

    def test_no_running_bank(self):
        assert vmod.parse_hw_versions("FW A Version : 95.10.0750\nFW B Version : 95.10.0752\n") is None
        assert vmod.parse_hw_versions("-E- Failed\n") is None

    def test_unparsable_version_reads_nothing(self):
        assert vmod.parse_hw_versions(vmod_query(a='95.10.0750', b='N/A', running='B')) is None
        assert vmod.parse_hw_versions(vmod_query(a='garbage', b='95.10.0752', running='B', boot='A')) is None

    def test_file_version(self):
        assert vmod.parse_file_version("Image type:   FSVMOD\nFW Version:   95.2020.0752\n") == "95.2020.0752"
        assert vmod.parse_file_version("-E- Failed\n") is None


class TestVersionsMatch:

    @pytest.mark.parametrize("hw, mfa, expected", [
        ("95.10.0752", "95.2020.0752", True),
        ("95.10.0752", "95_2020_0752-002", True),
        ("95.10.0752", "95.2020.0756", False),
        ("95.11.0752", "95.2020.0752", False),
        ("96.10.0752", "95.2020.0752", False),
        ("95.10.0752", "garbage", False),
    ])
    def test_compare(self, hw, mfa, expected):
        assert vmod.versions_match(hw, mfa) is expected

    @pytest.mark.parametrize("mfa, expected", [
        ("95.2020.0768", "95.10.0768"),
        ("95_2020_0762-002", "95.10.0762"),
        ("95.2222.0001", "95.11.0001"),
        ("95.0768", None),
        ("garbage", None),
    ])
    def test_to_device_version(self, mfa, expected):
        assert vmod.to_device_version(mfa) == expected

    # MFA VMOD version and what flint --vmod reported for it on an SN6810_LD.
    # A failure here means the firmware changed the rule.
    @pytest.mark.parametrize("mfa, device", [
        ("95_2020_0768", "95.10.0768"),
        ("95_2020_0770-002", "95.10.0770"),
        ("95_2020_0770-008", "95.10.0770"),
    ])
    def test_rule_matches_the_hardware(self, mfa, device):
        assert vmod.versions_match(device, mfa)


class TestIsCpoPlatform:

    @mock.patch('sonic_py_common.device_info.get_cpo_data', return_value={'oe': {}})
    def test_cpo(self, _data):
        assert vmod.is_cpo_platform() is True

    @mock.patch('sonic_py_common.device_info.get_cpo_data', return_value=None)
    def test_non_cpo(self, _data):
        assert vmod.is_cpo_platform() is False

    @mock.patch('sonic_py_common.device_info.get_cpo_data', side_effect=OSError("no platform"))
    def test_error_is_logged(self, _data):
        with mock.patch.object(vmod.logger, 'warning') as mock_warning:
            assert vmod.is_cpo_platform() is False
        mock_warning.assert_called_once()


class TestReads:

    @staticmethod
    def _run(cmd, **kwargs):
        out = ""
        if cmd[:3] == ['flint', '-d', '0000:02:00.0'] and '--vmod' in cmd:
            out = DEVICE_QUERY
        elif cmd == ['flint', '-d', '0000:02:00.0', 'q']:
            out = "FW Version:  39_2020_0762-002\nPSID:  MT_0000001916\n"
        elif cmd[0] == 'mlxfwmanager':
            assert cmd[cmd.index('-i') + 1].endswith('fw-SPC6.mfa')
            assert cmd[cmd.index('--psid') + 1] == 'MT_0000001916'
            open(cmd[cmd.index('--extract-file') + 1], 'w').close()
        elif cmd[:2] == ['flint', '-i']:
            out = "Image type:   FSVMOD\nFW Version:   95_2020_0762-002\n"
        return mock.Mock(returncode=0, stdout=out, stderr="")

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_hw_versions_by_pci_id(self, mock_run):
        mock_run.side_effect = self._run
        assert vmod.get_hw_versions('0000:02:00.0') == ("95.10.0752", "95.10.0752")
        assert mock_run.call_args[0][0] == ['flint', '-d', '0000:02:00.0', '--vmod',
                                            '--downstream_device_ids', '0', 'q']

    @pytest.fixture(autouse=True)
    def cache(self, tmp_path):
        self.cache_file = tmp_path / 'cache' / 'vmod.json'
        self.mfa = tmp_path / 'fw-SPC6.mfa'
        self.mfa.write_bytes(b'mfa')
        with mock.patch.object(vmod, 'MFA_VERSION_CACHE', str(self.cache_file)):
            yield

    def _extracts(self, mock_run):
        return sum(1 for c in mock_run.call_args_list if c[0][0][0] == 'mlxfwmanager')

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_mfa_version(self, mock_run):
        mock_run.side_effect = self._run
        assert vmod.get_mfa_version(str(self.mfa), '0000:02:00.0') == "95_2020_0762-002"

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_mfa_version_is_extracted_once(self, mock_run):
        mock_run.side_effect = self._run
        for _ in range(3):
            assert vmod.get_mfa_version(str(self.mfa), '0000:02:00.0') == "95_2020_0762-002"
        assert self._extracts(mock_run) == 1
        assert self.cache_file.exists()

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_changed_mfa_is_extracted_again(self, mock_run):
        mock_run.side_effect = self._run
        vmod.get_mfa_version(str(self.mfa), '0000:02:00.0')
        os.utime(self.mfa, ns=(1, 2))
        vmod.get_mfa_version(str(self.mfa), '0000:02:00.0')
        assert self._extracts(mock_run) == 2

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_cache_keeps_only_the_current_mfa(self, mock_run):
        mock_run.side_effect = self._run
        for mtime in range(1, 6):
            os.utime(self.mfa, ns=(mtime, mtime))
            vmod.get_mfa_version(str(self.mfa), '0000:02:00.0')
        assert list(vmod._load_cache()) == [f'{self.mfa}:3:5:MT_0000001916']

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_failed_extract_is_not_cached(self, mock_run):
        mock_run.side_effect = lambda cmd, **kw: (mock.Mock(returncode=1, stdout="", stderr="")
                                                  if cmd[0] == 'mlxfwmanager' else self._run(cmd))
        assert vmod.get_mfa_version(str(self.mfa), '0000:02:00.0') is None
        assert not self.cache_file.exists()

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_unwritable_or_corrupt_cache_still_works(self, mock_run):
        mock_run.side_effect = self._run
        self.cache_file.parent.mkdir()
        self.cache_file.write_text("not json")
        assert vmod.get_mfa_version(str(self.mfa), '0000:02:00.0') == "95_2020_0762-002"
        with mock.patch.object(vmod, 'MFA_VERSION_CACHE', '/proc/no/such/dir/x.json'):
            assert vmod.get_mfa_version(str(self.mfa), '0000:02:00.0') == "95_2020_0762-002"

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_failed_cache_write_leaves_no_temp_file(self, mock_run):
        mock_run.side_effect = self._run
        self.cache_file.mkdir(parents=True)  # a directory: the final rename fails
        assert vmod.get_mfa_version(str(self.mfa), '0000:02:00.0') == "95_2020_0762-002"
        assert sorted(p.name for p in self.cache_file.parent.iterdir()) == ['vmod.json', 'vmod.json.lock']

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_cache_update_is_locked(self, mock_run):
        mock_run.side_effect = self._run
        held = []
        with mock.patch.object(vmod.fcntl, 'flock', side_effect=lambda fd, op: held.append(op)):
            vmod.get_mfa_version(str(self.mfa), '0000:02:00.0')
        assert held == [vmod.fcntl.LOCK_EX | vmod.fcntl.LOCK_NB]

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_contended_lock_is_logged_then_waited_for(self, mock_run):
        mock_run.side_effect = self._run
        held = []

        def flock(fd, op):
            held.append(op)
            if op & vmod.fcntl.LOCK_NB:
                raise BlockingIOError()
        with mock.patch.object(vmod.fcntl, 'flock', side_effect=flock), \
                mock.patch.object(vmod, 'logger') as mock_logger:
            assert vmod.get_mfa_version(str(self.mfa), '0000:02:00.0') == "95_2020_0762-002"
        assert held == [vmod.fcntl.LOCK_EX | vmod.fcntl.LOCK_NB, vmod.fcntl.LOCK_EX]
        assert "Waiting for" in mock_logger.info.call_args[0][0]

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    def test_failed_query_reads_nothing(self, mock_run):
        mock_run.return_value = mock.Mock(returncode=1, stdout="FW Version: 1.2.3", stderr="err")
        assert vmod.get_hw_versions('0000:02:00.0') is None
        assert vmod.get_mfa_version('/etc/mlnx/fw-SPC6.mfa', '0000:02:00.0') is None

    @mock.patch('mellanox_fw_manager.vmod.run_command')
    @mock.patch('mellanox_fw_manager.vmod._asic_manager')
    def test_component_versions_helpers(self, mock_asic_manager, mock_run):
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ['0000:02:00.0']
        mock_asic_manager.return_value.get_asic_type.return_value = 'SPC6'
        mock_run.side_effect = self._run
        assert vmod.get_running_versions() == ["95.10.0752"]
        with mock.patch.object(vmod, 'FW_FILE_DIR', str(self.mfa.parent)):
            assert vmod.get_image_version() == "95.10.0762"


class TestSpectrumVmodCompare:

    @pytest.fixture(autouse=True)
    def reads(self):
        with mock.patch.object(vmod, 'get_hw_versions') as self.mock_hw, \
                mock.patch.object(vmod, 'get_mfa_version') as self.mock_mfa:
            yield

    def _manager(self, cpo=True, hw=("95.10.0752", "95.10.0752"), mfa="95.2020.0752"):
        self.mock_hw.return_value = vmod.HwVersions(*hw) if hw else None
        self.mock_mfa.return_value = mfa
        with mock.patch.object(vmod, 'is_cpo_platform', return_value=cpo), \
                mock.patch('mellanox_fw_manager.firmware_base.FirmwareManagerBase._initialize_asic'):
            manager = SpectrumFirmwareManager(asic_index=0, pci_id="0000:02:00.0", asic_type='SPC6')
        manager.fw_file = "/etc/mlnx/fw-SPC6.mfa"
        manager.current_version = manager.available_version = "39.2020.0752"
        return manager

    def test_up_to_date(self):
        assert self._manager().is_upgrade_required() is False
        self.mock_hw.assert_called_once_with("0000:02:00.0")

    def test_vmod_differs_requires_upgrade(self):
        assert self._manager(mfa="95.2020.0756").is_upgrade_required() is True

    def test_burned_image_waiting_for_reset_counts_as_present(self):
        assert self._manager(hw=("95.10.0752", "95.10.0756"), mfa="95.2020.0756").is_upgrade_required() is False

    def test_burn_waiting_for_reset_that_differs_requires_upgrade(self):
        assert self._manager(hw=("95.10.0756", "95.10.0752"), mfa="95.2020.0756").is_upgrade_required() is True

    def test_old_image_in_the_other_bank_requires_upgrade(self):
        # The OE was moved to 0768 and still has 0770 in the other bank.
        self.mock_hw.side_effect = lambda pci_id: vmod.parse_hw_versions(
            vmod_query(a='95.10.0768', b='95.10.0770', running='A'))
        assert self._manager(mfa="95_2020_0770-008").is_upgrade_required() is True

    def test_unreadable_does_not_force_upgrade(self):
        assert self._manager(hw=None).is_upgrade_required() is False
        assert self._manager(mfa=None).is_upgrade_required() is False

    def test_read_that_raises_does_not_force_upgrade(self):
        self.mock_mfa.side_effect = OSError("mlxfwmanager not found")
        manager = self._manager()
        assert manager.is_upgrade_required() is False
        assert manager.get_vmod_status().startswith("unknown")

    def test_asic_differs_skips_the_vmod_read(self):
        manager = self._manager()
        manager.available_version = "39.2020.0756"
        assert manager.is_upgrade_required() is True
        self.mock_hw.assert_not_called()
        self.mock_mfa.assert_not_called()

    def test_versions_are_read_once(self):
        manager = self._manager()
        manager.is_upgrade_required()
        manager.is_upgrade_required()
        self.mock_hw.assert_called_once()
        self.mock_mfa.assert_called_once_with("/etc/mlnx/fw-SPC6.mfa", "0000:02:00.0")

    def test_init_reads_nothing(self):
        self._manager()
        self.mock_hw.assert_not_called()
        self.mock_mfa.assert_not_called()

    def test_non_cpo_platform_reads_nothing(self):
        assert self._manager(cpo=False).is_upgrade_required() is False
        self.mock_hw.assert_not_called()

    @pytest.mark.parametrize("hw, mfa, expected", [
        (("95.10.0752", "95.10.0752"), "95.2020.0752", "up to date (95.10.0752)"),
        (("95.10.0750", "95.10.0752"), "95.2020.0752",
         "up to date (95.10.0752, active after the next ASIC reset)"),
        (("95.10.0750", "95.10.0750"), "95.2020.0752", "upgrade needed (current 95.10.0750, available 95.10.0752)"),
        (("95.10.0752", "95.10.0750"), "95.2020.0752",
         "upgrade needed (current 95.10.0752, 95.10.0750 after the next ASIC reset, available 95.10.0752)"),
        (None, "95.2020.0752", "unknown, unable to read the version (current N/A, available 95.10.0752)"),
        (("95.10.0752", "95.10.0752"), None,
         "unknown, unable to read the version (current 95.10.0752, available N/A)"),
        (("95.10.0752", "95.10.0752"), "garbage",
         "unknown, unable to read the version (current 95.10.0752, available N/A)"),
    ])
    def test_vmod_status(self, hw, mfa, expected):
        manager = self._manager(hw=hw, mfa=mfa)
        manager.is_upgrade_required()
        assert manager.get_vmod_status() == expected

    def test_vmod_status_not_checked_when_asic_differs(self):
        manager = self._manager()
        manager.available_version = "39.2020.0756"
        manager.is_upgrade_required()
        assert manager.get_vmod_status() == "not checked (ASIC firmware upgrade needed)"

    def test_vmod_status_not_checked_after_an_error(self):
        manager = self._manager()
        with mock.patch('mellanox_fw_manager.firmware_base.FirmwareManagerBase.is_upgrade_required',
                        side_effect=Exception("flint failed")):
            with pytest.raises(Exception):
                manager.is_upgrade_required()
        assert manager.get_vmod_status() == "not checked (ASIC firmware check failed)"

    def test_unparsable_mfa_version_does_not_force_upgrade(self):
        assert self._manager(mfa="garbage").is_upgrade_required() is False

    def test_vmod_status_non_cpo(self):
        manager = self._manager(cpo=False)
        manager.is_upgrade_required()
        assert manager.get_vmod_status() is None


class TestStatusReport:

    @pytest.mark.parametrize("vmod_status, line", [
        ("up to date (95.10.0752)", "  OE: up to date (95.10.0752)"),
        (None, None),
    ])
    def test_vmod_line(self, capsys, vmod_status, line):
        from mellanox_fw_manager import main
        manager = mock.Mock(asic_index=0)
        manager.is_upgrade_required.return_value = False
        manager.get_vmod_status.return_value = vmod_status
        with mock.patch.object(main, 'FirmwareCoordinator') as coordinator:
            coordinator.return_value.get_asic_count.return_value = 1
            coordinator.return_value.managers = [manager]
            assert main.handle_status('0', False, False) == 0
        lines = capsys.readouterr().out.splitlines()
        assert lines[1] == "ASIC 0: Up to date"
        assert lines[2] == (line or "Summary: 1 up to date, 0 need upgrade, 0 errors")

    def test_vmod_status_failure_keeps_the_exit_code(self, capsys):
        from mellanox_fw_manager import main
        manager = mock.Mock(asic_index=0)
        manager.is_upgrade_required.return_value = False
        manager.get_vmod_status.side_effect = RuntimeError("boom")
        with mock.patch.object(main, 'FirmwareCoordinator') as coordinator, \
                mock.patch.object(main, 'logger'):
            coordinator.return_value.get_asic_count.return_value = 1
            coordinator.return_value.managers = [manager]
            coordinator.return_value.check_upgrade_required.return_value = False
            assert main.handle_status('0', False, False) == 0
            assert main.handle_dry_run(False, False) == 0
        out = capsys.readouterr().out.splitlines()
        assert "  OE: unknown, status unavailable" in out
        assert "  ASIC 0 OE: unknown, status unavailable" in out

    @pytest.mark.parametrize("upgrade_required, first_line, rc", [
        (False, "Firmware is up to date.", 0),
        (True, "Firmware upgrade is required.", 10),
    ])
    def test_dry_run_vmod_line(self, capsys, upgrade_required, first_line, rc):
        from mellanox_fw_manager import main
        cpo, other = mock.Mock(asic_index=0), mock.Mock(asic_index=1)
        cpo.get_vmod_status.return_value = "up to date (95.10.0752)"
        other.get_vmod_status.return_value = None
        with mock.patch.object(main, 'FirmwareCoordinator') as coordinator:
            coordinator.return_value.check_upgrade_required.return_value = upgrade_required
            coordinator.return_value.managers = [cpo, other]
            assert main.handle_dry_run(False, False) == rc
        assert capsys.readouterr().out.splitlines() == [first_line, "  ASIC 0 OE: up to date (95.10.0752)"]
