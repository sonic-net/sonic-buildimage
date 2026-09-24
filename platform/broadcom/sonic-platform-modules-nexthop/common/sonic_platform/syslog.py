#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Syslog utilities and constants module for Nexthop platform modules.

This module provides:
1. Shared syslog identifiers for consistent logging across platform modules
2. Enhanced logger mixin class that automatically prefixes messages with class names
3. Standardized logging interface for all Nexthop platform components

The syslog identifiers follow the naming convention 'nh_<module>' to ensure
uniqueness and easy identification in system logs.
"""

from typing import Any
from sonic_py_common import syslogger


# Syslog identifiers for different platform modules
# These identifiers are used to categorize log messages by functional area
SYSLOG_IDENTIFIER_THERMAL: str = "nh_thermal"  # Thermal management and fan control


class NhLoggerMixin(syslogger.SysLogger):
    """
    Logger mixin that extends SysLogger with automatic class name prefixing.

    Args:
        log_identifier (str): Syslog identifier (e.g., SYSLOG_IDENTIFIER_THERMAL)

    Example:
        class ThermalManager(NhLoggerMixin):
            def __init__(self):
                super().__init__(SYSLOG_IDENTIFIER_THERMAL)
                self.log_info("Manager started")  # Logs: "ThermalManager: Manager started"
    """

    def __init__(self, log_identifier: str) -> None:
        """
        Initialize the logger with the specified identifier.

        Args:
            log_identifier (str): Syslog identifier for categorizing messages
        """
        # Use enable_runtime_config=True to configure log levels at init time
        super().__init__(log_identifier, enable_runtime_config=True)
        self._class_name: str = self.__class__.__name__

    def log(self, priority: Any, msg: str, also_print_to_console: bool = False) -> None:
        """
        Log a message with specified priority, automatically prefixed with class name.

        Args:
            priority: Syslog priority level
            msg (str): Message to log
            also_print_to_console (bool): Whether to also print to console
        """
        super().log(priority, f"{self._class_name}: {msg}", also_print_to_console)
