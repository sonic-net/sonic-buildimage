from unittest.mock import MagicMock

import pytest

from bgpcfgd.directory import Directory
from bgpcfgd.manager import Manager, swsscommon


def make_manager(batch_deps=True, wait_for_all_deps=True):
    directory = Directory()
    common = {'directory': directory, 'cfg_mgr': MagicMock(), 'constants': {}}
    manager = Manager(common, [('CONFIG_DB', 'DEPENDENCY', 'ready')], 'CONFIG_DB', 'PEERS',
                      wait_for_all_deps=wait_for_all_deps, batch_deps=batch_deps)
    manager.set_handler = MagicMock(return_value=True)
    manager.del_handler = MagicMock()
    return manager


def test_notifications_coalesce_and_retry_before_batch_returns():
    manager = make_manager()
    manager.begin_batch()
    manager.handler('peer', swsscommon.SET_COMMAND, {'value': 'latest'})
    for value in range(100):
        manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', value)
    manager.set_handler.assert_not_called()
    manager.end_batch()
    manager.set_handler.assert_called_once_with('peer', {'value': 'latest'})
    assert not manager.set_queue


def test_missing_dependency_survives_until_later_batch():
    manager = make_manager()
    manager.begin_batch()
    manager.handler('peer', swsscommon.SET_COMMAND, {})
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'unrelated', {})
    manager.end_batch()
    manager.set_handler.assert_not_called()
    assert any(key == 'peer' for key, _ in manager.set_queue)
    manager.begin_batch()
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
    manager.end_batch()
    manager.set_handler.assert_called_once()
    assert not manager.set_queue


def test_not_ready_retries_only_on_later_change():
    manager = make_manager()
    manager.set_handler.return_value = False
    manager.begin_batch()
    manager.handler('peer', swsscommon.SET_COMMAND, {})
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
    manager.end_batch()
    assert manager.set_handler.call_count == 1
    assert any(key == 'peer' for key, _ in manager.set_queue)
    manager.begin_batch()
    manager.end_batch()
    assert manager.set_handler.call_count == 1
    manager.set_handler.return_value = True
    manager.begin_batch()
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
    manager.end_batch()
    assert manager.set_handler.call_count == 2
    assert not manager.set_queue


def test_queued_sets_keep_latest_snapshot_and_delete_cancels_pending_peer():
    manager = make_manager()
    manager.begin_batch()
    manager.handler('keep', swsscommon.SET_COMMAND, {'value': 'old'})
    manager.handler('keep', swsscommon.SET_COMMAND, {'value': 'new'})
    manager.handler('delete', swsscommon.SET_COMMAND, {})
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
    manager.handler('delete', swsscommon.DEL_COMMAND, {})
    manager.end_batch()
    assert [call.args for call in manager.set_handler.call_args_list] == [
        ('keep', {'value': 'new'})]
    manager.del_handler.assert_called_once_with('delete')
    assert not manager.set_queue


def test_immediate_notifications_for_other_managers_and_direct_calls():
    for batch_deps, begin in [(False, True), (True, False)]:
        manager = make_manager(batch_deps=batch_deps)
        if begin:
            manager.begin_batch()
        manager.handler('peer', swsscommon.SET_COMMAND, {})
        manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
        manager.set_handler.assert_called_once()


def test_partial_dependency_manager_keeps_its_contract():
    manager = make_manager(batch_deps=False, wait_for_all_deps=False)
    manager.set_handler.return_value = False
    manager.begin_batch()
    manager.handler('peer', swsscommon.SET_COMMAND, {})
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
    assert manager.set_handler.call_count == 2


@pytest.mark.parametrize('batch_deps', [False, True])
@pytest.mark.parametrize('mode', ['direct', 'same_pass', 'later_pass'])
def test_ready_replacement_cancels_pending_set_without_dependency_change(batch_deps, mode):
    manager = make_manager(batch_deps=batch_deps)
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
    manager.set_handler.side_effect = [False, True]
    if mode != 'direct':
        manager.begin_batch()
    manager.handler('peer', swsscommon.SET_COMMAND, {'value': 'old'})
    if mode == 'later_pass':
        manager.end_batch()
        manager.begin_batch()
    manager.handler('peer', swsscommon.SET_COMMAND, {'value': 'new'})
    manager.end_batch()
    assert [call.args for call in manager.set_handler.call_args_list] == [
        ('peer', {'value': 'old'}), ('peer', {'value': 'new'})]
    assert not manager.set_queue
    # No old payload may be replayed by a later unrelated notification.
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
    assert manager.set_handler.call_count == 2


@pytest.mark.parametrize('batch_deps', [False, True])
@pytest.mark.parametrize('across_passes', [False, True])
@pytest.mark.parametrize('operations,expected', [
    (['one', 'two', 'three'], 'three'),
    (['one', 'two', 'DEL'], None),
    (['one', 'DEL', 'two', 'three'], 'three'),
    (['one', 'DEL', 'two', 'DEL'], None),
])
def test_pending_snapshots_and_deletes(batch_deps, across_passes, operations, expected):
    manager = make_manager(batch_deps=batch_deps)
    manager.begin_batch()
    for operation in operations:
        if operation == 'DEL':
            manager.handler('peer', swsscommon.DEL_COMMAND, {})
        else:
            manager.handler('peer', swsscommon.SET_COMMAND, {'value': operation})
        if across_passes:
            manager.end_batch()
            manager.begin_batch()
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
    manager.end_batch()
    expected_calls = [] if expected is None else [('peer', {'value': expected})]
    assert [call.args for call in manager.set_handler.call_args_list] == expected_calls
    assert manager.del_handler.call_count == operations.count('DEL')
    assert not manager.set_queue


def test_pending_replacement_does_not_merge_removed_fields():
    manager = make_manager()
    manager.begin_batch()
    manager.handler('peer', swsscommon.SET_COMMAND, {'value': 'old', 'removed': 'field'})
    manager.handler('peer', swsscommon.SET_COMMAND, {'value': 'new'})
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
    manager.end_batch()
    manager.set_handler.assert_called_once_with('peer', {'value': 'new'})
