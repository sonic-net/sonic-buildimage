#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
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

from dataclasses import dataclass
from typing import List, ClassVar

platform_sample = """
{
    "chassis": {
        "name": "Nvidia-MBF2H536C",
        "components": [],
        "fans": [],
        "fan_drawers": [],
        "psus": [],
        "thermals": [],
        "sfps": [
            {
                "name": "p0"
            },
            {
                "name": "p1"
            }
        ]
    },
    "interfaces": {
        "Ethernet0": {
            "index": "1,1,1,1",
            "lanes": "0,1,2,3",
            "breakout_modes": {
                "1x100G": ["etp1"]
            }
        },
        "Ethernet4": {
            "index": "2,2,2,2",
            "lanes": "4,5,6,7",
            "breakout_modes": {
                "1x100G": ["etp2"]
            }
        }
    }
}
"""

platform_sample_bf3 = """
{
    "chassis": {
        "name": "arm64-nvda_bf-9009d3b600cvaa",
        "components": [],
        "fans": [],
        "fan_drawers": [],
        "psus": [],
        "thermals": [],
        "sfps": [
            {
                "name": "p0"
            },
            {
                "name": "p1"
            }
        ]
    },
    "interfaces": {
        "Ethernet0": {
            "index": "1,1,1,1",
            "lanes": "0,1,2,3",
            "breakout_modes": {
                "1x100G": ["etp1"]
            }
        },
        "Ethernet4": {
            "index": "2,2,2,2",
            "lanes": "4,5,6,7",
            "breakout_modes": {
                "1x100G": ["etp2"]
            }
        }
    }
}
"""

# Utilities for throttling tests
class LogRecorderMock(object):
    def __init__(self):
        self.msgs = []

    def log_warning(self, msg):
        self.msgs.append(('warning', msg))

    def log_error(self, msg):
        self.msgs.append(('error', msg))

    def get_messages(self):
        return self.msgs

@dataclass
class TTestFSState:
    sensors: List[int]
    storage: List[int]
    sensor_names: ClassVar = ['power_throttling_state', 'power_throttling_event_count',
                              'thermal_throttling_state', 'thermal_throttling_event_count', 'ddr_temp']
    def get_dicts(self):
        return dict(zip(self.sensor_names, self.sensors)), dict(zip(self.sensor_names, self.storage))


@dataclass
class TTestData:
    title: str
    fs_state: TTestFSState
    result: str
    log_msgs: List[str]

    def validate(self, result: str, log_recording: LogRecorderMock, t):
        assert f'Throttling\n{t}:{self.result}' == result
        assert len(self.log_msgs) == len(log_recording.get_messages()), log_recording.get_messages()
        for expected, actual in zip(self.log_msgs, log_recording.get_messages()):
            esev, emsg = expected
            asev, amsg = actual
            assert esev == asev
            assert amsg.startswith(emsg), f'{amsg} != {emsg}'
