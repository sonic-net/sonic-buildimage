#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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

import json
import os
from pathlib import Path
import sys
import time

from sonic_platform import leakage_sensor as sensor_module
from sonic_platform import utils
from sonic_platform.liquid_cooling import LiquidCooling
from sonic_platform_base.liquid_cooling_base import LeakSeverity


root, store = Path(sys.argv[1]), Path(sys.argv[2])
utils.HW_MANAGEMENT_ROOT = str(root)
sensor_module.LeakageSensor.LEAKAGE_CHANNEL_DIR = str(root / "leakage/{a2d_index}/{channel_index}")
sensor_module.LEAK_TEST_DIR = str(store)
cooling = LiquidCooling()
action, name, enable, severity, pause = json.loads(sys.argv[3])
interface = cooling.get_leak_sensor_test() if action != "poll" else None
if action != "poll":
    assert interface is not None, "get_leak_sensor_test() must return the leak test interface"
if pause:
    def stopped(original):
        def rename(source, destination, *args, **kwargs):
            if Path(destination).parent == store:
                Path(str(store) + ".ready").write_text(str(source))
                while not Path(str(store) + ".go").exists():
                    time.sleep(0.01)
            return original(source, destination, *args, **kwargs)
        return rename

    # Pause at the rename that publishes the marker (os.replace or os.rename).
    os.replace = stopped(os.replace)
    os.rename = stopped(os.rename)
if action == "set":
    result = interface.set_test_leak(name, enable, severity=LeakSeverity(severity))
elif action == "clear":
    result = interface.clear_test_leaks()
else:
    result = []
    for sensor in cooling.get_all_leak_sensors():
        ok, leak = sensor.is_leak_sensor_ok(), sensor.is_leak()
        severity = sensor.get_leak_severity()
        result.append([ok, leak, severity.value if severity else None, sensor.is_test_leak()])
print(json.dumps(result))
