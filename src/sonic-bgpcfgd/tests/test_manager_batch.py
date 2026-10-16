from unittest.mock import MagicMock

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


def test_queued_sets_replay_in_order_and_delete_cancels_pending_peer():
    manager = make_manager()
    manager.begin_batch()
    manager.handler('keep', swsscommon.SET_COMMAND, {'value': 'old'})
    manager.handler('keep', swsscommon.SET_COMMAND, {'value': 'new'})
    manager.handler('delete', swsscommon.SET_COMMAND, {})
    manager.directory.put('CONFIG_DB', 'DEPENDENCY', 'ready', {})
    manager.handler('delete', swsscommon.DEL_COMMAND, {})
    manager.end_batch()
    assert [call.args for call in manager.set_handler.call_args_list] == [
        ('keep', {'value': 'old'}), ('keep', {'value': 'new'})]
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
