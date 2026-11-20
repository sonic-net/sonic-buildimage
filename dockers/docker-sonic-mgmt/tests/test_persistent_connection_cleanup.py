"""Run with pytest in docker-sonic-mgmt to check its Ansible cleanup patch."""

import multiprocessing
import os
import queue
import socket
import threading

import pytest

from ansible.cli.scripts.ansible_connection_cli_stub import ConnectionProcess
from ansible.errors import AnsibleError
from ansible.module_utils.connection import Connection
from ansible.plugins import strategy as strategy_module


class Endpoint:
    _conn_closed = False
    _connected = False
    close_on_reset = True

    def get_option(self, name):
        return {
            "persistent_log_messages": False,
            "persistent_connect_timeout": 5,
            "persistent_command_timeout": 5,
        }[name]

    def set_options(self, direct=None):
        if direct and "close_on_reset" in direct:
            self.close_on_reset = direct["close_on_reset"]

    def reset(self):
        if self.close_on_reset:
            self.close()

    def close(self):
        self._conn_closed = True

    def pop_messages(self):
        return []


def serve(path, ready):
    process = ConnectionProcess(None, None, path, os.getcwd())
    process.connection = Endpoint()
    process.srv.register(process.connection)
    process.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    process.sock.bind(path)
    process.sock.listen(1)
    ready.set()
    process.run()


@pytest.fixture
def connection_server(tmp_path):
    path = str(tmp_path / "control")
    context = multiprocessing.get_context("fork")
    ready = context.Event()
    process = context.Process(target=serve, args=(path, ready))
    process.start()
    try:
        assert ready.wait(5), "connection server did not start"
        Connection(path).set_options(direct={})
        yield path
    finally:
        process.join(0.2)
        if process.is_alive():
            process.terminate()
            process.join(5)
        assert not process.is_alive(), "connection server did not stop"


@pytest.fixture
def strategy():
    instance = object.__new__(strategy_module.StrategyBase)
    instance._active_connections = {}
    instance._final_q = queue.Queue()
    instance._results_thread = threading.Thread(target=instance._final_q.get)
    instance._results_thread.start()
    yield instance
    if instance._results_thread.is_alive():
        instance._final_q.put(None)
        instance._results_thread.join(5)
    assert not instance._results_thread.is_alive()


@pytest.mark.parametrize("attempt", range(3))
def test_cleanup_finishes_before_next_play(strategy, connection_server, attempt):
    strategy._active_connections = {"neighbor": connection_server}
    strategy.cleanup()

    # The next play reuses a socket whenever its path still exists.
    if os.path.exists(connection_server):
        Connection(connection_server).set_options(direct={})
    assert not os.path.exists(connection_server)
    assert not strategy._results_thread.is_alive()


def test_cleanup_reports_stuck_daemon(strategy, connection_server, monkeypatch):
    Connection(connection_server).set_options(direct={"close_on_reset": False})
    strategy._active_connections = {"neighbor": connection_server}
    monkeypatch.setattr(strategy_module.C, "PERSISTENT_CONNECT_TIMEOUT", 0.05)

    with pytest.raises(AnsibleError, match="Timed out waiting for persistent connection"):
        strategy.cleanup()
    assert not strategy._results_thread.is_alive()


def test_cleanup_accepts_already_removed_socket(strategy, tmp_path):
    strategy._active_connections = {"neighbor": str(tmp_path / "missing")}
    strategy.cleanup()
    assert not strategy._results_thread.is_alive()


def test_cleanup_does_not_wait_for_replacement_socket(strategy, tmp_path, monkeypatch):
    path = tmp_path / "control"
    path.touch()
    old_fd = os.open(path, os.O_RDONLY)
    old_inode = os.fstat(old_fd).st_ino

    class ReplacingConnection:
        def __init__(self, socket_path):
            assert socket_path == str(path)

        def reset(self):
            path.unlink()
            path.touch()

    strategy._active_connections = {"neighbor": str(path)}
    monkeypatch.setattr(strategy_module, "Connection", ReplacingConnection)
    monkeypatch.setattr(strategy_module.C, "PERSISTENT_CONNECT_TIMEOUT", 0)
    try:
        strategy.cleanup()
        assert path.exists()
        assert path.stat().st_ino != old_inode
        assert not strategy._results_thread.is_alive()
    finally:
        os.close(old_fd)
