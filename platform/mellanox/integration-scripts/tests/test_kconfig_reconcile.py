#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
""" Tests for the kconfig reconcile, the explicit "is not set" handling, the patch name
    collision check and fail-before-write of `post`, on a throw-away build root. """

import io
import os
import sys
import shutil
import tempfile
from contextlib import redirect_stdout
from unittest import mock, TestCase

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from hwmgmt_helper import *
from hwmgmt_kernel_patches import *

SLK = "src/sonic-linux-kernel/"
HWMGMT = "platform/mellanox/hw-management/hw-mgmt/recipes-kernel/linux/"

COMMON_CFG = """\
CONFIG_LOG_BUF_SHIFT=20
###-> mellanox_common-start
###-> mellanox_common-end
CONFIG_COMMON_X=y
"""

# PMBUS: same value before the block (Dell) -> duplicate, dropped
# PCA:   m before the block, hw-mgmt downstream wants y -> downstream difference, kept with a NOTICE
# GPIO_ICH: same value after the block -> duplicate, dropped
# FTG:   y after the block, hw-mgmt wants m -> conflict
AMD64_CFG = """\
# For Dell
CONFIG_PMBUS=m
CONFIG_PCA=m
###-> mellanox_amd64-start
CONFIG_STALE=y
###-> mellanox_amd64-end
# For Cisco
CONFIG_GPIO_ICH=m
CONFIG_FTG=y
"""

ARM64_COMMON_CFG = """\
CONFIG_ARM_COMMON=y
CONFIG_JTAG=m
"""

# MCTP: duplicate; I2C_AST: conflict (block wins); BT_IPMI: hw-mgmt says "is not set" -> conflict
ASPEED_CFG = """\
CONFIG_ARCH_ASPEED=y
CONFIG_MCTP=y
CONFIG_I2C_AST=m
CONFIG_BT_IPMI=m
###-> nvidia_aspeed_bmc-start
CONFIG_STALE_BMC=y
###-> nvidia_aspeed_bmc-end
"""

SERIES = """\
# header
0001-foreign.patch
###-> mellanox_sdk-start
###-> mellanox_sdk-end
###-> mellanox_hw_mgmt-start
0001-old-hw.patch
###-> mellanox_hw_mgmt-end
###-> aspeed
0001-aspeed-sdk.patch
###-> aspeed-end
###-> nvidia_aspeed_bmc-start
0001-old-bmc.patch
###-> nvidia_aspeed_bmc-end
"""

DEPLOY_SERIES = """\

# Patches updated from hw-mgmt 7.0070.9999

0002-new-hw.patch
0003-new-bmc.patch
"""

PATCH_TABLE = """\
Kernel-6.12
----------------------
| patch name        | Upstream commit id | status           | subversion | Notes |
----------------------
|0002-new-hw.patch  | abc1234            | Feature upstream |            |       |
----------------------

Legend:
Feature upstream: Has commit Id in upstream
Rejected:         Not take this patch
(padding)
(padding)
"""

HWMGMT_KCONFIG_TXT = """\
[amd64:upstream]
CONFIG_NEW=y
# CONFIG_X86_DIS is not set

[amd64:downstream]
# CONFIG_DOWN_DIS is not set

[aspeed:upstream]
# BT/KCS IPMI not used on NVIDIA BMC
# CONFIG_BT_IPMI is not set
CONFIG_JTAG=y
#CONFIG_COMMENTED_OUT=m
"""


