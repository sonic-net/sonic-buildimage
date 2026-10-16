#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Transceiver presence cache, served while the ASIC is powered down.

The file lives under /var/run/platform_cache: cleared on reboot, and mounted
into PMon so xcvrd can read it. YAML, sfp.port_index (int) -> present (bool).
"""

import os
import tempfile
import time
from pathlib import Path
from typing import Any, Callable

import yaml

XCVR_PRESENCE_CACHE_FILE: Path = Path(
    "/var/run/platform_cache/xcvr_presence_cache.yaml"
)
# Backstops a killed asic_init.sh; ~2.5x the worst case (3 attempts, ~45s).
POWER_CYCLE_TTL_S: int = 120
XCVR_PRESENCE_CACHE_MAX_AGE_SECS: int = POWER_CYCLE_TTL_S

PathLike = str | os.PathLike[str]


def snapshot_presence(chassis: Any) -> dict[int, bool]:
    """Return {sfp.port_index: presence} for every SFP on the chassis."""
    result: dict[int, bool] = {}
    for i in range(chassis.get_num_sfps()):
        sfp = chassis.get_sfp(i)
        result[sfp.port_index] = sfp.get_presence()
    return result


def format_cache(presence: dict[int, bool]) -> str:
    """Serialize a presence dict to the cache file's YAML form."""
    lines = [f"{k}: {'true' if v else 'false'}" for k, v in sorted(presence.items())]
    return "\n".join(lines) + "\n"


def read_cached_presence(
    port_index: int,
    path: PathLike = XCVR_PRESENCE_CACHE_FILE,
    max_age_secs: float = XCVR_PRESENCE_CACHE_MAX_AGE_SECS,
    log_warning: Callable[[str], None] | None = None,
) -> bool | None:
    """Return cached presence for port_index, or None if there is no usable entry.

    None means the caller should read the hardware: the file is missing, stale,
    unreadable, or has no entry for this port. log_warning, if given, is called
    when a file that exists cannot be read.
    """
    try:
        if time.time() - os.path.getmtime(path) >= max_age_secs:
            return None
        with open(path) as f:
            data = yaml.safe_load(f)
        if data and port_index in data:
            return data[port_index]
    except FileNotFoundError:
        pass
    except (OSError, yaml.YAMLError) as e:
        if log_warning is not None:
            log_warning(f"xcvr presence cache read failed: {e}")
    return None


def write_presence_cache(path: PathLike, presence: dict[int, bool]) -> int:
    """Write a presence dict to path atomically. Returns the number of ports."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    # Renamed into place so a concurrent xcvrd read never sees a partial file.
    fd, tmp = tempfile.mkstemp(prefix=".xcvr_presence_cache.", dir=path.parent)
    try:
        with os.fdopen(fd, "w") as f:
            f.write(format_cache(presence))
        os.rename(tmp, path)
    except Exception:
        try:
            os.unlink(tmp)
        except OSError:
            pass
        raise
    return len(presence)
