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

import os
import io
import re
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

# Reconcile result: key/value to be written into `target` block and the other definitions of the key;
# `after` are the differing hits below the block in its own file, the lines that win over the block in the merge
Finding = namedtuple("Finding", ["target", "kind", "key", "value", "hits", "after"], defaults=((),))
DUPLICATE = "DUPLICATE"  # same value in a loose line outside the block: not written
REDUNDANT = "REDUNDANT"  # same value, but only inside another tool's block: kept
CONFLICT = "CONFLICT"
DOWNSTREAM = "DOWNSTREAM"  # downstream entry, another value outside the block: kept, not a conflict
# deploy_kernel_patches.py reads the hw-mgmt kconfig table with "#\s*(\S+) is not set": its grammar, not Debian's
HWMGMT_UNSET_RE = re.compile(r"^#\s*(CONFIG_\w+) is not set\b")


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
    # Aspeed BMC kconfig (populated when --config_base_aspeed is provided)
    aspeed_base = OrderedDict()
    aspeed_updated = OrderedDict()
    aspeed_incl = OrderedDict()
    aspeed_excl = OrderedDict()


HWMGMT_KCONFIG_SECTIONS = ("amd64:upstream", "amd64:downstream", "arm64:upstream", "arm64:downstream",
                           "aspeed:upstream", "aspeed:downstream")


