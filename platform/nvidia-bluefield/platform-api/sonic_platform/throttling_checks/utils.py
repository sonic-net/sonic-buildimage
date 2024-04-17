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

MLXBF_SENSOR_PATH = '/sys/kernel/debug/mlxbf-ptm/monitors/status'
LAST_VALUES_STORAGE_PATH = '/var/run/mlxbf-last-read/'
DDR_THROTTLING_THRESHOLD = 95

def read_sensor(path, ok_if_not_found = False):
    try:
        with open(path) as f:
            return int(f.readline().strip())
    except OSError:
        if ok_if_not_found:
            return None
        raise Exception(f'Could not read the {path}')
