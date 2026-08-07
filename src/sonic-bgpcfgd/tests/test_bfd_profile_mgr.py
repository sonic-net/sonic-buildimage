from unittest.mock import MagicMock, patch

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test
import pytest

import sys
sys.modules["swsscommon"] = swsscommon_test

from bgpcfgd.managers_bfd_profile import BfdProfileMgr


@pytest.fixture
def bfd_profile_mgr():
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
    }
    return BfdProfileMgr(common_objs, "CONFIG_DB", "BFD_PROFILE")


def test_constructor(bfd_profile_mgr):
    assert len(bfd_profile_mgr.profiles) == 0


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_basic(mocked_run_command, bfd_profile_mgr):
    """Test creating a profile with only required fields."""
    key = "fast-bfd"
    data = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
    }
    expected_cmd = [
        'vtysh', '-c', 'conf t', '-c', 'bfd',
        '-c', 'profile fast-bfd',
        '-c', 'detect-multiplier 3',
        '-c', 'receive-interval 300',
        '-c', 'transmit-interval 300',
        '-c', 'exit',
    ]
    assert bfd_profile_mgr.set_handler(key, data) == True
    mocked_run_command.assert_called_once_with(expected_cmd)
    assert 'fast-bfd' in bfd_profile_mgr.profiles


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_all_fields(mocked_run_command, bfd_profile_mgr):
    """Test creating a profile with all fields."""
    key = "full-profile"
    data = {
        "detect_multiplier": "5",
        "receive_interval": "500",
        "transmit_interval": "500",
        "echo_interval": "100",
        "minimum_ttl": "250",
        "echo_mode": "true",
        "passive_mode": "false",
    }
    assert bfd_profile_mgr.set_handler(key, data) == True
    args = mocked_run_command.call_args[0][0]
    assert 'profile full-profile' in args
    assert 'detect-multiplier 5' in args
    assert 'receive-interval 500' in args
    assert 'transmit-interval 500' in args
    assert 'echo-interval 100' in args
    assert 'minimum-ttl 250' in args
    assert 'echo-mode' in args
    assert 'no passive-mode' in args
    assert 'full-profile' in bfd_profile_mgr.profiles


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(1, '', 'Error'))
@patch('bgpcfgd.managers_bfd_profile.log_err')
def test_set_handler_failure(mocked_log_err, mocked_run_command, bfd_profile_mgr):
    """Test vtysh command failure."""
    key = "fail-profile"
    data = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
    }
    assert bfd_profile_mgr.set_handler(key, data) == False
    assert 'fail-profile' not in bfd_profile_mgr.profiles
    mocked_log_err.assert_called_with("Can't configure BFD profile 'fail-profile': Error")


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_partial_profile_accepted(mocked_run_command, bfd_profile_mgr):
    """A profile with only a subset of fields is accepted; FRR fills in defaults
    for the rest. YANG does not mark detect_multiplier/receive_interval/
    transmit_interval as mandatory, so the manager must not reject these."""
    key = "echo-only"
    data = {"echo_interval": "100", "echo_mode": "true"}
    assert bfd_profile_mgr.set_handler(key, data) is True
    args = mocked_run_command.call_args[0][0]
    assert "profile echo-only" in args
    assert "echo-interval 100" in args
    assert "echo-mode" in args
    # No 'detect-multiplier'/'receive-interval'/'transmit-interval' lines
    for arg in args:
        assert "detect-multiplier" not in arg
        assert "receive-interval" not in arg
        assert "transmit-interval" not in arg
    assert "echo-only" in bfd_profile_mgr.profiles


