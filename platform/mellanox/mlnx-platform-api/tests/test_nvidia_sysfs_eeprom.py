#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
"""Layer A: the sx_core per-page EEPROM transport and the sysfs path hierarchy.

The path assertions below spell out the expected strings literally rather than
rebuilding them from the helpers, so that they pin the layout the sx_core
driver actually exposes instead of restating the implementation.
"""

import os
import sys
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform_base.sonic_xcvr.cpo.cpo_base import CpoHardwareInfo, OeId

from sonic_platform.cpo_device import NvidiaSysfsElsfp, NvidiaSysfsOe
from sonic_platform.nvidia_sysfs_eeprom import (
    CMIS_ARCH_PAGES,
    CMIS_BYTES_PER_BANK,
    SFP_UPPER_PAGE_OFFSET,
    SX_CORE_ASIC_ROOT,
    NvidiaSysfsEeprom,
    NvidiaSysfsEepromBanked,
    NvidiaSysfsEepromUnbanked,
    bank_pages_root,
    bank_root,
    module_attr_path,
    module_pages_root,
    module_root,
)
from sonic_platform.cpo import CpoPort
from sonic_platform.sfp import SFP, NvidiaSFPCommon


def _hw_info():
    return CpoHardwareInfo(oe_id=OeId.NVIDIA_SPC6_CPO, elsfp_id=None)


class _Coordinates:
    """Bare host carrying nothing but the sysfs coordinates."""

    def __init__(self, sdk_index=0):
        self._sdk_index = sdk_index

    def get_sdk_index(self):
        return self._sdk_index


class _UnbankedDevice(_Coordinates, NvidiaSysfsEepromUnbanked):
    pass


class _BankedDevice(_Coordinates, NvidiaSysfsEepromBanked):
    pass


class TestPathHierarchy:
    def test_asic_and_module_roots(self):
        assert SX_CORE_ASIC_ROOT == '/sys/module/sx_core/asic0'
        assert module_root(5) == '/sys/module/sx_core/asic0/module5'
        assert bank_root(5, 2) == '/sys/module/sx_core/asic0/module5/bank2'

    def test_module_attr_path_matches_the_old_inline_strings(self):
        assert module_attr_path(12, 'present') == '/sys/module/sx_core/asic0/module12/present'
        assert module_attr_path(12, 'statuserror') == '/sys/module/sx_core/asic0/module12/statuserror'
        assert module_attr_path(12, 'power_mode_policy') == \
            '/sys/module/sx_core/asic0/module12/power_mode_policy'

    def test_page_tree_renderers_are_byte_identical_to_the_old_templates(self):
        # '/sys/module/sx_core/asic0/module{}/' + 'eeprom/pages'
        assert module_pages_root(5) == '/sys/module/sx_core/asic0/module5/eeprom/pages'
        # '/sys/module/sx_core/asic0/module{}/' + 'bank{}/eeprom/pages'
        assert bank_pages_root(5, 0) == '/sys/module/sx_core/asic0/module5/bank0/eeprom/pages'
        assert bank_pages_root(5, 2) == '/sys/module/sx_core/asic0/module5/bank2/eeprom/pages'


class TestEepromPathPerFlavour:
    def test_unbanked_ignores_the_bank(self):
        device = _UnbankedDevice(sdk_index=5)
        expected = '/sys/module/sx_core/asic0/module5/eeprom/pages'
        assert device._get_eeprom_path(bank_id=0) == expected
        assert device._get_eeprom_path(bank_id=3) == expected

    def test_banked_renders_the_bank(self):
        device = _BankedDevice(sdk_index=5)
        assert device._get_eeprom_path(bank_id=0) == \
            '/sys/module/sx_core/asic0/module5/bank0/eeprom/pages'
        assert device._get_eeprom_path(bank_id=2) == \
            '/sys/module/sx_core/asic0/module5/bank2/eeprom/pages'

    def test_plain_sfp_renders_the_unbanked_path(self):
        sfp = SFP(5)
        assert sfp._get_eeprom_path(bank_id=0) == \
            '/sys/module/sx_core/asic0/module5/eeprom/pages'

    def test_cpo_oe_renders_the_banked_path(self):
        oe = NvidiaSysfsOe(_hw_info(), bank=2, sdk_index=5)
        assert oe._get_eeprom_path(bank_id=0) == \
            '/sys/module/sx_core/asic0/module5/bank0/eeprom/pages'
        assert oe._get_eeprom_path(bank_id=2) == \
            '/sys/module/sx_core/asic0/module5/bank2/eeprom/pages'

    def test_a_module_on_another_asic_still_renders_under_asic0(self):
        oe = NvidiaSysfsOe(_hw_info(), bank=2, sdk_index=5, asic_index=3)
        assert oe.get_asic_index() == 3
        assert oe._get_eeprom_path(bank_id=2) == \
            '/sys/module/sx_core/asic0/module5/bank2/eeprom/pages'
        assert oe.module_attr_path('present') == '/sys/module/sx_core/asic0/module5/present'


