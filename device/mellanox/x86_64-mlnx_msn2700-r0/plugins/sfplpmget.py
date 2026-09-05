#!/usr/bin/env python3
#
# Copyright (c) 2018-2021 NVIDIA CORPORATION & AFFILIATES.
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

"""
This utility get the power mode of a given module.
"""

import sys
import errno
from python_sdk_api.sx_api import *


def mgmt_phy_mod_pwr_attr_get(handle, module_id, power_attr_type):
    sx_mgmt_phy_mod_pwr_attr_p = new_sx_mgmt_phy_mod_pwr_attr_t_p()
    sx_mgmt_phy_mod_pwr_attr = sx_mgmt_phy_mod_pwr_attr_t()
    sx_mgmt_phy_mod_pwr_attr.power_attr_type = power_attr_type
    sx_mgmt_phy_mod_pwr_attr_t_p_assign(sx_mgmt_phy_mod_pwr_attr_p, sx_mgmt_phy_mod_pwr_attr)
    try:
        rc = sx_mgmt_phy_mod_pwr_attr_get(handle, module_id, sx_mgmt_phy_mod_pwr_attr_p)
        assert SX_STATUS_SUCCESS == rc, "sx_mgmt_phy_mod_pwr_attr_get failed"
        sx_mgmt_phy_mod_pwr_attr = sx_mgmt_phy_mod_pwr_attr_t_p_value(sx_mgmt_phy_mod_pwr_attr_p)
        pwr_mode_attr = sx_mgmt_phy_mod_pwr_attr.pwr_mode_attr
        return pwr_mode_attr.admin_pwr_mode_e, pwr_mode_attr.oper_pwr_mode_e
    finally:
        delete_sx_mgmt_phy_mod_pwr_attr_t_p(sx_mgmt_phy_mod_pwr_attr_p)


# Check if SFP port number is provided
if len(sys.argv) < 2:
    print("SFP module number is missed.")
    print("Usage: sfplpmget.py <SFP module>")
    sys.exit(errno.EINVAL)

# Init SDK API
rc, handle = sx_api_open(None)
if (rc != SX_STATUS_SUCCESS):
    print("Failed to open api handle.\nPlease check that SDK is running.")
    sys.exit(errno.EACCES)

# Get SFP module number
sfp_module = int(sys.argv[1]) - 1

admin_pwr_mode, oper_pwr_mode = mgmt_phy_mod_pwr_attr_get(handle, sfp_module, SX_MGMT_PHY_MOD_PWR_ATTR_PWR_MODE_E)

lpm_status = None
if oper_pwr_mode == SX_MGMT_PHY_MOD_PWR_MODE_HIGH_E:
    lpm_status = False
elif oper_pwr_mode == SX_MGMT_PHY_MOD_PWR_MODE_LOW_E:
    lpm_status = True
else:
    print("LPM UNKNOWN")

print("LPM ON" if lpm_status else "LPM OFF")

sx_api_close(handle)
