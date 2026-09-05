#
# Copyright (c) 2020-2021 NVIDIA CORPORATION & AFFILIATES.
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
class MockFan:
    speed = 60
    def __init__(self):
        self.presence = True
        self.name = None
        self.status = True

    def get_presence(self):
        return self.presence

    def set_speed(self, speed):
        MockFan.speed = speed

    def get_status(self):
        return self.status

    def get_target_speed(self):
        return MockFan.speed

    def get_name(self):
        return self.name


class MockPsu:
    def __init__(self):
        self.presence = True
        self.powergood = True

    def get_presence(self):
        return self.presence

    def get_powergood_status(self):
        return self.powergood

    def get_all_fans(self):
        return []


class MockFanDrawer:
    def __init__(self):
        self.fan_list = []

    def get_all_fans(self):
        return self.fan_list


class MockChassis:
    def __init__(self):
        self.fan_list = []
        self.psu_list = []
        self.fan_drawer_list = []

    def get_all_psus(self):
        return self.psu_list

    def get_all_fans(self):
        return self.fan_list

    def get_all_fan_drawers(self):
        return self.fan_drawer_list

    def get_thermal_manager(self):
        from sonic_platform.thermal_manager import ThermalManager
        return ThermalManager

    def make_fan_absence(self):
        fan = MockFan()
        fan.presence = False
        fan_drawer = MockFanDrawer()
        self.fan_drawer_list.append(fan_drawer)
        fan_drawer.fan_list.append(fan)

    def make_psu_absence(self):
        psu = MockPsu()
        psu.presence = False
        self.psu_list.append(psu)