class TestPageTranslation:
    """Byte-identical to what SFP._get_page_and_page_offset used to return."""

    ROOT = '/sys/module/sx_core/asic0/module0'

    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    def test_absent_tree_yields_nothing(self):
        device = _UnbankedDevice()
        assert device._get_page_and_page_offset(0) == (None, None, None)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_lower_memory_and_page_00h(self):
        device = _UnbankedDevice()
        pages = f'{self.ROOT}/eeprom/pages'
        assert device._get_page_and_page_offset(0) == (0, f'{pages}/0/i2c-0x50/data', 0)
        assert device._get_page_and_page_offset(255) == (0, f'{pages}/0/i2c-0x50/data', 255)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_upper_pages(self):
        device = _UnbankedDevice()
        pages = f'{self.ROOT}/eeprom/pages'
        assert device._get_page_and_page_offset(256) == (1, f'{pages}/1/data', 0)
        assert device._get_page_and_page_offset(383) == (1, f'{pages}/1/data', 127)
        assert device._get_page_and_page_offset(384) == (2, f'{pages}/2/data', 0)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_page_ffh_reads_page_ffh(self):
        """A flat device has one bank, so no offset can fall outside it.

        Page FFh's upper half starts at exactly the bank stride, so reducing the
        offset modulo that stride sends it back to page 00h. Nothing fails: page
        00h holds identifier and status, so the caller gets plausible bytes from
        the wrong page.
        """
        device = _UnbankedDevice()
        pages = f'{self.ROOT}/eeprom/pages'
        last_page = 0xFF

        assert device._get_page_and_page_offset(SFP_UPPER_PAGE_OFFSET * (last_page + 1)) == \
            (last_page, f'{pages}/{last_page}/data', 0)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_a2h_alias_when_the_host_asks_for_it(self):
        device = _UnbankedDevice()
        pages = f'{self.ROOT}/eeprom/pages'
        with mock.patch.object(_UnbankedDevice, '_uses_a2h_alias', return_value=True):
            assert device._get_page_and_page_offset(511) == (-1, f'{pages}/0/i2c-0x51/data', 255)
            assert device._get_page_and_page_offset(512) == (1, f'{pages}/1/data', 0)


class TestPageZeroIsNeverBanked:
    """Lower memory and page 00h are not banked, so they always render bank 0.

    This is the one behaviour where the sysfs layout is not simply 'the bank
    implied by the offset', and it is easy to get wrong when the bank is
    derived once at the top of the translation.
    """

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_page_zero_of_a_higher_bank_still_reads_bank_zero(self):
        device = _BankedDevice(sdk_index=5)
        bank1_page0 = CMIS_BYTES_PER_BANK  # first byte that derives bank 1

        page_num, page, page_offset = device._get_page_and_page_offset(bank1_page0)

        assert page_num == 0
        assert page == '/sys/module/sx_core/asic0/module5/bank0/eeprom/pages/0/i2c-0x50/data'
        assert page_offset == 0

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_upper_pages_of_a_higher_bank_do_follow_the_bank(self):
        device = _BankedDevice(sdk_index=5)

        page_num, page, page_offset = device._get_page_and_page_offset(CMIS_BYTES_PER_BANK + 256)

        assert page_num == 1
        assert page == '/sys/module/sx_core/asic0/module5/bank1/eeprom/pages/1/data'
        assert page_offset == 0

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_the_bank_stride_matches_the_one_the_mem_maps_address_on(self):
        """The stride is CmisPage.linear_offset's, and is not ours to redefine.

        The mem maps place bank B at B * CMIS_ARCH_PAGES * SFP_UPPER_PAGE_OFFSET.
        Widening the stride - to account for page 00h being 256 bytes, say - shifts
        every banked page down by one while still looking plausible.
        """
        device = _BankedDevice(sdk_index=5)
        page, bank = 0x11, 1
        linear = (bank * CMIS_ARCH_PAGES + page) * SFP_UPPER_PAGE_OFFSET + 130

        page_num, path, page_offset = device._get_page_and_page_offset(linear)

        assert page_num == page
        assert path == f'/sys/module/sx_core/asic0/module5/bank{bank}/eeprom/pages/{page}/data'
        assert page_offset == 2

    @mock.patch('os.path.exists')
    def test_the_presence_check_also_looks_at_bank_zero(self, mock_exists):
        mock_exists.return_value = True
        device = _BankedDevice(sdk_index=5)

        device._get_page_and_page_offset(CMIS_BYTES_PER_BANK + 256)

        mock_exists.assert_called_once_with(
            '/sys/module/sx_core/asic0/module5/bank0/eeprom/pages')


