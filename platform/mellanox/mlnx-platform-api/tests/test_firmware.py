#
# Copyright (c) 2021-2022 NVIDIA CORPORATION & AFFILIATES.
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
import pytest
from mock import MagicMock
from .mock_platform import MockFan

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.component import Component, ComponentSSD, ComponentCPLD

from sonic_platform_base.component_base import ComponentBase,           \
                                                FW_AUTO_INSTALLED,      \
                                                FW_AUTO_SCHEDULED,      \
                                                FW_AUTO_UPDATED,        \
                                                FW_AUTO_ERR_BOOT_TYPE,  \
                                                FW_AUTO_ERR_IMAGE,      \
                                                FW_AUTO_ERR_UNKNOWN


def mock_update_firmware_success(image_path, allow_reboot=False):
    return True

def mock_update_firmware_fail(image_path, allow_reboot=False):
    return False

def mock_update_notification_cold_boot(image_path):
    return "Immediate power cycle is required to complete NAME firmware update"

def mock_update_notification_warm_boot(image_path):
    return None

def mock_update_notification_error(image_path):
    raise RuntimeError("Failed to parse NAME firmware upgrade status")

test_data_default = [
        (None, False, None, FW_AUTO_ERR_IMAGE),
        (None, True, 'warm', FW_AUTO_ERR_BOOT_TYPE),
        (mock_update_firmware_fail, True, 'cold', FW_AUTO_ERR_UNKNOWN),
        (mock_update_firmware_success, True, 'cold', FW_AUTO_INSTALLED)
        ]

test_data_cpld = [
        (None, False, None, FW_AUTO_ERR_IMAGE),
        (None, True, 'warm', FW_AUTO_ERR_BOOT_TYPE),
        (mock_update_firmware_fail, True, 'cold', FW_AUTO_ERR_UNKNOWN),
        (mock_update_firmware_success, True, 'cold', FW_AUTO_SCHEDULED)
        ]

test_data_ssd = [
        (None, None, False, None, FW_AUTO_ERR_IMAGE),
        (None, mock_update_notification_error, True, None, FW_AUTO_ERR_UNKNOWN),
        (mock_update_firmware_success, mock_update_notification_cold_boot, True, 'warm', FW_AUTO_ERR_BOOT_TYPE),
        (mock_update_firmware_success, mock_update_notification_cold_boot, True, 'cold', FW_AUTO_SCHEDULED),
        (mock_update_firmware_success, mock_update_notification_warm_boot, True, 'warm', FW_AUTO_UPDATED),
        (mock_update_firmware_success, mock_update_notification_warm_boot, True, 'cold', FW_AUTO_UPDATED)
        ]

@pytest.mark.parametrize('update_func, image_found, boot_type, expect', test_data_default)
def test_auto_update_firmware_default(monkeypatch, update_func, image_found, boot_type, expect):

    def mock_path_exists(path):
        return image_found

    test_component = Component()

    monkeypatch.setattr(test_component, 'install_firmware', update_func)
    monkeypatch.setattr(os.path, 'exists', mock_path_exists)

    result = test_component.auto_update_firmware(None, boot_type)

    assert result == expect


@pytest.mark.parametrize('update_func, image_found, boot_type, expect', test_data_cpld)
def test_auto_update_firmware_cpld(monkeypatch, update_func, image_found, boot_type, expect):

    def mock_path_exists(path):
        return image_found

    test_component = ComponentCPLD(0)

    monkeypatch.setattr(test_component, 'install_firmware', update_func)
    monkeypatch.setattr(os.path, 'exists', mock_path_exists)

    result = test_component.auto_update_firmware(None, boot_type)

    assert result == expect


@pytest.mark.parametrize('update_func, notify, image_found, boot_type, expect', test_data_ssd)
def test_auto_update_firmware_ssd(monkeypatch, update_func, notify, image_found, boot_type, expect):

    def mock_path_exists(path):
        return image_found

    test_component_ssd = ComponentSSD()

    monkeypatch.setattr(test_component_ssd, 'update_firmware', update_func)
    monkeypatch.setattr(test_component_ssd, 'get_firmware_update_notification', notify)
    monkeypatch.setattr(os.path, 'exists', mock_path_exists)

    result = test_component_ssd.auto_update_firmware(None, boot_type)

    assert result == expect

