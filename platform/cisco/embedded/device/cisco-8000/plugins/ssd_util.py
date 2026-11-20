#!/usr/bin/env python
# Copyright 2019-2026 Cisco Systems
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# SsdUtil platform implementation for SONiC Platform Base API.
try:
    from sonic_platform.ssd_util import SsdUtil as Ssd
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

class SsdUtil(Ssd):
    """
    Platform specific class for interfacing with a SSD
    """
    pass
