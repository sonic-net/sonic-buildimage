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

import time


POLL_INTERVAL_IN_SEC = 1


class SfpEvent:
    ''' Listen to insert/remove sfp events '''

    def __init__(self, sfp_list):
        self._sfp_list = sfp_list
        self._present_ports = 0

    def get_presence_bitmap(self):
        bitmap = 0
        for sfp in self._sfp_list:
            modpres = sfp.get_presence()
            if modpres:
                bitmap = bitmap | (1 << sfp.index)
        return bitmap

    def get_sfp_event(self, timeout=2000):
        port_dict = {}
        change_dict = {'sfp': port_dict}

        if timeout < 1000:
            cd_ms = 1000
        else:
            cd_ms = timeout

        while cd_ms > 0:
            bitmap = self.get_presence_bitmap()
            changed_ports = self._present_ports ^ bitmap
            if changed_ports != 0:
                break
            time.sleep(POLL_INTERVAL_IN_SEC)
            # timeout=0 means wait for event forever
            if timeout != 0:
                cd_ms = cd_ms - POLL_INTERVAL_IN_SEC * 1000

        if not changed_ports:
            return True, change_dict

        for sfp in self._sfp_list:
            if (changed_ports & (1 << sfp.index)):
                if (bitmap & (1 << sfp.index)) == 0:
                    port_dict[sfp.index + 1] = '0'
                else:
                    port_dict[sfp.index + 1] = '1'

        # Update the cache dict
        self._present_ports = bitmap
        return True, change_dict
