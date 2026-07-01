"""Tests for BFD profile handling in frrcfgd.

Covers:
  - hdl_bfd_timers precedence: bfd_profile > legacy timers > bare bfd
  - BGPConfigDaemon.bfd_profile_handler vtysh command construction
"""
from unittest.mock import MagicMock, NonCallableMagicMock, patch

# Mock external modules that frrcfgd imports at module load time
swsscommon_module_mock = MagicMock(ConfigDBConnector=NonCallableMagicMock)
mockmapping = {
    'swsscommon.swsscommon': swsscommon_module_mock,
    'bgpcfgd': MagicMock(),
    'bgpcfgd.managers_bfd': MagicMock(),
    'bgpcfgd.directory': MagicMock(),
    'bgpcfgd.log': MagicMock(),
    'bgpcfgd.utils': MagicMock(),
}

with patch.dict('sys.modules', **mockmapping):
    from frrcfgd.frrcfgd import (
        BGPConfigDaemon,
        CachedDataWithOp,
        hdl_bfd_timers,
    )


# ---------------------------------------------------------------------------
# hdl_bfd_timers precedence
# ---------------------------------------------------------------------------

def _call(args):
    """Invoke hdl_bfd_timers with SET op and the given (neighbor, *fields)."""
    # st_idx=1: args[0] is neighbor, args[1..] are field values matching the
    # key_map order ['bfd', '+detect_mult', '+min_rx', '+min_tx', '+bfd_profile']
    return hdl_bfd_timers('bgpd', '', CachedDataWithOp.OP_ADD, 1, args, {})


def test_bfd_profile_takes_priority_over_legacy_timers():
    """When bfd_profile is set, emit two commands and ignore legacy timers."""
    cmds = _call(['10.0.0.1', 'true', '5', '300', '300', 'fast-failover'])
    assert len(cmds) == 2
    assert 'neighbor 10.0.0.1 bfd' in cmds[0]
    assert '5' not in cmds[0]  # legacy timers must NOT be in the bare enable line
    assert 'neighbor 10.0.0.1 bfd profile fast-failover' in cmds[1]


def test_bfd_profile_alone_no_timers():
    """Profile set, no timer fields — still emits both enable + profile."""
    cmds = _call(['10.0.0.1', 'true', '', '', '', 'fast-failover'])
    assert len(cmds) == 2
    assert 'neighbor 10.0.0.1 bfd' in cmds[0]
    assert 'neighbor 10.0.0.1 bfd profile fast-failover' in cmds[1]


def test_legacy_timers_when_no_profile():
    """Profile absent, all three timers present — emits the legacy combined form."""
    cmds = _call(['10.0.0.1', 'true', '5', '300', '300', None])
    assert len(cmds) == 1
    assert 'neighbor 10.0.0.1 bfd 5 300 300' in cmds[0]


def test_bare_bfd_when_no_profile_no_timers():
    """No profile, no timers — bare enable."""
    cmds = _call(['10.0.0.1', 'true', '', '', '', None])
    assert len(cmds) == 1
    assert cmds[0].rstrip().endswith('neighbor 10.0.0.1 bfd')


def test_bfd_disabled_returns_none():
    """bfd=false short-circuits."""
    assert _call(['10.0.0.1', 'false', '', '', '', 'fast-failover']) is None


def test_delete_op_returns_no_neighbor_bfd():
    cmds = hdl_bfd_timers('bgpd', '', CachedDataWithOp.OP_DELETE, 1,
                          ['10.0.0.1', 'true', '', '', '', 'fast-failover'], {})
    assert len(cmds) == 1
    assert 'no neighbor 10.0.0.1 bfd' in cmds[0]


def test_backward_compat_no_bfd_profile_field():
    """Old key_map shape with only 4 fields (no +bfd_profile) still works."""
    cmds = _call(['10.0.0.1', 'true', '5', '300', '300'])  # no 6th arg
    assert len(cmds) == 1
    assert 'neighbor 10.0.0.1 bfd 5 300 300' in cmds[0]


# ---------------------------------------------------------------------------
# bfd_profile_handler — vtysh command construction
# ---------------------------------------------------------------------------

