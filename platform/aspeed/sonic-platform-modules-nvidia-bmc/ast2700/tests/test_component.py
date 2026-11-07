#
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

import subprocess
from contextlib import contextmanager
from unittest import mock
from unittest.mock import MagicMock, patch

import pytest

from sonic_platform import component
from sonic_platform.component import ComponentBMC, ComponentCPLD, ISPVME, VERSION_NA
from sonic_platform.mctp import MctpError


@contextmanager
def patch_mctp_address(return_value="1,8", side_effect=None, network=1):
    """
    Patch the MCTP lookups used by ``ComponentBMC._address()``.

    ``_address()`` resolves the IRoT network via ``get_interface_network()``
    before calling ``mctp_address()``. Both must be mocked whenever a test
    also patches ``subprocess.run``: that patch is on the shared ``subprocess``
    module, so an unmocked ``get_interface_network()`` would otherwise hit the
    same mock as ``pldm-fw`` (or real ``busctl``) and leak into the test.
    """
    addr_kwargs = (
        {"side_effect": side_effect}
        if side_effect is not None
        else {"return_value": return_value}
    )
    with mock.patch.object(
        component.mctp, "get_interface_network", return_value=network
    ) as get_net, mock.patch.object(
        component.mctp, "mctp_address", **addr_kwargs
    ) as addr:
        yield get_net, addr


# Representative `pldm-fw inventory` output (trimmed).
INVENTORY_OUT = """\
Device: 0x0000:0x0000
Firmware Parameters:
  Active version:  BMC-1.2.3
  Pending version: BMC-1.2.4
  Update caps: [0x0]: none
  Components:
    [0]
      Classification:  Firmware
      Active Version:  BMC-1.2.3
      Pending Version: BMC-1.2.4
"""

# Representative `pldm-fw pkg-info` output (trimmed). Note the device section
# also carries a `version:` line that must NOT be mistaken for the component's.
# Real AST2700 packages render Firmware classification as Value(13) with
# identifier 0x000a.
PKGINFO_OUT = """\
Package:
  Identifier:   1234
  Version:      pkg-9.9
  Applicable devices:
   0: 0x0000:0x0000
       version:    DEVICE-0.0
       options:    0x0
       components: 0
  Components:
   0:
       classification: Value(13)
       identifier:     0x000a
       version:        BMC-1.2.4
       comparison:     0x00000000
"""


def _completed(stdout="", returncode=0):
    return subprocess.CompletedProcess(["pldm-fw"], returncode, stdout=stdout, stderr="")


def _pldm_error(stderr, returncode=1):
    return subprocess.CalledProcessError(returncode, ["pldm-fw"], stderr=stderr)


PLDM_ERR_IDENTICAL = _pldm_error(
    "component version identical to installed image; use --force-update to override"
)
PLDM_ERR_DOWNGRADE = _pldm_error(
    "component version downgrade blocked; use --force-update to override"
)
PLDM_ERR_CORRUPT_PKG = _pldm_error("failed to parse firmware package: invalid header")
PLDM_ERR_VERIFY = _pldm_error("verify failed: checksum mismatch")
PLDM_ERR_TRANSFER = _pldm_error("Update failed: transfer timeout")


def _image(tmp_path, name="bmc.fwpkg"):
    path = tmp_path / name
    path.write_bytes(b"fake-fwpkg")
    return path


@pytest.fixture
def comp():
    return ComponentBMC()


@pytest.fixture
def image(tmp_path):
    return _image(tmp_path)


@pytest.fixture
def mctp_and_pldm_ok():
    """Patch MCTP address resolution and successful pldm-fw subprocess calls."""
    with patch_mctp_address(), \
         mock.patch.object(component.subprocess, "run", return_value=_completed()):
        yield


@pytest.fixture
def mctp_ok_pldm_fail():
    """Patch MCTP address resolution; pldm-fw failure supplied by the test."""
    with patch_mctp_address():
        yield


class TestStaticInfo:

    def test_name(self, comp):
        assert comp.get_name() == "BMC"

    def test_description(self, comp):
        assert "BMC" in comp.get_description()

    def test_update_notification_mentions_bmc_power_cycle(self, comp):
        note = comp.get_firmware_update_notification("/some/image.fwpkg")
        assert "power cycle" in note.lower()
        assert "reboot" in note.lower()  # clarifies it is NOT a system reboot

    def test_address_passes_irot_network_to_mctp_address(self, comp):
        with mock.patch.object(component.mctp, "get_interface_network", return_value=1) as get_net, \
             mock.patch.object(component.mctp, "mctp_address", return_value="1,8") as addr:
            assert comp._address() == "1,8"
        get_net.assert_called_once_with(component.mctp.IROT_INTERFACE)
        addr.assert_called_once_with(network=1)

    def test_address_uses_explicit_network(self):
        comp = ComponentBMC(network=9)
        with mock.patch.object(component.mctp, "get_interface_network") as get_net, \
             mock.patch.object(component.mctp, "mctp_address", return_value="9,8") as addr:
            assert comp._address() == "9,8"
        get_net.assert_not_called()
        addr.assert_called_once_with(network=9)


