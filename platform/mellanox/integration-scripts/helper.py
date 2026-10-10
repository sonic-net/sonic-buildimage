#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2023-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
import re
import sys
import glob
from collections import OrderedDict, namedtuple

MARK_ID = "###->"
MLNX_KFG_MARKER = "mellanox_amd64"
MLNX_NOARCH_MARKER = "mellanox_common"
MLNX_ARM_KFG_SECTION = "mellanox-arm64"
SDK_MARKER = "mellanox_sdk"
HW_MGMT_MARKER = "mellanox_hw_mgmt"
MLNX_ASPEED_MARKER = "nvidia_aspeed_bmc"
SLK_PATCH_LOC = "src/sonic-linux-kernel/patches-sonic/"
SLK_KCONFIG_DIR = "src/sonic-linux-kernel/config.local/"
SLK_KCONFIG = SLK_KCONFIG_DIR+"featureset-sonic/config"
SLK_KCONFIG_AMD64 = SLK_KCONFIG_DIR+"amd64/config.sonic"
SLK_KCONFIG_ARM64 = SLK_KCONFIG_DIR+"arm64/config.sonic-mellanox"
SLK_KCONFIG_ARM64_COMMON = SLK_KCONFIG_DIR+"arm64/config.sonic"
SLK_KCONFIG_ASPEED = SLK_KCONFIG_DIR+"arm64/config.sonic-aspeed"
SLK_SERIES = SLK_PATCH_LOC + "series"
NON_UP_PATCH_DIR = "platform/mellanox/non-upstream-patches/"
NON_UP_PATCH_LOC = NON_UP_PATCH_DIR + "patches"
NON_UP_DIFF = NON_UP_PATCH_DIR + "external-changes.patch"
HWMGMT_LINUX_DIR = "platform/mellanox/hw-management/hw-mgmt/recipes-kernel/linux/"
BMC_PATCH_TABLE_NAME = "Patch_BMC_Status_Table.txt"
KCFG_HDR_RE = "\[(.*)\]"
KERNEL_BACKPORTS = "kernel_backports"
 # kconfig_inclusion headers to consider
HDRS = ["common", "amd64"]

# CONFIG assignment in a kconfig fragment, read like Debian's KconfigFile.read()
# (debian/lib/python/debian_linux/kconfig.py), which merges config.local at kernel build time:
# "CONFIG_X=v" is an assignment, exactly "# CONFIG_X is not set" is X=n, any other "#" line is a comment.
# owner: the ###-> block enclosing the line (None for a loose line)
KcfgEntry = namedtuple("KcfgEntry", ["path", "lineno", "key", "value", "owner"], defaults=(None,))
KCFG_SET_RE = re.compile(r"^(CONFIG_\w+)=(.*)$")
KCFG_UNSET_RE = re.compile(r"^# (CONFIG_\w+) is not set$")
# "###-> <name>-start" / "###-> <name>-end", trailing text tolerated; one parser for every reader
MARKER_RE = re.compile(r"^" + re.escape(MARK_ID) + r"\s*(\S+?)-(start|end)(\s|$)")