def _make_daemon_for_handler():
    """Construct a minimal BGPConfigDaemon-like object for handler testing."""
    daemon = BGPConfigDaemon.__new__(BGPConfigDaemon)
    daemon._BGPConfigDaemon__run_command = MagicMock(return_value=True)
    daemon._bfd_profile_state = {}
    return daemon


def _all_commands(daemon):
    """Return list of vtysh command strings passed across all __run_command calls."""
    return [c.args[1] for c in daemon._BGPConfigDaemon__run_command.call_args_list]


def _last_command(daemon):
    """Return the vtysh command string passed to the most recent __run_command."""
    return daemon._BGPConfigDaemon__run_command.call_args[0][1]


def test_handler_set_with_required_fields_only():
    daemon = _make_daemon_for_handler()
    daemon.bfd_profile_handler('BFD_PROFILE', 'fast-failover', {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
    })
    cmd = _last_command(daemon)
    assert "'profile fast-failover'" in cmd
    assert "'detect-multiplier 3'" in cmd
    assert "'receive-interval 100'" in cmd
    assert "'transmit-interval 100'" in cmd
    assert "'exit'" in cmd


def test_handler_set_with_all_fields():
    daemon = _make_daemon_for_handler()
    daemon.bfd_profile_handler('BFD_PROFILE', 'full-profile', {
        'detect_multiplier': '5',
        'receive_interval': '500',
        'transmit_interval': '500',
        'echo_interval': '100',
        'minimum_ttl': '250',
        'echo_mode': 'true',
        'passive_mode': 'false',
    })
    cmd = _last_command(daemon)
    assert "'profile full-profile'" in cmd
    assert "'echo-interval 100'" in cmd
    assert "'minimum-ttl 250'" in cmd
    assert "'echo-mode'" in cmd
    assert "'no passive-mode'" in cmd


def test_handler_delete():
    daemon = _make_daemon_for_handler()
    daemon.bfd_profile_handler('BFD_PROFILE', 'fast-failover', None)
    cmd = _last_command(daemon)
    assert "'no profile fast-failover'" in cmd


def test_handler_partial_profile_accepted():
    """A profile with only a subset of fields is accepted; FRR fills in defaults
    for the rest. YANG does not mark detect_multiplier/receive_interval/
    transmit_interval as mandatory."""
    daemon = _make_daemon_for_handler()
    daemon.bfd_profile_handler('BFD_PROFILE', 'echo-only', {
        'echo_interval': '100',
        'echo_mode': 'true',
    })
    cmd = _last_command(daemon)
    assert "'profile echo-only'" in cmd
    assert "'echo-interval 100'" in cmd
    assert "'echo-mode'" in cmd
    assert "detect-multiplier" not in cmd
    assert "receive-interval" not in cmd
    assert "transmit-interval" not in cmd
    assert 'echo-only' in daemon._bfd_profile_state


def test_handler_passive_mode_true():
    daemon = _make_daemon_for_handler()
    daemon.bfd_profile_handler('BFD_PROFILE', 'passive-test', {
        'detect_multiplier': '3',
        'receive_interval': '300',
        'transmit_interval': '300',
        'passive_mode': 'true',
    })
    cmd = _last_command(daemon)
    assert "'passive-mode'" in cmd
    assert "'no passive-mode'" not in cmd


def test_handler_clears_dropped_numeric_field_on_update():
    """Removing echo_interval across an update emits 'no echo-interval'."""
    daemon = _make_daemon_for_handler()
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
        'echo_interval': '200',
    })
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
    })
    cmd = _all_commands(daemon)[-1]
    assert "'no echo-interval'" in cmd


def test_handler_clears_dropped_boolean_field_on_update():
    """Removing echo_mode across an update emits 'no echo-mode'."""
    daemon = _make_daemon_for_handler()
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
        'echo_mode': 'true',
    })
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
    })
    cmd = _all_commands(daemon)[-1]
    # Order: 'no <removed>' precedes the rendered fields, but both forms of
    # 'no echo-mode' (cleanup and explicit-false) collapse to the same string;
    # only the cleanup branch fires here because echo_mode is absent in data.
    assert "'no echo-mode'" in cmd


