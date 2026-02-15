#!/usr/bin/env python
#
# Copyright (c) 2023-2024 NVIDIA CORPORATION & AFFILIATES.
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

from hwmgmt_helper import *

COMMIT_TITLE = "Integrate HW-MGMT {} Changes"

PATCH_TABLE_LOC = "platform/mellanox/hw-management/hw-mgmt/recipes-kernel/linux/"
PATCHWORK_LOC = "linux-{}/patchwork"
PATCH_TABLE_NAME = "Patch_Status_Table.txt"
PATCH_TABLE_DELIMITER = "----------------------"
PATCH_NAME = "patch name"
COMMIT_ID = "Upstream commit id"

# Strips the subversion
def get_kver(k_version):
    major, minor, subversion = k_version.split(".")
    k_ver = "{}.{}".format(major, minor)
    return k_ver

def trim_array_str(str_list):
    ret = [elem.strip() for elem in str_list]
    return ret

def get_line_elements(line):
    columns_raw = line.split("|")
    if len(columns_raw) < 3:
        return False
    # remove empty firsta and last elem
    columns_raw = columns_raw[1:-1]
    columns = trim_array_str(columns_raw)
    return columns

def load_patch_table(path, k_ver):
    patch_table_filename = os.path.join(path, PATCH_TABLE_NAME)
    print("Loading patch table {} kver:{}".format(patch_table_filename, k_ver))

    if not os.path.isfile(patch_table_filename):
        print("-> ERR: file {} not found".format(patch_table_filename))
        return None

    # opening the file
    patch_table_file = open(patch_table_filename, "r")
    # reading the data from the file
    patch_table_data = patch_table_file.read()
    # splitting the file data into lines
    patch_table_lines = patch_table_data.splitlines()
    patch_table_file.close()

    # Extract patch table for specified kernel version
    kversion_line = "Kernel-{}".format(k_ver)
    table_ofset = 0
    for table_ofset, line in enumerate(patch_table_lines):
        if line == kversion_line:
            break

    # if kernel version not found
    if table_ofset >= len(patch_table_lines)-5:
        print ("Err: kernel version {} not found in {}".format(k_ver, patch_table_filename))
        return None

    table = []
    delimiter_count = 0
    column_names = None
    for idx, line in enumerate(patch_table_lines[table_ofset:]):
        if PATCH_TABLE_DELIMITER in line:
            delimiter_count += 1
            if delimiter_count >= 3:
                print ("Err: too much leading delimers line #{}: {}".format(table_ofset + idx, line))
                return None
            elif table:
                break
            continue

        # line without delimiter but header still not found
        if delimiter_count > 0:
            if not column_names:
                column_names = get_line_elements(line)
                if not column_names:
                    print ("Err: parsing table header line #{}: {}".format(table_ofset + idx, line))
                    return None
                delimiter_count = 0
                continue
            elif column_names:
                line_arr = get_line_elements(line)
                if len(line_arr) != len(column_names):
                    print ("Err: patch table wrong format linex #{}: {}".format(table_ofset + idx, line))
                    return None
                else:
                    table_line = dict(zip(column_names, line_arr))
                    table.append(table_line)
    return table


class Data:
    # list of new upstream patches
    new_up = list()
    # list of new non-upstream patches
    new_non_up = list()
    # old upstream patches
    old_up_patches = list()
    # current series file raw data
    old_series = list()
    # current non-upstream patch list
    old_non_up = list()
    # New series file written by hw_mgmt integration script
    new_series = list()
    # index of the mlnx_hw_mgmt patches start marker in old_series
    i_mlnx_start = -1 
    # index of the mlnx_hw_mgmt patches end marker in old_series
    i_mlnx_end = -1
    # Updated sonic-linux-kernel/patch/series file contents
    up_slk_series = list()
    # SLK series file content updated with non-upstream patches, used to generate diff
    agg_slk_series = list()
    # kernel version
    k_ver = ""


class HwMgmtAction(Action):

    @staticmethod
    def get(args):
        action = None
        if args.action.lower() == "pre":
            action = PreProcess(args)
        elif args.action.lower() == "post":
            action = PostProcess(args)
        
        if not action.check():
            print("-> ERR: Argument Checks Failed")
            sys.exit(1)

        return action

    def return_false(self, str_):
        print(str_)
        return False

    def check(self):
        if not self.args.kernel_version:
            return self.return_false("-> ERR: Kernel Version is missing")
        if not self.args.build_root:
            return self.return_false("-> ERR: build_root is missing")
        if not os.path.exists(self.args.build_root):
            return self.return_false("-> ERR: Build Root {} doesn't exist".format(self.args.build_root))
        if not os.path.isfile(self.args.config_base_amd):
            return self.return_false("-> ERR: config_base {} doesn't exist".format(self.args.config_base_amd))
        if not os.path.isfile(self.args.config_base_arm):
            return self.return_false("-> ERR: config_base_arm {} doesn't exist".format(self.args.config_base_arm))
        if not os.path.isfile(self.args.config_inc_amd):
            return self.return_false("-> ERR: config_inclusion {} doesn't exist".format(self.args.config_inc_amd))
        if not os.path.isfile(self.args.config_inc_arm):
            return self.return_false("-> ERR: config_inclusion {} doesn't exist".format(self.args.config_inc_arm))
        return True


