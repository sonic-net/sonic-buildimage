import importlib.machinery
import importlib.util
import os
from pathlib import Path
import signal
import socket
import subprocess
import sys
import time
from unittest import mock
from xmlrpc.client import Fault, ServerProxy

import pytest
from supervisor.xmlrpc import Faults
from supervisord_dependent_startup import supervisord_dependent_startup as startup


SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "supervisor-dependent-startup"


@pytest.fixture
def launcher():
    loader = importlib.machinery.SourceFileLoader("supervisor_dependent_startup", str(SCRIPT))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    handlers = {sig: signal.getsignal(sig) for sig in (signal.SIGTERM, signal.SIGINT)}
    try:
        yield module
    finally:
        for sig, handler in handlers.items():
            signal.signal(sig, handler)


@pytest.mark.parametrize("signum", [signal.SIGTERM, signal.SIGINT])
def test_signal_cancels_startup(launcher, signum, caplog, capsys):
    def interrupted_startup():
        os.kill(os.getpid(), signum)
        pytest.fail("Startup continued after cancellation")

    with mock.patch.object(startup, "main", side_effect=interrupted_startup):
        with caplog.at_level("INFO"), pytest.raises(SystemExit) as result:
            launcher.main()
    assert result.value.code == 0
    assert "cancelled by signal" in caplog.text
    assert "Dependency startup failed" not in caplog.text
    assert capsys.readouterr().out == ""


def test_supervisor_shutdown_cancels_startup(launcher, caplog):
    with mock.patch.object(startup, "main", side_effect=Fault(Faults.SHUTDOWN_STATE, "SHUTDOWN_STATE")):
        with caplog.at_level("INFO"):
            assert launcher.main() == 0
    assert "supervisord is shutting down" in caplog.text
    assert "Dependency startup failed" not in caplog.text


@pytest.mark.parametrize("error", [
    RuntimeError("dependency failed"),
    Fault(Faults.BAD_NAME, "BAD_NAME"),
    ConnectionRefusedError("supervisor socket unavailable"),
    EOFError("event stream closed"),
])
def test_real_startup_failure_is_not_cancellation(launcher, error, caplog):
    with mock.patch.object(startup, "main", side_effect=error):
        assert launcher.main() == 3
    assert "Dependency startup failed" in caplog.text
    assert "cancelled" not in caplog.text
    assert caplog.records[-1].exc_info[1] is error


@pytest.mark.parametrize("result", [None, 0, 2, 4])
def test_startup_result_is_preserved(launcher, result):
    with mock.patch.object(startup, "main", return_value=result):
        assert launcher.main() == (result or 0)


@pytest.mark.parametrize("code", [0, 2])
def test_system_exit_is_not_converted_to_unhandled_failure(launcher, code, caplog):
    with mock.patch.object(startup, "main", side_effect=SystemExit(code)):
        with pytest.raises(SystemExit) as result:
            launcher.main()
    assert result.value.code == code
    assert "Dependency startup failed" not in caplog.text


def test_missing_configuration_fails(tmp_path):
    result = subprocess.run(
        [sys.executable, str(SCRIPT), "-c", str(tmp_path / "missing.conf")],
        capture_output=True, text=True, timeout=10
    )
    assert result.returncode == 2
    assert "does not exist" in result.stderr
    assert "cancelled" not in result.stderr
    assert result.stdout == ""


def wait_for(predicate, timeout=10):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.05)
    pytest.fail("Timed out waiting for Supervisor state")