def test_handler_delete_clears_state():
    """After a delete the state is forgotten and a later set has no 'no' lines."""
    daemon = _make_daemon_for_handler()
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
        'echo_interval': '200',
    })
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', None)
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
    })
    cmd = _all_commands(daemon)[-1]
    assert "'no echo-interval'" not in cmd


def test_handler_set_failure_does_not_cache_state():
    """A vtysh failure must not poison the state cache. The next successful
    update should still see the FRR-applied prev state, not the failed one."""
    daemon = _make_daemon_for_handler()
    daemon._BGPConfigDaemon__run_command.return_value = False
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
        'echo_interval': '200',
    })
    assert 'p1' not in daemon._bfd_profile_state


def test_handler_delete_failure_does_not_drop_state():
    """A vtysh failure on delete must not drop the state — the next set call
    needs the prev mapping to compute correct cleanup commands."""
    daemon = _make_daemon_for_handler()
    daemon._bfd_profile_state['p1'] = {'echo_interval': '200'}
    daemon._BGPConfigDaemon__run_command.return_value = False
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', None)
    assert 'p1' in daemon._bfd_profile_state


def test_handler_boolean_flip_true_to_false():
    """Boolean field flipped from 'true' (in prev) to 'false' (in data) hits
    the boolean-render branch (emits 'no echo-mode'), not the cleanup-on-absent
    branch — both converge on the same vtysh fragment but are distinct paths."""
    daemon = _make_daemon_for_handler()
    daemon._bfd_profile_state['p1'] = {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
        'echo_mode': 'true',
    }
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
        'echo_mode': 'false',
    })
    cmd = _last_command(daemon)
    assert "'no echo-mode'" in cmd
    # The cleanup loop must NOT also fire (echo_mode is in data),
    # so 'no echo-mode' appears exactly once.
    assert cmd.count("'no echo-mode'") == 1
    # And the bare 'echo-mode' setter must NOT appear.
    assert "'echo-mode'" not in cmd.replace("'no echo-mode'", '')


def test_handler_boolean_flip_false_to_true():
    """Symmetric flip false→true emits 'echo-mode' (without 'no')."""
    daemon = _make_daemon_for_handler()
    daemon._bfd_profile_state['p1'] = {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
        'echo_mode': 'false',
    }
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
        'echo_mode': 'true',
    })
    cmd = _last_command(daemon)
    assert "'echo-mode'" in cmd
    assert "'no echo-mode'" not in cmd


def test_handler_numeric_value_change():
    """Changing a numeric field's value emits the new line without a stale
    'no receive-interval' — cleanup only fires for prev-only fields."""
    daemon = _make_daemon_for_handler()
    daemon._bfd_profile_state['p1'] = {
        'detect_multiplier': '3',
        'receive_interval': '100',
        'transmit_interval': '100',
    }
    daemon.bfd_profile_handler('BFD_PROFILE', 'p1', {
        'detect_multiplier': '3',
        'receive_interval': '200',
        'transmit_interval': '100',
    })
    cmd = _last_command(daemon)
    assert "'receive-interval 200'" in cmd
    assert "'no receive-interval'" not in cmd


# ---------------------------------------------------------------------------
# hdl_bfd_timers — additional edge cases
# ---------------------------------------------------------------------------

def test_hdl_bfd_timers_empty_string_profile_falls_through():
    """An empty-string bfd_profile must NOT trigger the two-step emission;
    it should fall through to the legacy/timers branch. The handler relies
    on Python truthiness ('' is falsy) — pin that behavior."""
    cmds = _call(['10.0.0.1', 'true', '5', '300', '300', ''])
    assert len(cmds) == 1
    assert 'neighbor 10.0.0.1 bfd 5 300 300' in cmds[0]
    assert 'profile' not in cmds[0]


def test_hdl_bfd_timers_profile_without_timers_emits_two_steps():
    """bfd_profile present, all three timers absent — still two-step."""
    cmds = _call(['10.0.0.1', 'true', None, None, None, 'fast-failover'])
    assert len(cmds) == 2
    assert cmds[0].rstrip().endswith('neighbor 10.0.0.1 bfd')
    assert cmds[1].rstrip().endswith('neighbor 10.0.0.1 bfd profile fast-failover')