class TestReadWrite:
    @mock.patch('sonic_platform.nvidia_sysfs_eeprom.NvidiaSysfsEeprom._get_page_and_page_offset')
    def test_read_returns_none_when_the_page_is_missing(self, mock_page):
        mock_page.return_value = (None, None, None)
        assert _UnbankedDevice().read_eeprom(0, 1) is None

    @mock.patch('sonic_platform.nvidia_sysfs_eeprom.NvidiaSysfsEeprom._get_page_and_page_offset')
    def test_read_spans_pages(self, mock_page):
        mock_page.return_value = (0, '/tmp/mock_page', 0)
        mo = mock.mock_open()
        with mock.patch('sonic_platform.nvidia_sysfs_eeprom.open', mo):
            handle = mo()
            handle.read.side_effect = [b'\x00' * 128, b'\x01' * 128, b'\x02' * 64]
            handle.seek.side_effect = [0, 128, 0, 128, 0]
            assert _UnbankedDevice().read_eeprom(0, 320) == \
                bytearray([0] * 128 + [1] * 128 + [2] * 64)

    @mock.patch('sonic_platform.nvidia_sysfs_eeprom.NvidiaSysfsEeprom._get_page_and_page_offset')
    def test_read_swallows_io_errors_and_honours_log_on_error(self, mock_page):
        mock_page.return_value = (0, '/tmp/mock_page', 0)
        mo = mock.mock_open()
        with mock.patch('sonic_platform.nvidia_sysfs_eeprom.open', mo), \
             mock.patch('sonic_platform.nvidia_sysfs_eeprom.logger') as mock_logger:
            mo().read.side_effect = OSError('')
            device = _UnbankedDevice()
            assert device._read_eeprom(0, 1, log_on_error=False) is None
            mock_logger.log_warning.assert_not_called()
            assert device._read_eeprom(0, 1) is None
            mock_logger.log_warning.assert_called_once()

    @mock.patch('sonic_platform.nvidia_sysfs_eeprom.NvidiaSysfsEeprom._get_page_and_page_offset')
    def test_write_rejects_a_length_mismatch(self, mock_page):
        assert not _UnbankedDevice().write_eeprom(0, 1, bytearray())
        mock_page.assert_not_called()

    @mock.patch('sonic_platform.nvidia_sysfs_eeprom.NvidiaSysfsEeprom._get_page_and_page_offset')
    def test_write_respects_the_write_protection_hook(self, mock_page):
        mock_page.return_value = (0, '/tmp/mock_page', 0)
        with mock.patch.object(_UnbankedDevice, '_is_write_protected', return_value=True):
            assert not _UnbankedDevice().write_eeprom(0, 1, bytearray([1]))

    @mock.patch('sonic_platform.nvidia_sysfs_eeprom.NvidiaSysfsEeprom._get_page_and_page_offset')
    def test_write_spans_pages(self, mock_page):
        mock_page.return_value = (0, '/tmp/mock_page', 0)
        mo = mock.mock_open()
        with mock.patch('sonic_platform.nvidia_sysfs_eeprom.open', mo):
            handle = mo()
            handle.write.side_effect = [128, 128, 64]
            handle.seek.side_effect = [0, 128, 0, 128, 0]
            buffer = bytearray([0] * 128 + [1] * 128 + [2] * 64)
            assert _UnbankedDevice().write_eeprom(0, 320, buffer)
            handle.write.assert_has_calls(
                [mock.call(buffer), mock.call(buffer[128:]), mock.call(buffer[256:])])


class TestAttachment:
    def test_pluggable_ports_carry_the_unbanked_transport(self):
        assert issubclass(NvidiaSFPCommon, NvidiaSysfsEepromUnbanked)

    def test_the_transport_precedes_optoe_in_the_mro(self):
        mro = [c.__name__ for c in NvidiaSFPCommon.__mro__]
        assert mro.index('NvidiaSysfsEeprom') < mro.index('OptoeEepromReadWriteMixin')

    def test_cpo_devices_carry_the_banked_transport(self):
        assert issubclass(NvidiaSysfsOe, NvidiaSysfsEepromBanked)
        assert issubclass(NvidiaSysfsElsfp, NvidiaSysfsEepromBanked)

    def test_cpo_devices_keep_their_community_api_factory(self):
        oe = NvidiaSysfsOe(_hw_info(), bank=2, sdk_index=5, asic_index=1)
        elsfp = NvidiaSysfsElsfp(_hw_info(), bank=2, sdk_index=5, asic_index=1)
        assert type(oe._api_factory).__name__ == 'OeApiFactory'
        assert type(elsfp._api_factory).__name__ == 'ElsfpApiFactory'
        assert oe.bank == 2
        assert oe.get_sdk_index() == 5
        assert oe.get_asic_index() == 1

    def test_a_cpo_port_reads_its_eeprom_through_its_optical_engine(self):
        cpo = CpoPort(0, 2, 5, 0)
        assert cpo.oe._get_eeprom_path(bank_id=0) == \
            '/sys/module/sx_core/asic0/module5/bank0/eeprom/pages'
        assert cpo.oe._get_eeprom_path(bank_id=2) == \
            '/sys/module/sx_core/asic0/module5/bank2/eeprom/pages'

        with mock.patch.object(cpo.oe, 'read_eeprom', return_value=b'\x11') as read:
            assert cpo.read_eeprom(0, 1) == b'\x11'
        read.assert_called_once_with(0, 1)

    def test_the_transport_stays_abstract_without_coordinates(self):
        class _Incomplete(NvidiaSysfsEeprom):
            pass

        try:
            _Incomplete()
        except TypeError:
            return
        raise AssertionError('NvidiaSysfsEeprom must not be instantiable without coordinates')
