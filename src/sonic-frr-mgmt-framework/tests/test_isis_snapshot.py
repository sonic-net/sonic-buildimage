"""Scoped ConfigDB capture and a real stalled-producer timeout boundary."""

import json
import subprocess
import sys
import time
from types import SimpleNamespace
from unittest.mock import Mock

import pytest

from frrcfgd.isis_manager import SnapshotReader
from frrcfgd.isis_recovery import RecoveryError
from frrcfgd.isis_snapshot import TABLES, capture


def test_capture_reads_only_component_dependencies_and_serializes_compound_keys():
    rows = {name: {} for name in TABLES}
    rows['INTERFACE'] = {('Ethernet0', '2001:db8::1/64'): {}}
    rows['SRV6_MY_SIDS'] = {('LOC', '2001:db8:1:fe01::/64'): {'action': 'uN'}}
    connector = Mock()
    connector.get_table.side_effect = lambda name: rows[name]
    snapshot = capture(connector)
    assert set(snapshot) == set(TABLES)
    assert 'Ethernet0|2001:db8::1/64' in snapshot['INTERFACE']
    assert snapshot['SRV6_MY_SIDS']['LOC|2001:db8:1:fe01::/64']['action'] == 'uN'
    assert 'BGP_GLOBALS' not in [call.args[0] for call in connector.get_table.call_args_list]


def test_duplicate_serialized_rows_never_become_an_empty_snapshot():
    connector = Mock()
    connector.get_table.side_effect = lambda name: (
        {('LOC', '2001:db8::/64'): {}, 'LOC|2001:db8::/64': {}}
        if name == 'SRV6_MY_SIDS' else {})
    with pytest.raises(ValueError):
        capture(connector)


@pytest.mark.parametrize('output', ['{}', '[]', '{"ISIS_GLOBALS": {}}', 'partial-json'])
def test_incomplete_child_output_is_rejected_before_desired_render(monkeypatch, output):
    monkeypatch.setattr('frrcfgd.isis_manager.subprocess.run',
                        lambda *args, **kwargs: SimpleNamespace(stdout=output))
    with pytest.raises((RecoveryError, ValueError)):
        SnapshotReader()()


def test_complete_empty_component_snapshot_is_explicit_not_a_transport_failure(monkeypatch):
    output = {name: {} for name in TABLES}
    monkeypatch.setattr('frrcfgd.isis_manager.subprocess.run',
                        lambda *args, **kwargs: SimpleNamespace(stdout=json.dumps(output)))
    assert SnapshotReader()() == output


def test_real_stalled_child_is_killed_within_the_parent_bound(tmp_path, monkeypatch):
    child = tmp_path / 'stalled-producer'
    child.write_text('#!' + sys.executable + '\nimport time\ntime.sleep(60)\n')
    child.chmod(0o700)
    monkeypatch.setattr('frrcfgd.isis_manager.sys.executable', str(child))
    started = time.monotonic()
    with pytest.raises(subprocess.TimeoutExpired):
        SnapshotReader(timeout=0.1)()
    assert time.monotonic() - started < 2
