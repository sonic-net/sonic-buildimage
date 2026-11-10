import base64
import subprocess
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]


@pytest.fixture
def cache_flags(tmp_path):
    source = (ROOT / "Makefile.cache").read_text()
    targets = source.split("# Set the target path for each target.", 1)[1]
    targets = targets.split("# define the DEP files", 1)[0]
    rules = source.split("define FLAGS_DEP_RULES", 1)[1].split("endef", 1)[0]
    key_rule = source.split("define GET_MOD_SHA", 1)[1].split("endef", 1)[0]
    makefile = tmp_path / "Makefile"
    makefile.write_text(
        "SONIC_COMMON_FLAGS_LIST := common\n"
        "SONIC_RFS_TARGETS := rootfs\n"
        "rootfs_INSTALLER := installer\n"
        "SONIC_MAKE_DEBS := package.deb\n"
        "TARGET_PATH := target\n"
        "DEBS_PATH := target\n"
        + targets
        + "package.deb_DEP_FLAGS := $(SONIC_COMMON_FLAGS_LIST)\n"
        + "define FLAGS_DEP_RULES" + rules + "endef\n"
        + "$(eval $(call FLAGS_DEP_RULES,rootfs package.deb,target,flags))\n"
        + "define GET_MOD_SHA" + key_rule + "endef\n"
        + "rootfs_DEP_FLAGS_FILE := target/rootfs.flags\n"
        + "package.deb_DEP_FLAGS_FILE := target/package.deb.flags\n"
        + "keys: target/rootfs.flags target/package.deb.flags\n"
        + "\t$(call GET_MOD_SHA,rootfs)\n"
        + "\t$(call GET_MOD_SHA,package.deb)\n"
        + "\t$(file >target/rootfs.key,$(rootfs_MOD_HASH))\n"
        + "\t$(file >target/package.deb.key,$(package.deb_MOD_HASH))\n"
    )
    (tmp_path / "target").mkdir()

    def run(certificate="", **variables):
        result = subprocess.run(
            ["make", "--no-print-directory", "-f", str(makefile),
             "keys",
             "SECURE_BOOT_DB_CERT=" + str(certificate).replace("$", "$$")]
            + [f"{name}={value}" for name, value in variables.items()],
            cwd=tmp_path, capture_output=True, text=True,
        )
        assert result.returncode == 0, result.stderr
        return tuple((tmp_path / "target" / name).read_bytes()
                     for name in ("rootfs.flags", "package.deb.flags",
                                  "rootfs.key", "package.deb.key"))

    return run


def test_certificate_contents_invalidate_only_rootfs_flags(tmp_path, cache_flags):
    certificate = tmp_path / "db $' # cert.auth"
    original = bytes(range(256)) * 16
    certificate.write_bytes(original)
    before = cache_flags(certificate)
    rootfs_before, package_before, key_before, package_key_before = before
    fields = dict(field.split(b"=", 1) for field in rootfs_before.split()[1:])
    assert base64.b64decode(fields[b"SECURE_BOOT_DB_CERT"]) == bytes(certificate)
    assert base64.b64decode(fields[b"SECURE_BOOT_DB_CERT_CONTENT"]) == original
    assert cache_flags(certificate) == before

    certificate.write_bytes(original[:-1] + b"\x00")
    rootfs_after, package_after, key_after, package_key_after = cache_flags(certificate)
    assert rootfs_after != rootfs_before
    assert key_after != key_before
    assert package_after == package_before
    assert package_key_after == package_key_before


@pytest.mark.parametrize("filename", ["", "missing.auth"])
def test_absent_certificate_has_empty_contents(tmp_path, cache_flags, filename):
    certificate = tmp_path / filename if filename else ""
    rootfs, *_ = cache_flags(certificate)
    assert b"SECURE_BOOT_DB_CERT_CONTENT=" in rootfs.split()


@pytest.mark.parametrize("variable", ["USERNAME", "installer_IMAGE_TYPE"])
def test_existing_rootfs_inputs_remain_tracked(cache_flags, variable):
    rootfs_before, package_before, key_before, package_key_before = cache_flags(
        **{variable: "before"}
    )
    rootfs_after, package_after, key_after, package_key_after = cache_flags(
        **{variable: "after"}
    )
    assert rootfs_after != rootfs_before
    assert key_after != key_before
    assert package_after == package_before
    assert package_key_after == package_key_before