class TestGetFirmwareVersion:

    def test_parses_active_version(self, comp):
        with patch_mctp_address(), \
             mock.patch.object(component.subprocess, "run", return_value=_completed(INVENTORY_OUT)) as run:
            assert comp.get_firmware_version() == "BMC-1.2.3"
        cmd = run.call_args[0][0]
        assert cmd[:3] == ["pldm-fw", "inventory", "1,8"]

    def test_na_when_eid_discovery_fails(self, comp):
        with patch_mctp_address(side_effect=MctpError("no ep")):
            assert comp.get_firmware_version() == VERSION_NA

    def test_na_when_pldm_fw_fails(self, comp):
        err = subprocess.CalledProcessError(1, ["pldm-fw"], stderr="boom")
        with patch_mctp_address(), \
             mock.patch.object(component.subprocess, "run", side_effect=err):
            assert comp.get_firmware_version() == VERSION_NA

    def test_na_when_version_absent(self, comp):
        with patch_mctp_address(), \
             mock.patch.object(component.subprocess, "run", return_value=_completed("nothing here")):
            assert comp.get_firmware_version() == VERSION_NA

    def test_parse_active_version_reads_first_component_not_top_level(self):
        # fwutil should report Component [0]'s active version, not the
        # top-level device/package active version.
        text = """\
Firmware Parameters:
  Active version:  TOP-1.0
  Components:
    [0]
      Active version:  COMP-2.0
"""
        assert ComponentBMC._parse_active_version(text) == "COMP-2.0"

    def test_parse_active_version_strips_comparison_stamp_and_ignores_later_components(self):
        # Real inventory renders the component version with a trailing
        # comparison stamp in brackets; only Component [0]'s string is used.
        text = """\
Firmware Parameters:
  Active version:  00.01.0018
  Components:
    [0]
      Classification:  Value(13)
      Identifier:      0x000a
      Active Version:  88.0060.2212 [88006022]
      Pending Version:
    [1]
      Active Version:  0.0
"""
        assert ComponentBMC._parse_active_version(text) == "88.0060.2212"


class TestGetAvailableFirmwareVersion:

    def test_parses_component_version(self, comp, tmp_path):
        img = tmp_path / "bmc.fwpkg"
        img.write_bytes(b"x")
        with mock.patch.object(component.subprocess, "run", return_value=_completed(PKGINFO_OUT)) as run:
            assert comp.get_available_firmware_version(str(img)) == "BMC-1.2.4"
        cmd = run.call_args[0][0]
        assert cmd[:2] == ["pldm-fw", "pkg-info"]

    def test_parse_component_version_matches_bmc_entry(self):
        pkginfo = """\
Package:
  Components:
   0:
       classification: Other
       identifier:     0x0020
       version:        OTHER-1.0
   1:
       classification: Firmware
       identifier:     0x000a
       version:        BMC-9.9.9
"""
        assert ComponentBMC._parse_component_version(pkginfo) == "BMC-9.9.9"

    def test_parse_component_version_accepts_value13_classification(self):
        # Live pldm-fw pkg-info on AST2700 emits unresolved Value(13) rather
        # than the symbolic Firmware name.
        pkginfo = """\
Package:
  Components:
    0:
       classification: Value(13)
       identifier:     0x000a
       version:        88.0060.2218
       comparison:     0x003c08aa
"""
        assert ComponentBMC._parse_component_version(pkginfo) == "88.0060.2218"

    def test_no_matching_component_returns_na(self, comp, tmp_path):
        pkginfo = """\
Package:
  Components:
   0:
       classification: Other
       identifier:     0x0020
       version:        OTHER-1.0
"""
        img = _image(tmp_path)
        with mock.patch.object(component.subprocess, "run", return_value=_completed(pkginfo)):
            assert comp.get_available_firmware_version(str(img)) == VERSION_NA

    def test_wrong_identifier_returns_na(self, comp, tmp_path):
        pkginfo = """\
Package:
  Components:
   0:
       classification: Value(13)
       identifier:     0x0010
       version:        BMC-WRONG-ID
"""
        img = _image(tmp_path)
        with mock.patch.object(component.subprocess, "run", return_value=_completed(pkginfo)):
            assert comp.get_available_firmware_version(str(img)) == VERSION_NA

    def test_missing_image_returns_na(self, comp):
        assert comp.get_available_firmware_version("/no/such/file") == VERSION_NA

    def test_na_when_pkginfo_fails(self, comp, tmp_path):
        img = tmp_path / "bmc.fwpkg"
        img.write_bytes(b"x")
        with mock.patch.object(component.subprocess, "run", side_effect=subprocess.TimeoutExpired(["pldm-fw"], 30)):
            assert comp.get_available_firmware_version(str(img)) == VERSION_NA