def write(root, rel, text):
    path = os.path.join(root, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(text)
    return path


def read(root, rel):
    with open(os.path.join(root, rel)) as f:
        return f.read()


def block(text, marker):
    """ CONFIG lines between the marker start/end lines. """
    lines = text.splitlines()
    start = next(i for i, l in enumerate(lines) if marker + "-start" in l)
    end = next(i for i, l in enumerate(lines) if marker + "-end" in l)
    return [l for l in lines[start + 1:end] if l.startswith("CONFIG_")]


def block_names(lines, marker):
    """ Non-comment entries between the series marker start/end lines. """
    start = next(i for i, l in enumerate(lines) if marker + "-start" in l)
    end = next(i for i, l in enumerate(lines) if marker + "-end" in l)
    return [l.strip() for l in lines[start + 1:end] if l.strip() and not l.startswith("#")]


def reset_state():
    for field in ['x86_base', 'x86_updated', 'arm_base', 'arm_updated', 'x86_incl', 'arm_incl',
                  'x86_excl', 'arm_excl', 'x86_down', 'arm_down', 'noarch_incl', 'noarch_excl',
                  'noarch_down', 'aspeed_base', 'aspeed_updated', 'aspeed_incl', 'aspeed_excl']:
        getattr(KCFGData, field).clear()
    Data.new_up = []
    Data.new_non_up = []
    Data.old_up_patches = []
    Data.old_series = []
    Data.old_non_up = []
    Data.new_series = []
    Data.up_slk_series = []
    Data.agg_slk_series = []
    Data.bmc_patches = []
    Data.i_mlnx_start = -1
    Data.i_mlnx_end = -1


def build_tree(root):
    # sonic-linux-kernel
    write(root, SLK + "config.local/featureset-sonic/config", COMMON_CFG)
    write(root, SLK + "config.local/amd64/config.sonic", AMD64_CFG)
    write(root, SLK + "config.local/arm64/config.sonic", ARM64_COMMON_CFG)
    write(root, SLK + "config.local/arm64/config.sonic-aspeed", ASPEED_CFG)
    write(root, SLK + "config.local/arm64/config.sonic-mellanox", "[mellanox-arm64]\n")
    write(root, SLK + "patches-sonic/series", SERIES)
    for name in ["0001-foreign.patch", "0001-old-hw.patch", "0001-aspeed-sdk.patch", "0001-old-bmc.patch"]:
        write(root, SLK + "patches-sonic/" + name, "old " + name + "\n")
    # buildimage side
    write(root, "platform/mellanox/hw-management/hwmgmt_nonup_patches",
          "# Current non-upstream patch list, should be updated by hwmgmt_kernel_patches.py script\n")
    write(root, HWMGMT + "Patch_Status_Table.txt", PATCH_TABLE)
    write(root, HWMGMT + "kconfig_6_12.txt", HWMGMT_KCONFIG_TXT)
    # what deploy_kernel_patches.py would have produced
    write(root, "deploy/series", DEPLOY_SERIES)
    write(root, "deploy/patches/0002-new-hw.patch", "new hw\n")
    write(root, "deploy/non_up/0003-new-bmc.patch", "new bmc (candidate folder)\n")
    write(root, "deploy/bmc_only_patches", "0003-new-bmc.patch\n")
    # kconfig base / updated pairs
    write(root, "kcfg/x86_base.config",
          "CONFIG_A=y\n# CONFIG_PMBUS is not set\n# CONFIG_GPIO_ICH is not set\nCONFIG_OLD=y\n")
    write(root, "kcfg/x86_updated.config",
          "CONFIG_A=y\nCONFIG_PMBUS=m\nCONFIG_GPIO_ICH=m\n\n# New hw-mgmt flags\n\n"
          "CONFIG_NEW=y\nCONFIG_I2C_MUX=m\nCONFIG_FTG=m\n")
    write(root, "kcfg/x86_down.config", "CONFIG_I2C_MUX=y\nCONFIG_PCA=y\n")
    write(root, "kcfg/arm_base.config", "CONFIG_ARM=y\n")
    write(root, "kcfg/arm_updated.config", "CONFIG_ARM=y\n")
    write(root, "kcfg/arm_down.config", "")
    write(root, "kcfg/aspeed_base.config", "# CONFIG_MCTP is not set\nCONFIG_I2C_AST=m\n")
    write(root, "kcfg/aspeed_updated.config", "CONFIG_MCTP=y\nCONFIG_I2C_AST=y\nCONFIG_JTAG=y\n")


def run(action):
    """ Run action.perform() capturing stdout; returns (exit code or None, output). """
    out = io.StringIO()
    with redirect_stdout(out):
        try:
            action.perform()
        except SystemExit as e:
            return e.code, out.getvalue()
    return None, out.getvalue()


def make_args(root, force=False, bmc=True):
    argv = ["hwmgmt_kernel_patches.py", "post",
            "--patches", os.path.join(root, "deploy/patches"),
            "--non_up_patches", os.path.join(root, "deploy/non_up"),
            "--config_base_amd", os.path.join(root, "kcfg/x86_base.config"),
            "--config_base_arm", os.path.join(root, "kcfg/arm_base.config"),
            "--config_inc_amd", os.path.join(root, "kcfg/x86_updated.config"),
            "--config_inc_arm", os.path.join(root, "kcfg/arm_updated.config"),
            "--config_inc_down_amd", os.path.join(root, "kcfg/x86_down.config"),
            "--config_inc_down_arm", os.path.join(root, "kcfg/arm_down.config"),
            "--config_base_aspeed", os.path.join(root, "kcfg/aspeed_base.config"),
            "--config_inc_aspeed", os.path.join(root, "kcfg/aspeed_updated.config"),
            "--series", os.path.join(root, "deploy/series"),
            "--current_non_up_patches", os.path.join(root, "platform/mellanox/hw-management/hwmgmt_nonup_patches"),
            "--kernel_version", "6.12.41",
            "--hw_mgmt_ver", "7.0070.9999",
            "--build_root", root]
    if bmc:
        argv += ["--bmc_patches", os.path.join(root, "deploy/bmc_only_patches")]
    if force:
        argv += ["--force-overwrite"]
    with mock.patch("sys.argv", argv):
        return create_parser().parse_args()


class TestFileHandlerKconfigEntries(TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.root)

    def test_read_kconfig_entries(self):
        path = write(self.root, "f", "CONFIG_A=y\n# CONFIG_B is not set\nCONFIG_C=n\n"
                                     "#CONFIG_D=m # TODO\n# a comment\n\nCONFIG_S=\"x=y\"\n")
        entries = FileHandler.read_kconfig_entries(path)
        assert [(e.lineno, e.key, e.value) for e in entries] == [
            (1, "CONFIG_A", "y"), (2, "CONFIG_B", "n"), (3, "CONFIG_C", "n"), (7, "CONFIG_S", '"x=y"')]
        assert all(e.path == path for e in entries)

    def test_unset_lines_are_read_like_debian_kconfig_py(self):
        # KconfigFile.read(): only "# CONFIG_X is not set" is X=n; a commented-out or annotated one is a comment
        path = write(self.root, "f", "# CONFIG_A is not set\n#CONFIG_B is not set\n#  CONFIG_C is not set\n"
                                     "# CONFIG_D is not set # why\n# CONFIG_E is disabled here\n")
        assert [(e.key, e.value) for e in FileHandler.read_kconfig_entries(path)] == [("CONFIG_A", "n")]

    def test_entries_outside_block(self):
        path = write(self.root, "f", AMD64_CFG)
        outside = FileHandler.entries_outside_block(path, MLNX_KFG_MARKER)
        assert [e.key for e in outside] == ["CONFIG_PMBUS", "CONFIG_PCA", "CONFIG_GPIO_ICH", "CONFIG_FTG"]
        assert [e.lineno for e in outside] == [2, 3, 8, 9]

    def test_entries_outside_block_without_marker_is_the_whole_file(self):
        path = write(self.root, "f", "CONFIG_A=y\nCONFIG_B=m\n")
        assert [e.key for e in FileHandler.entries_outside_block(path, MLNX_KFG_MARKER)] == ["CONFIG_A", "CONFIG_B"]

    def test_entries_are_tagged_with_the_block_enclosing_them(self):
        path = write(self.root, "f", AMD64_CFG)
        owners = {e.key: e.owner for e in FileHandler.read_kconfig_entries(path)}
        assert owners == {"CONFIG_PMBUS": None, "CONFIG_PCA": None, "CONFIG_STALE": MLNX_KFG_MARKER,
                          "CONFIG_GPIO_ICH": None, "CONFIG_FTG": None}

    def test_broken_foreign_markers_fail_closed(self):
        # start without end: the rest of the file belongs to the block; end without start: everything
        # before it does. Our own block in between is still attributed to us, and closing it does not
        # close the foreign one.
        path = write(self.root, "f", "CONFIG_A=y\n###-> acme-start\nCONFIG_B=y\n###-> mellanox_amd64-start\n"
                                     "CONFIG_C=y\n###-> mellanox_amd64-end\nCONFIG_D=y\n")
        owners = {e.key: e.owner for e in FileHandler.read_kconfig_entries(path)}
        assert owners == {"CONFIG_A": None, "CONFIG_B": "acme", "CONFIG_C": MLNX_KFG_MARKER, "CONFIG_D": "acme"}
        path = write(self.root, "g", "CONFIG_A=y\n###-> acme-end\nCONFIG_B=y\n")
        owners = {e.key: e.owner for e in FileHandler.read_kconfig_entries(path)}
        assert owners == {"CONFIG_A": "acme", "CONFIG_B": None}

    def test_crossed_or_repeated_foreign_starts_stay_open(self):
        # closing acme must not close beta, and one of two acme starts stays open
        path = write(self.root, "f", "###-> acme-start\n###-> beta-start\n###-> acme-end\nCONFIG_A=y\n")
        assert [e.owner for e in FileHandler.read_kconfig_entries(path)] == ["beta"]
        path = write(self.root, "g", "###-> acme-start\n###-> acme-start\n###-> acme-end\nCONFIG_A=y\n")
        assert [e.owner for e in FileHandler.read_kconfig_entries(path)] == ["acme"]

    def test_marker_lines_are_read_the_same_way_everywhere(self):
        # trailing text tolerated, name matched exactly, both for ownership and for our own blocks
        assert parse_marker("###-> acme_switch-start # managed by acme-tool") == ("acme_switch", "start")
        assert parse_marker("###->mellanox_amd64-end") == ("mellanox_amd64", "end")
        assert parse_marker("###-> aspeed") is None and parse_marker("# ###-> x-start") is None
        path = write(self.root, "f", "###-> acme_switch-start # managed by acme-tool\nCONFIG_A=y\n###-> acme_switch-end # acme\n"
                                     "###-> old_mellanox_amd64-start\nCONFIG_B=y\n###-> old_mellanox_amd64-end\n"
                                     "###-> mellanox_amd64-start\nCONFIG_C=y\n###-> mellanox_amd64-end\n")
        owners = {e.key: e.owner for e in FileHandler.read_kconfig_entries(path)}
        assert owners == {"CONFIG_A": "acme_switch", "CONFIG_B": "old_mellanox_amd64", "CONFIG_C": MLNX_KFG_MARKER}
        lines = FileHandler.read_raw(path)
        assert FileHandler.find_marker_indices(lines, MLNX_KFG_MARKER) == (6, 8)
        assert FileHandler.find_marker_indices(lines, "acme_switch") == (0, 2)


class TestReadKconfigValues(TestCase):
    def test_values_containing_equals_are_kept(self):
        root = tempfile.mkdtemp()
        try:
            path = write(root, "c.config", 'CONFIG_CMDLINE="console=ttyS0 root=/dev/sda"\nCONFIG_PLAIN=y\nCONFIG_EMPTY=\n# CONFIG_OFF is not set\n')
            data = FileHandler.read_kconfig(path)
            assert data == {"CONFIG_CMDLINE": '"console=ttyS0 root=/dev/sda"', "CONFIG_PLAIN": "y", "CONFIG_EMPTY": ""}
        finally:
            shutil.rmtree(root)


class TestReconcile(TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp()
        self.target = write(self.root, "amd64/config.sonic", AMD64_CFG)
        self.common = write(self.root, "featureset-sonic/config", COMMON_CFG)
        self.task = KConfigTask(make_args(self.root))

    def tearDown(self):
        shutil.rmtree(self.root)
        reset_state()

    def run_reconcile(self, final, other_files=None):
        others = [(self.common, MLNX_NOARCH_MARKER)] if other_files is None else other_files
        return self.task.reconcile(OrderedDict(final), "t", self.target, MLNX_KFG_MARKER, others)

    def run_reconcile_downstream(self, final):
        return self.task.reconcile(OrderedDict(final), "t", self.target, MLNX_KFG_MARKER, [(self.common, MLNX_NOARCH_MARKER)],
                                   downstream=True)

    def report(self, findings):
        self.task.findings = findings
        return self.task.format_findings()

    def test_duplicate_before_block_is_dropped(self):
        kept, findings = self.run_reconcile([("CONFIG_PMBUS", "m")])
        assert kept == OrderedDict()
        assert len(findings) == 1 and findings[0].kind == DUPLICATE and findings[0].hits[0].lineno == 2

    def test_duplicate_after_block_is_dropped(self):
        kept, findings = self.run_reconcile([("CONFIG_GPIO_ICH", "m")])
        assert kept == OrderedDict()
        assert findings[0].kind == DUPLICATE

    def test_duplicate_in_another_file_is_dropped(self):
        kept, findings = self.run_reconcile([("CONFIG_COMMON_X", "y")])
        assert kept == OrderedDict()
        assert findings[0].kind == DUPLICATE and findings[0].hits[0].path == self.common

    def test_conflict_before_block_is_kept(self):
        kept, findings = self.run_reconcile([("CONFIG_PCA", "y")])
        assert kept == OrderedDict([("CONFIG_PCA", "y")])
        assert findings[0].kind == CONFLICT and findings[0].hits[0].value == "m"

    def test_conflict_after_block_is_kept(self):
        kept, findings = self.run_reconcile([("CONFIG_FTG", "m")])
        assert kept == OrderedDict([("CONFIG_FTG", "m")])
        assert findings[0].kind == CONFLICT and findings[0].hits[0].lineno == 9

    def test_conflict_in_another_file_is_kept(self):
        other = write(self.root, "other", "CONFIG_HI=m\n")
        kept, findings = self.run_reconcile([("CONFIG_HI", "y")], other_files=[(other, None)])
        assert kept == OrderedDict([("CONFIG_HI", "y")])
        assert findings[0].kind == CONFLICT and findings[0].hits[0].path == other

    def test_downstream_difference_before_the_block_is_kept_with_a_notice(self):
        # the downstream value is written into the block, below the loose line, and applies only with
        # INCLUDE_EXTERNAL_PATCHES=y: differing from the loose line is what the override is for
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_PCA", "y")]), "t", self.target, MLNX_KFG_MARKER, [],
                                             downstream=True)
        assert kept == OrderedDict([("CONFIG_PCA", "y")])
        assert findings[0].kind == DOWNSTREAM and findings[0].hits[0].lineno == 3 and findings[0].after == ()
        lines = self.report(findings)  # paths are relative to build_root/config.local, here ../../../
        assert lines[0].startswith("NOTICE    [t] CONFIG_PCA=y differs from ") and lines[0].endswith(
            "amd64/config.sonic:3 =m -> kept, applies only with INCLUDE_EXTERNAL_PATCHES=y, where the block comes after that line and wins")
        assert lines[-1] == "kconfig reconcile: 0 duplicate(s) not written, 0 conflict(s), 1 downstream difference(s) kept"

    def test_downstream_difference_below_the_block_is_a_warning(self):
        # a loose line below the block wins over the downstream value in the INCLUDE_EXTERNAL_PATCHES=y build
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_FTG", "m")]), "t", self.target, MLNX_KFG_MARKER, [],
                                             downstream=True)
        assert kept == OrderedDict([("CONFIG_FTG", "m")])
        assert findings[0].kind == DOWNSTREAM and [e.lineno for e in findings[0].after] == [9]
        lines = self.report(findings)
        assert lines[0].startswith("WARNING   [t] CONFIG_FTG=m differs from ") and lines[0].endswith(
            "amd64/config.sonic:9 =y -> kept, but that line comes after the block and wins when INCLUDE_EXTERNAL_PATCHES=y")
        assert lines[-1] == ("kconfig reconcile: 0 duplicate(s) not written, 0 conflict(s), 1 downstream difference(s) kept "
                             "(1 overridden by a line below the block)")

    def test_downstream_difference_above_and_below_the_block(self):
        # the line below the block wins over both the block and the line above it: the NOTICE for the
        # line above must not claim that the block wins
        target = write(self.root, SLK + "config.local/f", "CONFIG_A=m\n###-> mellanox_amd64-start\n###-> mellanox_amd64-end\nCONFIG_A=m\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_A", "y")]), "t", target, MLNX_KFG_MARKER, [], downstream=True)
        assert kept == OrderedDict([("CONFIG_A", "y")])
        assert [e.lineno for e in findings[0].hits] == [1, 4] and [e.lineno for e in findings[0].after] == [4]
        lines = self.report(findings)
        assert lines[0] == "NOTICE    [t] CONFIG_A=y differs from f:1 =m -> kept, applies only with INCLUDE_EXTERNAL_PATCHES=y"
        assert lines[1] == ("WARNING   [t] CONFIG_A=y differs from f:4 =m -> kept, but that line comes after the block and wins "
                            "when INCLUDE_EXTERNAL_PATCHES=y")
        assert lines[-1] == ("kconfig reconcile: 0 duplicate(s) not written, 0 conflict(s), 1 downstream difference(s) kept "
                             "(1 overridden by a line below the block)")

    def test_downstream_difference_inside_a_foreign_block(self):
        # another tool's block above ours loses to the block, one below ours wins over it, like loose lines
        target = write(self.root, SLK + "config.local/f", "###-> acme_switch-start\nCONFIG_A=m\n###-> acme_switch-end\n"
                                                          "###-> mellanox_amd64-start\n###-> mellanox_amd64-end\n"
                                                          "###-> acme_bmc-start\nCONFIG_B=m\n###-> acme_bmc-end\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_A", "y"), ("CONFIG_B", "y")]), "t", target, MLNX_KFG_MARKER, [],
                                             downstream=True)
        assert kept == OrderedDict([("CONFIG_A", "y"), ("CONFIG_B", "y")])
        assert [f.kind for f in findings] == [DOWNSTREAM, DOWNSTREAM] and findings[0].after == () and len(findings[1].after) == 1
        lines = self.report(findings)
        assert lines[0] == ("NOTICE    [t] CONFIG_A=y differs from f:2 (inside acme_switch block) =m -> kept, applies only with "
                            "INCLUDE_EXTERNAL_PATCHES=y, where the block comes after that line and wins")
        assert lines[1] == ("WARNING   [t] CONFIG_B=y differs from f:7 (inside acme_bmc block) =m -> kept, but that line comes after "
                            "the block and wins when INCLUDE_EXTERNAL_PATCHES=y")

    def test_downstream_difference_in_another_file_is_a_notice(self):
        # featureset-sonic/config is merged before amd64/config.sonic, so the block wins
        kept, findings = self.run_reconcile_downstream([("CONFIG_COMMON_X", "m")])
        assert kept == OrderedDict([("CONFIG_COMMON_X", "m")])
        assert findings[0].kind == DOWNSTREAM and findings[0].after == () and findings[0].hits[0].path == self.common
        lines = self.report(findings)
        assert lines[0].startswith("NOTICE    [t] CONFIG_COMMON_X=m differs from ") and lines[0].endswith(
            "featureset-sonic/config:4 =y -> kept, applies only with INCLUDE_EXTERNAL_PATCHES=y, where the block comes after that line and wins")

    def test_downstream_same_value_is_still_a_duplicate(self):
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_PCA", "m")]), "t", self.target, MLNX_KFG_MARKER, [],
                                             downstream=True)
        assert kept == OrderedDict() and findings[0].kind == DUPLICATE

    def test_is_not_set_equals_n(self):
        target = write(self.root, "f", "# CONFIG_A is not set\n###-> mellanox_amd64-start\n###-> mellanox_amd64-end\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_A", "n")]), "t", target, MLNX_KFG_MARKER, [])
        assert kept == OrderedDict() and findings[0].kind == DUPLICATE

    def test_commented_out_unset_line_is_not_a_definition(self):
        # Debian's merge ignores "#CONFIG_A is not set", so our A=n must be written, not dropped as a duplicate
        target = write(self.root, "f", "#CONFIG_A is not set\n###-> mellanox_amd64-start\n###-> mellanox_amd64-end\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_A", "n")]), "t", target, MLNX_KFG_MARKER, [])
        assert kept == OrderedDict([("CONFIG_A", "n")]) and findings == []

    def test_same_before_and_different_after_is_a_conflict(self):
        # the agreeing line 1 gets no report line; line 4 sits below the block and wins the merge
        target = write(self.root, SLK + "config.local/f", "CONFIG_A=y\n###-> mellanox_amd64-start\n###-> mellanox_amd64-end\nCONFIG_A=m\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_A", "y")]), "t", target, MLNX_KFG_MARKER, [])
        assert kept == OrderedDict([("CONFIG_A", "y")])
        assert findings[0].kind == CONFLICT and [e.lineno for e in findings[0].hits] == [1, 4]
        assert self.report(findings) == [
            "CONFLICT  [t] CONFIG_A: block=y vs f:4 =m, that line comes after the block and wins even with HWMGMT_KCFG_FORCE_OVERWRITE=y",
            "kconfig reconcile: 0 duplicate(s) not written, 1 conflict(s)"]

    def test_conflict_above_the_block_has_no_winner_note(self):
        # PCA=m above the block loses to the block, so the force switch does write an effective value
        kept, findings = self.run_reconcile([("CONFIG_PCA", "y")])
        assert findings[0].kind == CONFLICT and findings[0].after == ()
        lines = self.report(findings)
        assert lines[0].startswith("CONFLICT  [t] CONFIG_PCA: block=y vs ") and lines[0].endswith("amd64/config.sonic:3 =m")

    def test_downstream_agreeing_line_below_the_block_is_not_an_override(self):
        # only a differing line below the block overrides it; the agreeing one gets no report line either
        target = write(self.root, SLK + "config.local/f", "CONFIG_A=m\n###-> mellanox_amd64-start\n###-> mellanox_amd64-end\nCONFIG_A=y\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_A", "y")]), "t", target, MLNX_KFG_MARKER, [], downstream=True)
        assert findings[0].kind == DOWNSTREAM and [e.lineno for e in findings[0].hits] == [1, 4] and findings[0].after == ()
        assert self.report(findings) == [
            "NOTICE    [t] CONFIG_A=y differs from f:1 =m -> kept, applies only with INCLUDE_EXTERNAL_PATCHES=y, where the block "
            "comes after that line and wins",
            "kconfig reconcile: 0 duplicate(s) not written, 0 conflict(s), 1 downstream difference(s) kept"]

    def test_line_numbers_of_another_file_do_not_place_it_below_the_block(self):
        # featureset-sonic/config is merged before amd64/config.sonic wherever its line sits: the block wins
        common = write(self.root, "featureset-sonic/config", "###-> mellanox_common-start\n###-> mellanox_common-end\n"
                                                             + "# padding\n" * 8 + "CONFIG_COMMON_X=y\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_COMMON_X", "m")]), "t", self.target, MLNX_KFG_MARKER,
                                             [(common, MLNX_NOARCH_MARKER)], downstream=True)
        assert findings[0].kind == DOWNSTREAM and findings[0].hits[0].lineno == 11 and findings[0].after == ()
        lines = self.report(findings)
        assert lines[0].startswith("NOTICE    ") and lines[0].endswith(", where the block comes after that line and wins")
        assert lines[-1] == "kconfig reconcile: 0 duplicate(s) not written, 0 conflict(s), 1 downstream difference(s) kept"

    def test_unknown_key_is_kept_silently(self):
        kept, findings = self.run_reconcile([("CONFIG_NEW", "y")])
        assert kept == OrderedDict([("CONFIG_NEW", "y")]) and findings == []

    def test_missing_other_file_is_ignored(self):
        kept, findings = self.run_reconcile([("CONFIG_NEW", "y")], other_files=[(os.path.join(self.root, "does-not-exist"), None)])
        assert kept == OrderedDict([("CONFIG_NEW", "y")]) and findings == []

    def test_managed_block_of_another_file_is_not_compared(self):
        # the mellanox_common block is rewritten by this run, so its current content is no evidence
        common = write(self.root, "common2", "###-> mellanox_common-start\nCONFIG_BLK=y\n###-> mellanox_common-end\nCONFIG_OUT=y\n")
        kept, findings = self.run_reconcile([("CONFIG_BLK", "y"), ("CONFIG_OUT", "y")], other_files=[(common, MLNX_NOARCH_MARKER)])
        assert kept == OrderedDict([("CONFIG_BLK", "y")])
        assert [f.key for f in findings] == ["CONFIG_OUT"]

    def test_same_value_inside_a_foreign_block_is_kept_with_a_notice(self):
        # another generator rewrites that block wholesale; its line is no place to leave our value
        target = write(self.root, SLK + "config.local/f", "###-> acme_switch-start\nCONFIG_A=y\n###-> acme_switch-end\n"
                                                          "###-> mellanox_amd64-start\n###-> mellanox_amd64-end\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_A", "y")]), "t", target, MLNX_KFG_MARKER, [])
        assert kept == OrderedDict([("CONFIG_A", "y")])
        assert findings[0].kind == REDUNDANT and findings[0].hits[0].owner == "acme_switch"
        lines = self.report(findings)
        assert "NOTICE    [t] CONFIG_A=y also set by f:2 (inside acme_switch block) -> kept" in lines
        assert lines[-1] == "kconfig reconcile: 0 duplicate(s) not written, 0 conflict(s), 1 same-value line(s) kept"

    def test_other_value_inside_a_foreign_block_is_a_conflict(self):
        target = write(self.root, "f", "###-> mellanox_amd64-start\n###-> mellanox_amd64-end\n"
                                       "###-> acme_switch-start\nCONFIG_A=m\n###-> acme_switch-end\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_A", "y")]), "t", target, MLNX_KFG_MARKER, [])
        assert kept == OrderedDict([("CONFIG_A", "y")])
        assert findings[0].kind == CONFLICT and findings[0].hits[0].owner == "acme_switch"

    def test_a_loose_hit_beside_a_foreign_one_is_still_a_duplicate(self):
        target = write(self.root, SLK + "config.local/f", "###-> acme_switch-start\nCONFIG_A=y\n###-> acme_switch-end\n"
                                                          "###-> mellanox_amd64-start\n###-> mellanox_amd64-end\nCONFIG_A=y\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_A", "y")]), "t", target, MLNX_KFG_MARKER, [])
        assert kept == OrderedDict() and findings[0].kind == DUPLICATE and len(findings[0].hits) == 2
        lines = self.report(findings)
        assert "DUPLICATE [t] CONFIG_A=y already set by f:2 (inside acme_switch block) -> not written" in lines
        assert "DUPLICATE [t] CONFIG_A=y already set by f:6 -> not written" in lines

    def test_unclosed_foreign_block_keeps_our_line(self):
        target = write(self.root, "f", "###-> acme_switch-start\nCONFIG_A=y\n"
                                       "###-> mellanox_amd64-start\n###-> mellanox_amd64-end\n")
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_A", "y")]), "t", target, MLNX_KFG_MARKER, [])
        assert kept == OrderedDict([("CONFIG_A", "y")]) and findings[0].kind == REDUNDANT

    def test_override_keys_are_never_dropped(self):
        # a downstream line overriding the block's own upstream value: dropping it would leave the
        # block at the upstream value; a differing outside line is a conflict like for any other key
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_PMBUS", "m")]), "t", self.target, MLNX_KFG_MARKER, [],
                                             override_keys={"CONFIG_PMBUS"})
        assert kept == OrderedDict([("CONFIG_PMBUS", "m")]) and findings == []
        kept, findings = self.task.reconcile(OrderedDict([("CONFIG_PCA", "y")]), "t", self.target, MLNX_KFG_MARKER, [],
                                             override_keys={"CONFIG_PCA"})
        assert kept == OrderedDict([("CONFIG_PCA", "y")]) and findings[0].kind == CONFLICT

