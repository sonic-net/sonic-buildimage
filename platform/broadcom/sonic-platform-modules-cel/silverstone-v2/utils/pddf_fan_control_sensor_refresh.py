#!/usr/bin/python
# -*- coding: utf-8 -*-#

# @Time   : 2023/7/31 13:15
# @Mail   : yajiang@celestica.com
# @Author : jiang tao

import sys
import time
import json
sys.path.append(r"/usr/local/bin")
from FanControl import FanControl
import pddf_sensor_list_refresh


pddf_device_path = '/usr/share/sonic/platform/pddf/pddf-device.json'
with open(pddf_device_path) as f:
    json_data = json.load(f)
bmc_present = json_data["PLATFORM"]["bmc_present"]
# Wait for a while to ensure that the corresponding system files are ready
time.sleep(30)
if bmc_present == "False":
    FanControl.main()

if bmc_present == "True":
    pddf_sensor_list_refresh.main()