class TestGoldenPath:

    def test_version_query_and_install(self, comp, image):
        with patch_mctp_address(), \
             mock.patch.object(
                 component.subprocess,
                 "run",
                 side_effect=[
                     _completed(INVENTORY_OUT),
                     _completed(PKGINFO_OUT),
                     _completed(),  # pre-update cancel
                     _completed(),  # update
                 ],
             ) as run:
            assert comp.get_firmware_version() == "BMC-1.2.3"
            assert comp.get_available_firmware_version(str(image)) == "BMC-1.2.4"
            assert comp.install_firmware(str(image)) is True

        commands = [call.args[0] for call in run.call_args_list]
        assert commands[0][:3] == ["pldm-fw", "inventory", "1,8"]
        assert commands[1][:3] == ["pldm-fw", "pkg-info", str(image)]
        # install_firmware issues a preventative cancel before the update.
        assert commands[2][:3] == ["pldm-fw", "cancel", "1,8"]
        assert commands[3][:2] == ["pldm-fw", "update"]
        assert commands[3][2] == "1,8"
        assert str(image) in commands[3]
        assert "--force-update" not in commands[3]

    def test_update_golden_path(self, comp, image, mctp_and_pldm_ok):
        assert comp.update_firmware(str(image)) is None


class TestPreventativeCancel:

    def test_cancel_precedes_update(self, comp, image):
        # A preventative 'pldm-fw cancel' clears any stuck FD state left by a
        # prior aborted update, and must run before the update itself.
        with patch_mctp_address(), \
             mock.patch.object(component.subprocess, "run", return_value=_completed()) as run:
            assert comp.install_firmware(str(image)) is True
        commands = [call.args[0] for call in run.call_args_list]
        assert commands[0][:3] == ["pldm-fw", "cancel", "1,8"]
        assert commands[1][:2] == ["pldm-fw", "update"]

    def test_cancel_failure_does_not_block_update(self, comp, image):
        # cancel is best-effort: a failing cancel must not prevent the update.
        def run_side_effect(cmd, *args, **kwargs):
            if cmd[:2] == ["pldm-fw", "cancel"]:
                raise subprocess.CalledProcessError(1, cmd, stderr="cancel boom")
            return _completed()

        with patch_mctp_address(), \
             mock.patch.object(component.subprocess, "run", side_effect=run_side_effect) as run:
            assert comp.install_firmware(str(image)) is True
        commands = [call.args[0] for call in run.call_args_list]
        assert commands[0][:2] == ["pldm-fw", "cancel"]
        assert commands[-1][:2] == ["pldm-fw", "update"]


class TestInstallFirmware:

    def test_success_returns_true(self, comp, tmp_path):
        img = tmp_path / "bmc.fwpkg"
        img.write_bytes(b"x")
        with patch_mctp_address(), \
             mock.patch.object(component.subprocess, "run", return_value=_completed()) as run:
            assert comp.install_firmware(str(img)) is True
        cmd = run.call_args[0][0]
        assert cmd[:2] == ["pldm-fw", "update"]
        assert cmd[2] == "1,8" and str(img) in cmd and "-y" in cmd

    def test_missing_image_returns_false(self, comp):
        assert comp.install_firmware("/no/such/file") is False

    def test_failure_returns_false(self, comp, tmp_path):
        img = tmp_path / "bmc.fwpkg"
        img.write_bytes(b"x")
        err = subprocess.CalledProcessError(1, ["pldm-fw"], stderr="device busy")
        with patch_mctp_address(), \
             mock.patch.object(component.subprocess, "run", side_effect=err):
            assert comp.install_firmware(str(img)) is False

    def test_eid_discovery_failure_returns_false(self, comp, tmp_path):
        img = tmp_path / "bmc.fwpkg"
        img.write_bytes(b"x")
        with patch_mctp_address(side_effect=MctpError("no ep")):
            assert comp.install_firmware(str(img)) is False

    def test_force_update_passes_flag(self, comp, tmp_path):
        img = tmp_path / "bmc.fwpkg"
        img.write_bytes(b"x")
        with patch_mctp_address(), \
             mock.patch.object(component.subprocess, "run", return_value=_completed()) as run:
            assert comp.install_firmware(str(img), force_update=True) is True
        cmd = run.call_args[0][0]
        assert cmd[:3] == ["pldm-fw", "update", "--force-update"]
        assert cmd[3] == "1,8" and str(img) in cmd and "-y" in cmd

    @pytest.mark.parametrize("pldm_error", [PLDM_ERR_IDENTICAL, PLDM_ERR_DOWNGRADE])
    def test_same_or_downgrade_version_fails_without_force(
        self, comp, image, mctp_ok_pldm_fail, pldm_error
    ):
        with mock.patch.object(component.subprocess, "run", side_effect=pldm_error):
            assert comp.install_firmware(str(image)) is False

    @pytest.mark.parametrize("pldm_error", [PLDM_ERR_IDENTICAL, PLDM_ERR_DOWNGRADE])
    def test_same_or_downgrade_version_succeeds_with_force(
        self, comp, image, mctp_ok_pldm_fail, pldm_error
    ):
        def run_side_effect(cmd, *args, **kwargs):
            if "--force-update" in cmd:
                return _completed()
            raise pldm_error

        with mock.patch.object(
            component.subprocess, "run", side_effect=run_side_effect
        ) as run:
            assert comp.install_firmware(str(image), force_update=True) is True
        cmd = run.call_args[0][0]
        assert cmd[:3] == ["pldm-fw", "update", "--force-update"]

    def test_corrupt_package_fails_install(self, comp, image, mctp_ok_pldm_fail):
        with mock.patch.object(component.subprocess, "run", side_effect=PLDM_ERR_CORRUPT_PKG):
            assert comp.install_firmware(str(image)) is False

    def test_pkginfo_corrupt_returns_na(self, comp, image):
        with mock.patch.object(component.subprocess, "run", side_effect=PLDM_ERR_CORRUPT_PKG):
            assert comp.get_available_firmware_version(str(image)) == VERSION_NA

    def test_pldm_verify_failure_returns_false(self, comp, image, mctp_ok_pldm_fail):
        with mock.patch.object(component.subprocess, "run", side_effect=PLDM_ERR_VERIFY):
            assert comp.install_firmware(str(image)) is False

    def test_pldm_transfer_timeout_returns_false(self, comp, image, mctp_ok_pldm_fail):
        with mock.patch.object(
            component.subprocess,
            "run",
            side_effect=subprocess.TimeoutExpired(["pldm-fw"], comp._UPDATE_TIMEOUT),
        ):
            assert comp.install_firmware(str(image)) is False

    def test_no_force_update_flag_by_default(self, comp, image, mctp_and_pldm_ok):
        with mock.patch.object(component.subprocess, "run", return_value=_completed()) as run:
            comp.install_firmware(str(image))
        cmd = run.call_args[0][0]
        assert "--force-update" not in cmd