class KConfigTask():
    def __init__(self, args):
        self.args = args
        self.findings = []
        self.table_errors = []
        self.has_standard_changes = False


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

        KCFGData.aspeed_base = FileHandler.read_kconfig(self.args.config_base_aspeed)
        KCFGData.aspeed_updated = FileHandler.read_kconfig(self.args.config_inc_aspeed)
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
        # Filter the common inc/excl out from the arch specific inc/excl.
        # The keys moved here leave the per-arch sets, so analyze() reconciles them in the common
        # block's scope only (its own file; the arm64 deploy that would fill it is disabled). The
        # exclusion and downstream branches match on key only: a downstream key whose amd64 and
        # arm64 values differ is moved with the amd64 value.
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


    def aspeed_compat_skip(self, config: list) -> bool:
        """ True for a sonic-linux-kernel that predates BMC support: no nvidia_aspeed_bmc block at
            all and nothing to write into one. A half-present or reversed marker pair is a broken
            file, so it is not a compat case and stays fatal. """
        if KCFGData.aspeed_incl or KCFGData.aspeed_excl:
            return False
        (i_start, i_end) = FileHandler.find_marker_indices(config, MLNX_ASPEED_MARKER)
        return FileHandler.marker_block_absent(config, i_start, i_end)

    def get_aspeed_kconfig(self):
        """ Rebuild the nvidia_aspeed_bmc block in config.sonic-aspeed.
            Returns the rebuilt config lines. The block is rebuilt unconditionally so that
            configs dropped by hw-mgmt (including a full drop of BMC support) are cleared.
            Returns None on the compat case above; check_markers() has already failed the run
            for any other broken marker block.
        """
        aspeed_config_path = os.path.join(self.args.build_root, SLK_KCONFIG_ASPEED)
        config = FileHandler.read_raw(aspeed_config_path)

        if self.aspeed_compat_skip(config):
            print("-> NOTICE: {} markers not in {}, skipping aspeed kconfig update".format(
                MLNX_ASPEED_MARKER, SLK_KCONFIG_ASPEED))
            return None
        start, end = FileHandler.find_marker_indices_checked(config, MLNX_ASPEED_MARKER, SLK_KCONFIG_ASPEED)

        final = OrderedDict(
            list(KCFGData.aspeed_incl.items()) +
            [(k, "n") for k in KCFGData.aspeed_excl.keys()]
        )
        config = FileHandler.insert_kcfg_data(config, start, end, final)
        print("\n -> INFO: aspeed kconfig file generated \n {}".format("".join(config)))
        return config


    @staticmethod
    def scan_files(files: list) -> list:
        """ Entries of (path, managed marker or None) files; a managed block is rewritten by this run
            and skipped. """
        entries = []
        for path, managed in files:
            if not os.path.isfile(path):
                print("-> NOTICE: {} not found, not considered for kconfig reconcile".format(path))
            elif managed:
                entries += FileHandler.entries_outside_block(path, managed)
            else:
                entries += FileHandler.read_kconfig_entries(path)
        return entries

    def reconcile(self, final: OrderedDict, target: str, target_path: str, marker: str, other_files: list,
                  override_keys=(), downstream=False) -> tuple:
        """ Compare final with the same file outside the markers and with other_files, the fragments
            the kernel build merges before it, each (path, managed marker or None).
            Same key, another value anywhere: CONFLICT, kept.
            Same value everywhere: DUPLICATE, dropped, but only if one of the hits is a loose line;
            a line inside another tool's block can vanish when that tool runs, so it does not carry
            our value for us: REDUNDANT, kept with a NOTICE.
            override_keys are downstream lines that override the block's own upstream value: never
            dropped, the block would be left at the upstream value.
            downstream: final goes to external-changes.patch, into this block, and reaches the kernel
            only with INCLUDE_EXTERNAL_PATCHES=y. Differing from a loose line is what such an
            override is for, so another value is DOWNSTREAM, kept, not a conflict; the report says
            whether that line is below the block, where it wins over the override. """
        others = FileHandler.entries_outside_block(target_path, marker) + self.scan_files(other_files)
        _, end = FileHandler.find_marker_indices(FileHandler.read_raw(target_path), marker)
        kept, findings = OrderedDict(), []
        for key, val in final.items():
            hits = [e for e in others if e.key == key]
            differing = [e for e in hits if e.value != val]
            if differing:
                after = tuple(e for e in differing if e.path == target_path and e.lineno - 1 > end)
                findings.append(Finding(target, DOWNSTREAM if downstream else CONFLICT, key, val, hits, after))
            elif hits and key not in override_keys:
                if any(e.owner is None for e in hits):
                    findings.append(Finding(target, DUPLICATE, key, val, hits))
                    continue
                findings.append(Finding(target, REDUNDANT, key, val, hits))
            kept[key] = val
        return kept, findings

    def _reconcile_block(self, target, incl: OrderedDict, excl: OrderedDict, target_path, marker, other_files,
                         override_keys=(), downstream=False) -> list:
        # Reconcile incl (key=value) + excl (key=n); duplicates are dropped from both dicts in place
        final = OrderedDict(list(incl.items()) + [(k, "n") for k in excl.keys()])
        kept, findings = self.reconcile(final, target, target_path, marker, other_files, override_keys, downstream)
        for key in final.keys():
            if key not in kept:
                incl.pop(key, None)
                excl.pop(key, None)
        return findings

    def conflicts(self) -> list:
        return [f for f in self.findings if f.kind == CONFLICT]

    def format_findings(self) -> list:
        base = os.path.join(self.args.build_root, SLK_KCONFIG_DIR)

        def where(entry):
            at = "{}:{}".format(os.path.relpath(entry.path, base), entry.lineno)
            return "{} (inside {} block)".format(at, entry.owner) if entry.owner else at

        lines = []
        for f in self.findings:
            for e in f.hits:
                if f.kind == DUPLICATE:
                    lines.append("DUPLICATE [{}] {}={} already set by {} -> not written".format(f.target, f.key, f.value, where(e)))
                elif f.kind == REDUNDANT:
                    lines.append("NOTICE    [{}] {}={} also set by {} -> kept".format(f.target, f.key, f.value, where(e)))
                elif e.value == f.value:
                    continue
                elif f.kind == DOWNSTREAM and e in f.after:
                    lines.append("WARNING   [{}] {}={} differs from {} ={} -> kept, but that line comes after the block "
                                 "and wins when INCLUDE_EXTERNAL_PATCHES=y".format(f.target, f.key, f.value, where(e), e.value))
                elif f.kind == DOWNSTREAM:
                    # the block wins over this line, unless another line below the block (WARNING above) wins over both
                    wins = "" if f.after else ", where the block comes after that line and wins"
                    lines.append("NOTICE    [{}] {}={} differs from {} ={} -> kept, applies only with INCLUDE_EXTERNAL_PATCHES=y{}".format(
                        f.target, f.key, f.value, where(e), e.value, wins))
                else:
                    wins = ", that line comes after the block and wins even with HWMGMT_KCFG_FORCE_OVERWRITE=y" if e in f.after else ""
                    lines.append("CONFLICT  [{}] {}: block={} vs {} ={}{}".format(f.target, f.key, f.value, where(e), e.value, wins))
        n_dup = sum(1 for f in self.findings if f.kind == DUPLICATE)
        n_kept = sum(1 for f in self.findings if f.kind == REDUNDANT)
        n_down = sum(1 for f in self.findings if f.kind == DOWNSTREAM)
        n_lost = sum(1 for f in self.findings if f.kind == DOWNSTREAM and f.after)
        summary = "kconfig reconcile: {} duplicate(s) not written, {} conflict(s)".format(n_dup, len(self.conflicts()))
        if n_kept:
            summary += ", {} same-value line(s) kept".format(n_kept)
        if n_down:
            summary += ", {} downstream difference(s) kept".format(n_down)
        if n_lost:
            summary += " ({} overridden by a line below the block)".format(n_lost)
        lines.append(summary)
        return lines

    def hwmgmt_kconfig_txt(self) -> str:
        major, minor = self.args.kernel_version.split(".")[0:2]
        return os.path.join(self.args.build_root, HWMGMT_LINUX_DIR, "kconfig_{}_{}.txt".format(major, minor))

    @staticmethod
    def iter_hwmgmt_kconfig_txt(path):
        """ Yields (section, KcfgEntry) in file order, None entry for a section header.
            Lines are classified like deploy_kernel_patches.py does: a line containing '=' is only
            ever an assignment, so '# CONFIG_X is not set because CONFIG_Y=n' is a comment. """
        current = None
        for lineno, raw in enumerate(FileHandler.read_raw(path), 1):
            line = raw.strip()
            m = re.match(r".*\[([a-z0-9:]+)\]", line)
            if m:
                current = m.group(1)
                yield current, None
                continue
            if current is None:
                continue
            if "=" in line:
                m = re.match(r"^(CONFIG_\w+)=(\S+)", line)
                if m:
                    yield current, KcfgEntry(path, lineno, m.group(1), m.group(2))
            elif "is not set" in line:
                m = HWMGMT_UNSET_RE.match(line)
                if m:
                    yield current, KcfgEntry(path, lineno, m.group(1), "n")

    @staticmethod
    def read_hwmgmt_kconfig_txt(path) -> OrderedDict:
        # {"amd64:upstream": OrderedDict(key -> value), ...}; "# CONFIG_X is not set" -> "n"; last value wins like deploy
        sections = OrderedDict()
        for section, entry in KConfigTask.iter_hwmgmt_kconfig_txt(path):
            sections.setdefault(section, OrderedDict())
            if entry:
                sections[section][entry.key] = entry.value
        return sections

    @staticmethod
    def hwmgmt_kconfig_repeats(path) -> list:
        """ A key set more than once with different values inside one section of the hw-mgmt kconfig
            table (deploy silently applies the last one), as messages. """
        seen = OrderedDict()
        for section, entry in KConfigTask.iter_hwmgmt_kconfig_txt(path):
            if entry:
                seen.setdefault((section, entry.key), []).append(entry)
        return ["[{}] sets {} more than once with different values (lines {}), deploy applied the last one".format(
                    section, key, ", ".join("{} ={}".format(e.lineno, e.value) for e in hits))
                for (section, key), hits in seen.items() if any(e.value != hits[0].value for e in hits)]

    @staticmethod
    def hwmgmt_kconfig_table_errors(path) -> list:
        """ Lines of the hw-mgmt kconfig table that deploy silently misreads, as messages. deploy takes
            any line containing "[name]" as a header (so a comment naming a section switches sections),
            cannot read a header with other characters ("[Aspeed:Upstream]", trailing text, no closing
            bracket) and leaves the previous section open, applies an unknown name to nothing, takes
            any "X=v" as an assignment (a key without CONFIG_ is written as is and breaks Debian's
            merge), cuts a value at the first space (a quoted string with spaces is truncated) and
            skips "CONFIG_X=" or "CONFIG_X = y" (the previous block entry for X vanishes). """
        errors, current = [], None

        def error(lineno, what, line):
            errors.append("line {} {}: {}".format(lineno, what, line))

        for lineno, raw in enumerate(FileHandler.read_raw(path), 1):
            line = raw.strip()
            m = re.match(r".*\[([a-z0-9:]+)\]", line)
            if m:
                current = m.group(1)
                if line != "[{}]".format(current):
                    error(lineno, "is read by deploy as a section header", line)
                elif current not in HWMGMT_KCONFIG_SECTIONS:
                    error(lineno, "is not a section deploy applies", line)
                continue
            if line.startswith("["):
                error(lineno, "looks like a section header but deploy does not read it as one", line)
                continue
            if not current or not line or line.startswith("#"):
                continue
            # key up to the first "=": a value may contain "=" (deploy writes such a line unchanged)
            m = re.match(r"^([^=\s]+)=(\S+)", line)
            if not m:
                error(lineno, "is not applied by deploy", line)
            elif not re.match(r"^CONFIG_\w+$", m.group(1)):
                error(lineno, "is not a CONFIG_ option, deploy would write it as is", line)
            elif m.group(2).startswith('"') and not (len(m.group(2)) > 1 and m.group(2).endswith('"')):
                error(lineno, "has a value deploy would cut at the first space", line)
        return errors

    def find_missing_sections(self, sections) -> list:
        """ Sections missing or without entries, which would empty a marker block this run rewrites:
            amd64:upstream (this run always deploys amd64; amd64 downstream goes to a separate file),
            both aspeed sections when the release ships a BMC patch table (they share the block).
            deploy applies nothing for an empty section, just like for a missing one. """
        missing = []
        if not sections.get("amd64:upstream"):
            missing.append("amd64:upstream")
        bmc_table = os.path.join(self.args.build_root, HWMGMT_LINUX_DIR, BMC_PATCH_TABLE_NAME)
        if os.path.isfile(bmc_table) and not (sections.get("aspeed:upstream") or sections.get("aspeed:downstream")):
            missing.append("aspeed:upstream")
        return missing

    def active_sections(self) -> list:
        """ hw-mgmt kconfig sections deploy applied in this run: the arch's updated file differs from
            its base, or (amd64) deploy filled the separate downstream file """
        active = []
        if (FileHandler.read_raw(self.args.config_inc_amd) != FileHandler.read_raw(self.args.config_base_amd)
                or (os.path.isfile(self.args.config_inc_down_amd) and FileHandler.read_kconfig(self.args.config_inc_down_amd))):
            active += ["amd64:upstream", "amd64:downstream"]
        if FileHandler.read_raw(self.args.config_inc_aspeed) != FileHandler.read_raw(self.args.config_base_aspeed):
            active += ["aspeed:upstream", "aspeed:downstream"]
        return active

    def add_explicit_n(self):
        """ Add hw-mgmt's explicit "# CONFIG_X is not set" entries as =n. read_kconfig() skips
            comments, so they only survive the base/updated diff when the base had X enabled.
            Only for arches deploy processed in this run (updated file differs from base). """
        path = self.hwmgmt_kconfig_txt()
        if not os.path.isfile(path):
            print("-> NOTICE: {} not found, explicit 'is not set' entries not checked".format(path))
            return
        sections = self.read_hwmgmt_kconfig_txt(path)

        def n_keys(section):
            return [k for k, v in sections.get(section, OrderedDict()).items() if v == "n"]

        active = self.active_sections()
        if "amd64:upstream" in active:
            for key in n_keys("amd64:upstream"):
                if key not in KCFGData.x86_incl and key not in KCFGData.x86_excl:
                    print("-> INFO: hw-mgmt explicitly disables {} (amd64), adding =n".format(key))
                    KCFGData.x86_excl[key] = "n"
            for key in n_keys("amd64:downstream"):
                if key not in KCFGData.x86_down:
                    print("-> INFO: hw-mgmt explicitly disables {} (amd64 downstream), adding =n".format(key))
                    KCFGData.x86_down[key] = "n"
        if "aspeed:upstream" in active:
            # aspeed upstream and downstream share one block, deploy applies downstream last
            merged = OrderedDict(sections.get("aspeed:upstream", OrderedDict()))
            merged.update(sections.get("aspeed:downstream", OrderedDict()))
            for key in [k for k, v in merged.items() if v == "n"]:
                if key not in KCFGData.aspeed_incl and key not in KCFGData.aspeed_excl:
                    print("-> INFO: hw-mgmt explicitly disables {} (aspeed), adding =n".format(key))
                    KCFGData.aspeed_excl[key] = "n"

    def check_markers(self):
        """ Markers of the blocks write() rewrites, so a broken one stops the run before the first
            write. Runs after analyze(), which is what decides which blocks those are: an aspeed
            file without markers is still accepted when the release has nothing to write into it. """
        blocks = []
        if self.has_standard_changes:
            blocks += [(SLK_KCONFIG, MLNX_NOARCH_MARKER), (SLK_KCONFIG_AMD64, MLNX_KFG_MARKER)]
            # the [mellanox-arm64] section is regenerated through configparser at write time; a file
            # it cannot parse must stop the run here, not after the patches were moved
            try:
                self.insert_arm64_section(FileHandler.read_raw(os.path.join(self.args.build_root, SLK_KCONFIG_ARM64)), OrderedDict())
            except Exception as e:
                print("-> FATAL: cannot rewrite {}: {}".format(SLK_KCONFIG_ARM64, e))
                sys.exit(1)
        aspeed_path = os.path.join(self.args.build_root, SLK_KCONFIG_ASPEED)
        if not os.path.isfile(aspeed_path):
            if KCFGData.aspeed_incl or KCFGData.aspeed_excl:
                print("-> FATAL: {} not found, but the release has aspeed kconfig changes to write into it".format(
                    SLK_KCONFIG_ASPEED))
                sys.exit(1)
        elif not self.aspeed_compat_skip(FileHandler.read_raw(aspeed_path)):
            blocks.append((SLK_KCONFIG_ASPEED, MLNX_ASPEED_MARKER))
        for (path, marker) in blocks:
            lines = FileHandler.read_raw(os.path.join(self.args.build_root, path))
            FileHandler.find_marker_indices_checked(lines, marker, path)

    def analyze(self) -> list:
        """ Read inputs and reconcile; nothing is written. """
        self.read_data()
        KCFGData.x86_incl, KCFGData.x86_excl = self.parse_inc_exc(KCFGData.x86_base, KCFGData.x86_updated)
        KCFGData.arm_incl, KCFGData.arm_excl = self.parse_inc_exc(KCFGData.arm_base, KCFGData.arm_updated)
        KCFGData.aspeed_incl, KCFGData.aspeed_excl = self.parse_inc_exc(
            KCFGData.aspeed_base, KCFGData.aspeed_updated
        )
        self.add_explicit_n()

        # Standard amd64/arm64 kconfig processing. The recipe always deploys amd64, so a release that
        # ships a kconfig table has its amd64 blocks rewritten even when the delta is empty: the
        # previous release's lines must not survive because the new requests all equal the base.
        # Without a table (or a manual aspeed-only run) an untouched amd64 delta leaves the blocks alone.
        self.has_standard_changes = os.path.isfile(self.hwmgmt_kconfig_txt()) or bool(
            KCFGData.x86_incl or KCFGData.x86_excl or
            KCFGData.arm_incl or KCFGData.arm_excl or
            KCFGData.x86_down or KCFGData.arm_down
        )
        if self.has_standard_changes:
            self.parse_noarch_inc_exc()

        self.findings = []
        self.table_errors = []
        txt = self.hwmgmt_kconfig_txt()
        if os.path.isfile(txt):
            for section in self.find_missing_sections(self.read_hwmgmt_kconfig_txt(txt)):
                block = MLNX_KFG_MARKER if section.startswith("amd64") else MLNX_ASPEED_MARKER
                self.table_errors.append("hw-mgmt kconfig table has no [{}] entries, the {} block would be emptied".format(
                    section, block))
            self.table_errors += ["{} {}".format(os.path.basename(txt), e)
                                  for e in self.hwmgmt_kconfig_repeats(txt) + self.hwmgmt_kconfig_table_errors(txt)]
        common_path = os.path.join(self.args.build_root, SLK_KCONFIG)
        amd64_path = os.path.join(self.args.build_root, SLK_KCONFIG_AMD64)
        aspeed_path = os.path.join(self.args.build_root, SLK_KCONFIG_ASPEED)
        common = (common_path, MLNX_NOARCH_MARKER)
        arm64_common = (os.path.join(self.args.build_root, SLK_KCONFIG_ARM64_COMMON), None)
        if self.has_standard_changes:
            # the common block against its own file only; it is merged into every sonic build, and
            # reconciling it against the arch fragments needs per-build results (not done, the arm64
            # deploy that fills it is disabled)
            self.findings += self._reconcile_block(MLNX_NOARCH_MARKER, KCFGData.noarch_incl, KCFGData.noarch_excl,
                                                   common_path, MLNX_NOARCH_MARKER, [])
            self.findings += self._reconcile_block(MLNX_NOARCH_MARKER + " downstream", KCFGData.noarch_down, OrderedDict(),
                                                   common_path, MLNX_NOARCH_MARKER, [],
                                                   override_keys=set(KCFGData.noarch_incl) | set(KCFGData.noarch_excl),
                                                   downstream=True)
            self.findings += self._reconcile_block(MLNX_KFG_MARKER, KCFGData.x86_incl, KCFGData.x86_excl,
                                                   amd64_path, MLNX_KFG_MARKER, [common])
            # downstream lands in the same block through external-changes.patch (INCLUDE_EXTERNAL_PATCHES=y
            # only); a downstream key that overrides the block's own upstream value is never dropped as a
            # duplicate, and one that differs from a loose line is not a conflict
            self.findings += self._reconcile_block(MLNX_KFG_MARKER + " downstream", KCFGData.x86_down, OrderedDict(),
                                                   amd64_path, MLNX_KFG_MARKER, [common],
                                                   override_keys=set(KCFGData.x86_incl) | set(KCFGData.x86_excl),
                                                   downstream=True)
        if os.path.isfile(aspeed_path):
            self.findings += self._reconcile_block(MLNX_ASPEED_MARKER, KCFGData.aspeed_incl, KCFGData.aspeed_excl,
                                                   aspeed_path, MLNX_ASPEED_MARKER, [common, arm64_common])
        return self.findings

    def write(self) -> list:
        """ Write the blocks; returns the downstream diff lines. """
        all_lines = []
        if self.has_standard_changes:
            common_config, amd64_config, arm64_config = self.get_upstream_kconfig()
            FileHandler.write_lines(os.path.join(self.args.build_root, SLK_KCONFIG), common_config, True)
            FileHandler.write_lines(os.path.join(self.args.build_root, SLK_KCONFIG_AMD64), amd64_config, True)
            FileHandler.write_lines(os.path.join(self.args.build_root, SLK_KCONFIG_ARM64), arm64_config, True)
            all_lines = self.get_downstream_kconfig_diff(common_config, amd64_config, arm64_config)

        # Aspeed BMC kconfig — always rebuild the marker block so that configs dropped
        # by hw-mgmt (including a full drop of BMC support) are cleared from disk.
        aspeed_path = os.path.join(self.args.build_root, SLK_KCONFIG_ASPEED)
        if os.path.isfile(aspeed_path):
            aspeed_config = self.get_aspeed_kconfig()
            if aspeed_config is not None:
                FileHandler.write_lines(aspeed_path, aspeed_config, True)
        return all_lines

    def perform(self):
        self.analyze()
        return self.write()
