#!/usr/bin/python
# -*- coding: utf-8 -*-#

# @Time   : 2023/7/31 13:15
# @Mail   : yajiang@celestica.com
# @Author : jiang tao

import sys
import os
import json
sys.path.append(r"/usr/local/bin")
from FanControl import FanControl


pddf_device_path = '/usr/share/sonic/platform/pddf/pddf-device.json'
with open(pddf_device_path) as f:
    json_data = json.load(f)
bmc_present = json_data["PLATFORM"]["bmc_present"]
if bmc_present == "False":
    FanControl.main(sys.argv[1:])
