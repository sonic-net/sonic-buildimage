#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# Importing the base registers the shared policy types the thermal_policy.json file names.
from sonic_platform_base.sonic_thermal_control.pid_thermal_manager_base import PidThermalManagerBase

from .syslog import SYSLOG_IDENTIFIER_THERMAL


class ThermalManager(PidThermalManagerBase):
    SYSLOG_IDENTIFIER = SYSLOG_IDENTIFIER_THERMAL
