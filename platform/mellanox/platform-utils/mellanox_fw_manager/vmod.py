#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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
OE (Optical Engine) firmware version helpers for CPO platforms.

The OE firmware is a component of the ASIC MFA, so it is burned together
with the ASIC firmware; this module only reads and compares versions. The
ASIC is addressed by its PCI ID, so neither MST nor the SDK is needed.
"""

import fcntl
import json
import logging
import os
import re
import tempfile
from contextlib import contextmanager
from typing import List, NamedTuple, Optional

from .platform_utils import run_command, _detect_platform

FW_FILE_DIR = '/etc/mlnx'
# MFA OE versions, keyed by MFA file and PSID. Extracting the version
# takes ~5 s; the cache makes that once per MFA rather than once per run. It
# lives in the image's persistent /var, so it also survives reboots.
MFA_VERSION_CACHE = '/var/cache/mlnx-fw-manager/vmod-mfa-versions.json'
VMOD_ID = 0

# flint --vmod reports each bank as "FW A Version : 95.10.0770", and in
# "Running state is : ..." the bank that runs ("Image B is running") and the
# one the OE boots from at the next reset ("module boots from image A").
_BANK_VERSION_RE = re.compile(r'^\s*FW\s+([AB])\s+Version\s*:\s*(\S+)', re.M | re.I)
_RUNNING_BANK_RE = re.compile(r'\bImage\s+([AB])\s+is\s+running', re.I)
# The (?:...) group is for the spell checker, which reads \b and the word after it as one word.
_BOOT_BANK_RE = re.compile(r'\b(?:boots)\s+from\s+image\s+([AB])', re.I)
_FW_VERSION_RE = re.compile(r'^\s*FW\s+Version\s*:\s*(\S+)', re.M | re.I)
_PSID_RE = re.compile(r'^\s*PSID\s*:\s*(\S+)', re.M)

logger = logging.getLogger()


class HwVersions(NamedTuple):
    running: str
    boot: str  # what runs after the next ASIC reset


def _query(cmd: List[str]) -> str:
    """Run a flint/mlxfwmanager query; stdout, or '' on failure."""
    result = run_command(cmd, logger=logger, capture_output=True, text=True)
    if result.returncode != 0:
        logger.warning(f"{' '.join(cmd)} failed (rc={result.returncode}): {result.stderr}")
        return ''
    return result.stdout or ''


def is_cpo_platform() -> bool:
    try:
        from sonic_py_common import device_info
        return bool(device_info.get_cpo_data())
    except Exception as e:
        logger.warning(f"Unable to determine whether this is a CPO platform, "
                       f"OE firmware check disabled: {e}")
        return False


def parse_hw_versions(output: str) -> Optional[HwVersions]:
    """
    The running and the boot bank versions in a flint --vmod query.

    A burn writes the inactive bank and makes the OE boot from it, so until the
    next ASIC reset the boot bank holds the new image. The other bank keeps
    whatever ran before, so only the boot bank says what will run.
    """
    # A version that doesn't parse reads as unknown: comparing it would
    # request a burn on every boot.
    banks = {bank.upper(): version for bank, version in _BANK_VERSION_RE.findall(output) if _fields(version)}
    running = _RUNNING_BANK_RE.search(output)
    running_version = running and banks.get(running.group(1).upper())
    if not running_version:
        return None
    boot = _BOOT_BANK_RE.search(output)
    if not boot:
        return HwVersions(running_version, running_version)
    boot_version = banks.get(boot.group(1).upper())
    return HwVersions(running_version, boot_version) if boot_version else None


def parse_file_version(output: str) -> Optional[str]:
    """The version in a flint query of an image file."""
    match = _FW_VERSION_RE.search(output)
    return match.group(1) if match else None


def get_hw_versions(pci_id: str) -> Optional[HwVersions]:
    """OE versions on the hardware, or None if they can't be read."""
    return parse_hw_versions(_query(['flint', '-d', pci_id, '--vmod',
                                     '--downstream_device_ids', str(VMOD_ID), 'q']))


def get_mfa_version(mfa_file: str, pci_id: str) -> Optional[str]:
    """The VMOD component version in the MFA package for this ASIC's PSID."""
    psid = _PSID_RE.search(_query(['flint', '-d', pci_id, 'q']))
    if not psid:
        return None
    try:
        stat = os.stat(mfa_file)
    except OSError as e:
        logger.warning(f"Cannot access {mfa_file}: {e}")
        return None
    # A new MFA under the same path changes its size or mtime, so it misses.
    mfa_id = f'{os.path.realpath(mfa_file)}:{stat.st_size}:{stat.st_mtime_ns}'
    key = f'{mfa_id}:{psid.group(1)}'
    # The per-ASIC workers run in parallel: the lock keeps their read, change
    # and write of the cache apart, and lets the others reuse the one extract.
    with _cache_lock():
        cache = _load_cache()
        if key not in cache:
            version = _extract_mfa_version(mfa_file, psid.group(1))
            if not version:
                return None
            # Keep only this MFA's entries, so the cache doesn't grow with every
            # MFA change. Another MFA read later just costs one extract.
            cache = {k: v for k, v in cache.items() if k.startswith(f'{mfa_id}:')}
            cache[key] = version
            _save_cache(cache)
        return cache[key]


