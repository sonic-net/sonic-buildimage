#!/usr/bin/env python3

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for nexthop.xcvr_presence_cache."""

import os
from unittest.mock import MagicMock

import pytest
import yaml


@pytest.fixture(scope="function", autouse=True)
def xcvr_presence_cache_module():
    from nexthop import xcvr_presence_cache

    yield xcvr_presence_cache


def make_chassis(presence):
    """Mock chassis whose SFPs carry a port_index offset from the chassis index."""
    sfps = []
    for i, (port_index, present) in enumerate(sorted(presence.items())):
        sfp = MagicMock()
        sfp.port_index = port_index
        sfp.get_presence.return_value = present
        sfps.append(sfp)

    chassis = MagicMock()
    chassis.get_num_sfps.return_value = len(sfps)
    chassis.get_sfp.side_effect = lambda i: sfps[i]
    return chassis


class TestSnapshotPresence:
    def test_keyed_by_port_index_not_chassis_index(self, xcvr_presence_cache_module):
        """sfp.py looks the cache up by port_index, which need not be the index."""
        chassis = make_chassis({10: True, 11: False, 12: True})
        assert xcvr_presence_cache_module.snapshot_presence(chassis) == {
            10: True,
            11: False,
            12: True,
        }


class TestWritePresenceCache:
    def test_writes_a_file_the_reader_can_parse(
        self, xcvr_presence_cache_module, tmp_path
    ):
        path = tmp_path / "cache.yaml"

        count = xcvr_presence_cache_module.write_presence_cache(
            path, {0: True, 1: False, 2: True}
        )

        assert count == 3
        assert yaml.safe_load(path.read_text()) == {0: True, 1: False, 2: True}

    def test_no_temp_file_left_on_failure(
        self, xcvr_presence_cache_module, tmp_path, monkeypatch
    ):
        path = tmp_path / "cache.yaml"

        def boom(*args, **kwargs):
            raise OSError("rename failed")

        monkeypatch.setattr(xcvr_presence_cache_module.os, "rename", boom)

        with pytest.raises(OSError):
            xcvr_presence_cache_module.write_presence_cache(path, {0: True})

        assert list(tmp_path.iterdir()) == []


class TestReadCachedPresence:
    def _write(self, xcvr_presence_cache_module, tmp_path, presence, mtime=None):
        path = tmp_path / "cache.yaml"
        xcvr_presence_cache_module.write_presence_cache(path, presence)
        if mtime is not None:
            os.utime(path, (mtime, mtime))
        return path

    def test_missing_file_returns_none_without_warning(
        self, xcvr_presence_cache_module, tmp_path
    ):
        log = MagicMock()

        result = xcvr_presence_cache_module.read_cached_presence(
            0, path=tmp_path / "nope.yaml", log_warning=log
        )

        assert result is None
        log.assert_not_called()

    def test_returns_the_cached_value(self, xcvr_presence_cache_module, tmp_path):
        path = self._write(xcvr_presence_cache_module, tmp_path, {0: True, 1: False})

        assert xcvr_presence_cache_module.read_cached_presence(0, path=path) is True
        assert xcvr_presence_cache_module.read_cached_presence(1, path=path) is False

    def test_port_not_in_cache_returns_none(self, xcvr_presence_cache_module, tmp_path):
        path = self._write(xcvr_presence_cache_module, tmp_path, {0: True})
        assert xcvr_presence_cache_module.read_cached_presence(99, path=path) is None

    def test_empty_cache_file_returns_none(self, xcvr_presence_cache_module, tmp_path):
        path = tmp_path / "cache.yaml"
        path.write_text("")
        assert xcvr_presence_cache_module.read_cached_presence(0, path=path) is None

    def test_stale_cache_returns_none(
        self, xcvr_presence_cache_module, tmp_path, monkeypatch
    ):
        """A cache left behind by a killed asic_init.sh must not be served forever."""
        path = self._write(
            xcvr_presence_cache_module, tmp_path, {0: True}, mtime=1000.0
        )
        monkeypatch.setattr(xcvr_presence_cache_module.time, "time", lambda: 1050.0)

        result = xcvr_presence_cache_module.read_cached_presence(
            0, path=path, max_age_secs=30
        )

        assert result is None

    def test_corrupt_yaml_returns_none_and_warns(
        self, xcvr_presence_cache_module, tmp_path
    ):
        path = tmp_path / "cache.yaml"
        path.write_text("{not: valid: yaml: at: all")
        log = MagicMock()

        result = xcvr_presence_cache_module.read_cached_presence(
            0, path=path, log_warning=log
        )

        assert result is None
        log.assert_called_once()
        assert log.call_args[0][0].startswith("xcvr presence cache read failed: ")