class TestUpdateFirmware:

    def test_success_returns_none(self, comp, tmp_path):
        img = _image(tmp_path)
        with patch_mctp_address(), \
             mock.patch.object(component.subprocess, "run", return_value=_completed()):
            assert comp.update_firmware(str(img)) is None

    def test_missing_image_returns_false(self, comp):
        assert comp.update_firmware("/no/such/file") is False

    def test_failure_raises_runtime_error(self, comp, image, mctp_ok_pldm_fail):
        with mock.patch.object(component.subprocess, "run", side_effect=PLDM_ERR_VERIFY):
            with pytest.raises(RuntimeError, match="verify failed"):
                comp.update_firmware(str(image))

    def test_eid_discovery_failure_raises_runtime_error(self, comp, image):
        with patch_mctp_address(side_effect=MctpError("no ep")):
            with pytest.raises(RuntimeError, match="no ep"):
                comp.update_firmware(str(image))

    @pytest.mark.parametrize("pldm_error", [PLDM_ERR_IDENTICAL, PLDM_ERR_DOWNGRADE])
    def test_same_or_downgrade_version_raises_without_force(
        self, comp, image, mctp_ok_pldm_fail, pldm_error
    ):
        with mock.patch.object(component.subprocess, "run", side_effect=pldm_error):
            with pytest.raises(RuntimeError, match="--force-update"):
                comp.update_firmware(str(image))

    @pytest.mark.parametrize("pldm_error", [PLDM_ERR_IDENTICAL, PLDM_ERR_DOWNGRADE])
    def test_same_or_downgrade_version_succeeds_with_force(
        self, comp, image, mctp_ok_pldm_fail, pldm_error
    ):
        def run_side_effect(cmd, *args, **kwargs):
            if "--force-update" in cmd:
                return _completed()
            raise pldm_error

        with mock.patch.object(
            component.subprocess, "run", side_effect=run_side_effect
        ) as run:
            assert comp.update_firmware(str(image), force_update=True) is None
        cmd = run.call_args[0][0]
        assert cmd[:3] == ["pldm-fw", "update", "--force-update"]

    def test_corrupt_package_raises_runtime_error(self, comp, image, mctp_ok_pldm_fail):
        with mock.patch.object(component.subprocess, "run", side_effect=PLDM_ERR_CORRUPT_PKG):
            with pytest.raises(RuntimeError, match="invalid header"):
                comp.update_firmware(str(image))

    def test_pldm_transfer_timeout_raises_runtime_error(self, comp, image, mctp_ok_pldm_fail):
        with mock.patch.object(
            component.subprocess,
            "run",
            side_effect=subprocess.TimeoutExpired(["pldm-fw"], comp._UPDATE_TIMEOUT),
        ):
            with pytest.raises(RuntimeError):
                comp.update_firmware(str(image))

    def test_force_update_passes_flag(self, comp, tmp_path):
        img = _image(tmp_path)
        with patch_mctp_address(), \
             mock.patch.object(component.subprocess, "run", return_value=_completed()) as run:
            comp.update_firmware(str(img), force_update=True)
        cmd = run.call_args[0][0]
        assert cmd[:3] == ["pldm-fw", "update", "--force-update"]



