#!/usr/bin/env python3
"""Run inside the assembled config-engine image, without source checkouts."""

import ctypes
import hashlib
import importlib
import importlib.metadata
import json
import os
from pathlib import Path
import platform
import pwd
import grp
import subprocess
import sys
import tempfile


def command(*arguments, input=None):
    result = subprocess.run(arguments, input=input, text=True, capture_output=True, timeout=120)
    if result.returncode:
        raise RuntimeError(repr(arguments) + "\n" + result.stdout + result.stderr)
    return result.stdout + result.stderr


def main():
    report = json.loads(Path(sys.argv[1]).read_text())
    machine = platform.machine()
    triplet, loader = {
        "x86_64": ("x86_64-linux-gnu", "/lib64/ld-linux-x86-64.so.2"),
        "aarch64": ("aarch64-linux-gnu", "/lib/ld-linux-aarch64.so.1"),
    }[machine]
    # Account additions must preserve the inherited Debian users and groups.
    assert pwd.getpwnam("root").pw_uid == 0
    assert pwd.getpwnam("nobody").pw_uid == 65534
    redis = pwd.getpwnam("redis")
    assert (redis.pw_uid, redis.pw_gid, redis.pw_dir, redis.pw_shell) == (
        100, 101, "/var/lib/redis", "/usr/sbin/nologin")
    assert grp.getgrnam("redis").gr_gid == 101
    for alias, target in {"/bin": "usr/bin", "/sbin": "usr/sbin", "/lib": "usr/lib", "/var/run": "/run"}.items():
        assert os.path.islink(alias) and os.readlink(alias) == target, alias
    assert Path("/var/run/redis/sonic-db/database_config.json").resolve() == Path("/run/redis/sonic-db/database_config.json")
    assert "DATABASES" in json.loads(Path("/var/run/redis/sonic-db/database_config.json").read_text())
    database_config = os.stat("/run/redis/sonic-db/database_config.json")
    assert (database_config.st_uid, database_config.st_gid, database_config.st_mode & 0o7777) == (0, 0, 0o644)
    for name in ("classid", "pktloc"):
        assert (Path("/etc/libnl-3") / name).is_file(), name
    for name, item in report["native"].items():
        binary = Path("/") / name
        assert hashlib.sha256(binary.read_bytes()).hexdigest() == item["sha256"], name
        output = command(loader, "--list", str(binary))
        assert "not found" not in output, (name, output)
    for name in ("swsscommon.swsscommon", "libyang", "sonic_yang", "sonic_yang_ext", "sonic_yang_path",
                 "sonic_py_common", "openconfig_acl", "minigraph",
                 "sonic_yang_cfg_generator", "supervisor", "supervisord_dependent_startup"):
        module = importlib.import_module(name)
        assert str(Path(module.__file__).resolve()).startswith(("/usr/lib/", "/usr/local/lib/")), name
    # Importing the daemon starts its boot wait against SONiC's external Redis.
    # A standalone base image has no Redis service; verify the installed entry
    # point here and leave that connection to the complete service-image check.
    daemon = importlib.util.find_spec("containercfgd.containercfgd")
    assert daemon and str(Path(daemon.origin).resolve()).startswith("/usr/local/lib/")
    assert importlib.metadata.version("sonic-yang-mgmt") == "1.0"
    # Make-built child containers resolve the config-engine wheel's dependency
    # on this distribution. Model files alone do not satisfy pip's resolver.
    assert importlib.metadata.version("sonic-yang-models") == "1.0"
    for directory in ("yang-models", "cvlyang-models"):
        assert list((Path("/usr/local") / directory).glob("*.yang")), directory
    os.environ["PLATFORM"] = "sonic-bazel-build"
    with tempfile.TemporaryDirectory() as temporary:
        template = Path(temporary) / "config.j2"
        template.write_text("{{ prefix|network }}/{{ prefix|prefixlen }} {{ DEVICE_METADATA.localhost.hostname }}")
        rendered = command("sonic-cfggen", "-a", json.dumps({"prefix": "192.0.2.9/24", "DEVICE_METADATA": {
            "localhost": {"hostname": "source-base"}}}), "-t", str(template))
        assert rendered.strip() == "192.0.2.0/24 source-base", rendered
        template.write_text("{{ name }}")
        assert command("j2", "--quiet", "-f", "json", str(template), "-", input='{"name":"source-base"}').strip() == "source-base"
    import sonic_yang
    import sonic_yang_ext
    manager = sonic_yang.SonicYang("/usr/local/yang-models", print_log_enabled=False)
    assert manager.loadYangModel() is True
    manager.loadData({"SYSLOG_SERVER": {"192.0.2.1": {"port": "514", "protocol": "udp", "severity": "info"}}}, quiet=True)
    manager.validate_data_tree()
    assert not manager.tablesWithOutYang
    manager.root.free()
    manager.root = None
    try:
        manager.loadData({"SYSLOG_SERVER": {"192.0.2.1": {"port": "70000"}}}, quiet=True)
        manager.validate_data_tree()
    except sonic_yang_ext.SonicYangException:
        pass
    else:
        raise AssertionError("installed YANG accepted an invalid port")
    assert "--table" in command("sonic-cfg-help", "--help")
    assert "6.15.0" in command("ip", "-Version")
    assert json.loads(command("ip", "-j", "link", "show", "dev", "lo"))[0]["ifname"] == "lo"
    assert "1.8.0.2" in command("socat", "-V")
    assert "3.5.7" in command("openssl", "version")
    assert "active" in command("openssl", "list", "-providers", "-provider", "legacy")
    libdir = Path("/usr/lib") / triplet
    ctypes.CDLL(str(libdir / "libsymcrypt.so.103.13.0"))  # Runs the sealed library's integrity/self-tests.
    providers = command("openssl", "list", "-providers", "-provider", "symcryptprovider")
    assert "symcryptprovider" in providers and "active" in providers
    digest = hashlib.sha256(b"abc").hexdigest()
    assert digest in command("openssl", "dgst", "-provider", "symcryptprovider", "-propquery",
                             "provider=symcryptprovider", "-sha256", input="abc")
    engine = str(libdir / "engines-3/symcryptengine.so")
    output = command("openssl", "engine", "dynamic", "-pre", "SO_PATH:" + engine,
                     "-pre", "ID:symcrypt", "-pre", "LIST_ADD:1", "-pre", "LOAD", "-t", "-c")
    assert "[ available ]" in output and "SCOSSL" in output
    assert digest in command("openssl", "dgst", "-engine", engine, "-sha256", input="abc")
    assert os.stat("/usr/bin/vim").st_ino == os.stat("/usr/bin/vim.tiny").st_ino
    for name in ("editor", "ex", "rview", "vi", "view"):
        assert os.readlink("/usr/bin/" + name) == "/etc/alternatives/" + name
        assert os.readlink("/etc/alternatives/" + name) == "/usr/bin/vim.tiny"
        assert Path("/usr/bin/" + name).resolve() == Path("/usr/bin/vim.tiny")
    assert "VIM - Vi IMproved" in command("vi", "--version")
    assert "--nodaemon" in command("supervisord", "--help")
    assert "--container-name" in command("/usr/bin/supervisor-proc-exit-listener-rs", "--help")
    print(json.dumps({"status": "passed", "machine": machine, "source_elf_count": len(report["native"]),
                      "checks": ["installed source hashes and library loading", "cfggen render", "YANG valid/invalid data",
                                 "Python dependencies and command scripts", "native ip/socat", "legacy provider",
                                 "sealed SymCrypt constructor", "provider and engine SHA256", "Vim hard link", "inherited and service accounts"]}, indent=2))


if __name__ == "__main__":
    main()
