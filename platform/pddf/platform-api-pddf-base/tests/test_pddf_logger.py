# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Test script for testing PddfLogger rate limiting.
This script tests whether logs are properly emitted and rate limited in case of errors
"""

import logging
from unittest.mock import patch, call

import pytest
from sonic_platform_pddf_base.pddf_logger import PddfLogger

@pytest.fixture
def mock_time():
    with patch("sonic_platform_pddf_base.pddf_logger.time.monotonic", return_value=0.0) as m:
        yield m

@pytest.fixture
def mock_syslogger_log():
    with patch("sonic_platform_pddf_base.pddf_logger.SysLogger.log") as mock_log:
        yield mock_log

@pytest.fixture
def pddf_logger():
    return PddfLogger("test")

class TestPddfLogger:

    _NUM_CALLS = 10

    def test_default_no_rate_limit(self, mock_syslogger_log, pddf_logger):
        """Test that when rate_limit is False(default) and no max_log_count is provided, all logs go through
        """
        for i in range(self._NUM_CALLS):
            pddf_logger.log_error(f"test_log_{i}")

        expected = [call(logging.ERROR, f"test_log_{i}", False) for i in range(self._NUM_CALLS)]
        assert mock_syslogger_log.call_args_list == expected

    def test_rate_limit_by_count(self, mock_syslogger_log):
        """Test that PddfLogger rate limits logs by count when max_log_count is provided
        """
        max_log_count = 5
        pddf_logger = PddfLogger("test")

        for i in range(self._NUM_CALLS):
            pddf_logger.log_warning(f"test_log_{i}", limit_key="key", max_log_count=max_log_count)

        expected = [call(logging.WARNING, f"test_log_{i}", False) for i in range(max_log_count)]
        assert mock_syslogger_log.call_args_list == expected

    def test_rate_limit_by_count_multiple_keys(self, mock_syslogger_log):
        """Test that PddfLogger rate limiting by count limits based on keys not messages
        """
        max_log_count_0 = 3
        max_log_count_1 = 2
        pddf_logger = PddfLogger("test")

        for i in range(self._NUM_CALLS):
            pddf_logger.log_notice(f"test_0_log_{i}", limit_key="key0", max_log_count=max_log_count_0)

        for i in range(self._NUM_CALLS):
            pddf_logger.log_notice(f"test_1_log_{i}", limit_key="key1", max_log_count=max_log_count_1)

        # No key logs are not rate limited, even if the same messages are printed
        for i in range(self._NUM_CALLS):
            pddf_logger.log_notice(f"test_0_log_{i}")

        expected = [call(logging.NOTICE, f"test_0_log_{i}", False) for i in range(max_log_count_0)] + \
                   [call(logging.NOTICE, f"test_1_log_{i}", False) for i in range(max_log_count_1)] + \
                   [call(logging.NOTICE, f"test_0_log_{i}", False) for i in range(self._NUM_CALLS)]
        assert mock_syslogger_log.call_args_list == expected

    def test_rate_limit_by_interval(self, mock_time, mock_syslogger_log):
        """Test that PddfLogger rate limits to one log per log_interval, and then permits an additional
        log log_interval seconds after the previous printed log
        """
        pddf_logger = PddfLogger("test", log_interval=self._NUM_CALLS)

        for i in range(self._NUM_CALLS):
            mock_time.return_value = i
            pddf_logger.log_info(f"test_log_{i}", rate_limit=True, limit_key="key")

        assert mock_syslogger_log.call_count == 1
        assert mock_syslogger_log.call_args_list[0] == call(logging.INFO, "test_log_0", False)

        for i in range(self._NUM_CALLS, 2 * self._NUM_CALLS):
            mock_time.return_value = i
            pddf_logger.log_info(f"test_log_{i}", rate_limit=True, limit_key="key")

        assert mock_syslogger_log.call_count == 2
        assert mock_syslogger_log.call_args_list[1] == call(logging.INFO, f"test_log_{self._NUM_CALLS}", False)

    def test_rate_limit_by_interval_multiple_keys(self, mock_time, mock_syslogger_log):
        """Test that PddfLogger rate limiting by interval limits based on limit_key
        """
        log_interval = 5
        pddf_logger = PddfLogger("test", log_interval=log_interval)

        for i in range(self._NUM_CALLS):
            mock_time.return_value = i
            pddf_logger.log_debug(f"test_0_log_{i}", rate_limit=True, limit_key="key0")

        for i in range(self._NUM_CALLS):
            mock_time.return_value = i
            pddf_logger.log_debug(f"test_1_log_{i}", rate_limit=True, limit_key="key1")

        for i in range(self._NUM_CALLS):
            mock_time.return_value = i
            pddf_logger.log_debug(f"test_0_log_{i}")

        expected = [call(logging.DEBUG, f"test_0_log_{i * log_interval}", False) for i in range(self._NUM_CALLS // log_interval)] + \
                   [call(logging.DEBUG, f"test_1_log_{i * log_interval}", False) for i in range(self._NUM_CALLS // log_interval)] + \
                   [call(logging.DEBUG, f"test_0_log_{i}", False) for i in range(self._NUM_CALLS)]
        assert mock_syslogger_log.call_args_list == expected

    def test_rate_limit_mixed(self, mock_time, mock_syslogger_log):
        """Test that PddfLogger rate limits by both count and interval with a priority for interval rate
        """
        pddf_logger = PddfLogger("test", log_interval=2)

        for i in range(self._NUM_CALLS):
            mock_time.return_value = i
            pddf_logger.log_debug(f"test_log_{i}", rate_limit=True, limit_key="key", max_log_count=3)

        expected = [
            call(logging.DEBUG, f"test_log_0", False),
            call(logging.DEBUG, f"test_log_2", False),
            call(logging.DEBUG, f"test_log_4", False),
        ]
        assert mock_syslogger_log.call_args_list == expected
