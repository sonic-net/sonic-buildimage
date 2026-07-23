#!/usr/bin/python
# -*- coding: utf-8 -*-#

# @Time   : 2023/7/31 13:15
# @Mail   : yajiang@celestica.com
# @Author : jiang tao

import sys
import time
import json
import subprocess
sys.path.append(r"/usr/local/bin")
from FanControl import FanControl

set_sys_led = "i2cset -y -f 6 0x0d 0x62 0xdc"
# Wait for a while to ensure that the corresponding system files are ready
time.sleep(180)

pddf_device_path = '/usr/share/sonic/platform/pddf/pddf-device.json'
with open(pddf_device_path) as f:
    json_data = json.load(f)
bmc_present = json_data["PLATFORM"]["bmc_present"]

if bmc_present == "False":
    result = subprocess.run(set_sys_led, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        print("Setting SYS LED to green failed")
    FanControl.main()
else:
    while True:
        time.sleep(60)
        pass

