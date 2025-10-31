#
# Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES.
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

import os
import glob
from collections import OrderedDict

MARK_ID = "###->"
MLNX_KFG_MARKER = "mellanox_amd64"
MLNX_NOARCH_MARKER = "mellanox_common"
MLNX_ARM_KFG_SECTION = "mellanox-arm64"
SDK_MARKER = "mellanox_sdk"
HW_MGMT_MARKER = "mellanox_hw_mgmt"
SLK_PATCH_LOC = "src/sonic-linux-kernel/patches-sonic/"
SLK_KCONFIG_DIR = "src/sonic-linux-kernel/config.local/"
SLK_KCONFIG = SLK_KCONFIG_DIR+"featureset-sonic/config"
SLK_KCONFIG_AMD64 = SLK_KCONFIG_DIR+"amd64/config.sonic"
SLK_KCONFIG_ARM64 = SLK_KCONFIG_DIR+"arm64/config.sonic-mellanox" 
SLK_SERIES = SLK_PATCH_LOC + "series"
NON_UP_PATCH_DIR = "platform/mellanox/non-upstream-patches/"
NON_UP_PATCH_LOC = NON_UP_PATCH_DIR + "patches"
NON_UP_DIFF = NON_UP_PATCH_DIR + "external-changes.patch"
KCFG_HDR_RE = "\[(.*)\]"
KERNEL_BACKPORTS = "kernel_backports"
 # kconfig_inclusion headers to consider
HDRS = ["common", "amd64"]

class FileHandler:
    
    @staticmethod
    def write_lines(path, lines, raw=False):
        # Create the dir if it doesn't exist already
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w') as f:
            for line in lines:
                if raw:
                    f.write(f"{line}")
                else:
                    f.write(f"{line}\n")

    @staticmethod
    def read_raw(path):
        # Read the data line by line into a list
        data = []
        with open(path) as im:
            data = im.readlines()
        return data

    @staticmethod
    def read_strip(path, as_string=False):
        # Read the data line by line into a list and strip whitelines
        data = FileHandler.read_raw(path)
        data = [d.strip() for d in data]
        if as_string:
            return "\n".join(data)
        return data

    @staticmethod
    def read_strip_minimal(path, as_string=False, ignore_start_with="#"):
        # Read the data line by line into a list, strip spaces and ignore comments
        data = FileHandler.read_raw(path)
        filtered_data = []
        for l in data:
            l = l.strip()
            if l and not l.startswith(ignore_start_with):
                filtered_data.append(l)
        if as_string:
            return "\n".join(filtered_data)
        return filtered_data
       
    @staticmethod
    def read_dir(path, ext="*") -> list:
        return [os.path.basename(f) for f in glob.glob(os.path.join(path, ext))]

    @staticmethod
    def find_marker_indices(lines: list, marker=None) -> tuple:
        i_start = -1
        i_end = len(lines)
        # print("TEST", marker, lines)
        if marker:
            for index, line in enumerate(lines):
                # assumes one unique marker per file
                # if multiple marker sections are present, reads the first one
                if line.strip().startswith(MARK_ID):
                    if marker+"-start" in line:
                        i_start = index
                    elif marker+"-end" in line:
                        i_end = index
        # print(i_start, i_end)             
        return (i_start, i_end)

    @staticmethod
    def read_kconfig(path) -> dict:
        # read the .config file generated during kernel compilation
        lines = FileHandler.read_strip_minimal(path)
        config_data = OrderedDict()
        for line in lines:
            if line.strip().startswith("#"):
                continue
            tokens = line.strip().split('=')
            if len(tokens) == 2:
                key = tokens[0].strip()
                value = tokens[1].strip()
                config_data[key] = value
        return config_data

    @staticmethod
    def insert_lines(lines: list, start: int, end: int, new_data: list) -> list:
        return lines[0:start+1] + new_data + lines[end:]
    
    @staticmethod
    def insert_kcfg_data(lines: list, start: int, end: int, new_data: OrderedDict) -> dict:
        # inserts data into the lines, escape every lines
        new_data_lines = ["{}={}\n".format(cfg, val) for (cfg, val) in new_data.items()]
        return FileHandler.insert_lines(lines, start, end, new_data_lines)


class Action():
    def __init__(self, args):
        self.args = args
    
    def perform(self):
        pass

    def write_user_out(self):
        pass


def build_commit_description(changes):
    if not changes:
        return ""
    content = "\n"
    content = content + " ## Patch List\n"
    for key, value in changes.items():
        content = content + f"* {key} : {value}\n"
    return content


def parse_id(id_):
    if id_ and id_ != "N/A":
        id_ = "https://github.com/torvalds/linux/commit/" + id_
    
    if id_ == "N/A":
        id_ = ""

    return id_
