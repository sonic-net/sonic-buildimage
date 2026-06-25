#!/usr/bin/env python3

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the platform_utils module.
"""

import pytest
from unittest.mock import patch


@pytest.fixture(scope="function", autouse=True)
def platform_utils_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop_utils import platform_utils

    yield platform_utils


class TestWaitUntil:
    """Tests for the wait_until helper."""

    def test_returns_true_immediately_when_condition_is_true(self, platform_utils_module):
        # When
        result = platform_utils_module.wait_until(lambda: True, timeout_secs=10)
        # Then
        assert result is True

    def test_returns_false_on_timeout(self, platform_utils_module):
        # Given
        monotonic_values = iter([0.0, 100.0])
        with (
            patch("nexthop_utils.platform_utils.time.sleep"),
            patch("nexthop_utils.platform_utils.time.monotonic", side_effect=monotonic_values),
        ):
            # When
            result = platform_utils_module.wait_until(lambda: False, timeout_secs=5)
            # Then
            assert result is False

    def test_returns_true_when_condition_becomes_true(self, platform_utils_module):
        # Given
        call_count = 0

        def condition():
            nonlocal call_count
            call_count += 1
            return call_count >= 3

        monotonic_values = iter([0.0, 1.0, 2.0, 3.0])
        with (
            patch("nexthop_utils.platform_utils.time.sleep"),
            patch("nexthop_utils.platform_utils.time.monotonic", side_effect=monotonic_values),
        ):
            # When
            result = platform_utils_module.wait_until(condition, timeout_secs=10)
            # Then
            assert result is True
        assert call_count == 3

    def test_exponential_backoff(self, platform_utils_module):
        # Given
        sleep_durations = []

        def mock_sleep(duration):
            sleep_durations.append(duration)

        monotonic_values = iter([0.0, 1.0, 3.0, 7.0, 15.0, 100.0])
        with (
            patch("nexthop_utils.platform_utils.time.sleep", side_effect=mock_sleep),
            patch("nexthop_utils.platform_utils.time.monotonic", side_effect=monotonic_values),
        ):
            # When
            platform_utils_module.wait_until(
                lambda: False, timeout_secs=30, initial_interval_secs=1.0, backoff_factor=2.0
            )
        # Then
        assert sleep_durations == [1.0, 2.0, 4.0, 8.0]

    def test_max_interval_caps_backoff(self, platform_utils_module):
        # Given
        sleep_durations = []

        def mock_sleep(duration):
            sleep_durations.append(duration)

        monotonic_values = iter([0.0, 1.0, 3.0, 6.0, 100.0])
        with (
            patch("nexthop_utils.platform_utils.time.sleep", side_effect=mock_sleep),
            patch("nexthop_utils.platform_utils.time.monotonic", side_effect=monotonic_values),
        ):
            # When
            platform_utils_module.wait_until(
                lambda: False,
                timeout_secs=30,
                initial_interval_secs=1.0,
                backoff_factor=2.0,
                max_interval_secs=3.0,
            )
        # Then
        assert sleep_durations == [1.0, 2.0, 3.0]

    def test_fixed_interval_with_backoff_factor_one(self, platform_utils_module):
        # Given
        sleep_durations = []

        def mock_sleep(duration):
            sleep_durations.append(duration)

        monotonic_values = iter([0.0, 5.0, 10.0, 100.0])
        with (
            patch("nexthop_utils.platform_utils.time.sleep", side_effect=mock_sleep),
            patch("nexthop_utils.platform_utils.time.monotonic", side_effect=monotonic_values),
        ):
            # When
            platform_utils_module.wait_until(
                lambda: False, timeout_secs=30, initial_interval_secs=5.0, backoff_factor=1.0
            )
        # Then
        assert sleep_durations == [5.0, 5.0]