class TestAnalyze(TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp()
        build_tree(self.root)
        reset_state()

    def tearDown(self):
        shutil.rmtree(self.root)
        reset_state()

    def test_findings_and_dedup(self):
        task = KConfigTask(make_args(self.root))
        findings = task.analyze()
        by_key = {(f.target, f.key): f for f in findings}
        assert by_key[(MLNX_KFG_MARKER, "CONFIG_PMBUS")].kind == DUPLICATE
        assert by_key[(MLNX_KFG_MARKER, "CONFIG_GPIO_ICH")].kind == DUPLICATE
        assert by_key[(MLNX_KFG_MARKER, "CONFIG_FTG")].kind == CONFLICT
        assert by_key[(MLNX_KFG_MARKER + " downstream", "CONFIG_PCA")].kind == DOWNSTREAM
        assert by_key[(MLNX_ASPEED_MARKER, "CONFIG_MCTP")].kind == DUPLICATE
        assert by_key[(MLNX_ASPEED_MARKER, "CONFIG_I2C_AST")].kind == CONFLICT
        assert by_key[(MLNX_ASPEED_MARKER, "CONFIG_JTAG")].kind == CONFLICT  # arm64/config.sonic has =m
        assert by_key[(MLNX_ASPEED_MARKER, "CONFIG_BT_IPMI")].kind == CONFLICT  # explicit "is not set" vs =m
        assert len(findings) == 8 and len(task.conflicts()) == 4
        # duplicates are gone from what will be written, conflicts, downstream differences and the rest stay
        assert KCFGData.x86_down["CONFIG_PCA"] == "y"
        assert "CONFIG_PMBUS" not in KCFGData.x86_incl and "CONFIG_GPIO_ICH" not in KCFGData.x86_incl
        assert list(KCFGData.x86_incl.keys()) == ["CONFIG_NEW", "CONFIG_I2C_MUX", "CONFIG_FTG"]
        assert "CONFIG_MCTP" not in KCFGData.aspeed_incl and "CONFIG_I2C_AST" in KCFGData.aspeed_incl
        # downstream overriding hw-mgmt's own upstream value is not a conflict
        assert KCFGData.x86_down["CONFIG_I2C_MUX"] == "y"
        assert not any(f.key == "CONFIG_I2C_MUX" for f in findings)

    def test_downstream_override_differing_from_a_line_below_the_block_is_a_warning(self):
        # upstream I2C_MUX=m is a duplicate of the loose line; the downstream y that overrides it
        # differs from that line, which sits below the block and wins there: kept, reported, not fatal
        write(self.root, SLK + "config.local/amd64/config.sonic", AMD64_CFG + "CONFIG_I2C_MUX=m\n")
        task = KConfigTask(make_args(self.root))
        by_key = {(f.target, f.key): f for f in task.analyze()}
        assert by_key[(MLNX_KFG_MARKER, "CONFIG_I2C_MUX")].kind == DUPLICATE and "CONFIG_I2C_MUX" not in KCFGData.x86_incl
        down = by_key[(MLNX_KFG_MARKER + " downstream", "CONFIG_I2C_MUX")]
        assert down.kind == DOWNSTREAM and [e.lineno for e in down.after] == [10] and KCFGData.x86_down["CONFIG_I2C_MUX"] == "y"
        assert len(task.conflicts()) == 4
        lines = task.format_findings()
        assert ("WARNING   [mellanox_amd64 downstream] CONFIG_I2C_MUX=y differs from amd64/config.sonic:10 =m -> kept, "
                "but that line comes after the block and wins when INCLUDE_EXTERNAL_PATCHES=y") in lines
        assert lines[-1] == ("kconfig reconcile: 4 duplicate(s) not written, 4 conflict(s), 2 downstream difference(s) kept "
                             "(1 overridden by a line below the block)")

    def test_aspeed_explicit_disable_re_enabled_downstream_is_not_written(self):
        # deploy applies [aspeed:downstream] after [aspeed:upstream] on the same file, so the downstream
        # value is the release's intent
        write(self.root, HWMGMT + "kconfig_6_12.txt",
              HWMGMT_KCONFIG_TXT + "\n[aspeed:downstream]\nCONFIG_BT_IPMI=m\n# CONFIG_KCS is not set\n")
        task = KConfigTask(make_args(self.root))
        task.analyze()
        assert "CONFIG_BT_IPMI" not in KCFGData.aspeed_excl and KCFGData.aspeed_excl["CONFIG_KCS"] == "n"

    def test_table_errors_cover_what_deploy_misreads(self):
        txt = write(self.root, "t.txt",
                    "[amd64:upstream]\nCONFIG_OK=y\n# see [amd64:downstream]\n[Amd64:Downstream] # notes\n"
                    "THERMAL=y\nCONFIG_CMDLINE=\"a b\"\nCONFIG_BAD=\n# CONFIG_OFF is not set\n\n")
        assert KConfigTask.hwmgmt_kconfig_table_errors(txt) == [
            "line 3 is read by deploy as a section header: # see [amd64:downstream]",
            "line 4 looks like a section header but deploy does not read it as one: [Amd64:Downstream] # notes",
            "line 5 is not a CONFIG_ option, deploy would write it as is: THERMAL=y",
            "line 6 has a value deploy would cut at the first space: CONFIG_CMDLINE=\"a b\"",
            "line 7 is not applied by deploy: CONFIG_BAD="]
        assert KConfigTask.hwmgmt_kconfig_table_errors(write(self.root, "ok.txt", HWMGMT_KCONFIG_TXT)) == []

    def test_value_containing_equals_is_not_a_table_error(self):
        # deploy writes a line whose value contains "=" unchanged, so only the space rule applies to it
        txt = write(self.root, "eq.txt",
                    "[amd64:upstream]\nCONFIG_CMDLINE=\"console=ttyS0,115200\"\nCONFIG_EXTRA=\"root=/dev/sda console=ttyS0\"\n")
        assert KConfigTask.hwmgmt_kconfig_table_errors(txt) == [
            "line 3 has a value deploy would cut at the first space: CONFIG_EXTRA=\"root=/dev/sda console=ttyS0\""]

    def test_common_block_is_reconciled_against_its_own_file(self):
        # a key hw-mgmt sets identically on amd64 and arm64 moves to the mellanox_common block, which is
        # compared with the loose lines of featureset-sonic/config, not with the amd64 file any more
        write(self.root, "kcfg/arm_updated.config", "CONFIG_ARM=y\nCONFIG_NEW=y\n")
        write(self.root, SLK + "config.local/featureset-sonic/config", COMMON_CFG + "CONFIG_NEW=y\n")
        task = KConfigTask(make_args(self.root))
        by_key = {(f.target, f.key): f for f in task.analyze()}
        assert by_key[(MLNX_NOARCH_MARKER, "CONFIG_NEW")].kind == DUPLICATE
        assert "CONFIG_NEW" not in KCFGData.noarch_incl and "CONFIG_NEW" not in KCFGData.x86_incl
        reset_state()
        write(self.root, SLK + "config.local/featureset-sonic/config", COMMON_CFG + "CONFIG_NEW=n\n")
        task = KConfigTask(make_args(self.root))
        by_key = {(f.target, f.key): f for f in task.analyze()}
        assert by_key[(MLNX_NOARCH_MARKER, "CONFIG_NEW")].kind == CONFLICT
        assert KCFGData.noarch_incl["CONFIG_NEW"] == "y"

    def test_explicit_is_not_set_from_hwmgmt_txt(self):
        task = KConfigTask(make_args(self.root))
        task.analyze()
        assert "CONFIG_X86_DIS" in KCFGData.x86_excl
        assert KCFGData.x86_down["CONFIG_DOWN_DIS"] == "n"
        assert "CONFIG_BT_IPMI" in KCFGData.aspeed_excl
        assert "CONFIG_COMMENTED_OUT" not in KCFGData.aspeed_incl and "CONFIG_COMMENTED_OUT" not in KCFGData.aspeed_excl

    def test_downstream_override_of_own_upstream_is_kept_even_if_outside_matches(self):
        # outside says y, hw-mgmt upstream says m, hw-mgmt downstream says y: the downstream entry
        # must survive, otherwise --force-overwrite would leave the block at m
        write(self.root, SLK + "config.local/amd64/config.sonic", AMD64_CFG.replace("CONFIG_PCA=m\n", "CONFIG_PCA=m\nCONFIG_DOWNY=y\n"))
        with open(os.path.join(self.root, "kcfg/x86_updated.config"), "a") as f:
            f.write("CONFIG_DOWNY=m\n")
        with open(os.path.join(self.root, "kcfg/x86_down.config"), "a") as f:
            f.write("CONFIG_DOWNY=y\n")
        task = KConfigTask(make_args(self.root))
        findings = task.analyze()
        assert KCFGData.x86_incl["CONFIG_DOWNY"] == "m" and KCFGData.x86_down["CONFIG_DOWNY"] == "y"
        kinds = {(f.target, f.key): f.kind for f in findings}
        assert kinds[(MLNX_KFG_MARKER, "CONFIG_DOWNY")] == CONFLICT
        assert (MLNX_KFG_MARKER + " downstream", "CONFIG_DOWNY") not in kinds

    def test_explicit_is_not_set_only_change_is_applied(self):
        # the only amd64 change is an "is not set": parsed base == parsed updated, the raw files differ
        write(self.root, "kcfg/x86_updated.config", read(self.root, "kcfg/x86_base.config") + "# CONFIG_ONLY_N is not set\n")
        write(self.root, "kcfg/x86_down.config", "")
        write(self.root, HWMGMT + "kconfig_6_12.txt", "[amd64:upstream]\n# CONFIG_ONLY_N is not set\n")
        task = KConfigTask(make_args(self.root))
        task.analyze()
        assert KCFGData.x86_excl == OrderedDict([("CONFIG_ONLY_N", "n")]) and task.has_standard_changes

    def test_explicit_is_not_set_skipped_for_unchanged_arch(self):
        # aspeed base == updated: deploy did not run for aspeed, do not start populating its block
        write(self.root, "kcfg/aspeed_updated.config", read(self.root, "kcfg/aspeed_base.config"))
        task = KConfigTask(make_args(self.root))
        task.analyze()
        assert KCFGData.aspeed_incl == OrderedDict() and KCFGData.aspeed_excl == OrderedDict()

    def test_missing_hwmgmt_txt_is_tolerated(self):
        os.remove(os.path.join(self.root, HWMGMT + "kconfig_6_12.txt"))
        task = KConfigTask(make_args(self.root))
        findings = task.analyze()
        assert "CONFIG_BT_IPMI" not in KCFGData.aspeed_excl
        assert len(task.conflicts()) == 3 and len(findings) == 7

    def test_repeated_key_with_another_value_in_hwmgmt_txt_is_a_table_error(self):
        write(self.root, HWMGMT + "kconfig_6_12.txt",
              HWMGMT_KCONFIG_TXT.replace("[amd64:downstream]", "CONFIG_NEW=m\n\n[amd64:downstream]"))
        task = KConfigTask(make_args(self.root))
        task.analyze()
        assert task.table_errors == ["kconfig_6_12.txt [amd64:upstream] sets CONFIG_NEW more than once with different values "
                                     "(lines 2 =y, 5 =m), deploy applied the last one"]
        assert len(task.conflicts()) == 4

    def test_repeated_key_with_the_same_value_is_accepted(self):
        write(self.root, HWMGMT + "kconfig_6_12.txt",
              HWMGMT_KCONFIG_TXT.replace("[amd64:downstream]", "CONFIG_NEW=y\n\n[amd64:downstream]"))
        task = KConfigTask(make_args(self.root))
        task.analyze()
        assert task.table_errors == [] and len(task.conflicts()) == 4

    def test_repeat_in_a_section_deploy_did_not_apply_is_a_table_error_too(self):
        write(self.root, HWMGMT + "kconfig_6_12.txt", HWMGMT_KCONFIG_TXT + "\n[arm64:upstream]\nCONFIG_R=y\nCONFIG_R=m\n")
        task = KConfigTask(make_args(self.root))
        task.analyze()
        assert len(task.table_errors) == 1 and "[arm64:upstream] sets CONFIG_R more than once" in task.table_errors[0]

    def test_hwmgmt_txt_is_classified_like_deploy(self):
        # deploy_kernel_patches.py: a line with '=' is only an assignment, "is not set" is tried otherwise,
        # and its "#\s*(\S+) is not set" also takes "#CONFIG_W is not set" (unlike Debian's reader of config.local)
        write(self.root, HWMGMT + "kconfig_6_12.txt",
              "[amd64:upstream]\nCONFIG_X=y\n# CONFIG_X is not set because CONFIG_PARENT=n\n"
              "#CONFIG_Y=m\n# CONFIG_Z is not set (unused)\n#CONFIG_W is not set\n")
        path = os.path.join(self.root, HWMGMT + "kconfig_6_12.txt")
        assert KConfigTask.read_hwmgmt_kconfig_txt(path)["amd64:upstream"] == OrderedDict(
            [("CONFIG_X", "y"), ("CONFIG_Z", "n"), ("CONFIG_W", "n")])
        assert KConfigTask.hwmgmt_kconfig_repeats(path) == []

    def test_read_hwmgmt_kconfig_txt(self):
        sections = KConfigTask.read_hwmgmt_kconfig_txt(os.path.join(self.root, HWMGMT + "kconfig_6_12.txt"))
        assert sections["amd64:upstream"] == OrderedDict([("CONFIG_NEW", "y"), ("CONFIG_X86_DIS", "n")])
        assert sections["amd64:downstream"] == OrderedDict([("CONFIG_DOWN_DIS", "n")])
        assert sections["aspeed:upstream"] == OrderedDict([("CONFIG_BT_IPMI", "n"), ("CONFIG_JTAG", "y")])

    def test_report_lines(self):
        task = KConfigTask(make_args(self.root))
        task.analyze()
        lines = task.format_findings()
        assert "DUPLICATE [mellanox_amd64] CONFIG_PMBUS=m already set by amd64/config.sonic:2 -> not written" in lines
        assert ("CONFLICT  [mellanox_amd64] CONFIG_FTG: block=m vs amd64/config.sonic:9 =y, that line comes after the block "
                "and wins even with HWMGMT_KCFG_FORCE_OVERWRITE=y") in lines
        assert "CONFLICT  [nvidia_aspeed_bmc] CONFIG_JTAG: block=y vs arm64/config.sonic:2 =m" in lines
        assert ("NOTICE    [mellanox_amd64 downstream] CONFIG_PCA=y differs from amd64/config.sonic:3 =m -> kept, applies "
                "only with INCLUDE_EXTERNAL_PATCHES=y, where the block comes after that line and wins") in lines
        assert not any(l.startswith("CONFLICT  [mellanox_amd64 downstream]") for l in lines)
        assert lines[-1] == "kconfig reconcile: 3 duplicate(s) not written, 4 conflict(s), 1 downstream difference(s) kept"


class TestActiveSections(TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp()
        build_tree(self.root)
        reset_state()

    def tearDown(self):
        shutil.rmtree(self.root)
        reset_state()

    def test_downstream_only_amd64_change_still_counts_as_applied(self):
        # every amd64 upstream value already equals the base, only the downstream file has content
        write(self.root, "kcfg/x86_updated.config", read(self.root, "kcfg/x86_base.config"))
        task = KConfigTask(make_args(self.root, force=True))
        assert task.active_sections() == ["amd64:upstream", "amd64:downstream", "aspeed:upstream", "aspeed:downstream"]
        task.analyze()
        assert KCFGData.x86_down.get("CONFIG_DOWN_DIS") == "n"

    def test_missing_downstream_file_does_not_crash(self):
        write(self.root, "kcfg/x86_updated.config", read(self.root, "kcfg/x86_base.config"))
        os.remove(os.path.join(self.root, "kcfg/x86_down.config"))
        task = KConfigTask(make_args(self.root, force=True))
        assert task.active_sections() == ["aspeed:upstream", "aspeed:downstream"]

    def test_arch_deploy_did_not_touch_is_not_active(self):
        write(self.root, "kcfg/x86_updated.config", read(self.root, "kcfg/x86_base.config"))
        write(self.root, "kcfg/x86_down.config", "")
        task = KConfigTask(make_args(self.root, force=True))
        assert task.active_sections() == ["aspeed:upstream", "aspeed:downstream"]


class TestPostPerform(TestCase):
    """ End-to-end `post` on the throw-away tree. """

    def setUp(self):
        self.root = tempfile.mkdtemp()
        build_tree(self.root)
        reset_state()
        self.snapshot = {rel: read(self.root, rel) for rel in [
            SLK + "config.local/featureset-sonic/config", SLK + "config.local/amd64/config.sonic",
            SLK + "config.local/arm64/config.sonic-aspeed", SLK + "config.local/arm64/config.sonic-mellanox",
            SLK + "patches-sonic/series"]}

    def tearDown(self):
        shutil.rmtree(self.root)
        reset_state()

    def assert_tree_untouched(self):
        for rel, content in self.snapshot.items():
            assert read(self.root, rel) == content, rel
        assert os.path.isfile(os.path.join(self.root, SLK + "patches-sonic/0001-old-hw.patch"))
        assert os.path.isfile(os.path.join(self.root, SLK + "patches-sonic/0001-old-bmc.patch"))
        assert not os.path.isfile(os.path.join(self.root, SLK + "patches-sonic/0002-new-hw.patch"))
        assert not os.path.isfile(os.path.join(self.root, "platform/mellanox/non-upstream-patches/external-changes.patch"))

    def test_conflict_fails_before_writing(self):
        code, out = run(HwMgmtAction.get(make_args(self.root)))
        assert code == 1
        self.assert_tree_untouched()
        assert out.count("CONFLICT ") == 4 and "FATAL" in out and "HWMGMT_KCFG_FORCE_OVERWRITE=y" in out

    def drop_lines(self, rel, text_in_line):
        """ Remove a marker line from a tracked file and keep the snapshot in sync. """
        text = "".join(l for l in read(self.root, rel).splitlines(True) if text_in_line not in l)
        write(self.root, rel, text)
        self.snapshot[rel] = text

    def test_missing_series_end_marker_fails_before_writing(self):
        self.drop_lines(SLK + "patches-sonic/series", HW_MGMT_MARKER + "-end")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and HW_MGMT_MARKER + " markers not found" in out
        self.assert_tree_untouched()
        # the entries of the other platforms below the block are still there
        assert os.path.isfile(os.path.join(self.root, SLK + "patches-sonic/0001-foreign.patch"))

    def test_missing_bmc_markers_fail_before_writing(self):
        self.drop_lines(SLK + "patches-sonic/series", MLNX_ASPEED_MARKER)
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and MLNX_ASPEED_MARKER + " markers not found" in out
        self.assert_tree_untouched()

    def test_missing_amd64_kconfig_marker_fails_before_writing(self):
        self.drop_lines(SLK + "config.local/amd64/config.sonic", MLNX_KFG_MARKER + "-start")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and MLNX_KFG_MARKER + " markers not found" in out
        self.assert_tree_untouched()

    def swap_markers(self, rel, marker):
        """ Put the end marker above the start marker and keep the snapshot in sync. """
        lines = read(self.root, rel).splitlines(True)
        start = next(i for i, l in enumerate(lines) if marker + "-start" in l)
        end = next(i for i, l in enumerate(lines) if marker + "-end" in l)
        lines[start], lines[end] = lines[end], lines[start]
        text = "".join(lines)
        write(self.root, rel, text)
        self.snapshot[rel] = text

    def test_reversed_aspeed_kconfig_markers_fail_before_writing(self):
        self.swap_markers(SLK + "config.local/arm64/config.sonic-aspeed", MLNX_ASPEED_MARKER)
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and MLNX_ASPEED_MARKER + " markers not found" in out
        self.assert_tree_untouched()

    def test_missing_aspeed_kconfig_markers_fail_before_writing(self):
        self.drop_lines(SLK + "config.local/arm64/config.sonic-aspeed", MLNX_ASPEED_MARKER)
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and MLNX_ASPEED_MARKER + " markers not found" in out
        self.assert_tree_untouched()

    def no_aspeed_delta(self):
        """ A release that changes nothing in the aspeed kconfig, i.e. nothing to write there. """
        write(self.root, "kcfg/aspeed_updated.config", read(self.root, "kcfg/aspeed_base.config"))

    def test_reversed_aspeed_kconfig_markers_fail_when_release_has_no_aspeed_delta(self):
        # a reversed pair must not pass as the compat case below
        self.no_aspeed_delta()
        self.swap_markers(SLK + "config.local/arm64/config.sonic-aspeed", MLNX_ASPEED_MARKER)
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and MLNX_ASPEED_MARKER + " markers not found" in out
        self.assert_tree_untouched()

    def test_half_present_aspeed_kconfig_markers_fail_when_release_has_no_aspeed_delta(self):
        self.no_aspeed_delta()
        self.drop_lines(SLK + "config.local/arm64/config.sonic-aspeed", MLNX_ASPEED_MARKER + "-start")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and MLNX_ASPEED_MARKER + " markers not found" in out
        self.assert_tree_untouched()

    def test_absent_aspeed_kconfig_markers_are_skipped_when_release_has_no_aspeed_delta(self):
        # sonic-linux-kernel that predates BMC support: skip that block, write the rest
        self.no_aspeed_delta()
        aspeed = SLK + "config.local/arm64/config.sonic-aspeed"
        self.drop_lines(aspeed, MLNX_ASPEED_MARKER)
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code is None and "skipping aspeed kconfig update" in out
        assert read(self.root, aspeed) == self.snapshot[aspeed]
        assert "CONFIG_NEW=y" in block(read(self.root, SLK + "config.local/amd64/config.sonic"), MLNX_KFG_MARKER)

    def test_dotfile_in_non_upstream_dir_fails_before_writing(self):
        # glob('*') does not list hidden files, but mv_new_non_up_mlnx() rmdir's that directory
        write(self.root, "platform/mellanox/non-upstream-patches/patches/.keep", "sentinel\n")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and "not listed by hw-mgmt" in out and ".keep" in out
        self.assert_tree_untouched()

    def test_explicitly_given_bmc_patches_list_must_exist(self):
        # a path that is given but unusable is an argument error, not the legacy no-BMC mode: that
        # mode would leave the BMC block stale and file the BMC patches as hw-mgmt ones
        args = make_args(self.root, force=True, bmc=False)
        args.bmc_patches = os.path.join(self.root, "deploy/no-such-list")
        out = io.StringIO()
        with redirect_stdout(out), self.assertRaises(SystemExit) as e:
            HwMgmtAction.get(args)
        assert e.exception.code == 1 and "bmc_patches list doesn't exist" in out.getvalue()
        self.assert_tree_untouched()

    def test_bmc_markers_not_required_without_the_bmc_patches_argument(self):
        # legacy/manual run: without --bmc_patches the BMC block is not rewritten, so a series
        # without those markers must not be rejected
        series = SLK + "patches-sonic/series"
        write(self.root, series, "".join(l for l in read(self.root, series).splitlines(True)
                                         if MLNX_ASPEED_MARKER not in l and "0001-old-bmc.patch" not in l))
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True, bmc=False)))
        assert code is None and MLNX_ASPEED_MARKER + " markers not found" not in out

    def test_stray_non_patch_file_in_non_upstream_dir_fails_before_writing(self):
        write(self.root, "platform/mellanox/non-upstream-patches/patches/README", "notes\n")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and "not listed by hw-mgmt" in out and "README" in out
        self.assert_tree_untouched()

    def add_candidate_patch(self):
        """ hw-mgmt ships one non-upstream (candidate) patch, to be copied into non-upstream-patches/patches/ """
        write(self.root, "deploy/series", DEPLOY_SERIES + "0004-candidate.patch\n")
        write(self.root, "deploy/non_up/0004-candidate.patch", "new candidate\n")
        return "platform/mellanox/non-upstream-patches/patches/0004-candidate.patch"

    def test_symlink_in_non_upstream_dir_fails_before_writing(self):
        # a listed name that is a symlink: the copy would write through it to the target
        link = os.path.join(self.root, self.add_candidate_patch())
        target = write(self.root, "elsewhere/target", "untouched\n")
        os.makedirs(os.path.dirname(link), exist_ok=True)
        os.symlink(target, link)
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and "not regular files in platform/mellanox/non-upstream-patches/patches: 0004-candidate.patch" in out
        self.assert_tree_untouched()
        assert read(self.root, "elsewhere/target") == "untouched\n" and os.path.islink(link)

    def test_directory_in_non_upstream_dir_fails_before_writing(self):
        os.makedirs(os.path.join(self.root, self.add_candidate_patch()))
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and "not regular files" in out and "0004-candidate.patch" in out
        self.assert_tree_untouched()

    def test_candidate_patch_is_copied_when_the_dir_is_clean(self):
        rel = self.add_candidate_patch()
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code is None and read(self.root, rel) == "new candidate\n"

    def test_force_overwrite_writes_hwmgmt_values(self):
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code is None
        amd64 = read(self.root, SLK + "config.local/amd64/config.sonic")
        assert block(amd64, MLNX_KFG_MARKER) == [
            "CONFIG_NEW=y", "CONFIG_I2C_MUX=m", "CONFIG_FTG=m", "CONFIG_OLD=n", "CONFIG_X86_DIS=n"]
        # nothing outside the markers was touched
        assert amd64.startswith("# For Dell\nCONFIG_PMBUS=m\nCONFIG_PCA=m\n")
        assert amd64.endswith("# For Cisco\nCONFIG_GPIO_ICH=m\nCONFIG_FTG=y\n")
        aspeed = read(self.root, SLK + "config.local/arm64/config.sonic-aspeed")
        assert block(aspeed, MLNX_ASPEED_MARKER) == ["CONFIG_I2C_AST=y", "CONFIG_JTAG=y", "CONFIG_BT_IPMI=n"]
        assert read(self.root, SLK + "config.local/featureset-sonic/config") == COMMON_CFG
        # downstream goes to external-changes.patch, with hw-mgmt's own m -> y override and the explicit n
        ext = read(self.root, "platform/mellanox/non-upstream-patches/external-changes.patch")
        assert "-CONFIG_I2C_MUX=m" in ext and "+CONFIG_I2C_MUX=y" in ext
        assert "+CONFIG_PCA=y" in ext and "+CONFIG_DOWN_DIS=n" in ext
        # patches: old block content gone, new content in, BMC patch taken from the candidate folder
        series = read(self.root, SLK + "patches-sonic/series")
        assert "0001-old-hw.patch" not in series and "0002-new-hw.patch" in series
        assert "0001-old-bmc.patch" not in series and "0003-new-bmc.patch" in series
        assert "0001-foreign.patch" in series and "0001-aspeed-sdk.patch" in series
        assert not os.path.isfile(os.path.join(self.root, SLK + "patches-sonic/0001-old-hw.patch"))
        assert os.path.isfile(os.path.join(self.root, SLK + "patches-sonic/0002-new-hw.patch"))
        assert os.path.isfile(os.path.join(self.root, SLK + "patches-sonic/0003-new-bmc.patch"))
        assert "4 kconfig conflict(s) overridden" in out
        assert "BMC patch 0003-new-bmc.patch is marked Downstream" in out

    def test_downstream_difference_alone_does_not_stop_the_run(self):
        # the only finding left is PCA: loose =m above the block, hw-mgmt downstream =y. Default mode
        # writes, leaves the loose line alone and puts the downstream value into external-changes.patch
        write(self.root, SLK + "config.local/amd64/config.sonic",
              "CONFIG_PCA=m\n###-> mellanox_amd64-start\n###-> mellanox_amd64-end\n")
        write(self.root, SLK + "config.local/arm64/config.sonic", "CONFIG_ARM_COMMON=y\n")
        write(self.root, SLK + "config.local/arm64/config.sonic-aspeed",
              "CONFIG_ARCH_ASPEED=y\n###-> nvidia_aspeed_bmc-start\n###-> nvidia_aspeed_bmc-end\n")
        code, out = run(HwMgmtAction.get(make_args(self.root)))
        assert code is None and "FATAL" not in out and "CONFLICT " not in out
        assert ("NOTICE    [mellanox_amd64 downstream] CONFIG_PCA=y differs from amd64/config.sonic:1 =m -> kept, applies "
                "only with INCLUDE_EXTERNAL_PATCHES=y, where the block comes after that line and wins") in out
        assert "kconfig reconcile: 0 duplicate(s) not written, 0 conflict(s), 1 downstream difference(s) kept" in out
        amd64 = read(self.root, SLK + "config.local/amd64/config.sonic")
        assert amd64.startswith("CONFIG_PCA=m\n") and "CONFIG_PCA" not in block(amd64, MLNX_KFG_MARKER)
        ext = read(self.root, "platform/mellanox/non-upstream-patches/external-changes.patch")
        assert "+CONFIG_PCA=y" in ext and "-CONFIG_PCA=m" not in ext

    def test_no_findings_is_silent_and_writes(self):
        # make the tree agree with hw-mgmt: nothing outside the blocks overlaps
        write(self.root, SLK + "config.local/amd64/config.sonic",
              "###-> mellanox_amd64-start\n###-> mellanox_amd64-end\n")
        write(self.root, SLK + "config.local/arm64/config.sonic", "CONFIG_ARM_COMMON=y\n")
        write(self.root, SLK + "config.local/arm64/config.sonic-aspeed",
              "CONFIG_ARCH_ASPEED=y\n###-> nvidia_aspeed_bmc-start\n###-> nvidia_aspeed_bmc-end\n")
        code, out = run(HwMgmtAction.get(make_args(self.root)))
        assert code is None
        assert "kconfig reconcile: 0 duplicate(s) not written, 0 conflict(s)" in out
        amd64 = read(self.root, SLK + "config.local/amd64/config.sonic")
        assert block(amd64, MLNX_KFG_MARKER) == [
            "CONFIG_PMBUS=m", "CONFIG_GPIO_ICH=m", "CONFIG_NEW=y", "CONFIG_I2C_MUX=m", "CONFIG_FTG=m",
            "CONFIG_OLD=n", "CONFIG_X86_DIS=n"]

    def test_patch_name_collision_fails_before_writing(self):
        # a new hw-mgmt patch name already used by another section of the series
        write(self.root, SLK + "patches-sonic/series", SERIES.replace("0001-foreign.patch", "0002-new-hw.patch"))
        self.snapshot[SLK + "patches-sonic/series"] = read(self.root, SLK + "patches-sonic/series")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "patch name collision" in out

    def test_conflicts_and_collisions_are_both_reported(self):
        write(self.root, SLK + "patches-sonic/series", SERIES.replace("0001-foreign.patch", "0002-new-hw.patch"))
        self.snapshot[SLK + "patches-sonic/series"] = read(self.root, SLK + "patches-sonic/series")
        code, out = run(HwMgmtAction.get(make_args(self.root)))
        assert code == 1
        self.assert_tree_untouched()
        assert out.count("CONFLICT ") == 4 and "patch name collision" in out
        assert out.rstrip().endswith("-> FATAL: nothing written")

    def test_missing_aspeed_config_file_with_aspeed_changes_fails_before_writing(self):
        os.remove(os.path.join(self.root, SLK + "config.local/arm64/config.sonic-aspeed"))
        del self.snapshot[SLK + "config.local/arm64/config.sonic-aspeed"]
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "config.sonic-aspeed not found, but the release has aspeed kconfig changes" in out

    def test_missing_aspeed_config_file_is_skipped_when_the_release_has_no_aspeed_changes(self):
        os.remove(os.path.join(self.root, SLK + "config.local/arm64/config.sonic-aspeed"))
        write(self.root, "kcfg/aspeed_updated.config", read(self.root, "kcfg/aspeed_base.config"))
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code is None
        assert not os.path.exists(os.path.join(self.root, SLK + "config.local/arm64/config.sonic-aspeed"))
        # only the amd64 FTG conflict remains (PCA is a downstream difference), the amd64 block is still written
        assert "1 kconfig conflict(s) overridden" in out
        assert "CONFIG_NEW=y" in block(read(self.root, SLK + "config.local/amd64/config.sonic"), MLNX_KFG_MARKER)

    def test_repeated_marker_fails_before_writing(self):
        # a second end marker would make the block swallow the loose lines in between
        rel = SLK + "config.local/amd64/config.sonic"
        text = read(self.root, rel) + "###-> mellanox_amd64-end\n"
        write(self.root, rel, text)
        self.snapshot[rel] = text
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and "has 1 mellanox_amd64-start and 2 mellanox_amd64-end markers" in out
        self.assert_tree_untouched()

    def test_repeated_series_marker_fails_before_writing(self):
        rel = SLK + "patches-sonic/series"
        text = read(self.root, rel) + "###-> mellanox_hw_mgmt-start\n"
        write(self.root, rel, text)
        self.snapshot[rel] = text
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and "has 2 mellanox_hw_mgmt-start and 1 mellanox_hw_mgmt-end markers" in out
        self.assert_tree_untouched()

    def test_marker_inside_a_rewritten_block_fails_before_writing(self):
        rel = SLK + "config.local/amd64/config.sonic"
        text = read(self.root, rel).replace("CONFIG_STALE=y\n", "CONFIG_STALE=y\n###-> acme-start\nCONFIG_Z=y\n###-> acme-end\n")
        write(self.root, rel, text)
        self.snapshot[rel] = text
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "other markers inside the mellanox_amd64 block (###-> acme-start, ###-> acme-end), blocks must not overlap" in out

    def test_nested_series_blocks_fail_before_writing(self):
        # the hw-mgmt block inside the BMC block: each pair is well formed, rewriting the outer one
        # would drop the inner one and every hw-mgmt patch with it
        rel = SLK + "patches-sonic/series"
        text = ("# header\n0001-foreign.patch\n###-> mellanox_sdk-start\n###-> mellanox_sdk-end\n"
                "###-> nvidia_aspeed_bmc-start\n0001-old-bmc.patch\n###-> mellanox_hw_mgmt-start\n0001-old-hw.patch\n"
                "###-> mellanox_hw_mgmt-end\n###-> nvidia_aspeed_bmc-end\n###-> aspeed\n0001-aspeed-sdk.patch\n###-> aspeed-end\n")
        write(self.root, rel, text)
        self.snapshot[rel] = text
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and "blocks must not overlap" in out
        self.assert_tree_untouched()

    def test_empty_amd64_delta_still_rewrites_the_block(self):
        # the release's amd64 requests all equal the base: the previous release's lines must go
        write(self.root, "kcfg/x86_updated.config", read(self.root, "kcfg/x86_base.config"))
        write(self.root, "kcfg/x86_down.config", "")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code is None
        assert block(read(self.root, SLK + "config.local/amd64/config.sonic"), MLNX_KFG_MARKER) == []

    def test_bmc_release_without_aspeed_section_fails_even_without_the_aspeed_file(self):
        write(self.root, HWMGMT + "Patch_BMC_Status_Table.txt", PATCH_TABLE)
        write(self.root, HWMGMT + "kconfig_6_12.txt", HWMGMT_KCONFIG_TXT.split("[aspeed:upstream]")[0])
        write(self.root, "kcfg/aspeed_updated.config", read(self.root, "kcfg/aspeed_base.config"))
        os.remove(os.path.join(self.root, SLK + "config.local/arm64/config.sonic-aspeed"))
        del self.snapshot[SLK + "config.local/arm64/config.sonic-aspeed"]
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "no [aspeed:upstream] entries, the nvidia_aspeed_bmc block would be emptied" in out

    def test_unparseable_arm64_config_fails_before_writing(self):
        rel = SLK + "config.local/arm64/config.sonic-mellanox"
        write(self.root, rel, "CONFIG_BAD=y\n")
        self.snapshot[rel] = "CONFIG_BAD=y\n"
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1 and "cannot rewrite src/sonic-linux-kernel/config.local/arm64/config.sonic-mellanox" in out
        self.assert_tree_untouched()

    def test_unknown_section_header_fails_before_writing(self):
        write(self.root, HWMGMT + "kconfig_6_12.txt", HWMGMT_KCONFIG_TXT.replace("[aspeed:upstream]", "[aspeed:legacy]"))
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "kconfig_6_12.txt line 8 is not a section deploy applies: [aspeed:legacy]" in out

    def test_missing_amd64_upstream_section_fails_even_without_an_amd64_delta(self):
        # the section's absence is what produces the empty delta, so the check cannot depend on it
        write(self.root, HWMGMT + "kconfig_6_12.txt", HWMGMT_KCONFIG_TXT.replace("[amd64:upstream]\nCONFIG_NEW=y\n# CONFIG_X86_DIS is not set\n\n", ""))
        write(self.root, "kcfg/x86_updated.config", read(self.root, "kcfg/x86_base.config"))
        write(self.root, "kcfg/x86_down.config", "")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "no [amd64:upstream] entries, the mellanox_amd64 block would be emptied" in out

    def test_empty_amd64_upstream_section_fails_like_a_missing_one(self):
        # deploy applies nothing for a header without entries, exactly like for a missing header
        write(self.root, HWMGMT + "kconfig_6_12.txt", HWMGMT_KCONFIG_TXT.replace("CONFIG_NEW=y\n# CONFIG_X86_DIS is not set\n", ""))
        write(self.root, "kcfg/x86_updated.config", read(self.root, "kcfg/x86_base.config"))
        write(self.root, "kcfg/x86_down.config", "")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "no [amd64:upstream] entries, the mellanox_amd64 block would be emptied" in out

    def test_bmc_release_with_an_empty_aspeed_section_fails_before_writing(self):
        # both aspeed headers present, nothing under either
        write(self.root, HWMGMT + "Patch_BMC_Status_Table.txt", PATCH_TABLE)
        write(self.root, HWMGMT + "kconfig_6_12.txt",
              HWMGMT_KCONFIG_TXT.split("[aspeed:upstream]")[0] + "[aspeed:upstream]\n\n[aspeed:downstream]\n")
        write(self.root, "kcfg/aspeed_updated.config", read(self.root, "kcfg/aspeed_base.config"))
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "no [aspeed:upstream] entries, the nvidia_aspeed_bmc block would be emptied" in out

    def test_bmc_release_with_only_aspeed_downstream_entries_is_written(self):
        # deploy applies both aspeed sections to one file, so downstream entries alone fill the block
        write(self.root, HWMGMT + "Patch_BMC_Status_Table.txt", PATCH_TABLE)
        write(self.root, HWMGMT + "kconfig_6_12.txt",
              HWMGMT_KCONFIG_TXT.split("[aspeed:upstream]")[0] + "[aspeed:upstream]\n\n[aspeed:downstream]\nCONFIG_ASPEED_DOWN=y\n")
        write(self.root, "kcfg/aspeed_updated.config", read(self.root, "kcfg/aspeed_base.config") + "CONFIG_ASPEED_DOWN=y\n")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code is None and "would be emptied" not in out
        assert block(read(self.root, SLK + "config.local/arm64/config.sonic-aspeed"), MLNX_ASPEED_MARKER) == ["CONFIG_ASPEED_DOWN=y"]

    def test_patch_listed_twice_by_hwmgmt_fails_before_writing(self):
        write(self.root, "deploy/series", DEPLOY_SERIES + "0002-new-hw.patch\n")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "0002-new-hw.patch is listed twice for the mellanox_hw_mgmt block" in out

    def test_bmc_patch_listed_twice_by_hwmgmt_fails_before_writing(self):
        write(self.root, "deploy/series", DEPLOY_SERIES + "0003-new-bmc.patch\n")
        write(self.root, "deploy/bmc_only_patches", "0003-new-bmc.patch\n0003-new-bmc.patch\n")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "0003-new-bmc.patch is listed twice for the nvidia_aspeed_bmc block" in out

    def test_patch_moving_from_bmc_block_to_hwmgmt_block_fails_before_writing(self):
        # hw-mgmt moved 0002-new-hw.patch from its BMC table to the main table; the old BMC cleanup
        # would delete the copy made for the mellanox_hw_mgmt block and leave a dangling series entry
        write(self.root, SLK + "patches-sonic/series", SERIES.replace("0001-old-bmc.patch", "0002-new-hw.patch"))
        write(self.root, SLK + "patches-sonic/0002-new-hw.patch", "old copy\n")
        self.snapshot[SLK + "patches-sonic/series"] = read(self.root, SLK + "patches-sonic/series")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        for rel, content in self.snapshot.items():
            assert read(self.root, rel) == content, rel
        assert read(self.root, SLK + "patches-sonic/0002-new-hw.patch") == "old copy\n"
        assert "0002-new-hw.patch is still listed in the nvidia_aspeed_bmc block (series line 12)" in out

    def test_candidate_patch_moving_from_bmc_block_fails_before_writing(self):
        # same move for a main-table Downstream (candidate) patch: it would end up in no series at all
        write(self.root, SLK + "patches-sonic/series", SERIES.replace("0001-old-bmc.patch", "0004-moved.patch"))
        write(self.root, SLK + "patches-sonic/0004-moved.patch", "old copy\n")
        write(self.root, "deploy/series", DEPLOY_SERIES + "0004-moved.patch\n")
        write(self.root, "deploy/non_up/0004-moved.patch", "new candidate\n")
        self.snapshot[SLK + "patches-sonic/series"] = read(self.root, SLK + "patches-sonic/series")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert read(self.root, SLK + "patches-sonic/0004-moved.patch") == "old copy\n"
        assert "0004-moved.patch is still listed in the nvidia_aspeed_bmc block" in out

    def test_patch_moving_from_hwmgmt_block_to_bmc_block_is_written_once(self):
        # the other direction is safe: the old mellanox_hw_mgmt copy is removed before the BMC copy is made
        write(self.root, SLK + "patches-sonic/series", SERIES.replace("0001-old-hw.patch", "0003-new-bmc.patch"))
        write(self.root, SLK + "patches-sonic/0003-new-bmc.patch", "old copy\n")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code is None
        series = read(self.root, SLK + "patches-sonic/series").splitlines()
        assert series.count("0003-new-bmc.patch") == 1
        assert "0003-new-bmc.patch" in block_names(series, MLNX_ASPEED_MARKER)
        assert "0003-new-bmc.patch" not in block_names(series, HW_MGMT_MARKER)
        assert read(self.root, SLK + "patches-sonic/0003-new-bmc.patch") == "new bmc (candidate folder)\n"

    def test_old_block_entry_with_trailing_comment_is_still_removed(self):
        write(self.root, SLK + "patches-sonic/series", SERIES.replace("0001-old-hw.patch\n", "0001-old-hw.patch # kept for X\n")
              .replace("0001-old-bmc.patch\n", "  0001-old-bmc.patch  \n"))
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code is None
        # hw-mgmt 7.0070.9999 ships neither, so both files must be gone
        assert not os.path.isfile(os.path.join(self.root, SLK + "patches-sonic/0001-old-hw.patch"))
        assert not os.path.isfile(os.path.join(self.root, SLK + "patches-sonic/0001-old-bmc.patch"))

    def test_bmc_list_entry_without_a_deploy_file_fails_before_writing(self):
        write(self.root, "deploy/bmc_only_patches", "0003-new-bmc.patch\n0009-ghost.patch\n")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "BMC patch 0009-ghost.patch not found either in upstream or non-upstream list" in out

    def test_bmc_release_without_aspeed_section_fails_before_writing(self):
        # the release ships a BMC patch table but its kconfig table has a mistyped aspeed section: deploy
        # applied nothing, the block would be emptied
        write(self.root, HWMGMT + "Patch_BMC_Status_Table.txt", PATCH_TABLE)
        write(self.root, HWMGMT + "kconfig_6_12.txt", HWMGMT_KCONFIG_TXT.replace("[aspeed:upstream]", "[Aspeed:Upstream]"))
        write(self.root, "kcfg/aspeed_updated.config", read(self.root, "kcfg/aspeed_base.config"))
        code, out = run(HwMgmtAction.get(make_args(self.root)))
        assert code == 1
        self.assert_tree_untouched()
        assert "no [aspeed:upstream] entries, the nvidia_aspeed_bmc block would be emptied" in out
        # a broken table is not a value conflict: the force switch does not apply
        reset_state()
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "FATAL: hw-mgmt kconfig table has no [aspeed:upstream] entries" in out
        assert "HWMGMT_KCFG_FORCE_OVERWRITE=y does not override this" in out
        # and the mistyped header itself is named: deploy keeps the previous section open across it
        assert "looks like a section header but deploy does not read it as one: [Aspeed:Upstream]" in out

    def test_same_value_only_inside_a_foreign_block_is_written_with_a_notice(self):
        write(self.root, SLK + "config.local/amd64/config.sonic",
              "###-> acme_switch-start\nCONFIG_NEW=y\n###-> acme_switch-end\n" + AMD64_CFG)
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code is None
        assert "NOTICE    [mellanox_amd64] CONFIG_NEW=y also set by amd64/config.sonic:2 (inside acme_switch block) -> kept" in out
        amd64 = read(self.root, SLK + "config.local/amd64/config.sonic")
        assert "CONFIG_NEW=y" in block(amd64, MLNX_KFG_MARKER)
        assert amd64.startswith("###-> acme_switch-start\nCONFIG_NEW=y\n###-> acme_switch-end\n")

    def test_missing_amd64_upstream_section_with_downstream_changes_fails(self):
        write(self.root, HWMGMT + "kconfig_6_12.txt", HWMGMT_KCONFIG_TXT.replace("[amd64:upstream]", "[Amd64:Upstream]"))
        write(self.root, "kcfg/x86_updated.config", read(self.root, "kcfg/x86_base.config"))
        code, out = run(HwMgmtAction.get(make_args(self.root, force=False)))
        assert code == 1
        self.assert_tree_untouched()
        assert "no [amd64:upstream] entries, the mellanox_amd64 block would be emptied" in out

    def test_unparsed_kconfig_lines_fail_before_writing(self):
        # deploy skips them, so the block would lose the previous entry for that key
        write(self.root, HWMGMT + "kconfig_6_12.txt",
              HWMGMT_KCONFIG_TXT.replace("CONFIG_NEW=y\n", "CONFIG_NEW=y\nCONFIG_BAD=\nCONFIG_SPACED = y\n"))
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "FATAL: kconfig_6_12.txt line 3 is not applied by deploy: CONFIG_BAD=" in out
        assert "FATAL: kconfig_6_12.txt line 4 is not applied by deploy: CONFIG_SPACED = y" in out
        assert "HWMGMT_KCFG_FORCE_OVERWRITE=y does not override this" in out

    def test_stray_file_in_non_upstream_dir_fails_before_writing(self):
        write(self.root, "platform/mellanox/non-upstream-patches/patches/zzz-stray.patch", "stray\n")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "not listed by hw-mgmt or hwmgmt_nonup_patches: zzz-stray.patch" in out

    def test_deleting_a_bmc_patch_another_section_lists_fails(self):
        # the old BMC block patch is also listed by the aspeed section
        write(self.root, SLK + "patches-sonic/series", SERIES.replace("0001-aspeed-sdk.patch", "0001-old-bmc.patch"))
        self.snapshot[SLK + "patches-sonic/series"] = read(self.root, SLK + "patches-sonic/series")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "0001-old-bmc.patch would be deleted but is still referenced" in out

    def test_new_bmc_patch_another_section_lists_fails(self):
        # the aspeed section already lists the patch hw-mgmt now ships for the BMC block: the copy would
        # overwrite that section's file and the series would list the name twice
        write(self.root, SLK + "patches-sonic/series", SERIES.replace("0001-aspeed-sdk.patch", "0003-new-bmc.patch"))
        self.snapshot[SLK + "patches-sonic/series"] = read(self.root, SLK + "patches-sonic/series")
        write(self.root, SLK + "patches-sonic/0003-new-bmc.patch", "aspeed copy\n")
        self.snapshot[SLK + "patches-sonic/0003-new-bmc.patch"] = "aspeed copy\n"
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "BMC patch 0003-new-bmc.patch is already referenced outside the nvidia_aspeed_bmc block (series line 9)" in out

    def test_missing_patch_table_fails_before_writing(self):
        os.remove(os.path.join(self.root, HWMGMT + "Patch_Status_Table.txt"))
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "could not load Patch_Status_Table.txt" in out

    def test_malformed_bmc_table_fails_before_writing(self):
        # the BMC table is only read for the commit message, but a broken one used to fail after the writes
        write(self.root, HWMGMT + "Patch_BMC_Status_Table.txt",
              PATCH_TABLE.replace("|0002-new-hw.patch  | abc1234            | Feature upstream |            |       |",
                                  "|0003-new-bmc.patch broken row"))
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "could not load Patch_BMC_Status_Table.txt" in out

    def test_bmc_table_is_listed_in_the_kernel_commit_message(self):
        write(self.root, HWMGMT + "Patch_BMC_Status_Table.txt",
              PATCH_TABLE.replace("0002-new-hw.patch  | abc1234           ", "0003-new-bmc.patch | def5678           "))
        slk_msg = os.path.join(self.root, "slk_msg")
        args = make_args(self.root, force=True)
        args.slk_msg = slk_msg
        code, out = run(HwMgmtAction.get(args))
        assert code is None
        msg = open(slk_msg).read()
        assert "BMC Patch List" in msg and "0003-new-bmc.patch" in msg

    def test_deleting_a_patch_another_section_lists_fails(self):
        # the old hw-mgmt block patch is also listed by the aspeed section
        write(self.root, SLK + "patches-sonic/series", SERIES.replace("0001-aspeed-sdk.patch", "0001-old-hw.patch"))
        self.snapshot[SLK + "patches-sonic/series"] = read(self.root, SLK + "patches-sonic/series")
        code, out = run(HwMgmtAction.get(make_args(self.root, force=True)))
        assert code == 1
        self.assert_tree_untouched()
        assert "would be deleted but is still referenced" in out


