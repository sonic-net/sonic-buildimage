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
import sys
import shutil
import argparse
import copy

from helper import *

COMMIT_TITLE = "Integrate SDK {} Kernel Patches"

DELIMITTER = "========================"

class Data:
    # old sonic-linux-kernel series file 
    old_series = list()
    # updated sonic-linux-kernel series file
    new_series = list()
    # list of any new SDK patches to add
    new_patches = list()
    # list of current patch list in sonic-linux-kernel
    old_patches = list()
    # index of the mlnx_hw_mgmt patches start marker in old_series
    i_sdk_start = -1 
    # index of the mlnx_hw_mgmt patches end marker in old_series
    i_sdk_end = -1
    # kernel directory to consider the patches
    k_dir = ""


class SDKAction(Action):
    def __init__(self, args):
        super().__init__(args)

    def check(self):
        if not (self.args.patches and os.path.exists(self.args.patches)):
            print("-> ERR: patch directory is missing ")
            return False
    
        if not (self.args.build_root and os.path.exists(self.args.build_root)):
            print("-> ERR: build_root is missing")
            return False

        return True
    
    def read_data(self):
        Data.old_series = FileHandler.read_raw(os.path.join(self.args.build_root, SLK_SERIES))
    
    def find_sdk_patches(self):
        for index in range(Data.i_sdk_start+1, Data.i_sdk_end):
            patch = Data.old_series[index].strip()
            if patch:
                Data.old_patches.append(Data.old_series[index].strip())
        print("-> INFO Current mellanox sdk upstream patches. \n{}".format("\n".join(Data.old_patches)))

    def get_kernel_dir(self):
        # Get the kernel dir name to get the patches
        try:
            (kernel, major, minor) = self.args.sonic_kernel_ver.split(".")
            minor_int = int(minor)
        except Exception as e:
            print("-> FATAL Kernel version formatting error: " + str(e))
            sys.exit(1)

        major_kernel_path = os.path.join(KERNEL_BACKPORTS, "{}.{}".format(kernel, major))

        # if the k_dir with actual minor doesn't exit, use the closest minor version
        for minor_i in range(minor_int, -1, -1):
            path = os.path.join(major_kernel_path, "{}.{}.{}".format(kernel, major, minor_i))
            if os.path.exists(os.path.join(self.args.patches, path)):
                minor = str(minor_i)
                Data.k_dir = path
                print(f"-> INFO Kernel minor version dir {Data.k_dir} found ")
                break
        
        if not Data.k_dir:
            print("-> Couldn't find the relevant kernel directory")
            sys.exit(1)

        path_to_check = os.path.join(self.args.patches, Data.k_dir)
        if not os.path.exists(path_to_check):
            print("-> FATAL Kernel dir with patches doesn't exist: {}".format(path_to_check))
            sys.exit(1)

    def update_series(self):
         # Remove patches if they are already present in existing series file
        patch_ids_remove = set()
        for id in range(0, len(Data.old_series)): 
            for patch in Data.new_patches:
                if patch in Data.old_series[id]:
                    patch_ids_remove.add(id)

        # Modify the series files
        temp_series = copy.deepcopy(Data.old_series)
        Data.old_series.clear()
        for id in range(0, len(temp_series)):
            if id not in patch_ids_remove:
                Data.old_series.append(temp_series[id])
            else:
                print("-> INFO Patch {} will be removed from the existing series".format(temp_series[id]))
        print("-> INFO Updated Series file after removing SDK patches: \n{}".format("".join(Data.old_series)))

    def add_new_patch_series(self):
        new_patches = [patch + "\n" for patch in Data.new_patches]
        Data.new_series = Data.old_series[0:Data.i_sdk_start+1] + new_patches + Data.old_series[Data.i_sdk_end:]
    
    def process_update(self):
        src_path = os.path.join(self.args.patches, Data.k_dir)
        dst_path = os.path.join(self.args.build_root, SLK_PATCH_LOC)

        for patch in Data.new_patches:
            print(f"-> Moving patch: {patch}")
            shutil.copy(os.path.join(src_path, patch), dst_path)

        FileHandler.write_lines(os.path.join(self.args.build_root, SLK_SERIES), Data.new_series, True)
        print("-> INFO Written sonic-linux-kernel series file \n{}".format("".join(Data.new_series)))

    def get_new_patches(self):
        patches_path = os.path.join(self.args.patches, Data.k_dir)
        Data.new_patches = FileHandler.read_dir(patches_path, "*.patch")
        Data.new_patches.sort()

    def cleanup_old_patches(self):
        patches_del = copy.deepcopy(Data.old_patches)
        for patch in Data.new_patches:
            if patch in Data.old_patches:
                patches_del.remove(patch)
        print(f" -> Patches to be removed are : {patches_del}")
        for patch in patches_del:
            file_n = os.path.join(self.args.build_root, os.path.join(SLK_PATCH_LOC, patch))
            if os.path.exists(file_n):
                print(f"{file_n} is deleted")
                os.remove(file_n)

    def refresh_markers(self):
        print("-> INFO Refreshing Markers ")
        (Data.i_sdk_start, Data.i_sdk_end) = FileHandler.find_marker_indices(Data.old_series, SDK_MARKER)
        if Data.i_sdk_start < 0 or Data.i_sdk_end > len(Data.old_series):
            print("-> FATAL mellanox_sdk marker not found. Couldn't continue.. exiting")
            sys.exit(1)
        print("-> INFO mellanox_sdk markers found. start: {}, end: {}".format(Data.i_sdk_start, Data.i_sdk_end))

    def fetch_patch_table(self, root_dir):
        lines = FileHandler.read_strip(os.path.join(root_dir, "README"))
        delim_id = 0
        for id in range(0, len(lines)):
            if DELIMITTER in lines[id]:
                delim_id = id
                break
        
        table = dict()
        for id in range(delim_id+1, len(lines)):
            tokens = lines[id].split()
            if len(tokens) != 3:
                print("-> INFO Error Formatted line, {}".format(" ".join(lines[id])))
                continue 
            patch, commit = tokens[0], tokens[1]
            if ".patch" not in patch:
                print(f"-> INFO Unexpected Patch name {patch}")
                continue
            table[patch.strip()] = commit.strip() 
        
        print(f"-> INFO Final Patch Table: {table}")
        return table

    def create_commit_msg(self, patch_table):
        changes = {}
        for patch in Data.new_patches:
            if patch not in Data.old_patches:
                changes[patch] = parse_id(patch_table.get(patch.strip(), ""))
        slk_commit_msg = COMMIT_TITLE.format(self.args.sdk_ver) + "\n" + build_commit_description(changes)
        print(f"-> INFO Commit Message: {slk_commit_msg}")
        return slk_commit_msg

    def perform(self):
        self.read_data()
        self.refresh_markers()
        self.find_sdk_patches()
        self.get_kernel_dir()
        self.get_new_patches()
        self.update_series()
        self.refresh_markers()
        self.add_new_patch_series()
        self.process_update()
        self.cleanup_old_patches()
        patch_table = self.fetch_patch_table(os.path.join(self.args.patches, Data.k_dir))
        slk_msg = self.create_commit_msg(patch_table)
        if self.args.slk_msg:
            with open(self.args.slk_msg, 'w') as f:
                f.write(slk_msg) 



def create_parser():
    # Create argument parser
    parser = argparse.ArgumentParser()

    # Optional arguments
    parser.add_argument("--sonic_kernel_ver", type=str)
    parser.add_argument("--patches", type=str)
    parser.add_argument("--slk_msg", type=str)
    parser.add_argument("--sdk_ver", type=str)
    parser.add_argument("--build_root", type=str)
    return parser

if __name__ == '__main__':
    parser = create_parser()
    action = SDKAction(parser.parse_args())
    action.check()
    action.perform()