#!/usr/bin/env python
#
# Name: platform.py, version: 1.0
#
# Description: Module contains the definition of SONiC platform API 
# which provide the platform specific details
#
# Copyright (c) 2020, Juniper Networks, Inc.
# All rights reserved.
#
# Notice and Disclaimer: This code is licensed to you under the GNU General 
# Public License as published by the Free Software Foundation, version 3 or 
# any later version. This code is not an official Juniper product. You can 
# obtain a copy of the License at <https://www.gnu.org/licenses/>
#
# OSS License:
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Third-Party Code: This code may depend on other components under separate 
# copyright notice and license terms.  Your use of the source code for those 
# components is subject to the terms and conditions of the respective license 
# as noted in the Third-Party source code file.
#


import sys

try:
   from sonic_platform_base.platform_base import PlatformBase
   from sonic_platform.chassis import Chassis
except ImportError as e:
   raise ImportError("%s - required module not found" % e)

class Platform(PlatformBase):
    """
    Juniper Platform-specific class
    """	
    def __init__(self):
	PlatformBase.__init__(self)
        self._chassis = Chassis()	