class TestLoadPatchTable(TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.root)

    def test_comments_and_blank_lines_are_skipped(self):
        text = PATCH_TABLE.replace("|0002-new-hw.patch  | abc1234            | Feature upstream |            |       |\n",
                                   "# a comment inside the table\n"
                                   "|0002-new-hw.patch  | abc1234            | Feature upstream |            |       |\n"
                                   "\n"
                                   "|0003-other.patch   |                    | Downstream       |            |       |\n")
        # blank line between the first delimiter and the header row as well
        text = text.replace("----------------------\n| patch name", "----------------------\n\n| patch name", 1)
        write(self.root, "Patch_Status_Table.txt", "# leading comment\n\n" + text)
        table = load_patch_table(self.root, "6.12")
        assert [row["patch name"] for row in table] == ["0002-new-hw.patch", "0003-other.patch"]
        assert table[0]["Upstream commit id"] == "abc1234"

    def test_row_with_too_few_columns_is_none(self):
        text = PATCH_TABLE.replace("|0002-new-hw.patch  | abc1234            | Feature upstream |            |       |\n",
                                   "|0002-new-hw.patch broken row\n")
        write(self.root, "Patch_Status_Table.txt", text)
        assert load_patch_table(self.root, "6.12") is None

    def test_empty_table_is_a_list_not_none(self):
        text = PATCH_TABLE.replace("|0002-new-hw.patch  | abc1234            | Feature upstream |            |       |\n", "")
        write(self.root, "Patch_Status_Table.txt", text)
        assert load_patch_table(self.root, "6.12") == []

    def test_unknown_kernel_is_none(self):
        write(self.root, "Patch_Status_Table.txt", PATCH_TABLE)
        assert load_patch_table(self.root, "6.1") is None
