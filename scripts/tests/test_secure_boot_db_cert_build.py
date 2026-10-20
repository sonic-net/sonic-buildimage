import re
import subprocess
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