class TestDeviceBaseAPIs:
    """Tests for DeviceBase methods added to satisfy platform API test suite."""

    def test_get_presence_always_true(self, comp):
        assert comp.get_presence() is True

    def test_get_position_in_parent_returns_minus_one(self, comp):
        # -1 is the standard value for firmware components that have no
        # physical slot position; matches all other SONiC platform impls.
        assert comp.get_position_in_parent() == -1

    def test_is_replaceable_false(self, comp):
        # AST2700 is soldered to the board.
        assert comp.is_replaceable() is False

    def test_get_status_true_when_hw_management_present(self, comp, tmp_path):
        with mock.patch("sonic_platform.component.HW_MANAGEMENT_ROOT", str(tmp_path)):
            assert comp.get_status() is True

    def test_get_status_false_when_hw_management_absent(self, comp, tmp_path):
        missing = str(tmp_path / "nonexistent")
        with mock.patch("sonic_platform.component.HW_MANAGEMENT_ROOT", missing):
            assert comp.get_status() is False

    def test_get_model_delegates_to_eeprom(self, comp):
        with mock.patch.object(comp._eeprom, "get_part_number", return_value="MSN1234") as m:
            assert comp.get_model() == "MSN1234"
        m.assert_called_once()

    def test_get_serial_delegates_to_eeprom(self, comp):
        with mock.patch.object(comp._eeprom, "get_serial_number", return_value="SN-ABCD") as m:
            assert comp.get_serial() == "SN-ABCD"
        m.assert_called_once()


# ---------------------------------------------------------------------------
# ComponentCPLD — CPLD firmware update via ispvme over JTAG
# ---------------------------------------------------------------------------

@pytest.fixture
def cpld(tmp_path):
    """Return a ComponentCPLD with a fake JTAG device and EepromBMC stub."""
    # Create a fake jtag device file so path existence checks pass.
    jtag = tmp_path / "jtag0"
    jtag.touch()
    with patch("sonic_platform.component.EepromBMC") as mock_eeprom_cls:
        eeprom = mock_eeprom_cls.return_value
        eeprom.get_part_number.return_value = "P3809-0001"
        eeprom.get_serial_number.return_value = "SN123456"
        comp = ComponentCPLD(jtag_device=str(jtag))
    return comp


class TestCPLDDeviceBaseAPIs:

    def test_get_name(self, cpld):
        assert cpld.get_name() == "CPLD1"

    def test_get_name_uses_index(self, tmp_path):
        comp = ComponentCPLD(3, jtag_device=str(tmp_path / "jtag0"), eeprom=MagicMock())
        assert comp.get_name() == "CPLD3"

    def test_get_description_matches_switch_host(self, cpld):
        assert cpld.get_description() == "CPLD - Complex Programmable Logic Device"

    def test_get_component_list_returns_cpld1_to_cpld4_sharing_eeprom(self):
        eeprom = MagicMock(name="EepromBMC")
        with patch("sonic_platform.component.EepromBMC") as mock_eeprom_cls:
            comps = ComponentCPLD.get_component_list(eeprom=eeprom)
        mock_eeprom_cls.assert_not_called()
        assert [c.get_name() for c in comps] == ["CPLD1", "CPLD2", "CPLD3", "CPLD4"]
        assert all(c._eeprom is eeprom for c in comps)

    def test_get_presence_always_true(self, cpld):
        assert cpld.get_presence() is True

    def test_get_position_in_parent_returns_minus_one(self, cpld):
        assert cpld.get_position_in_parent() == -1

    def test_is_replaceable_false(self, cpld):
        assert cpld.is_replaceable() is False

    def test_get_status_true_when_hw_management_present(self, cpld, tmp_path):
        hw_root = tmp_path / "hw-management"
        hw_root.mkdir()
        with patch("sonic_platform.component.HW_MANAGEMENT_ROOT", str(hw_root)):
            assert cpld.get_status() is True

    def test_get_status_false_when_hw_management_absent(self, cpld, tmp_path):
        with patch("sonic_platform.component.HW_MANAGEMENT_ROOT",
                   str(tmp_path / "nonexistent")):
            assert cpld.get_status() is False

    def test_get_model_delegates_to_eeprom(self, cpld):
        assert cpld.get_model() == "P3809-0001"

    def test_get_serial_delegates_to_eeprom(self, cpld):
        assert cpld.get_serial() == "SN123456"

    def test_uses_injected_eeprom_instead_of_constructing_one(self, tmp_path):
        """An injected eeprom (e.g. the chassis's own) must be reused rather
        than constructing a second EepromBMC, which would spawn a duplicate
        ipmi-fru subprocess call for the same FRU data."""
        jtag = tmp_path / "jtag0"
        jtag.touch()
        injected_eeprom = MagicMock(name="EepromBMC")
        with patch("sonic_platform.component.EepromBMC") as mock_eeprom_cls:
            comp = ComponentCPLD(jtag_device=str(jtag), eeprom=injected_eeprom)
        mock_eeprom_cls.assert_not_called()
        assert comp._eeprom is injected_eeprom


