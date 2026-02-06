#
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES.
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
from unittest import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.pwm import PWM, PWM_FILE_PATH, PWM_NAME, PWM_MIN_THRESHOLD, PWM_MAX_THRESHOLD


class TestPWM:
    def test_pwm_get_name(self):
        """Test get_name returns correct PWM name"""
        pwm = PWM()
        assert pwm.get_name() == PWM_NAME
        assert pwm.get_name() == "pwm"

    def test_pwm_get_presence_file_exists(self):
        """Test get_presence returns True when PWM file exists"""
        pwm = PWM()
        with mock.patch('os.path.exists', return_value=True):
            assert pwm.get_presence() is True

    def test_pwm_get_presence_file_not_exists(self):
        """Test get_presence returns False when PWM file does not exist"""
        pwm = PWM()
        with mock.patch('os.path.exists', return_value=False):
            assert pwm.get_presence() is False

    def test_pwm_get_model(self):
        """Test get_model returns N/A"""
        pwm = PWM()
        assert pwm.get_model() == "N/A"

    def test_pwm_get_serial(self):
        """Test get_serial returns N/A"""
        pwm = PWM()
        assert pwm.get_serial() == "N/A"

    def test_pwm_get_revision(self):
        """Test get_revision returns N/A"""
        pwm = PWM()
        assert pwm.get_revision() == "N/A"

    def test_pwm_get_status(self):
        """Test get_status returns presence status"""
        pwm = PWM()
        with mock.patch('os.path.exists', return_value=True):
            assert pwm.get_status() is True
        with mock.patch('os.path.exists', return_value=False):
            assert pwm.get_status() is False

    def test_pwm_get_position_in_parent(self):
        """Test get_position_in_parent returns 1"""
        pwm = PWM()
        assert pwm.get_position_in_parent() == 1

    def test_pwm_is_replaceable(self):
        """Test is_replaceable returns False"""
        pwm = PWM()
        assert pwm.is_replaceable() is False

    def test_pwm_get_pwm_value(self):
        """Test get_pwm_value reads value from file"""
        pwm = PWM()
        with mock.patch('sonic_platform.utils.read_int_from_file', return_value=128) as mock_read:
            result = pwm.get_pwm_value()
            assert result == 128
            mock_read.assert_called_once_with(PWM_FILE_PATH, default=0)

    def test_pwm_get_pwm_value_min(self):
        """Test get_pwm_value with minimum value"""
        pwm = PWM()
        with mock.patch('sonic_platform.utils.read_int_from_file', return_value=0):
            assert pwm.get_pwm_value() == 0

    def test_pwm_get_pwm_value_max(self):
        """Test get_pwm_value with maximum value"""
        pwm = PWM()
        with mock.patch('sonic_platform.utils.read_int_from_file', return_value=255):
            assert pwm.get_pwm_value() == 255

    def test_pwm_get_pwm_max_threshold(self):
        """Test get_pwm_max_threshold returns 255"""
        pwm = PWM()
        assert pwm.get_pwm_max_threshold() == PWM_MAX_THRESHOLD
        assert pwm.get_pwm_max_threshold() == 255

    def test_pwm_get_pwm_min_threshold(self):
        """Test get_pwm_min_threshold returns 0"""
        pwm = PWM()
        assert pwm.get_pwm_min_threshold() == PWM_MIN_THRESHOLD
        assert pwm.get_pwm_min_threshold() == 0

    def test_pwm_inherits_from_pwm_base(self):
        """Test PWM class inherits from PwmBase"""
        from sonic_platform_base.pwm_base import PwmBase
        pwm = PWM()
        assert isinstance(pwm, PwmBase)
