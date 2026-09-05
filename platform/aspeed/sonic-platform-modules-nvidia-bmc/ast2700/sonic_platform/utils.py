#
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

"""Helpers for paths and sysfs-style files under `/var/run/hw-management`."""

import os

from sonic_py_common.logger import Logger

HW_MANAGEMENT_ROOT = "/var/run/hw-management"

logger = Logger()


def hw_mgmt_path(*relative_parts):
    return os.path.join(HW_MANAGEMENT_ROOT, *relative_parts)


def _read_sysfs_strip(path, log_errors=True):
    """
    Read `path` and return its stripped content.

    Returns:
        tuple: `(True, stripped_str)` on success, `(False, None)` on I/O error.
    """
    try:
        with open(path, "r") as f:
            return True, f.read().strip()
    except OSError as exc:
        if log_errors:
            logger.log_error(f"failed to read {path}: {exc}")
        return False, None


def read_sysfs_text(path, default=None):
    ok, raw = _read_sysfs_strip(path)
    return raw if ok else default


def read_sysfs_int(path, base=0, default=None, log_errors=True):
    ok, raw = _read_sysfs_strip(path, log_errors=log_errors)
    if not ok:
        return default
    try:
        return int(raw, base)
    except ValueError:
        if log_errors:
            logger.log_error(f"failed to parse integer from {path}: {raw!r}")
        return default


def hwmgmt_flag_is_set(*relative_parts):
    """
    Return True when the hw-management attribute at `relative_parts` is non-zero.

    Missing or unreadable files return False without logging.
    """
    value = read_sysfs_int(hw_mgmt_path(*relative_parts), default=0, log_errors=False)
    return bool(value)


def read_sysfs_float(path, default=None):
    ok, raw = _read_sysfs_strip(path)
    if not ok:
        return default
    try:
        return float(raw)
    except ValueError:
        logger.log_error(f"failed to parse float from {path}: {raw!r}")
        return default
