# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Write the FDB and neighbor files swssconfig.sh loads after a fast reboot.

Runs after the kexec on the DBs restored from dump.rdb, before SWSS flushes APPL_DB.
"""

import json
import shutil
from pathlib import Path

FAST_REBOOT_DIR = Path("/host/fast-reboot")


def connect_dbs():
    from swsscommon.swsscommon import SonicV2Connector

    dbs = SonicV2Connector(use_unix_socket_path=True)
    dbs.connect(dbs.APPL_DB)
    dbs.connect(dbs.STATE_DB)
    return dbs


def get_rows(dbs, db_id, pattern: str) -> dict:
    return {key: dbs.get_all(db_id, key) for key in dbs.keys(db_id, pattern) or []}


def fdb_entries(dbs) -> list:
    # STATE_DB, because ASIC_DB is flushed before the dump.
    entries = []
    for key, row in sorted(get_rows(dbs, dbs.STATE_DB, "FDB_TABLE|Vlan*").items()):
        vlan, mac = key.split("|", 1)[1].split(":", 1)
        if int(mac[:2], 16) & 1 or "port" not in row:
            continue
        entries.append((vlan, mac.lower(), row["port"], row.get("type", "dynamic")))
    return entries


def fdb_rows(fdb: list) -> list:
    return [{f"FDB_TABLE:{vlan}:{mac.replace(':', '-')}": {"type": fdb_type, "port": port}, "OP": "SET"}
            for vlan, mac, port, fdb_type in fdb]


def neighbor_rows(dbs, fdb: list) -> list:
    # Without an FDB entry the neighbor would point at no port.
    learned = {(vlan, mac) for vlan, mac, _, _ in fdb}
    rows = []
    for key, row in sorted(get_rows(dbs, dbs.APPL_DB, "NEIGH_TABLE:Vlan*").items()):
        vlan = key.split(":")[1]
        if (vlan, row.get("neigh", "").lower()) in learned:
            rows.append({key: row, "OP": "SET"})
    return rows


def write_restore_files(dbs=None, target: Path = FAST_REBOOT_DIR) -> dict:
    # Clear first so a failure never leaves stale files for swss to load.
    shutil.rmtree(target, ignore_errors=True)
    dbs = dbs or connect_dbs()
    fdb = fdb_entries(dbs)
    files = {
        "fdb.json": fdb_rows(fdb),
        "arp.json": neighbor_rows(dbs, fdb),
    }
    try:
        target.mkdir(parents=True)
        for name, rows in files.items():
            (target / name).write_text(json.dumps(rows, separators=(",", ":")))
    except OSError:
        shutil.rmtree(target, ignore_errors=True)
        raise
    return {name: len(rows) for name, rows in files.items()}