@patch('bgpcfgd.managers_bfd_profile.log_err')
def test_set_handler_empty_key(mocked_log_err, bfd_profile_mgr):
    """Test empty profile name is rejected."""
    data = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
    }
    assert bfd_profile_mgr.set_handler("", data) == True
    assert len(bfd_profile_mgr.profiles) == 0
    mocked_log_err.assert_called_with("BFD profile name is empty")


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_del_handler(mocked_run_command, bfd_profile_mgr):
    """Test deleting a profile."""
    bfd_profile_mgr.profiles['test-profile'] = {"detect_multiplier": "3"}
    bfd_profile_mgr.del_handler('test-profile')
    expected_cmd = [
        'vtysh', '-c', 'conf t', '-c', 'bfd',
        '-c', 'no profile test-profile',
    ]
    mocked_run_command.assert_called_once_with(expected_cmd)
    assert 'test-profile' not in bfd_profile_mgr.profiles


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(1, '', 'Error'))
@patch('bgpcfgd.managers_bfd_profile.log_err')
def test_del_handler_failure(mocked_log_err, mocked_run_command, bfd_profile_mgr):
    """Test delete failure preserves profile in tracking."""
    bfd_profile_mgr.profiles['test-profile'] = {"detect_multiplier": "3"}
    bfd_profile_mgr.del_handler('test-profile')
    assert 'test-profile' in bfd_profile_mgr.profiles
    mocked_log_err.assert_called_with("Can't remove BFD profile 'test-profile': Error")


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_del_handler_not_tracked(mocked_run_command, bfd_profile_mgr):
    """Test deleting a profile that isn't tracked still sends vtysh command."""
    bfd_profile_mgr.del_handler('unknown-profile')
    expected_cmd = [
        'vtysh', '-c', 'conf t', '-c', 'bfd',
        '-c', 'no profile unknown-profile',
    ]
    mocked_run_command.assert_called_once_with(expected_cmd)


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_update(mocked_run_command, bfd_profile_mgr):
    """Test updating an existing profile."""
    bfd_profile_mgr.profiles['update-profile'] = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
    }
    data = {
        "detect_multiplier": "5",
        "receive_interval": "500",
        "transmit_interval": "500",
    }
    assert bfd_profile_mgr.set_handler('update-profile', data) == True
    assert bfd_profile_mgr.profiles['update-profile']['detect_multiplier'] == '5'
    assert bfd_profile_mgr.profiles['update-profile']['receive_interval'] == '500'


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_echo_mode_false(mocked_run_command, bfd_profile_mgr):
    """Test echo_mode=false renders 'no echo-mode'."""
    data = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
        "echo_mode": "false",
    }
    assert bfd_profile_mgr.set_handler('echo-test', data) == True
    args = mocked_run_command.call_args[0][0]
    assert 'no echo-mode' in args


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_passive_mode_true(mocked_run_command, bfd_profile_mgr):
    """Test passive_mode=true renders 'passive-mode' (not 'no passive-mode')."""
    data = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
        "passive_mode": "true",
    }
    assert bfd_profile_mgr.set_handler('passive-test', data) == True
    args = mocked_run_command.call_args[0][0]
    assert 'passive-mode' in args
    # Ensure 'no passive-mode' is NOT present
    assert 'no passive-mode' not in args


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_update_removes_stale_fields(mocked_run_command, bfd_profile_mgr):
    """Test that fields dropped between updates are explicitly cleared in FRR."""
    # Initial state: profile has echo_mode and echo_interval
    bfd_profile_mgr.profiles['clean-profile'] = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
        "echo_interval": "100",
        "echo_mode": "true",
    }
    # Update removes echo_interval and echo_mode
    data = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
    }
    assert bfd_profile_mgr.set_handler('clean-profile', data) is True
    args = mocked_run_command.call_args[0][0]
    # Stale fields must be explicitly cleared
    assert 'no echo-interval' in args
    assert 'no echo-mode' in args


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_optional_fields_only_when_present(mocked_run_command, bfd_profile_mgr):
    """Test that echo_interval and minimum_ttl are only rendered when present."""
    data = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
    }
    assert bfd_profile_mgr.set_handler('minimal', data) == True
    args = mocked_run_command.call_args[0][0]
    # These should NOT appear since they're not in data
    for arg in args:
        assert 'echo-interval' not in arg
        assert 'minimum-ttl' not in arg
        assert 'echo-mode' not in arg
        assert 'passive-mode' not in arg


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_boolean_flip_true_to_false(mocked_run_command, bfd_profile_mgr):
    """Boolean field flipped from 'true' (in prev) to 'false' (in data) takes
    the boolean-render branch, not the cleanup-on-absent branch. Both branches
    converge on emitting 'no echo-mode', but they are distinct code paths and
    must each be exercised."""
    bfd_profile_mgr.profiles['flip-profile'] = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
        "echo_mode": "true",
    }
    data = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
        "echo_mode": "false",
    }
    assert bfd_profile_mgr.set_handler('flip-profile', data) is True
    args = mocked_run_command.call_args[0][0]
    assert 'no echo-mode' in args
    # 'echo-mode' as a bare set must NOT also be emitted in the same call
    assert args.count('echo-mode') == 0
    # And the renderer must not duplicate 'no echo-mode' (cleanup loop is
    # skipped because echo_mode is in data; only the boolean-render branch fires).
    assert args.count('no echo-mode') == 1


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_boolean_flip_false_to_true(mocked_run_command, bfd_profile_mgr):
    """Symmetric to the true→false flip: 'false' in prev to 'true' in data
    emits 'echo-mode' (without 'no')."""
    bfd_profile_mgr.profiles['flip-profile'] = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
        "echo_mode": "false",
    }
    data = {
        "detect_multiplier": "3",
        "receive_interval": "300",
        "transmit_interval": "300",
        "echo_mode": "true",
    }
    assert bfd_profile_mgr.set_handler('flip-profile', data) is True
    args = mocked_run_command.call_args[0][0]
    assert 'echo-mode' in args
    assert 'no echo-mode' not in args


@patch('bgpcfgd.managers_bfd_profile.run_command', return_value=(0, '', ''))
def test_set_handler_numeric_value_change(mocked_run_command, bfd_profile_mgr):
    """Updating a numeric field's value (no field added/dropped) emits the
    new value without a stale-cleanup 'no <field>' line — the cleanup loop
    only fires for fields present in prev but absent in data."""
    bfd_profile_mgr.profiles['retune'] = {
        "detect_multiplier": "3",
        "receive_interval": "100",
        "transmit_interval": "100",
    }
    data = {
        "detect_multiplier": "3",
        "receive_interval": "200",
        "transmit_interval": "100",
    }
    assert bfd_profile_mgr.set_handler('retune', data) is True
    args = mocked_run_command.call_args[0][0]
    assert 'receive-interval 200' in args
    assert 'no receive-interval' not in args