class TestCPLDFirmwareVersion:

    @staticmethod
    def _write_attrs(tmp_path, idx, **attrs):
        hw_root = tmp_path / "hw-management"
        (hw_root / "system").mkdir(parents=True, exist_ok=True)
        for name, value in attrs.items():
            (hw_root / "system" / f"cpld{idx}_{name}").write_text(f"{value}\n")
        return hw_root

    def test_get_firmware_version_matches_switch_host_format(self, cpld, tmp_path):
        hw_root = self._write_attrs(tmp_path, 1, pn="454", version="4", version_min="0")
        with patch("sonic_platform.utils.HW_MANAGEMENT_ROOT", str(hw_root)):
            assert cpld.get_firmware_version() == "CPLD000454_REV0400"

    def test_get_firmware_version_reads_own_index(self, tmp_path):
        hw_root = self._write_attrs(tmp_path, 1, pn="454", version="4", version_min="0")
        self._write_attrs(tmp_path, 2, pn="439", version="2", version_min="6")
        comp = ComponentCPLD(2, jtag_device=str(tmp_path / "jtag0"), eeprom=MagicMock())
        with patch("sonic_platform.utils.HW_MANAGEMENT_ROOT", str(hw_root)):
            assert comp.get_firmware_version() == "CPLD000439_REV0206"

    def test_get_firmware_version_defaults_missing_pn_and_minor_to_zero(self, cpld, tmp_path):
        hw_root = self._write_attrs(tmp_path, 1, version="12")
        with patch("sonic_platform.utils.HW_MANAGEMENT_ROOT", str(hw_root)):
            assert cpld.get_firmware_version() == "CPLD000000_REV1200"

    def test_get_firmware_version_returns_na_when_major_missing(self, cpld, tmp_path):
        hw_root = self._write_attrs(tmp_path, 1, pn="454", version_min="0")
        with patch("sonic_platform.utils.HW_MANAGEMENT_ROOT", str(hw_root)):
            assert cpld.get_firmware_version() == VERSION_NA

    def test_get_available_firmware_version_returns_na(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00" * 64)
        assert cpld.get_available_firmware_version(str(vme)) == VERSION_NA

    def test_update_notification_mentions_full_system_power_cycle(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00")
        note = cpld.get_firmware_update_notification(str(vme))
        assert "full system power cycle" in note.lower()

    def test_update_notification_states_chain_wide_programming(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00")
        note = cpld.get_firmware_update_notification(str(vme))
        assert "every cpld it targets" in note.lower()


class TestCheckFileValidity:

    def test_valid_vme_file(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00")
        assert cpld._check_file_validity(str(vme)) is True

    def test_missing_file_is_invalid(self, cpld, tmp_path):
        assert cpld._check_file_validity(str(tmp_path / "missing.vme")) is False

    def test_wrong_extension_is_invalid(self, cpld, tmp_path):
        bad = tmp_path / "cpld.bin"
        bad.write_bytes(b"\x00")
        assert cpld._check_file_validity(str(bad)) is False

    def test_no_extension_is_invalid(self, cpld, tmp_path):
        bad = tmp_path / "cpld"
        bad.write_bytes(b"\x00")
        assert cpld._check_file_validity(str(bad)) is False

    def test_empty_path_is_invalid(self, cpld):
        assert cpld._check_file_validity("") is False

    def test_directory_is_invalid(self, cpld, tmp_path):
        bad = tmp_path / "cpld.vme"
        bad.mkdir()
        assert cpld._check_file_validity(str(bad)) is False


class TestCPLDInstallFirmware:

    def test_install_firmware_success(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00" * 32)
        with patch.object(cpld, "_run_ispvme") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            assert cpld.install_firmware(str(vme)) is True
        mock_run.assert_called_once_with(str(vme))

    def test_install_firmware_success_logs_notice_with_component_name(self, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00" * 32)
        comp = ComponentCPLD(2, jtag_device=str(vme), eeprom=MagicMock())
        with patch.object(comp, "_run_ispvme"), \
                patch("sonic_platform.component.logger") as mock_logger:
            assert comp.install_firmware(str(vme)) is True
        mock_logger.log_notice.assert_called_once_with("CPLD2 firmware install completed")

    def test_install_firmware_missing_image_returns_false(self, cpld, tmp_path):
        assert cpld.install_firmware(str(tmp_path / "missing.vme")) is False

    def test_install_firmware_wrong_extension_returns_false(self, cpld, tmp_path):
        bad = tmp_path / "cpld.bin"
        bad.write_bytes(b"\x00")
        with patch.object(cpld, "_run_ispvme") as mock_run:
            assert cpld.install_firmware(str(bad)) is False
        mock_run.assert_not_called()

    def test_install_firmware_missing_jtag_device_returns_false(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00")
        # Point to a JTAG device that doesn't exist.
        cpld._jtag_device = str(tmp_path / "no_jtag")
        assert cpld.install_firmware(str(vme)) is False

    def test_install_firmware_ispvme_failure_returns_false(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00")
        err = subprocess.CalledProcessError(1, ISPVME, stderr="| FAIL! |\n")
        with patch.object(cpld, "_run_ispvme", side_effect=err):
            assert cpld.install_firmware(str(vme)) is False

    def test_install_firmware_timeout_returns_false(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00")
        with patch.object(cpld, "_run_ispvme", side_effect=subprocess.TimeoutExpired(ISPVME, 300)):
            assert cpld.install_firmware(str(vme)) is False


class TestCPLDUpdateFirmware:

    def test_update_firmware_success_returns_none(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00" * 32)
        with patch.object(cpld, "_run_ispvme") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            result = cpld.update_firmware(str(vme))
        assert result is None

    def test_update_firmware_success_logs_notice_with_component_name(self, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00" * 32)
        comp = ComponentCPLD(3, jtag_device=str(vme), eeprom=MagicMock())
        with patch.object(comp, "_run_ispvme"), \
                patch("sonic_platform.component.logger") as mock_logger:
            assert comp.update_firmware(str(vme)) is None
        mock_logger.log_notice.assert_called_once_with("CPLD3 firmware update completed")

    def test_update_firmware_missing_image_returns_false(self, cpld, tmp_path):
        assert cpld.update_firmware(str(tmp_path / "missing.vme")) is False

    def test_update_firmware_wrong_extension_returns_false(self, cpld, tmp_path):
        bad = tmp_path / "cpld.bin"
        bad.write_bytes(b"\x00")
        with patch.object(cpld, "_run_ispvme") as mock_run:
            assert cpld.update_firmware(str(bad)) is False
        mock_run.assert_not_called()

    def test_update_firmware_ispvme_failure_raises_runtime_error(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00")
        err = subprocess.CalledProcessError(1, ISPVME, stderr="| FAIL! |\n")
        with patch.object(cpld, "_run_ispvme", side_effect=err):
            with pytest.raises(RuntimeError, match="CPLD firmware update failed"):
                cpld.update_firmware(str(vme))

    def test_update_firmware_missing_jtag_device_returns_false(self, cpld, tmp_path):
        vme = tmp_path / "cpld.vme"
        vme.write_bytes(b"\x00")
        cpld._jtag_device = str(tmp_path / "no_jtag")
        assert cpld.update_firmware(str(vme)) is False


class TestCPLDRunIspvme:

    @pytest.fixture(autouse=True)
    def burn_en_file(self, tmp_path):
        """Provide a writable GP_JTAG_BMC_CPLD_BURN_EN_L sysfs file for these tests."""
        hw_root = tmp_path / "hw-management"
        (hw_root / "system").mkdir(parents=True)
        burn_en = hw_root / "system" / "GP_JTAG_BMC_CPLD_BURN_EN_L"
        burn_en.write_text("1\n")
        with patch("sonic_platform.utils.HW_MANAGEMENT_ROOT", str(hw_root)):
            yield burn_en

    def test_run_ispvme_uses_jtag_device_and_vme_path(self, cpld, tmp_path):
        vme = tmp_path / "test.vme"
        vme.write_bytes(b"\x00")
        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            cpld._run_ispvme(str(vme))
        cmd = mock_run.call_args[0][0]
        assert cmd[0] == ISPVME
        assert "-d" in cmd
        assert cpld._jtag_device in cmd
        assert str(vme) in cmd

    def test_run_ispvme_passes_check_true(self, cpld, tmp_path):
        vme = tmp_path / "test.vme"
        vme.write_bytes(b"\x00")
        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            cpld._run_ispvme(str(vme))
        assert mock_run.call_args[1].get("check") is True

    def test_run_ispvme_captures_stdout_merged_with_stderr(self, cpld, tmp_path):
        """ispvme's diagnostics (including its pass/fail banner) go to
        stdout, not stderr; both streams must be captured so a failure
        carries ispvme's actual diagnostic text (see _format_error)."""
        vme = tmp_path / "test.vme"
        vme.write_bytes(b"\x00")
        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            cpld._run_ispvme(str(vme))
        assert mock_run.call_args[1].get("stdout") == subprocess.PIPE
        assert mock_run.call_args[1].get("stderr") == subprocess.STDOUT

    def test_run_ispvme_asserts_burn_enable_before_programming(self, cpld, tmp_path, burn_en_file):
        vme = tmp_path / "test.vme"
        vme.write_bytes(b"\x00")
        seen = {}

        def fake_run(*args, **kwargs):
            seen["burn_en_during_run"] = burn_en_file.read_text().strip()
            return MagicMock(returncode=0)

        with patch("subprocess.run", side_effect=fake_run):
            cpld._run_ispvme(str(vme))
        assert seen["burn_en_during_run"] == "0"

    def test_run_ispvme_restores_burn_enable_on_success(self, cpld, tmp_path, burn_en_file):
        vme = tmp_path / "test.vme"
        vme.write_bytes(b"\x00")
        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            cpld._run_ispvme(str(vme))
        assert burn_en_file.read_text().strip() == "1"

    def test_run_ispvme_restores_burn_enable_on_ispvme_failure(self, cpld, tmp_path, burn_en_file):
        vme = tmp_path / "test.vme"
        vme.write_bytes(b"\x00")
        err = subprocess.CalledProcessError(1, ISPVME, stderr="| FAIL! |\n")
        with patch("subprocess.run", side_effect=err):
            with pytest.raises(subprocess.CalledProcessError):
                cpld._run_ispvme(str(vme))
        assert burn_en_file.read_text().strip() == "1"

    def test_run_ispvme_restores_burn_enable_on_keyboard_interrupt(self, cpld, tmp_path, burn_en_file):
        """Interrupting a burn (e.g. operator Ctrl+C) must still restore the
        gate -- KeyboardInterrupt isn't a SubprocessError/OSError, so this
        needs a bare `finally`, not a narrow `except`."""
        vme = tmp_path / "test.vme"
        vme.write_bytes(b"\x00")
        with patch("subprocess.run", side_effect=KeyboardInterrupt):
            with pytest.raises(KeyboardInterrupt):
                cpld._run_ispvme(str(vme))
        assert burn_en_file.read_text().strip() == "1"

    def test_run_ispvme_raises_oserror_when_burn_enable_sysfs_missing(self, cpld, tmp_path):
        vme = tmp_path / "test.vme"
        vme.write_bytes(b"\x00")
        with patch("sonic_platform.utils.HW_MANAGEMENT_ROOT", str(tmp_path / "no-hw-mgmt")):
            with patch("subprocess.run") as mock_run:
                with pytest.raises(OSError):
                    cpld._run_ispvme(str(vme))
        mock_run.assert_not_called()

    @staticmethod
    def _flaky_open_failing_nth_call(n):
        """Return an `open` wrapper that raises OSError on the nth call
        (1-indexed) and otherwise behaves like the real `open`."""
        real_open = open
        call_count = {"n": 0}

        def flaky_open(path, mode="r", *args, **kwargs):
            call_count["n"] += 1
            if call_count["n"] == n:
                raise OSError("simulated write failure")
            return real_open(path, mode, *args, **kwargs)

        return flaky_open

    def test_run_ispvme_raises_when_deassert_fails_after_success(self, cpld, tmp_path, burn_en_file):
        """ispvme succeeding must not be reported as success if the board is
        left with JTAG burn-enable asserted afterward."""
        vme = tmp_path / "test.vme"
        vme.write_bytes(b"\x00")
        with patch("subprocess.run") as mock_run, \
             patch("builtins.open", side_effect=self._flaky_open_failing_nth_call(2)):
            mock_run.return_value = MagicMock(returncode=0)
            with pytest.raises(OSError, match="deassert"):
                cpld._run_ispvme(str(vme))

    def test_run_ispvme_preserves_original_error_when_deassert_also_fails(self, cpld, tmp_path, burn_en_file):
        """If both ispvme and the deassert write fail, the ispvme error is
        the more actionable diagnostic and must be what's raised."""
        vme = tmp_path / "test.vme"
        vme.write_bytes(b"\x00")
        err = subprocess.CalledProcessError(1, ISPVME, stderr="| FAIL! |\n")
        with patch("subprocess.run", side_effect=err), \
             patch("builtins.open", side_effect=self._flaky_open_failing_nth_call(2)):
            with pytest.raises(subprocess.CalledProcessError):
                cpld._run_ispvme(str(vme))


class TestCPLDFormatError:

    def test_format_error_uses_stdout(self):
        """ispvme writes its pass/fail banner and diagnostics via printf to
        stdout, not stderr (see _run_ispvme); stdout is the primary source."""
        exc = subprocess.CalledProcessError(1, ISPVME)
        exc.stdout = "| FAIL! |\n"
        exc.stderr = ""
        result = ComponentCPLD._format_error(exc)
        assert "FAIL" in result

    def test_format_error_no_output(self):
        exc = subprocess.CalledProcessError(1, ISPVME)
        exc.stdout = ""
        exc.stderr = ""
        result = ComponentCPLD._format_error(exc)
        assert result  # at minimum str(exc)

    def test_format_error_falls_back_to_stderr(self):
        """stderr is a defensive fallback when stdout is empty."""
        exc = subprocess.CalledProcessError(1, ISPVME)
        exc.stdout = ""
        exc.stderr = "OBMC JTAG handle not found"
        result = ComponentCPLD._format_error(exc)
        assert "JTAG" in result

    def test_format_error_prefers_stdout_over_stderr(self):
        exc = subprocess.CalledProcessError(1, ISPVME)
        exc.stdout = "| FAIL! |\n"
        exc.stderr = "should not appear"
        result = ComponentCPLD._format_error(exc)
        assert "FAIL" in result
        assert "should not appear" not in result

    def test_format_error_handles_none_stderr(self):
        """subprocess.TimeoutExpired leaves .stderr as None when no output was
        captured before the timeout; _format_error must not crash on it."""
        exc = subprocess.TimeoutExpired(ISPVME, 300)
        assert exc.stderr is None
        result = ComponentCPLD._format_error(exc)
        assert "timed out" in result

    def test_format_error_strips_embedded_nul_bytes(self):
        """ispvme's stdout can embed raw VME file bytes (including NUL) via
        its printf-based output; syslog.syslog() raises ValueError on an
        embedded NUL, so _format_error must not let one through."""
        exc = subprocess.CalledProcessError(1, ISPVME)
        exc.stdout = "| FAIL! |\x00garbage\n"
        exc.stderr = ""
        result = ComponentCPLD._format_error(exc)
        assert "\x00" not in result
        assert "FAIL" in result