@contextmanager
def _cache_lock():
    # Best effort, like the cache: without the lock file it runs unlocked.
    fd = None
    try:
        os.makedirs(os.path.dirname(MFA_VERSION_CACHE), exist_ok=True)
        fd = os.open(f'{MFA_VERSION_CACHE}.lock', os.O_CREAT | os.O_RDWR, 0o644)
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            # Another process holds it, normally for the ~5 s MFA extract.
            logger.info(f"Waiting for {MFA_VERSION_CACHE}: another process is reading the MFA version")
            fcntl.flock(fd, fcntl.LOCK_EX)
    except OSError as e:
        logger.warning(f"Cannot lock {MFA_VERSION_CACHE}: {e}")
    try:
        yield
    finally:
        if fd is not None:
            os.close(fd)


def _extract_mfa_version(mfa_file: str, psid: str) -> Optional[str]:
    with tempfile.TemporaryDirectory() as tmp_dir:
        fwpkg = os.path.join(tmp_dir, 'fw.fwpkg')
        _query(['mlxfwmanager', '-i', mfa_file, '--psid', psid, '-extract', '--extract-file', fwpkg])
        if not os.path.exists(fwpkg):
            return None
        return parse_file_version(_query(['flint', '-i', fwpkg, '--component_type', 'VMOD', 'q']))


def _load_cache() -> dict:
    try:
        with open(MFA_VERSION_CACHE) as f:
            cache = json.load(f)
        return cache if isinstance(cache, dict) else {}
    except (OSError, ValueError):
        return {}


def _save_cache(cache: dict) -> None:
    # Best effort: a failed write only costs another extract next time.
    tmp_file = f'{MFA_VERSION_CACHE}.{os.getpid()}'
    try:
        os.makedirs(os.path.dirname(MFA_VERSION_CACHE), exist_ok=True)
        with open(tmp_file, 'w') as f:
            json.dump(cache, f)
        os.replace(tmp_file, MFA_VERSION_CACHE)
    except OSError as e:
        logger.warning(f"Cannot write {MFA_VERSION_CACHE}: {e}")
        try:
            os.remove(tmp_file)
        except OSError:
            pass


def _fields(version: str) -> Optional[tuple]:
    # The MFA may add a respin suffix (95_2020_0966-002), which is ignored.
    fields = re.split(r'[._]', version.split('-')[0])
    if len(fields) != 3 or not all(f.isdigit() for f in fields):
        return None
    return tuple(int(f) for f in fields)


def to_device_version(mfa_version: str) -> Optional[str]:
    """
    The MFA OE version as the device reports it (95.2020.0768 -> 95.10.0768).

    The device reports the MFA's middle field divided by 2, last 2 digits.
    If a firmware changes that rule, the versions never match and every boot
    burns again; test_vmod.py pins the pairs read from the hardware.
    """
    fields = _fields(mfa_version)
    if not fields:
        return None
    major, minor, build = fields
    return f'{major}.{minor // 2 % 100}.{build:04d}'


def versions_match(hw_version: str, mfa_version: str) -> bool:
    """Whether a hardware version is the MFA's OE firmware."""
    target = _fields(to_device_version(mfa_version) or '')
    return target is not None and _fields(hw_version) == target


def _asic_manager():
    from .asic_manager import AsicManager
    return AsicManager(_detect_platform())


def get_running_versions() -> List[str]:
    """Running OE version of each ASIC."""
    versions = [get_hw_versions(pci_id) for pci_id in _asic_manager().get_asic_pci_ids()]
    return [v.running if v else 'N/A' for v in versions]


def get_image_version() -> Optional[str]:
    """
    The OE version in the installed image's MFA, as the device reports it.
    All ASICs share one MFA, so ASIC 0 is used.
    """
    asic_manager = _asic_manager()
    pci_ids = asic_manager.get_asic_pci_ids()
    if not pci_ids:
        return None
    mfa_file = os.path.join(FW_FILE_DIR, f'fw-{asic_manager.get_asic_type()}.mfa')
    mfa_version = get_mfa_version(mfa_file, pci_ids[0])
    return mfa_version and to_device_version(mfa_version)