class PreProcess(HwMgmtAction):
    def __init__(self, args):
        super().__init__(args)

    def check(self):
        return super(PreProcess, self).check()

    def perform(self):
        """ Move Base Kconfig to the loc pointed by config_inclusion """
        shutil.copy2(self.args.config_base_amd, self.args.config_inc_amd)
        shutil.copy2(self.args.config_base_arm, self.args.config_inc_arm)
        print("-> Kconfig amd64/arm64 copied to the relevant directory")
    

class PostProcess(HwMgmtAction):
    def __init__(self, args):
        super().__init__(args)
        self.kcfg_handler = KConfigTask(self.args)
    
    def check(self):
        if not super(PostProcess, self).check():
            return False
        if not (self.args.patches and os.path.exists(self.args.patches)):
            return self.return_false("-> ERR: upstream patch directory is missing ")
        if not (self.args.non_up_patches and os.path.exists(self.args.non_up_patches)):
            return self.return_false("-> ERR: non upstream patch directory is missing")
        if not (self.args.series and os.path.isfile(self.args.series)):
            return self.return_false("-> ERR: series file doesn't exist {}".format(self.args.series))
        if not (self.args.current_non_up_patches and os.path.exists(self.args.current_non_up_patches)):
            return self.return_false("-> ERR: current non_up_patches doesn't exist {}".format(self.args.current_non_up_patches))
        return True

    def read_data(self):
        # Read the data written by hw-mgmt script into the internal Data Structures
        Data.new_series = FileHandler.read_strip_minimal(self.args.series)
        Data.old_series = FileHandler.read_raw(os.path.join(self.args.build_root, SLK_SERIES))
        Data.old_non_up = FileHandler.read_strip_minimal(self.args.current_non_up_patches)

        new_up = set(FileHandler.read_dir(self.args.patches, "*.patch"))
        new_non_up = set(FileHandler.read_dir(self.args.non_up_patches, "*.patch"))

        for patch in Data.new_series:
            if patch in new_up:
                Data.new_up.append(patch)
            elif patch in new_non_up:
                Data.new_non_up.append(patch)
            else:
                print("-> FATAL: Patch {} not found either in upstream or non-upstream list".format(patch))
                if not self.args.is_test:
                    sys.exit(1)
        Data.k_ver = get_kver(self.args.kernel_version)

    def find_mlnx_hw_mgmt_markers(self):
        """ Find the indexes where the current mlnx patches sits in SLK_SERIES file """
        (Data.i_mlnx_start, Data.i_mlnx_end) = FileHandler.find_marker_indices(Data.old_series, marker=HW_MGMT_MARKER)
        if Data.i_mlnx_start < 0 or Data.i_mlnx_end > len(Data.old_series):
            print("-> FATAL mellanox_hw_mgmt markers not found. Couldn't continue.. exiting")
            sys.exit()

    def rm_old_up_mlnx(self):
        """ Delete the old mlnx upstream patches """
        print("\n -> POST: Removed the following upstream patches:")
        index = Data.i_mlnx_start
        while index <= Data.i_mlnx_end:
            file_n = os.path.join(self.args.build_root, os.path.join(SLK_PATCH_LOC, Data.old_series[index].strip()))
            if os.path.isfile(file_n):
                print(Data.old_series[index].strip())
                os.remove(file_n)
            index = index + 1

    def mv_new_up_mlnx(self):
        for patch in Data.new_up:
            src_path = os.path.join(self.args.patches, patch)
            shutil.copy(src_path, os.path.join(self.args.build_root, SLK_PATCH_LOC))

    def write_final_slk_series(self):
        tmp_new_up = [d+"\n" for d in Data.new_up]
        Data.up_slk_series = Data.old_series[0:Data.i_mlnx_start+1] + tmp_new_up + Data.old_series[Data.i_mlnx_end:]
        print("\n -> POST: Updated sonic-linux-kernel/series file: \n{}".format("".join(Data.up_slk_series)))
        FileHandler.write_lines(os.path.join(self.args.build_root, SLK_SERIES), Data.up_slk_series, True)

    def rm_old_non_up_mlnx(self):
        """ Remove the old non-upstream patches 
        """
        print("\n -> POST: Removed the following supposedly non-upstream patches:")
        # Remove all the old patches and any patches that got accepted with this kernel version
        for patch in Data.old_non_up + Data.new_up:
            file_n = os.path.join(self.args.build_root, os.path.join(NON_UP_PATCH_LOC, patch))
            if os.path.isfile(file_n):
                print(patch)
                os.remove(file_n)

        # Make sure the dir is now empty as all these patches are of hw-mgmt
        files = FileHandler.read_dir(os.path.join(self.args.build_root, NON_UP_PATCH_LOC), "*.patch")
        if files:
            # TODO: When there are SDK non-upstream patches, the logic has to be updated
            print("\n -> FATAL: Patches Remaining in {}: \n{}".format(NON_UP_PATCH_LOC, files))
            sys.exit(1)
        
    def mv_new_non_up_mlnx(self):
        dest_fpath = os.path.join(self.args.build_root, NON_UP_PATCH_LOC)
        if not Data.new_non_up and os.path.exists(dest_fpath):
            os.rmdir(dest_fpath)
            return

        for patch in Data.new_non_up:
            src_path = os.path.join(self.args.non_up_patches, patch)
            os.makedirs(dest_fpath, exist_ok=True)
            shutil.copy(src_path, dest_fpath)
    
    def construct_series_with_non_up(self):
        Data.agg_slk_series = copy.deepcopy(Data.up_slk_series) 
        lines = ["# Current non-upstream patch list, should be updated by hwmgmt_kernel_patches.py script"]
        for index, patch in enumerate(Data.new_series):
            patch = patch + "\n"
            if patch not in Data.agg_slk_series:
                if index == 0:
                    # if the first patch is a non-upstream patch, then use the marker as the prev index
                    prev_patch = Data.old_series[Data.i_mlnx_start]
                else:    
                    prev_patch = Data.new_series[index-1] + "\n"
                if prev_patch not in Data.agg_slk_series:
                    print("\n -> FATAL: ERR: patch {} is not found in agg_slk_series list: \n {}".format(prev_patch, "".join(Data.agg_slk_series)))
                    sys.exit(1)
                index_prev_patch =  Data.agg_slk_series.index(prev_patch)
                if index_prev_patch < len(Data.agg_slk_series) - 1:
                    Data.agg_slk_series = Data.agg_slk_series[0:index_prev_patch+1] + [patch] +  Data.agg_slk_series[index_prev_patch + 1:]
                else:
                    Data.agg_slk_series = Data.agg_slk_series + [patch]
                print("\n -> INFO: patch {} added to agg_slk_series:".format(patch.strip()))
                lines.append(patch.strip())

        # Update the non_up_current_patch_list file
        FileHandler.write_lines(self.args.current_non_up_patches, lines)
        print("\n -> POST: series file updated with non-upstream patches \n{}".format("".join(Data.agg_slk_series)))

    def get_series_diff(self):
        diff = difflib.unified_diff(Data.up_slk_series, Data.agg_slk_series, fromfile='a/patches-sonic/series', tofile="b/patches-sonic/series", lineterm="\n")
        lines = []
        for line in diff:
            lines.append(line)
        print("\n -> POST: final series.diff \n{}".format("".join(lines)))
        return lines
    
    def get_merged_diff(self, series_diff: list, kcfg_diff: list) -> list:
        if series_diff or kcfg_diff:
            return series_diff + ["\n"] + kcfg_diff
        else:
            return []

    def write_non_up_diff(self, series_diff, kcfg_diff):
        final_diff = self.get_merged_diff(series_diff, kcfg_diff)
        non_up_diff_path = os.path.join(self.args.build_root, NON_UP_DIFF)
        if "".join(final_diff).strip():
            # don't write anything if the diff just contains whitespaces
            FileHandler.write_lines(non_up_diff_path, final_diff, True)
        else:
            # if nothing to write, delete the patch file
            try:
                os.remove(non_up_diff_path)
            except OSError as error:
                print("-> NOTICE File {} remove failed with error {}".format(non_up_diff_path, error))

    def list_patches(self):
        old_up_patches = []
        for i in range(Data.i_mlnx_start, Data.i_mlnx_end):
            old_up_patches.append(Data.old_series[i].strip())
        old_non_up_patches = [ptch.strip() for ptch in Data.old_non_up]
        return old_up_patches, old_non_up_patches

    def _get_patchwork(self, patch_name):
        root_p = os.path.join(self.args.build_root, PATCH_TABLE_LOC)
        patchwork_loc =  PATCHWORK_LOC.format(Data.k_ver)
        file_dir = os.path.join(root_p, patchwork_loc)
        file_loc = os.path.join(file_dir, f"{patch_name}.txt")
        if not os.path.exists(file_loc):
            return ""
        print(f"-> INFO: Patchwork file {file_loc} is present")
        lines = FileHandler.read_strip(file_loc)
        for line in lines:
            if "patchwork_link" not in line:
                continue
            tokens = line.split(":")
            if len(tokens) < 2:
                print(f"-> WARN: Invalid entry {line}, did not follow <key>:<value>")
                continue
            key = tokens[0]
            values = tokens[1:]
            if key == "patchwork_link":
                desc = ":".join(values)
                desc = desc.strip()
                print(f"-> INFO: Patch work link for patch {patch_name} : {desc}")
                return desc
        return ""

    def _fetch_description(self, patch, id_):
        desc = parse_id(id_)
        if not desc:
            # not an upstream patch, check if the patchwork link is present and fetch it
            desc = self._get_patchwork(patch)
        return desc

    def create_commit_msg(self, table):
        title = COMMIT_TITLE.format(self.args.hw_mgmt_ver) 
        changes_slk, changes_sb = {}, {}
        old_up_patches, old_non_up_patches = self.list_patches()
        print(old_up_patches)
        for patch in table:
            patch_ = patch.get(PATCH_NAME)
            id_ = self._fetch_description(patch_, patch.get(COMMIT_ID, ""))
            if patch_ in Data.new_up and patch_ not in old_up_patches:
                changes_slk[patch_] = id_
                print(f"-> INFO: Patch: {patch_}, Commit: {id_}, added to linux-kernel description")
            elif patch_ in Data.new_non_up and patch_ not in old_non_up_patches:
                changes_sb[patch_] = id_
                print(f"-> INFO: Patch: {patch_}, Commit: {id_}, added to buildimage description")
            else:
                print(f"-> INFO: Patch: {patch_}, Commit: {id_}, is not added")
        slk_commit_msg = title + "\n" + build_commit_description(changes_slk)
        sb_commit_msg = title + "\n" + build_commit_description(changes_sb)
        print(f"-> INFO: SLK Commit Message: \n {slk_commit_msg}")
        print(f"-> INFO: SB Commit Message: \n {sb_commit_msg}")
        return sb_commit_msg, slk_commit_msg

    def perform(self):
        """ Read the data output from the deploy_kernel_patches.py script 
            and move to appropriate locations """
        # Handle Patches related logic
        self.read_data()
        self.find_mlnx_hw_mgmt_markers()
        self.rm_old_up_mlnx()
        self.mv_new_up_mlnx()
        self.write_final_slk_series()
        # Handle Non Upstream patches
        self.rm_old_non_up_mlnx()
        self.mv_new_non_up_mlnx()
        self.construct_series_with_non_up()
        series_diff = self.get_series_diff()
        # handle kconfig and get any diff
        kcfg_diff = self.kcfg_handler.perform()
        self.write_non_up_diff(series_diff, kcfg_diff)

        path = os.path.join(self.args.build_root, PATCH_TABLE_LOC)
        patch_table = load_patch_table(path, Data.k_ver)
        
        sb_msg, slk_msg = self.create_commit_msg(patch_table)

        if self.args.sb_msg and sb_msg:
            with open(self.args.sb_msg, 'w') as f:
                f.write(sb_msg)

        if self.args.slk_msg:
            with open(self.args.slk_msg, 'w') as f:
                f.write(slk_msg) 
        


