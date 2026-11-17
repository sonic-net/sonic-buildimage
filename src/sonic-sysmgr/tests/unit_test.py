"""Run the unchanged native Sysmgr GTests with a private declared Redis server."""
import argparse
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import tarfile
import xml.etree.ElementTree as ET

from installed_reboot_test import Redis, wait_for


def test_names(output):
    names, suite = set(), None
    for line in output.splitlines():
        text = line.split("#", 1)[0].strip()
        if not text:
            continue
        if not line.startswith(" "):
            assert text.endswith("."), line
            suite = text
        else:
            assert suite is not None and " " not in text, line
            name = suite + text
            assert name not in names, name
            names.add(name)
    assert len(names) == 52, ("Original 14 fixed and 19 x 2 parameterized tests", names)
    return names


def private(args):
    assert os.environ.get("SONIC_SYSMGR_PRIVATE_FIXTURE") == "1"
    config_dir = Path("/run/redis/sonic-db")
    config_dir.mkdir(parents=True)
    assert Path("/var/run/redis/sonic-db").resolve() == config_dir
    sock = config_dir.parent / "redis.sock"
    config = {
        "INSTANCES": {"redis": {"hostname": "127.0.0.1", "port": 0, "unix_socket_path": str(sock)}},
        "DATABASES": {name: {"id": number, "separator": "|", "instance": "redis"}
                      for name, number in (("CONFIG_DB", 4), ("STATE_DB", 6))},
    }
    (config_dir / "database_config.json").write_text(json.dumps(config))
    evidence = Path("/evidence")
    def command(program, *arguments):
        return [args.loader, "--library-path", args.library_path, program, *arguments]
    with (evidence / "redis.log").open("wb") as log:
        redis = subprocess.Popen(command("/usr/bin/redis-server", "--port", "0",
            "--unixsocket", str(sock), "--unixsocketperm", "700", "--save", "", "--appendonly", "no"),
            stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            def ready():
                probe = None
                try:
                    probe = Redis(sock)
                    return probe.call("PING") == "PONG"
                except (OSError, AssertionError):
                    return False
                finally:
                    if probe is not None:
                        probe.close()
            wait_for(ready, {"redis": redis})
            listed = subprocess.run(command("/work/tests", "--gtest_list_tests"),
                capture_output=True, text=True, timeout=30)
            (evidence / "test-list.log").write_text(listed.stdout + listed.stderr)
            assert listed.returncode == 0, listed.stdout + listed.stderr
            expected = test_names(listed.stdout)
            executed = subprocess.run(command("/work/tests", "--gtest_output=xml:/evidence/original-tests.xml"),
                cwd="/work", capture_output=True, text=True, timeout=300)
            (evidence / "original-tests.log").write_text(executed.stdout + executed.stderr)
            print(executed.stdout, end="")
            print(executed.stderr, end="")
            assert executed.returncode == 0, executed.returncode
            tree = ET.parse(evidence / "original-tests.xml")
            assert not any(list(tree.iter(kind)) for kind in ("failure", "error", "skipped"))
            cases = list(tree.iter("testcase"))
            actual = {case.attrib["classname"] + "." + case.attrib["name"] for case in cases}
            assert actual == expected and len(cases) == len(expected), (expected - actual, actual - expected)
            assert all(case.attrib.get("status") == "run" for case in cases)
            (evidence / "original-tests.json").write_text(json.dumps({"status": "passed",
                "tests": sorted(actual), "count": len(cases), "scope": "Original unsanitized native GTests; mock D-Bus interface and private Unix Redis"}, indent=2) + "\n")
        finally:
            if redis.poll() is None:
                os.killpg(redis.pid, signal.SIGTERM)
                try:
                    redis.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(redis.pid, signal.SIGKILL)
                    redis.wait(timeout=5)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--private", action="store_true")
    for flag in ("binary", "dependencies", "runtime-tar", "loader", "library-path"):
        parser.add_argument("--" + flag)
    args = parser.parse_args()
    if args.private:
        private(args)
        return
    # Reuse the installed fixture's declared-runtime extraction policy.
    from package_test import installed_member
    runtime = Path(os.environ["TEST_TMPDIR"]) / "sysmgr-unit-runtime"
    runtime.mkdir()
    evidence = Path(os.environ["TEST_UNDECLARED_OUTPUTS_DIR"])
    evidence.mkdir(parents=True, exist_ok=True)
    with tarfile.open(args.dependencies) as archive:
        archive.extractall(runtime, filter=installed_member)
    with tarfile.open(args.runtime_tar) as archive:
        archive.extractall(runtime, filter="data")
    loaders = {p.resolve() for p in runtime.rglob("ld-linux-x86-64.so.2") if p.is_file()}
    assert len(loaders) == 1
    loader = loaders.pop()
    libraries = sorted({p.parent.resolve() for p in runtime.rglob("lib*.so*") if p.is_file()})
    assert loader.is_relative_to(runtime) and all(p.is_relative_to(runtime) for p in libraries)
    for name in ("run", "tmp", "proc", "dev", "work", "evidence", "var"):
        (runtime / name).mkdir(exist_ok=True)
    var_run = runtime / "var/run"
    if not var_run.exists() and not var_run.is_symlink():
        var_run.symlink_to("../run")
    assert var_run.resolve() == runtime / "run"
    env = dict(os.environ, LC_ALL="C")
    for name in ("LD_PRELOAD", "LD_LIBRARY_PATH", "LD_AUDIT", "PYTHONPATH", "PYTHONHOME"):
        env.pop(name, None)
    script = Path(__file__).absolute()
    private_loader = "/" + str(loader.relative_to(runtime))
    private_libraries = ":".join("/" + str(p.relative_to(runtime)) for p in libraries)
    argv = [str(loader), "--library-path", ":".join(map(str, libraries)), str(runtime / "usr/bin/bwrap"),
        "--unshare-all", "--die-with-parent", "--new-session", "--ro-bind", str(runtime), "/",
        "--proc", "/proc", "--dev", "/dev", "--tmpfs", "/run", "--tmpfs", "/tmp", "--tmpfs", "/work",
        "--bind", str(evidence), "/evidence", "--ro-bind", str(Path(args.binary).resolve()), "/work/tests",
        "--ro-bind", str(script), "/work/unit_test.py", "--ro-bind",
        str(script.with_name("installed_reboot_test.py")), "/work/installed_reboot_test.py",
        "--clearenv", "--setenv", "SONIC_SYSMGR_PRIVATE_FIXTURE", "1", "--setenv", "LC_ALL", "C",
        "--setenv", "PYTHONHOME", "/usr", "--", private_loader, "--library-path", private_libraries,
        "/usr/bin/python3.13", "/work/unit_test.py", "--private", "--loader", private_loader,
        "--library-path", private_libraries]
    result = subprocess.run(argv, env=env, capture_output=True, text=True, timeout=360)
    (evidence / "namespace.log").write_text(result.stdout + result.stderr)
    assert result.returncode == 0, result.stdout + result.stderr
    # Preserve the original GTest report as Bazel's test.xml, not an empty wrapper report.
    shutil.copyfile(evidence / "original-tests.xml", os.environ["XML_OUTPUT_FILE"])


if __name__ == "__main__":
    main()
