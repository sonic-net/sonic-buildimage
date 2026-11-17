# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for nexthop.fast_boot_restore."""

import fnmatch
import json
from unittest.mock import patch

import pytest
from nexthop import fast_boot_restore


class FakeDbs:
    """In-memory SonicV2Connector."""

    APPL_DB = "APPL_DB"
    STATE_DB = "STATE_DB"

    def __init__(self, dbs):
        self.dbs = dbs

    def keys(self, db_id, pattern):
        return [key for key in self.dbs.get(db_id, {}) if fnmatch.fnmatchcase(key, pattern)]

    def get_all(self, db_id, key):
        return dict(self.dbs.get(db_id, {}).get(key, {}))


def restored_dbs():
    """Rows dump.rdb restores on a t0."""
    return FakeDbs({
        "APPL_DB": {
            "NEIGH_TABLE:Vlan1000:192.168.0.2": {"neigh": "72:06:00:01:00:02", "family": "IPv4"},
            "NEIGH_TABLE:Vlan1000:fc02:1000::2": {"neigh": "72:06:00:01:00:02", "family": "IPv6"},
            "NEIGH_TABLE:Vlan1000:192.168.0.9": {"neigh": "72:06:00:01:00:09", "family": "IPv4"},
            "NEIGH_TABLE:PortChannel101:10.0.0.57": {"neigh": "22:5c:1b:00:00:01", "family": "IPv4"},
            "ROUTE_TABLE:0.0.0.0/0": {"protocol": "bgp", "nexthop_group": "75773963"},
            "PORT_TABLE:Ethernet0": {"pre1": "0x0", "main": "0xa8", "post1": "0x0"},
        },
        "STATE_DB": {
            "FDB_TABLE|Vlan1000:72:06:00:01:00:02": {"port": "Ethernet8", "type": "dynamic"},
            "FDB_TABLE|Vlan1000:01:00:5E:00:00:01": {"port": "Ethernet8", "type": "dynamic"},
        },
    })


def read(target, name):
    return json.loads((target / name).read_text())


def test_writes_fdb_and_neighbors(tmp_path):
    target = tmp_path / "fast-reboot"
    counts = fast_boot_restore.write_restore_files(restored_dbs(), target)

    assert counts == {"fdb.json": 1, "arp.json": 2}
    # The multicast MAC is skipped.
    assert read(target, "fdb.json") == [
        {"FDB_TABLE:Vlan1000:72-06-00-01-00-02": {"type": "dynamic", "port": "Ethernet8"}, "OP": "SET"}]
    # 192.168.0.9 has no FDB entry and PortChannel101 is not a VLAN.
    assert read(target, "arp.json") == [
        {"NEIGH_TABLE:Vlan1000:192.168.0.2": {"neigh": "72:06:00:01:00:02", "family": "IPv4"}, "OP": "SET"},
        {"NEIGH_TABLE:Vlan1000:fc02:1000::2": {"neigh": "72:06:00:01:00:02", "family": "IPv6"}, "OP": "SET"},
    ]


def test_replaces_whatever_the_directory_held(tmp_path):
    target = tmp_path / "fast-reboot"
    target.mkdir()
    (target / "media_config.json").write_text("[]")
    fast_boot_restore.write_restore_files(restored_dbs(), target)

    assert sorted(path.name for path in target.iterdir()) == ["arp.json", "fdb.json"]


def test_failed_db_read_leaves_no_files_from_before_the_kexec(tmp_path):
    target = tmp_path / "fast-reboot"
    target.mkdir()
    (target / "media_config.json").write_text("[]")
    dbs = restored_dbs()
    with patch.object(dbs, "keys", side_effect=RuntimeError("Unable to connect to redis")), \
            pytest.raises(RuntimeError):
        fast_boot_restore.write_restore_files(dbs, target)

    assert not target.exists()
