#
# Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES.
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
import pytest
import subprocess
import sys
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.chassis import Chassis
from sonic_platform.component import ComponentONIE,       \
                                     ComponentSSD,        \
                                     ComponentBIOS,       \
                                     ComponentBIOSSN2201, \
                                     ComponentCPLD,       \
                                     ComponentCPLDSN2201, \
                                     MPFAManager,         \
                                     ONIEUpdater,         \
                                     Component
from sonic_platform_base.component_base import FW_AUTO_INSTALLED,      \
                                               FW_AUTO_UPDATED,        \
                                               FW_AUTO_SCHEDULED,      \
                                               FW_AUTO_ERR_BOOT_TYPE,  \
                                               FW_AUTO_ERR_IMAGE,      \
                                               FW_AUTO_ERR_UNKNOWN


class TestComponent:
    @mock.patch('sonic_platform.chassis.utils.is_host')
    @mock.patch('sonic_platform.chassis.DeviceDataManager.get_cpld_component_list', mock.MagicMock(return_value=[]))
    def test_chassis_component(self, mock_is_host):
        mock_is_host.return_value = False
        c = Chassis()
        assert not c.get_all_components()
        mock_is_host.return_value = True
        component_list = c.get_all_components()
        assert len(component_list) > 0
        assert c.get_num_components() > 0
        assert c.get_component(0) is not None

    @mock.patch('sonic_platform.component.ComponentONIE._check_file_validity')
    @mock.patch('sonic_platform.component.ONIEUpdater', mock.MagicMock())
    def test_onie_component(self, mock_check_file):
        c = ComponentONIE()
        assert c.get_name() == 'ONIE'
        assert c.get_description() == 'ONIE - Open Network Install Environment'
        c.onie_updater.get_onie_version = mock.MagicMock(return_value='1.0')
        assert c.get_firmware_version() == '1.0'

        c.onie_updater.get_onie_firmware_info = mock.MagicMock(return_value={})
        with pytest.raises(RuntimeError):
            c.get_available_firmware_version('')

        c.onie_updater.get_onie_firmware_info = mock.MagicMock(return_value={'image_version': '1.1'})
        assert c.get_available_firmware_version('') == '1.1'

        assert c.get_firmware_update_notification('') == \
            'Immediate cold reboot is required to complete ONIE firmware update'

        mock_check_file.return_value = False
        assert not c.install_firmware('')
        c.update_firmware('')

        mock_check_file.return_value = True
        c.onie_updater.update_firmware = mock.MagicMock()
        assert c.install_firmware('')

        c.onie_updater.update_firmware.side_effect = RuntimeError('')
        assert not c.install_firmware('')

    @mock.patch('sonic_platform.component.os.path.exists')
    @mock.patch('sonic_platform.component.subprocess.check_call')
    @mock.patch('sonic_platform.component.subprocess.check_output')
    def test_ssd_component(self, mock_check_output, mock_check_call, mock_exists):
        c = ComponentSSD()
        firmware_info = [
            'Firmware Version:1.0',
            'Available Firmware Version:1.1',
            'Upgrade Required:yes',
            'Power Cycle Required:yes'
        ]
        mock_check_output.return_value = '\n'.join(firmware_info)
        assert c.get_firmware_version() == '1.0'
        assert c.get_available_firmware_version('') == '1.1'
        assert c.get_firmware_update_notification('') == \
            'Immediate power cycle is required to complete SSD firmware update'
        mock_check_output.return_value = ''
        with pytest.raises(RuntimeError):
            c.get_firmware_version()
        with pytest.raises(RuntimeError):
            c.get_available_firmware_version('')

        mock_check_output.return_value = 'Upgrade Required:invalid'
        with pytest.raises(RuntimeError):
            c.get_available_firmware_version('')
        with pytest.raises(RuntimeError):
            c.get_firmware_update_notification('')
        mock_check_output.return_value = 'Upgrade Required:no'
        with pytest.raises(RuntimeError):
            c.get_available_firmware_version('')
        assert c.get_firmware_update_notification('') is None
        mock_check_output.return_value = 'Upgrade Required:yes'
        with pytest.raises(RuntimeError):
            c.get_available_firmware_version('')
        firmware_info = [
            'Power Cycle Required:invalid',
            'Upgrade Required:yes'
        ]
        mock_check_output.return_value = '\n'.join(firmware_info)
        with pytest.raises(RuntimeError):
            c.get_firmware_update_notification('')
        firmware_info = [
            'Firmware Version:1.0',
            'Upgrade Required:yes'
        ]
        mock_check_output.side_effect = subprocess.CalledProcessError(1, None)
        with pytest.raises(RuntimeError):
            c.get_firmware_version()
        with pytest.raises(RuntimeError):
            c.get_available_firmware_version('')
        with pytest.raises(RuntimeError):
            c.get_firmware_update_notification('')

        # install firmware
        c._check_file_validity = mock.MagicMock(return_value=False)
        assert not c.install_firmware('')
        c.update_firmware('')

        c._check_file_validity = mock.MagicMock(return_value=True)
        assert c.install_firmware('')
        mock_check_call.assert_called_with(c.SSD_FIRMWARE_UPDATE_COMMAND, universal_newlines=True)
        assert c.install_firmware('', False)
        mock_check_call.assert_called_with(c.SSD_FIRMWARE_INSTALL_COMMAND, universal_newlines=True)
        mock_check_call.side_effect = subprocess.CalledProcessError(1, None)
        assert not c.install_firmware('')

        # auto update firmware
        mock_exists.return_value = False
        assert c.auto_update_firmware('', '') == FW_AUTO_ERR_IMAGE
        c.get_firmware_update_notification = mock.MagicMock(side_effect=RuntimeError(''))
        mock_exists.return_value = True
        assert c.auto_update_firmware('', '') == FW_AUTO_ERR_UNKNOWN
        c.update_firmware = mock.MagicMock()
        c.get_firmware_update_notification = mock.MagicMock(return_value=None)
        assert c.auto_update_firmware('', '') == FW_AUTO_UPDATED
        c.get_firmware_update_notification = mock.MagicMock(return_value='yes')
        assert c.auto_update_firmware('', '') == FW_AUTO_ERR_BOOT_TYPE
        assert c.auto_update_firmware('', 'cold') == FW_AUTO_SCHEDULED

    @mock.patch('sonic_platform.component.subprocess.check_output')
    def test_bios_component(self, mock_check_output):
        c = ComponentBIOS()
        mock_check_output.return_value = '1.0'
        assert c.get_firmware_version() == '1.0'
        mock_check_output.side_effect = subprocess.CalledProcessError(1, None)
        with pytest.raises(RuntimeError):
            c.get_firmware_version()
        with pytest.raises(RuntimeError):
            c.get_available_firmware_version('')
        assert c.get_firmware_update_notification('') == \
            'Immediate cold reboot is required to complete BIOS firmware update'
        c.onie_updater.is_non_onie_firmware_update_supported = mock.MagicMock(return_value=False)
        assert not c.install_firmware('')
        c.update_firmware('')
        c.onie_updater.is_non_onie_firmware_update_supported = mock.MagicMock(return_value=True)
        c._check_file_validity = mock.MagicMock(return_value=False)
        assert not c.install_firmware('')
        c._check_file_validity = mock.MagicMock(return_value=True)
        c.onie_updater.update_firmware = mock.MagicMock()
        assert c.install_firmware('')
        c.onie_updater.update_firmware = mock.MagicMock(side_effect=RuntimeError(''))
        assert not c.install_firmware('')

    @mock.patch('sonic_platform.component.subprocess.check_output')
    def test_bios_2201_component(self, mock_check_output):
        c = ComponentBIOSSN2201()
        mock_check_output.return_value = 'Version: 1.0'
        assert c.get_firmware_version() == '1.0'
        mock_check_output.return_value = ''
        assert c.get_firmware_version() == 'Unknown version'
        mock_check_output.side_effect = subprocess.CalledProcessError(1, None)
        with pytest.raises(RuntimeError):
            c.get_firmware_version()

    @mock.patch('sonic_platform.component.MPFAManager.cleanup', mock.MagicMock())
    @mock.patch('sonic_platform.component.MPFAManager.extract', mock.MagicMock())
    @mock.patch('sonic_platform.component.subprocess.check_call')
    @mock.patch('sonic_platform.component.MPFAManager.get_path')
    @mock.patch('sonic_platform.component.MPFAManager.get_metadata')
    @mock.patch('sonic_platform.component.os.path.exists')
    def test_cpld_component(self, mock_exists, mock_get_meta_data, mock_get_path, mock_check_call):
        c = ComponentCPLD(1)
        c._read_generic_file = mock.MagicMock(side_effect=[None, '1', None])
        assert c.get_firmware_version() == 'CPLD000000_REV0100'

        assert c.get_firmware_update_notification('a.txt') == \
            'Immediate power cycle is required to complete CPLD1 firmware update'
        assert c.get_firmware_update_notification('a.vme') == \
            'Power cycle (with 30 sec delay) or refresh image is required to complete CPLD1 firmware update'

        mock_meta_data = mock.MagicMock()
        mock_meta_data.has_option = mock.MagicMock(return_value=False)
        mock_get_meta_data.return_value = mock_meta_data
        with pytest.raises(RuntimeError):
            c.get_available_firmware_version('')
        mock_meta_data.has_option = mock.MagicMock(return_value=True)
        mock_meta_data.get = mock.MagicMock(return_value='1.1')
        assert c.get_available_firmware_version('') == '1.1'

        c._check_file_validity = mock.MagicMock(return_value=False)
        assert not c._install_firmware('')
        c._check_file_validity = mock.MagicMock(return_value=True)
        c._ComponentCPLD__get_mst_device = mock.MagicMock(return_value=None)
        assert not c._install_firmware('')
        c._ComponentCPLD__get_mst_device = mock.MagicMock(return_value='some dev')
        assert c._install_firmware('')
        mock_check_call.side_effect = subprocess.CalledProcessError(1, None)
        assert not c._install_firmware('')

        c._install_firmware = mock.MagicMock()
        mock_meta_data.has_option = mock.MagicMock(return_value=False)
        with pytest.raises(RuntimeError):
            c.install_firmware('a.mpfa')
        mock_get_path.return_value = '/tmp'
        mock_meta_data.has_option = mock.MagicMock(return_value=True)
        mock_meta_data.get = mock.MagicMock(return_value='some_firmware')
        c.install_firmware('a.mpfa')
        c._install_firmware.assert_called_with('/tmp/some_firmware')
        c._install_firmware.reset_mock()
        c.install_firmware('a.txt')
        c._install_firmware.assert_called_with('a.txt')

        mock_meta_data.has_option = mock.MagicMock(return_value=False)
        with pytest.raises(RuntimeError):
            c.update_firmware('a.mpfa')
        mock_meta_data.has_option = mock.MagicMock(side_effect=[True, False])
        with pytest.raises(RuntimeError):
            c.update_firmware('a.mpfa')
        mock_meta_data.has_option = mock.MagicMock(return_value=True)
        mock_meta_data.get = mock.MagicMock(side_effect=['burn', 'refresh'])
        c._install_firmware.reset_mock()
        c.update_firmware('a.mpfa')
        assert c._install_firmware.call_count == 2
        c._install_firmware.reset_mock()
        c._install_firmware.return_value = False
        mock_meta_data.get = mock.MagicMock(side_effect=['burn', 'refresh'])
        c.update_firmware('a.mpfa')
        assert c._install_firmware.call_count == 1

        mock_exists.return_value = False
        assert c.auto_update_firmware('', '') == FW_AUTO_ERR_IMAGE
        mock_exists.return_value = True
        assert c.auto_update_firmware('', '') == FW_AUTO_ERR_BOOT_TYPE
        c.install_firmware = mock.MagicMock(return_value=False)
        assert c.auto_update_firmware('', 'cold') == FW_AUTO_ERR_UNKNOWN
        c.install_firmware = mock.MagicMock(return_value=True)
        assert c.auto_update_firmware('', 'cold') == FW_AUTO_SCHEDULED

    @mock.patch('sonic_platform.component.ComponentCPLD._read_generic_file', mock.MagicMock(return_value='3'))
    def test_cpld_get_component_list(self):
        component_list = ComponentCPLD.get_component_list()
        assert len(component_list) == 3
        for index, item in enumerate(component_list):
            assert item.name == 'CPLD{}'.format(index + 1)

    def test_cpld_get_mst_device(self):
        ComponentCPLD.MST_DEVICE_PATH = '/tmp/mst'
        os.system('rm -rf /tmp/mst')
        c = ComponentCPLD(1)
        assert c._ComponentCPLD__get_mst_device() is None
        os.makedirs(ComponentCPLD.MST_DEVICE_PATH)
        assert c._ComponentCPLD__get_mst_device() is None
        with open('/tmp/mst/mt0_pci_cr0', 'w+') as f:
            f.write('dummy')
        assert c._ComponentCPLD__get_mst_device() == '/tmp/mst/mt0_pci_cr0'

    @mock.patch('sonic_platform.component.subprocess.check_call')
    def test_cpld_2201_component(self, mock_check_call):
        c = ComponentCPLDSN2201(1)
        assert c._install_firmware('')
        mock_check_call.side_effect = subprocess.CalledProcessError(1, None)
        assert not c._install_firmware('')

    @mock.patch('sonic_platform.component.MPFAManager.cleanup')
    @mock.patch('sonic_platform.component.MPFAManager.extract')
    def test_mpfa_manager_context(self, mock_extract, mock_cleanup):
        with MPFAManager('some_path') as mpfa:
            assert isinstance(mpfa, MPFAManager)
            mock_extract.assert_called_once()
            mock_cleanup.assert_not_called()
        mock_cleanup.assert_called_once()

    @mock.patch('sonic_platform.component.tempfile.mkdtemp', mock.MagicMock(return_value='/tmp/mpfa'))
    @mock.patch('sonic_platform.component.subprocess.check_call', mock.MagicMock())
    @mock.patch('sonic_platform.component.os.path.isfile')
    def test_mpfa_manager_extract_cleanup(self, mock_isfile):
        m = MPFAManager('some_path')
        mock_isfile.return_value = False
        with pytest.raises(RuntimeError):
            m.extract()
        mock_isfile.return_value = True
        with pytest.raises(RuntimeError):
            m.extract()
        m = MPFAManager('some_path.mpfa')
        mock_isfile.side_effect = [True, False]
        with pytest.raises(RuntimeError):
            m.extract()
        mock_isfile.side_effect = None
        os.makedirs('/tmp/mpfa', exist_ok=True)
        with open('/tmp/mpfa/metadata.ini', 'w+') as f:
            f.write('[section1]\n')
            f.write('a=b')
        m = MPFAManager('some_path.mpfa')
        m.extract()
        assert m.get_path() == '/tmp/mpfa'
        assert m.is_extracted()
        assert m.get_metadata() is not None
        # extract twice and verify no issue
        m.extract()
        m.cleanup()
        assert m.get_path() is None
        assert not m.is_extracted()
        assert m.get_metadata() is None

    def test_onie_updater_parse_onie_version(self):
        o = ONIEUpdater()
        onie_year, onie_month, onie_major, onie_minor, onie_release, onie_baudrate = \
            o.parse_onie_version('2022.08-5.3.0010-9600')
        assert onie_year == '2022'
        assert onie_month == '08'
        assert onie_major == '5'
        assert onie_minor == '3'
        assert onie_release == '0010'
        assert onie_baudrate == '9600'
        with pytest.raises(RuntimeError):
            o.parse_onie_version('invalid', is_base=True)
        with pytest.raises(RuntimeError):
            o.parse_onie_version('invalid', is_base=False)
        onie_year, onie_month, onie_major, onie_minor, onie_release, onie_baudrate = \
            o.parse_onie_version('2022.08-5.3.0010-9600', is_base=True)
        assert onie_year is None
        assert onie_month is None
        assert onie_major == '5'
        assert onie_minor == '3'
        assert onie_release == '0010'
        assert onie_baudrate is None

        assert o.get_onie_required_version() == o.ONIE_VERSION_REQUIRED

    @mock.patch('sonic_platform.component.ONIEUpdater.get_onie_version')
    @mock.patch('sonic_platform.component.device_info.get_platform')
    def test_onie_updater_is_non_onie_firmware_update_supported(self, mock_platform, mock_version):
        mock_platform.return_value = 'x86_64-nvidia_sn5600-r0'
        o = ONIEUpdater()
        mock_version.return_value = '2022.08-5.3.0010-9600'
        assert o.is_non_onie_firmware_update_supported()
        mock_version.return_value = '2022.08-5.1.0010-9600'
        assert not o.is_non_onie_firmware_update_supported()

        mock_platform.return_value = 'x86_64-nvidia_sn2201-r0'
        o = ONIEUpdater()
        assert o.is_non_onie_firmware_update_supported()

    def test_onie_updater_get_onie_version(self):
        o = ONIEUpdater()
        o._ONIEUpdater__mount_onie_fs = mock.MagicMock()
        o._ONIEUpdater__umount_onie_fs = mock.MagicMock()
        mock_os_open = mock.mock_open(read_data='')
        with mock.patch('sonic_platform.component.open', mock_os_open):
            with pytest.raises(RuntimeError):
                o.get_onie_version()
                o._ONIEUpdater__umount_onie_fs.assert_called_once()

        mock_os_open = mock.mock_open(read_data='onie_version')
        with mock.patch('sonic_platform.component.open', mock_os_open):
            with pytest.raises(RuntimeError):
                o.get_onie_version()

        mock_os_open = mock.mock_open(read_data='onie_version=2022.08-5.1.0010-9600')
        with mock.patch('sonic_platform.component.open', mock_os_open):
            assert o.get_onie_version() == '2022.08-5.1.0010-9600'

    @mock.patch('sonic_platform.component.subprocess.check_output')
    def test_onie_updater_get_onie_firmware_info(self, mock_check_output):
        o = ONIEUpdater()
        o._ONIEUpdater__mount_onie_fs = mock.MagicMock()
        o._ONIEUpdater__umount_onie_fs = mock.MagicMock()
        mock_check_output.return_value = 'a'
        with pytest.raises(RuntimeError):
            o.get_onie_firmware_info('')
        o._ONIEUpdater__umount_onie_fs.assert_called_once()
        mock_check_output.return_value = 'a=b'
        fi = o.get_onie_firmware_info('')
        assert fi == {'a':'b'}
        mock_check_output.side_effect = subprocess.CalledProcessError(1, None)
        with pytest.raises(RuntimeError):
            o.get_onie_firmware_info('')

    def test_onie_updater_update_firmware(self):
        o = ONIEUpdater()
        o._ONIEUpdater__stage_update = mock.MagicMock()
        o._ONIEUpdater__trigger_update = mock.MagicMock()
        o._ONIEUpdater__is_update_staged = mock.MagicMock()
        o._ONIEUpdater__unstage_update = mock.MagicMock()
        o.update_firmware('')
        o._ONIEUpdater__stage_update.assert_called_once()
        o._ONIEUpdater__trigger_update.assert_called_once()
        o._ONIEUpdater__is_update_staged.assert_not_called()
        o._ONIEUpdater__unstage_update.assert_not_called()
        o._ONIEUpdater__is_update_staged.return_value = False
        o._ONIEUpdater__stage_update.side_effect = RuntimeError('')
        with pytest.raises(RuntimeError):
            o.update_firmware('')
            o._ONIEUpdater__unstage_update.assert_not_called()
        o._ONIEUpdater__is_update_staged.return_value = True
        with pytest.raises(RuntimeError):
            o.update_firmware('')
            o._ONIEUpdater__unstage_update.assert_called_once()

    @mock.patch('sonic_platform.component.os.path.lexists')
    @mock.patch('sonic_platform.component.os.path.exists', mock.MagicMock(return_value=False))
    @mock.patch('sonic_platform.component.os.mkdir', mock.MagicMock())
    @mock.patch('sonic_platform.component.subprocess.check_call', mock.MagicMock())
    @mock.patch('sonic_platform.component.os.symlink', mock.MagicMock())
    @mock.patch('sonic_platform.component.check_output_pipe', mock.MagicMock())
    def test_onie_updater_mount_onie_fs(self, mock_lexists):
        o = ONIEUpdater()
        o._ONIEUpdater__umount_onie_fs = mock.MagicMock()
        mock_lexists.return_value = False
        mp = o._ONIEUpdater__mount_onie_fs()
        assert mp == '/mnt/onie-fs'
        o._ONIEUpdater__umount_onie_fs.assert_not_called()
        mock_lexists.return_value = True
        o._ONIEUpdater__mount_onie_fs()
        o._ONIEUpdater__umount_onie_fs.assert_called_once()

    @mock.patch('sonic_platform.component.os.rmdir')
    @mock.patch('sonic_platform.component.os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch('sonic_platform.component.subprocess.check_call')
    @mock.patch('sonic_platform.component.os.path.ismount', mock.MagicMock(return_value=True))
    @mock.patch('sonic_platform.component.os.unlink')
    @mock.patch('sonic_platform.component.os.path.islink', mock.MagicMock(return_value=True))
    def test_onie_updater_umount_onie_fs(self, mock_unlink, mock_check_call, mock_rmdir):
        o = ONIEUpdater()
        o._ONIEUpdater__umount_onie_fs()
        mock_unlink.assert_called_once()
        mock_check_call.assert_called_once()
        mock_rmdir.assert_called_once()

    @mock.patch('sonic_platform.component.subprocess.check_output')
    @mock.patch('sonic_platform.component.subprocess.check_call')
    @mock.patch('sonic_platform.component.copyfile', mock.MagicMock())
    def test_onie_updater_stage(self, mock_check_call, mock_check_output):
        o = ONIEUpdater()
        o._ONIEUpdater__stage_update('')
        mock_check_call.assert_called_once()

        mock_check_call.reset_mock()
        o._ONIEUpdater__unstage_update('a.rom')
        mock_check_call.assert_called_once()

        mock_check_call.reset_mock()
        o._ONIEUpdater__trigger_update(True)
        mock_check_call.assert_called_with(o.ONIE_FW_UPDATE_CMD_UPDATE, universal_newlines=True)
        mock_check_call.reset_mock()
        o._ONIEUpdater__trigger_update(False)
        mock_check_call.assert_called_with(o.ONIE_FW_UPDATE_CMD_INSTALL, universal_newlines=True)

        mock_check_output.return_value = 'invalid/'
        assert not o._ONIEUpdater__is_update_staged('')
        mock_check_output.return_value = '00-'
        assert o._ONIEUpdater__is_update_staged('')

        mock_check_call.side_effect = subprocess.CalledProcessError(1, None)
        with pytest.raises(RuntimeError):
            o._ONIEUpdater__stage_update('')
        with pytest.raises(RuntimeError):
            o._ONIEUpdater__unstage_update('')
        with pytest.raises(RuntimeError):
            o._ONIEUpdater__trigger_update(True)
        mock_check_output.side_effect = subprocess.CalledProcessError(1, None)
        with pytest.raises(RuntimeError):
            o._ONIEUpdater__is_update_staged('')

    def test_read_generic_file(self):
        with pytest.raises(RuntimeError):
            Component._read_generic_file('invalid', 1)
        content = Component._read_generic_file(os.path.abspath(__file__), 1)
        assert content == '#'

    def test_check_file_validity(self):
        c = ComponentONIE()
        assert not c._check_file_validity('invalid')
        assert c._check_file_validity(os.path.abspath(__file__))
        c.image_ext_name = '.py'
        assert c._check_file_validity(os.path.abspath(__file__))
        c.image_ext_name = '.txt'
        assert not c._check_file_validity(os.path.abspath(__file__))
