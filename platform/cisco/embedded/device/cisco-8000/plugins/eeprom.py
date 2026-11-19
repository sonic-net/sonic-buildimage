#!/usr/bin/env python
# Copyright 2018 Cisco Systems
# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

'''
Based on Cisco Nexus N9K/N3K eeprom plugin
'''

try:
    import sonic_platform.eeprom
except ImportError as e:
    raise ImportError (str(e) + ' - required module not found')

class board(sonic_platform.eeprom.Eeprom):
    def __init__(self, path, start, status, ro):
        super(board, self).__init__()