def parse_marker(line: str):
    """ (name, "start"|"end") for a marker line, None for anything else """
    m = MARKER_RE.match(line.strip())
    return (m.group(1), m.group(2)) if m else None

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
        # exact marker name; with several matches the last one wins, find_marker_indices_checked
        # rejects that before a block is rewritten
        i_start = -1
        i_end = len(lines)
        if marker:
            for index, line in enumerate(lines):
                if parse_marker(line) == (marker, "start"):
                    i_start = index
                elif parse_marker(line) == (marker, "end"):
                    i_end = index
        return (i_start, i_end)

    @staticmethod
    def marker_block_missing(lines: list, i_start: int, i_end: int) -> bool:
        """ No block to rewrite: no start marker, no end marker, or the two reversed """
        return i_start < 0 or i_end >= len(lines) or i_end < i_start

    @staticmethod
    def marker_block_absent(lines: list, i_start: int, i_end: int) -> bool:
        """ Neither marker is present, i.e. a file that predates the block. A half-present or
            reversed pair is a broken file instead, and must never pass as an old file. """
        return i_start < 0 and i_end >= len(lines)

    @staticmethod
    def find_marker_indices_checked(lines: list, marker: str, path: str) -> tuple:
        """ find_marker_indices for a block that is about to be rewritten: a missing, reversed
            or repeated marker would make the block swallow lines that are not its own. """
        n_start = sum(1 for l in lines if parse_marker(l) == (marker, "start"))
        n_end = sum(1 for l in lines if parse_marker(l) == (marker, "end"))
        if (n_start, n_end) not in ((1, 1), (0, 0), (1, 0), (0, 1)):
            print("-> FATAL: {} has {} {}-start and {} {}-end markers, expected one of each".format(
                path, n_start, marker, n_end, marker))
            sys.exit(1)
        (i_start, i_end) = FileHandler.find_marker_indices(lines, marker)
        if FileHandler.marker_block_missing(lines, i_start, i_end):
            print("-> FATAL: {} markers not found in {}. "
                  "Please update sonic-linux-kernel to include the markers.".format(marker, path))
            sys.exit(1)
        # the block holds nothing but its own lines: a marker inside it means another block is
        # nested in or crossed with this one, and rewriting would destroy that block
        inside = [l.strip() for l in lines[i_start + 1:i_end] if parse_marker(l)]
        if inside:
            print("-> FATAL: {} has other markers inside the {} block ({}), blocks must not overlap".format(
                path, marker, ", ".join(inside)))
            sys.exit(1)
        return (i_start, i_end)

    @staticmethod
    def read_kconfig(path) -> dict:
        # read the .config file generated during kernel compilation
        lines = FileHandler.read_strip_minimal(path)
        config_data = OrderedDict()
        for line in lines:
            if line.strip().startswith("#"):
                continue
            tokens = line.strip().split('=', 1)
            if len(tokens) == 2:
                key = tokens[0].strip()
                value = tokens[1].strip()
                config_data[key] = value
        return config_data

    @staticmethod
    def read_kconfig_entries(path) -> list:
        """ All CONFIG assignments in the file, "# CONFIG_X is not set" included (as "n"), each
            tagged with the ###-> block enclosing it. Another tool's markers are not validated
            here, so a broken pair fails closed: a start without end owns the rest of the file,
            an end without start owns everything before it. A line of unknown ownership must
            never pass as a loose one. """
        entries, open_blocks = [], []
        for lineno, raw in enumerate(FileHandler.read_raw(path), 1):
            line = raw.strip()
            marker = parse_marker(line)
            if marker:
                name, kind = marker
                if kind == "start":
                    open_blocks.append(name)
                elif name in open_blocks:
                    # only this block closes: a crossed or repeated start stays open
                    open_blocks.remove(name)
                else:
                    entries = [e._replace(owner=e.owner or name) for e in entries]
                continue
            owner = open_blocks[-1] if open_blocks else None
            m = KCFG_SET_RE.match(line)
            if m:
                entries.append(KcfgEntry(path, lineno, m.group(1), m.group(2).strip(), owner))
                continue
            m = KCFG_UNSET_RE.match(line)
            if m:
                entries.append(KcfgEntry(path, lineno, m.group(1), "n", owner))
        return entries

    @staticmethod
    def entries_outside_block(path, marker) -> list:
        # Entries of the file outside the marker block (the block is rewritten by this run)
        start, end = FileHandler.find_marker_indices(FileHandler.read_raw(path), marker)
        return [e for e in FileHandler.read_kconfig_entries(path)
                if start < 0 or e.lineno - 1 < start or e.lineno - 1 > end]

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


def build_commit_description(changes, title="Patch List"):
    if not changes:
        return ""
    content = "\n"
    content = content + f" ## {title}\n"
    for key, value in changes.items():
        content = content + f"* {key} : {value}\n"
    return content


def parse_id(id_):
    if id_ and id_ != "N/A":
        id_ = "https://github.com/torvalds/linux/commit/" + id_
    
    if id_ == "N/A":
        id_ = ""

    return id_
