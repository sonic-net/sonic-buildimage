#!/usr/bin/env python
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
import io
import sys
import argparse
import shutil
import copy
import difflib
import configparser

from helper import *

################################################################################
####  KConfig Processing                                                    ####
################################################################################

class KCFGData:
    x86_base = OrderedDict()
    x86_updated = OrderedDict()
    arm_base = OrderedDict()
    arm_updated = OrderedDict()
    x86_incl = OrderedDict()
    arm_incl = OrderedDict()
    x86_excl = OrderedDict()
    arm_excl = OrderedDict()
    x86_down = OrderedDict()
    arm_down = OrderedDict()
    noarch_incl = OrderedDict()
    noarch_excl = OrderedDict()
    noarch_down = OrderedDict()


class KConfigTask():
    def __init__(self, args):
        self.args = args


    def read_data(self):
        KCFGData.x86_base = FileHandler.read_kconfig(self.args.config_base_amd)
        KCFGData.x86_updated = FileHandler.read_kconfig(self.args.config_inc_amd)
        if os.path.isfile(self.args.config_inc_down_amd):
            print(" -> Downstream Config for x86 found..")
            KCFGData.x86_down = FileHandler.read_kconfig(self.args.config_inc_down_amd)

        KCFGData.arm_base = FileHandler.read_kconfig(self.args.config_base_arm)
        KCFGData.arm_updated = FileHandler.read_kconfig(self.args.config_inc_arm)
        if os.path.isfile(self.args.config_inc_down_arm):
            print(" -> Downstream Config for arm64 found..")
            KCFGData.arm_down = FileHandler.read_kconfig(self.args.config_inc_down_arm)
        return 


    def parse_inc_exc(self, base: OrderedDict, updated: OrderedDict):
        # parse the updates/deletions in the Kconfig
        add, remove = OrderedDict(), copy.deepcopy(base)
        for (key, val) in updated.items():
            if val != base.get(key, "empty"):
                add[key] = val
            # items remaining in remove are the ones to be excluded
            if key in remove:
                del remove[key]
        return add, remove


    def parse_noarch_inc_exc(self):
        # Filter the common inc/excl out from the arch specific inc/excl
        x86_incl_base = copy.deepcopy(KCFGData.x86_incl)
        for (key, val) in x86_incl_base.items():
            if key in KCFGData.arm_incl and val == KCFGData.arm_incl[key]:
                print("-> INFO: NoArch KConfig Inclusion {}:{} found and moving to common marker".format(key, val))
                del KCFGData.arm_incl[key]
                del KCFGData.x86_incl[key]
                KCFGData.noarch_incl[key] = val

        x86_excl_base = copy.deepcopy(KCFGData.x86_excl)
        for (key, val) in x86_excl_base.items():
            if key in KCFGData.arm_excl:
                print("-> INFO: NoArch KConfig Exclusion {} found and moving to common marker".format(key))
                del KCFGData.arm_excl[key]
                del KCFGData.x86_excl[key]
                KCFGData.noarch_excl[key] = val

        if not (KCFGData.x86_down or KCFGData.arm_down):
            return

        # Filter the common inc config from the downstream kconfig
        x86_down_base = copy.deepcopy(KCFGData.x86_down)
        for (key, val) in x86_down_base.items():
            if key in KCFGData.arm_down:
                print("-> INFO: NoArch KConfig Downstream Inclusion {} found and moving to common marker".format(key))
                del KCFGData.arm_down[key]
                del KCFGData.x86_down[key]
                KCFGData.noarch_down[key] = val

    def insert_arm64_section(self, raw_lines: list, arm_data: OrderedDict, section=MLNX_ARM_KFG_SECTION) -> list:
        # For arm64, config is not added under markers, but it is added under the section [mellanox-arm64]
        # This design decision is taken because of the possibility that there might be conflicting options 
        # present between two different arm64 platforms
        try:
            # comment_prefixes needed to also read comments under a section
            configParser = configparser.ConfigParser(allow_no_value=True, strict=False, comment_prefixes='////')
            configParser.optionxform = str
            configParser.read_string("".join(raw_lines))
            if not configParser.has_section(MLNX_ARM_KFG_SECTION):
                configParser.add_section(MLNX_ARM_KFG_SECTION)
            for (key, val) in arm_data.items():
                configParser.set(MLNX_ARM_KFG_SECTION, key, val)
            str_io = io.StringIO()
            configParser.write(str_io, space_around_delimiters=False)
            return str_io.getvalue().splitlines(True)
        except Exception as e:
            print("-> FATAL: Exception {} found while adding opts under arm".format(str(e)))
            raise e
        return raw_lines


    def get_upstream_kconfig(self) -> list:
        # Insert common config
        common_config = FileHandler.read_raw(os.path.join(self.args.build_root, SLK_KCONFIG))
        noarch_start, noarch_end = FileHandler.find_marker_indices(common_config, MLNX_NOARCH_MARKER)
        noarch_final = OrderedDict(list(KCFGData.noarch_incl.items()) + [(k, "n") for k in KCFGData.noarch_excl.keys()])
        common_config = FileHandler.insert_kcfg_data(common_config, noarch_start, noarch_end, noarch_final)
        # Insert x86 config
        amd64_config = FileHandler.read_raw(os.path.join(self.args.build_root, SLK_KCONFIG_AMD64))
        x86_start, x86_end = FileHandler.find_marker_indices(amd64_config, MLNX_KFG_MARKER)
        x86_final = OrderedDict(list(KCFGData.x86_incl.items()) + [(k, "n") for k in KCFGData.x86_excl.keys()])
        amd64_config = FileHandler.insert_kcfg_data(amd64_config, x86_start, x86_end, x86_final)
        # Insert arm config
        arm64_config = FileHandler.read_raw(os.path.join(self.args.build_root, SLK_KCONFIG_ARM64))
        arm_final = OrderedDict(list(KCFGData.arm_incl.items()) + [(k, "n") for k in KCFGData.arm_excl.keys()])
        arm64_config = self.insert_arm64_section(arm64_config, arm_final)
        print("\n -> INFO: kconfig-inclusion file is generated \n {}".format("".join(arm64_config)))
        return common_config, amd64_config, arm64_config

    def get_downstream_kconfig_diff(self, common_config_upstream, amd64_config_upstream, arm64_config_upstream) -> list:
        # insert common Kconfig
        common_kcfg_final = copy.deepcopy(common_config_upstream)
        noarch_start, noarch_end = FileHandler.find_marker_indices(common_kcfg_final, MLNX_NOARCH_MARKER)        
        noarch_final = OrderedDict(
            list(KCFGData.noarch_incl.items()) +
            [(k, "n") for k in KCFGData.noarch_excl.keys()] +
            list(KCFGData.noarch_down.items())
        )
        common_kcfg_final = FileHandler.insert_kcfg_data(common_kcfg_final, noarch_start, noarch_end, noarch_final)

        # insert x86 Kconfig
        amd64_kcfg_final = copy.deepcopy(amd64_config_upstream)
        x86_start, x86_end = FileHandler.find_marker_indices(amd64_kcfg_final, MLNX_KFG_MARKER)        
        x86_final = OrderedDict(
            list(KCFGData.x86_incl.items()) +
            [(k, "n") for k in KCFGData.x86_excl.keys()] +
            list(KCFGData.x86_down.items())
        )
        amd64_kcfg_final = FileHandler.insert_kcfg_data(amd64_kcfg_final, x86_start, x86_end, x86_final)

        # insert arm Kconfig      
        arm64_kcfg_final = copy.deepcopy(arm64_config_upstream)
        arm_final = OrderedDict(
            list(KCFGData.arm_incl.items()) +
            [(k, "n") for k in KCFGData.arm_excl.keys()] +
            list(KCFGData.arm_down.items())
        )
        arm64_kcfg_final = self.insert_arm64_section(arm64_kcfg_final, arm_final)

        all_lines = []
        
        # Generate diff for common config
        common_diff = difflib.unified_diff(common_config_upstream, common_kcfg_final, 
                                        fromfile='a/config.local/featureset-sonic/config', 
                                        tofile="b/config.local/featureset-sonic/config", 
                                        lineterm="\n")
        
        for line in common_diff:
            all_lines.append(line)

        # Generate diff for amd64 config
        amd64_diff = difflib.unified_diff(amd64_config_upstream, amd64_kcfg_final,
                                        fromfile='a/config.local/amd64/config.sonic',
                                        tofile="b/config.local/amd64/config.sonic",
                                        lineterm="\n")

        for line in amd64_diff:
            all_lines.append(line)

        # Generate diff for arm64 config
        arm64_diff = difflib.unified_diff(arm64_config_upstream, arm64_kcfg_final,
                                        fromfile='a/config.local/arm64/config.sonic-mellanox',
                                        tofile="b/config.local/arm64/config.sonic-mellanox",
                                        lineterm="\n")
        
        for line in arm64_diff:
            all_lines.append(line)

        print("\n -> INFO: kconfig-inclusion.patch file is generated \n{}".format("".join(all_lines)))
        return all_lines


    def perform(self):
        self.read_data()
        KCFGData.x86_incl, KCFGData.x86_excl = self.parse_inc_exc(KCFGData.x86_base, KCFGData.x86_updated)
        KCFGData.arm_incl, KCFGData.arm_excl = self.parse_inc_exc(KCFGData.arm_base, KCFGData.arm_updated)
        self.parse_noarch_inc_exc()
        # Get the updated common, amd64, arm64 configs for each file
        common_config, amd64_config, arm64_config = self.get_upstream_kconfig()
        FileHandler.write_lines(os.path.join(self.args.build_root, SLK_KCONFIG), common_config, True)
        FileHandler.write_lines(os.path.join(self.args.build_root, SLK_KCONFIG_AMD64), amd64_config, True)
        FileHandler.write_lines(os.path.join(self.args.build_root, SLK_KCONFIG_ARM64), arm64_config, True)
        # return the downstream kconfig diff
        return self.get_downstream_kconfig_diff(common_config, amd64_config, arm64_config)