def create_parser():
    # Create argument parser
    parser = argparse.ArgumentParser()

    # Positional mandatory arguments
    parser.add_argument("action", type=str, choices=["pre", "post"])

    # Optional arguments
    parser.add_argument("--patches", type=str)
    parser.add_argument("--non_up_patches", type=str)
    parser.add_argument("--config_base_amd", type=str, required=True)
    parser.add_argument("--config_base_arm", type=str, required=True)
    parser.add_argument("--config_inc_amd", type=str, required=True)
    parser.add_argument("--config_inc_arm", type=str, required=True)
    parser.add_argument("--config_inc_down_amd", type=str)
    parser.add_argument("--config_inc_down_arm", type=str)
    parser.add_argument("--series", type=str)
    parser.add_argument("--current_non_up_patches", type=str)
    parser.add_argument("--build_root", type=str)
    parser.add_argument("--hw_mgmt_ver", type=str, required=True)
    parser.add_argument("--kernel_version", type=str, required=True)
    parser.add_argument("--sb_msg", type=str, required=False, default="")
    parser.add_argument("--slk_msg", type=str, required=False, default="")
    parser.add_argument("--is_test", action="store_true")
    return parser


if __name__ == '__main__':
    parser = create_parser()
    action = HwMgmtAction.get(parser.parse_args())
    action.perform()
