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

try:
    from sonic_platform_base.platform_base import PlatformBase
    from .chassis import Chassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Platform(PlatformBase):

    def __init__(self):
        PlatformBase.__init__(self)
        self._chassis = Chassis()
