#
# Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES.
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
module holding the correct values for the sfp_test.py
"""

read_eeprom_output = """
Sending access register...
Field Name            | Data
===================================
status                | 0x00000000
slot_index            | 0x00000000
module                | 0x00000001
l                     | 0x00000000
device_address        | 0x000000a8
page_number           | 0x00000000
i2c_device_address    | 0x00000050
size                  | 0x00000010
bank_number           | 0x00000000
dword[0]              | 0x43414331
dword[1]              | 0x31353332
dword[2]              | 0x31503250
dword[3]              | 0x41324d53
dword[4]              | 0x00000000
dword[5]              | 0x00000000
dword[6]              | 0x00000000
dword[7]              | 0x00000000
dword[8]              | 0x00000000
dword[9]              | 0x00000000
dword[10]             | 0x00000000
dword[11]             | 0x00000000
dword[12]             | 0x00000000
dword[13]             | 0x00000000
dword[14]             | 0x00000000
dword[15]             | 0x00000000
dword[16]             | 0x00000000
dword[17]             | 0x00000000
dword[18]             | 0x00000000
dword[19]             | 0x00000000
dword[20]             | 0x00000000
dword[21]             | 0x00000000
dword[22]             | 0x00000000
dword[23]             | 0x00000000
dword[24]             | 0x00000000
dword[25]             | 0x00000000
dword[26]             | 0x00000000
dword[27]             | 0x00000000
dword[28]             | 0x00000000
dword[29]             | 0x00000000
dword[30]             | 0x00000000
dword[31]             | 0x00000000
===================================
"""

y_cable_part_number = "CAC115321P2PA2MS"
write_eeprom_dword1 = "dword[0]=0x01020304"
write_eeprom_dword2 = "dword[0]=0x01020304,dword[1]=0x05060000"
