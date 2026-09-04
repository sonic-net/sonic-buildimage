#!/usr/bin/env python
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

from hwmgmt_helper import *

COMMIT_TITLE = "Integrate HW-MGMT {} Changes"

PATCH_TABLE_LOC = HWMGMT_LINUX_DIR
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

def load_patch_table(path, k_ver, table_name=None):
    table_name = table_name or PATCH_TABLE_NAME
    patch_table_filename = os.path.join(path, table_name)
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

    # Extract patch table for specified kernel version, skipping comment and blank lines
    kversion_line = "Kernel-{}".format(k_ver)
    table_ofset = 0
    for table_ofset, line in enumerate(patch_table_lines):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if stripped == kversion_line:
            break

    # if kernel version not found
    if table_ofset >= len(patch_table_lines)-5:
        print ("Err: kernel version {} not found in {}".format(k_ver, patch_table_filename))
        return None

    table = []
    delimiter_count = 0
    column_names = None
    for idx, line in enumerate(patch_table_lines[table_ofset:]):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if PATCH_TABLE_DELIMITER in line:
            delimiter_count += 1
            if delimiter_count >= 3:
                print ("Err: too much leading delimers line #{}: {}".format(table_ofset + idx, line))
                return None
            elif table or delimiter_count == 2:
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
                if not line_arr or len(line_arr) != len(column_names):
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
    # BMC-only patch names (inserted between nvidia_aspeed_bmc markers in series)
    bmc_patches = list()


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
        if not os.path.isfile(self.args.config_base_aspeed):
            return self.return_false("-> ERR: config_base_aspeed {} doesn't exist".format(self.args.config_base_aspeed))
        if not os.path.isfile(self.args.config_inc_aspeed):
            return self.return_false("-> ERR: config_inc_aspeed {} doesn't exist".format(self.args.config_inc_aspeed))
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
        shutil.copy2(self.args.config_base_aspeed, self.args.config_inc_aspeed)
        print("-> Kconfig amd64/arm64/aspeed copied to the relevant directory")
    

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
        if self.args.bmc_patches and not os.path.isfile(self.args.bmc_patches):
            return self.return_false("-> ERR: bmc_patches list doesn't exist {}".format(self.args.bmc_patches))
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
        (Data.i_mlnx_start, Data.i_mlnx_end) = FileHandler.find_marker_indices_checked(
            Data.old_series, HW_MGMT_MARKER, SLK_SERIES)

    @staticmethod
    def _series_entry(line):
        """ Patch file named by a series line, None for blank and comment lines """
        line = line.strip()
        return None if not line or line.startswith("#") else line.split()[0]

    def _rm_series_files(self, series, start, end):
        for line in series[start:end]:
            name = self._series_entry(line)
            file_n = os.path.join(self.args.build_root, SLK_PATCH_LOC, name) if name else None
            if file_n and os.path.isfile(file_n):
                print(name)
                os.remove(file_n)

    def rm_old_up_mlnx(self):
        """ Delete the old mlnx upstream patches """
        print("\n -> POST: Removed the following upstream patches:")
        self._rm_series_files(Data.old_series, Data.i_mlnx_start, Data.i_mlnx_end + 1)

    def mv_new_up_mlnx(self):
        for patch in Data.new_up:
            src_path = os.path.join(self.args.patches, patch)
            shutil.copy(src_path, os.path.join(self.args.build_root, SLK_PATCH_LOC))

    def bmc_block_managed(self) -> bool:
        """ The nvidia_aspeed_bmc series block is rewritten only when --bmc_patches is given (the
            Makefile always gives it; check() has verified the file); a legacy/manual run without
            it leaves the block alone, so its markers are not required either. """
        return bool(self.args.bmc_patches)

    def read_bmc_patches(self):
        """ Read BMC-only patch list and separate them from the standard hw-mgmt patches. """
        if not self.bmc_block_managed():
            return
        raw = FileHandler.read_strip_minimal(self.args.bmc_patches)
        Data.bmc_patches = [p + "\n" for p in raw]
        if Data.bmc_patches:
            bmc_set = set(raw)
            for patch in sorted(bmc_set - set(Data.new_up) - set(Data.new_non_up)):
                print("-> FATAL: BMC patch {} not found either in upstream or non-upstream list".format(patch))
                if not self.args.is_test:
                    sys.exit(1)
            # Remove BMC patches from standard lists so they don't go into mellanox_hw_mgmt block
            Data.new_up = [p for p in Data.new_up if p not in bmc_set]
            Data.new_non_up = [p for p in Data.new_non_up if p not in bmc_set]
            Data.new_series = [p for p in Data.new_series if p not in bmc_set]
            print("\n -> POST: {} BMC patches separated:\n{}".format(
                len(Data.bmc_patches), "".join(Data.bmc_patches)))

    def find_bmc_markers(self, series):
        """ Return (start, end) of nvidia_aspeed_bmc markers in series; exit if absent. """
        return FileHandler.find_marker_indices_checked(series, MLNX_ASPEED_MARKER, SLK_SERIES)

    def rm_old_bmc_patches(self):
        """ Delete old BMC patch files from patches-sonic/ (mirrors rm_old_up_mlnx). """
        series_path = os.path.join(self.args.build_root, SLK_SERIES)
        old_series = FileHandler.read_raw(series_path)
        start, end = self.find_bmc_markers(old_series)
        print("\n -> POST: Removed the following old BMC patches:")
        self._rm_series_files(old_series, start + 1, end)

    def write_bmc_series_block(self):
        """ Rebuild the nvidia_aspeed_bmc block in the series file from Data.bmc_patches.
            Mirrors the mellanox_hw_mgmt cleanup flow: rebuilding unconditionally ensures
            patches dropped by hw-mgmt (including a full drop of BMC support) are cleared
            from both the series file and patches-sonic/.
        """
        if not self.bmc_block_managed():
            return

        # Remove old BMC patch files first (same pattern as rm_old_up_mlnx)
        self.rm_old_bmc_patches()

        # 1. Update in-memory series (written by write_final_slk_series earlier) between markers
        start, end = self.find_bmc_markers(Data.up_slk_series)
        Data.up_slk_series = FileHandler.insert_lines(Data.up_slk_series, start, end, Data.bmc_patches)

        start, end = self.find_bmc_markers(Data.agg_slk_series)
        Data.agg_slk_series = FileHandler.insert_lines(Data.agg_slk_series, start, end, Data.bmc_patches)

        # 2. Write updated series to disk (block may be empty; that clears stale content)
        series_path = os.path.join(self.args.build_root, SLK_SERIES)
        FileHandler.write_lines(series_path, Data.up_slk_series, True)
        print("\n -> POST: nvidia_aspeed_bmc block written to series ({} patches)".format(len(Data.bmc_patches)))

        # 3. Copy BMC patches to patches-sonic/ (no-op when Data.bmc_patches is empty)
        for patch in Data.bmc_patches:
            name = patch.strip()
            src = os.path.join(self.args.patches, name)
            if not os.path.isfile(src):
                # candidate folder: the row is plain "Downstream" in Patch_BMC_Status_Table.txt
                src = os.path.join(self.args.non_up_patches, name)
                if os.path.isfile(src):
                    print("-> WARNING: BMC patch {} is marked Downstream (candidate) in Patch_BMC_Status_Table.txt "
                          "but taken into patches-sonic/; relabel it 'Downstream accepted'".format(name))
            if os.path.isfile(src):
                shutil.copy(src, os.path.join(self.args.build_root, SLK_PATCH_LOC))

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

    def create_commit_msg(self, table, bmc_table=None):
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

        # Append BMC patch list to the SLK commit message from Patch_BMC_Status_Table.txt
        if bmc_table:
            bmc_changes = {}
            for patch in bmc_table:
                patch_ = patch.get(PATCH_NAME)
                if patch_:
                    id_ = self._fetch_description(patch_, patch.get(COMMIT_ID, ""))
                    bmc_changes[patch_] = id_
            if bmc_changes:
                slk_commit_msg = slk_commit_msg + build_commit_description(bmc_changes, "BMC Patch List")

        print(f"-> INFO: SLK Commit Message: \n {slk_commit_msg}")
        print(f"-> INFO: SB Commit Message: \n {sb_commit_msg}")
        return sb_commit_msg, slk_commit_msg

    def report_kcfg_findings(self) -> bool:
        """ Print the reconcile report; returns True when it must stop the run: conflicts without
            --force-overwrite, or a broken hw-mgmt kconfig table, which no switch overrides. """
        print("\n -> POST: kconfig reconcile report:\n{}".format("\n".join(self.kcfg_handler.format_findings())))
        fatal = False
        conflicts = self.kcfg_handler.conflicts()
        if conflicts:
            if self.args.force_overwrite:
                print("-> WARNING: {} kconfig conflict(s) overridden (HWMGMT_KCFG_FORCE_OVERWRITE=y)".format(len(conflicts)))
            else:
                print("-> FATAL: {} kconfig conflict(s), see report above; fix them or rerun with "
                      "HWMGMT_KCFG_FORCE_OVERWRITE=y".format(len(conflicts)))
                fatal = True
        for error in self.kcfg_handler.table_errors:
            # a broken table, not a value conflict: HWMGMT_KCFG_FORCE_OVERWRITE=y does not apply
            print("-> FATAL: {}; fix the table (HWMGMT_KCFG_FORCE_OVERWRITE=y does not override this)".format(error))
            fatal = True
        return fatal

    def check_non_up_dir(self) -> bool:
        """ platform/mellanox/non-upstream-patches/patches/ must only hold the patches this script manages;
            a stray file would be committed or make the run stop half-written. Returns True on strays. """
        # TODO: When there are SDK non-upstream patches, the logic has to be updated
        # os.listdir, not read_dir: every entry, hidden files included, because mv_new_non_up_mlnx()
        # rmdir's the directory when the release has no non-upstream patches, which fails
        # half-written on anything else left in there
        loc = os.path.join(self.args.build_root, NON_UP_PATCH_LOC)
        present = set(os.listdir(loc)) if os.path.isdir(loc) else set()
        stray = sorted(present - set(p.strip() for p in Data.old_non_up) - set(Data.new_up) - set(Data.new_non_up))
        if stray:
            print("\n -> FATAL: patches in {} not listed by hw-mgmt or {}: {}".format(
                NON_UP_PATCH_LOC, os.path.basename(self.args.current_non_up_patches), ", ".join(stray)))
        # a symlink would be written through (the copy follows it), a directory fails half-written
        odd = sorted(n for n in present if os.path.islink(os.path.join(loc, n)) or not os.path.isfile(os.path.join(loc, n)))
        if odd:
            print("\n -> FATAL: not regular files in {}: {}".format(NON_UP_PATCH_LOC, ", ".join(odd)))
        return bool(stray or odd)

    @staticmethod
    def _series_names(series, ranges, inside):
        """ name -> first line number of the .patch entries inside (or outside) the (start, end) index ranges """
        names = OrderedDict()
        for idx, raw in enumerate(series):
            in_block = any(start >= 0 and start <= idx <= end for start, end in ranges)
            if in_block != inside:
                continue
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            name = line.split()[0]
            if name.endswith(".patch") and name not in names:
                names[name] = idx + 1
        return names

    def check_patch_name_collisions(self) -> bool:
        """ A patch name must belong to one series section only, and appear there once: fail before
            copying over or deleting a file another section lists. A patch hw-mgmt moved from its BMC
            table into the main table is one too: the old BMC cleanup would delete the fresh copy.
            Returns True on collisions. """
        hw_range = (Data.i_mlnx_start, Data.i_mlnx_end)
        bmc_start, bmc_end = FileHandler.find_marker_indices(Data.old_series, MLNX_ASPEED_MARKER)
        if not self.bmc_block_managed() or bmc_start < 0 or bmc_end >= len(Data.old_series):
            # BMC block is not rebuilt in this run: treat it as any other section
            bmc_start, bmc_end = -1, -1
        bmc_range = (bmc_start, bmc_end)
        in_hw = self._series_names(Data.old_series, [hw_range], True)
        in_bmc = self._series_names(Data.old_series, [bmc_range], True)
        outside = self._series_names(Data.old_series, [hw_range, bmc_range], False)
        new_hw = set(Data.new_up) | set(Data.new_non_up)
        new_bmc = set(p.strip() for p in Data.bmc_patches)

        problems = []
        for name in sorted(new_hw & set(in_bmc)):
            problems.append("hw-mgmt patch {} is still listed in the {} block (series line {}), move it by hand first".format(
                name, MLNX_ASPEED_MARKER, in_bmc[name]))
        for name in sorted(new_hw & set(outside)):
            problems.append("hw-mgmt patch {} is already referenced outside the {} block (series line {})".format(
                name, HW_MGMT_MARKER, outside[name]))
        for name in sorted(new_bmc & set(outside)):
            problems.append("BMC patch {} is already referenced outside the {} block (series line {})".format(
                name, MLNX_ASPEED_MARKER, outside[name]))
        for name in sorted((set(in_hw) | set(in_bmc)) & set(outside)):
            problems.append("patch {} would be deleted but is still referenced at series line {}".format(
                name, outside[name]))
        for label, names in ((HW_MGMT_MARKER, Data.new_up + Data.new_non_up),
                             (MLNX_ASPEED_MARKER, [p.strip() for p in Data.bmc_patches])):
            for name in sorted(set(n for n in names if names.count(n) > 1)):
                problems.append("{} is listed twice for the {} block, hw-mgmt's table has a duplicate row".format(name, label))
        if problems:
            print("\n -> " + "\n -> ".join("FATAL: patch name collision: " + p for p in problems))
        return bool(problems)

    def perform(self):
        """ Read the data output from the deploy_kernel_patches.py script
            and move to appropriate locations """
        # Handle Patches related logic
        self.read_data()
        # checks that can fail, before any file is touched
        path = os.path.join(self.args.build_root, PATCH_TABLE_LOC)
        patch_table = load_patch_table(path, Data.k_ver)
        if patch_table is None:
            print("-> FATAL: could not load {} for kernel {} from {}".format(PATCH_TABLE_NAME, Data.k_ver, path))
            sys.exit(1)
        # the BMC table is optional, but one that is there must load (it is read for the commit message)
        bmc_table = None
        if os.path.isfile(os.path.join(path, BMC_PATCH_TABLE_NAME)):
            bmc_table = load_patch_table(path, Data.k_ver, BMC_PATCH_TABLE_NAME)
            if bmc_table is None:
                print("-> FATAL: could not load {} for kernel {} from {}".format(BMC_PATCH_TABLE_NAME, Data.k_ver, path))
                sys.exit(1)
        # Separate BMC patches so they don't go into the mellanox_hw_mgmt block
        self.read_bmc_patches()
        # every marker block is rewritten from its markers, so check them all before the first delete
        self.find_mlnx_hw_mgmt_markers()
        if self.bmc_block_managed():
            self.find_bmc_markers(Data.old_series)
        self.kcfg_handler.analyze()
        self.kcfg_handler.check_markers()
        fatal = self.report_kcfg_findings()
        fatal = self.check_patch_name_collisions() or fatal
        fatal = self.check_non_up_dir() or fatal
        if fatal:
            print("-> FATAL: nothing written")
            sys.exit(1)
        self.rm_old_up_mlnx()
        self.mv_new_up_mlnx()
        self.write_final_slk_series()
        # Handle Non Upstream patches
        self.rm_old_non_up_mlnx()
        self.mv_new_non_up_mlnx()
        self.construct_series_with_non_up()
        # Insert BMC patches between nvidia_aspeed_bmc markers in the series file
        self.write_bmc_series_block()
        series_diff = self.get_series_diff()
        # write kconfig and get any diff
        kcfg_diff = self.kcfg_handler.write()
        self.write_non_up_diff(series_diff, kcfg_diff)

        sb_msg, slk_msg = self.create_commit_msg(patch_table, bmc_table)

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
    parser.add_argument("--config_base_aspeed", type=str, required=True)
    parser.add_argument("--config_inc_aspeed", type=str, required=True)
    parser.add_argument("--series", type=str)
    parser.add_argument("--current_non_up_patches", type=str)
    parser.add_argument("--build_root", type=str)
    parser.add_argument("--hw_mgmt_ver", type=str, required=True)
    parser.add_argument("--kernel_version", type=str, required=True)
    parser.add_argument("--sb_msg", type=str, required=False, default="")
    parser.add_argument("--slk_msg", type=str, required=False, default="")
    parser.add_argument("--bmc_patches", type=str, required=False, default="",
                        help="File listing BMC-only patch names (one per line).")
    parser.add_argument("--force-overwrite", dest="force_overwrite", action="store_true",
                        help="Do not fail on kconfig conflicts, write hw-mgmt's values anyway.")
    parser.add_argument("--is_test", action="store_true")
    return parser


if __name__ == '__main__':
    parser = create_parser()
    action = HwMgmtAction.get(parser.parse_args())
    action.perform()
