import json
import os
import pwd
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]
BUILD_RECIPES = re.findall(
    r"(?m)^\t+RFS_SQUASHFS_NAME=[^\n]*\\\n(?:[^\n]*\\\n)*\t+\./build_debian\.sh \$\(LOG\)",
    (ROOT / "slave.mk").read_text(),
)


def test_all_rootfs_build_recipes_are_covered():
    assert len(BUILD_RECIPES) == 2
    assert len(re.findall(r"(?m)^\t+\./build_debian\.sh ", (ROOT / "slave.mk").read_text())) == 2


@pytest.mark.parametrize("recipe", BUILD_RECIPES, ids=["rootfs", "installer"])
@pytest.mark.parametrize("cert_path", ["", "DB.auth", "/signing keys/DB.auth"])
def test_build_recipe_forwards_db_certificate(tmp_path, monkeypatch, recipe, cert_path):
    monkeypatch.delenv("SECURE_BOOT_DB_CERT", raising=False)
    build = tmp_path / "build_debian.sh"
    build.write_text('#!/bin/sh\nprintf "%s" "$SECURE_BOOT_DB_CERT"\n')
    build.chmod(0o755)
    makefile = tmp_path / "Makefile"
    makefile.write_text(f"SECURE_BOOT_DB_CERT := {cert_path}\n.PHONY: all\nall:\n{recipe}\n")
    result = subprocess.run(
        ["make", "--silent", "--no-print-directory", "all"],
        cwd=tmp_path, capture_output=True, text=True, check=True,
    )
    assert result.stdout == cert_path


@pytest.mark.parametrize("recipe", BUILD_RECIPES, ids=["rootfs", "installer"])
@pytest.mark.parametrize("use_real_su", [False, True], ids=["mock-su", "real-su"])
@pytest.mark.parametrize("cert_path", [
    "",
    "DB.auth",
    "signing keys/DB.auth",
    "/signing keys/DB.auth",
    "signing keys/../keys/DB.auth",
    "signing keys/DB's.auth",
])
def test_docker_and_nested_make_preserve_certificate_path(tmp_path, monkeypatch, recipe, cert_path, use_real_su):
    if use_real_su and (os.geteuid() != 0 or shutil.which("su") is None):
        pytest.skip("real su coverage requires root and su")
    monkeypatch.delenv("SECURE_BOOT_DB_CERT", raising=False)
    monkeypatch.setenv("PATH", str(tmp_path) + os.pathsep + os.environ["PATH"])
    source = (ROOT / "Makefile.work").read_text()
    mount = re.search(
        r"# Mount the Secure Boot db certificate[^\n]*\n(.*?)# Mount the Signing prod tool",
        source, re.DOTALL,
    ).group(1)
    assignment = re.findall(r"(?m)^\s+(SECURE_BOOT_DB_CERT=.*?) \\\n", source)
    launch = next(line for line in source.splitlines() if 'bash -c "' in line and "$(SONIC_BUILD_INSTRUCTION)" in line)
    docker = tmp_path / "docker.py"
    docker.write_text(
        "import json, os, subprocess, sys\n"
        "from pathlib import Path\n"
        "record, *args = sys.argv[1:]\n"
        "env = dict(os.environ)\n"
        "volumes = []\n"
        "while args and args[0] in ('-v', '-e'):\n"
        "    flag, value, *args = args\n"
        "    if flag == '-v':\n"
        "        volumes.append(value)\n"
        "    else:\n"
        "        key, value = value.split('=', 1)\n"
        "        env[key] = value\n"
        "Path(record).write_text(json.dumps(volumes))\n"
        "sys.exit(subprocess.run(args, env=env).returncode)\n"
    )
    if use_real_su:
        user = pwd.getpwuid(os.geteuid()).pw_name
    else:
        user = "builder"
        su = tmp_path / "su"
        su.write_text('#!/bin/sh\nexec /bin/bash "$2" "$3"\n')
        su.chmod(0o755)
    build = tmp_path / "build_debian.sh"
    build.write_text('#!/bin/sh\nprintf "%s" "$SECURE_BOOT_DB_CERT"\n')
    build.chmod(0o755)
    (tmp_path / "slave.mk").write_text(
        "SECURE_BOOT_DB_CERT ?=\n.PHONY: all\nall:\n" + recipe + "\n",
    )
    record = tmp_path / "volumes.json"
    (tmp_path / "Makefile").write_text(
        "SHELL := /bin/bash\n"
        f"SECURE_BOOT_DB_CERT := {cert_path}\n"
        f"DOCKER_RUN := {shlex.join([sys.executable, str(docker), str(record)])}\n"
        + mount
        + "\nSONIC_BUILD_INSTRUCTION := $(MAKE) --silent --no-print-directory -f slave.mk "
        + " ".join(assignment)
        + f"\nDOCKER_USER_COMMANDS := :\nUSER := {user}\nCOLLECT_BUILD_VERSION := :\nSLAVE_SHELL := :\n"
        + ".PHONY: all\nall:\n\t$(DOCKER_RUN) \\\n"
        + launch + "\n",
    )
    result = subprocess.run(
        ["make", "--silent", "--no-print-directory", "all"],
        cwd=tmp_path, capture_output=True, text=True, check=True,
    )
    expected_path = os.path.normpath(str(tmp_path / cert_path)) if cert_path else ""
    assert json.loads(record.read_text()) == ([f"{expected_path}:{expected_path}:ro"] if cert_path else [])
    assert result.stdout == expected_path
