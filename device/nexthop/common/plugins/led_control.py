# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# Nexthop Port LED Policy

try:
    from nexthop.led_control import LedControl
except ImportError as e:
    raise ImportError("%s - required module not found" % e)