@pytest.fixture
def supervisor(tmp_path):
    processes = []
    root = tmp_path

    def start(generation, ready=False):
        directory = root / str(generation)
        directory.mkdir()
        marker = directory / "ready"
        if ready:
            marker.touch()
        init = directory / "init.py"
        init.write_text(
            "from pathlib import Path\nimport time\n"
            "while not Path({!r}).exists():\n    time.sleep(0.05)\n".format(str(marker))
        )
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            port = sock.getsockname()[1]
        config = directory / "supervisord.conf"
        config.write_text(
            "[supervisord]\n"
            "nodaemon=true\n"
            "logfile={directory}/supervisord.log\n"
            "pidfile={directory}/supervisord.pid\n"
            "childlogdir={directory}\n"
            "[inet_http_server]\n"
            "port=127.0.0.1:{port}\n"
            "[rpcinterface:supervisor]\n"
            "supervisor.rpcinterface_factory=supervisor.rpcinterface:make_main_rpcinterface\n"
            "[eventlistener:dependent-startup]\n"
            "command={python} {script} -c {config}\n"
            "autostart=true\n"
            "autorestart=unexpected\n"
            "startsecs=0\n"
            "startretries=0\n"
            "exitcodes=0,3\n"
            "events=PROCESS_STATE\n"
            "buffer_size=1024\n"
            "stderr_logfile={directory}/startup.log\n"
            "[program:init]\n"
            "command={python} {init}\n"
            "autostart=false\n"
            "autorestart=false\n"
            "startsecs=0\n"
            "dependent_startup=true\n"
            "[program:relay]\n"
            'command={python} -c "import time; time.sleep(60)"\n'
            "autostart=false\n"
            "autorestart=false\n"
            "startsecs=0\n"
            "dependent_startup=true\n"
            "dependent_startup_wait_for=init:exited\n".format(
                directory=directory, port=port, python=sys.executable,
                script=SCRIPT, config=config, init=init
            )
        )
        process = subprocess.Popen(
            [sys.executable, "-m", "supervisor.supervisord", "-c", str(config)],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
        )
        processes.append(process)
        rpc = ServerProxy("http://127.0.0.1:{}".format(port)).supervisor

        def listening():
            if process.poll() is not None:
                pytest.fail(process.communicate()[0])
            try:
                return rpc.getState()["statename"] == "RUNNING"
            except OSError:
                return False

        wait_for(listening)
        return process, rpc, directory

    yield start
    for process in processes:
        if process.poll() is None:
            process.terminate()
        try:
            process.communicate(timeout=15)
        except subprocess.TimeoutExpired:
            process.kill()
            process.communicate()


@pytest.mark.parametrize("stop_method", ["SIGTERM", "shutdown"])
def test_supervisor_stops_during_initialization(supervisor, stop_method):
    process, rpc, directory = supervisor(0)
    wait_for(lambda: rpc.getProcessInfo("init")["statename"] == "RUNNING")
    assert rpc.getProcessInfo("relay")["statename"] == "STOPPED"
    if stop_method == "SIGTERM":
        process.terminate()
    else:
        rpc.shutdown()
    output = process.communicate(timeout=15)[0]
    assert process.returncode == 0, output
    assert "Dependency startup failed" not in (directory / "startup.log").read_text()
    supervisor_log = (directory / "supervisord.log").read_text()
    assert "exit status 3" not in supervisor_log
    assert "FATAL" not in supervisor_log


def test_repeated_interruption_then_convergence(supervisor):
    for generation in range(10):
        process, rpc, directory = supervisor(generation)
        wait_for(lambda: rpc.getProcessInfo("init")["statename"] == "RUNNING")
        helper = rpc.getProcessInfo("dependent-startup")
        os.kill(helper["pid"], signal.SIGTERM)
        wait_for(lambda: rpc.getProcessInfo("dependent-startup")["statename"] == "EXITED")
        assert rpc.getProcessInfo("dependent-startup")["exitstatus"] == 0
        assert rpc.getProcessInfo("relay")["statename"] == "STOPPED"
        rpc.shutdown()
        output = process.communicate(timeout=15)[0]
        assert process.returncode == 0, output
        assert "Dependency startup failed" not in (directory / "startup.log").read_text()

    process, rpc, directory = supervisor(10, ready=True)
    wait_for(lambda: rpc.getProcessInfo("relay")["statename"] == "RUNNING")
    wait_for(lambda: rpc.getProcessInfo("dependent-startup")["statename"] == "EXITED")
    assert rpc.getProcessInfo("dependent-startup")["exitstatus"] == 0
    assert "Dependency startup failed" not in (directory / "startup.log").read_text()
    rpc.shutdown()
    output = process.communicate(timeout=15)[0]
    assert process.returncode == 0, output
