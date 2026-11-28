"""Exercise the configured-host Bazel action with fresh output bases and a private disk cache."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/bazel/ci"))
import vs_configured_host as host
import vs_host_pypi as pypi


class ConfiguredHostTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = Path(self.tmp.name) / "checkout"
        self.cache = Path(self.tmp.name) / "cache"
        self.builder = "sha256:" + "a" * 64
        self.debs = "target/debs/trixie"
        self.package = self.debs + "/sonic-device-data_1.0_all.deb"
        self.wheel = "target/python-wheels/trixie/example-1.0-py3-none-any.whl"
        self.cfg = {"debs_path": self.debs, "TARGET_MACHINE": "vs"}
        for name in host.SOURCES:
            if name in (host.DRIVER, host.COMMON, host.DRIVER.with_suffix(".bzl")):
                dest = self.repo / name
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / name, dest)
            elif (ROOT / name).is_dir():
                self.put(Path(name) / "example", str(name))
            else:
                self.put(name, str(name))
        self.put(host.PRODUCER, """#!/bin/bash
set -eu
test -z "${PASSWORD:-}${SONIC_IMAGE_VERSION:-}"
root=$(mktemp -d)
trap 'rm -rf "$root"' EXIT
mkdir -p "$root/etc/sonic"
echo vs-configured-host-v1 > "$root/etc/sonic/.vs-configured-host-v1"
echo vs-host-base-v1 > "$root/etc/sonic/.vs-host-base-v1"
cp target/debs/trixie/sonic-device-data_1.0_all.deb "$root/package"
mksquashfs "$root" "$2" -noappend -processors 1 >/dev/null
""")
        (self.repo / host.PRODUCER).chmod(0o755)
        self.put(host.BASE, "sealed host base")
        self.put(host.BASE_RECEIPT, json.dumps({"builder": self.builder, "outputs": {
            "image": host.fingerprint(self.repo / host.BASE)}}))
        self.put(self.package, "version-one")
        self.put(self.wheel, "local wheel")
        self.release("release-a")

    def put(self, name, data):
        dest = self.repo / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(data)

    def release(self, version):
        self.put("sonic_debian_extension.sh", "install $debs_path/openssh-client_*.deb" +
                 host.BOUNDARY + "docker tag image:latest image:" + version + "\n")

    def key(self):
        return host.identity(self.repo, self.cfg, self.builder, [self.package, self.wheel])

    def test_offline_wheelhouse_includes_pyangbind_transient_dependency(self):
        constraints = ROOT / pypi.CONSTRAINTS
        archives = list(pypi.archives(pypi.LOCK, constraints))
        self.assertIn("enum34-1.1.10-py3-none-any.whl", [filename for filename, _, _ in archives])
        without_enum = self.repo / "incomplete.lock"
        without_enum.write_text("\n".join(line for line in pypi.LOCK.read_text().splitlines()
                                          if not line.startswith("enum34==")))
        with self.assertRaisesRegex(ValueError, "missing transient.*enum34"):
            list(pypi.archives(without_enum, constraints))

    def test_stock_fips_and_local_containers_preserve_release_independence(self):
        env = dict(self.cfg, RFS_SPLIT_FIRST_STAGE="n", RFS_SPLIT_LAST_STAGE="y", DOCKER_LOAD_ON_BOOT="y",
                   INCLUDE_FIPS="y", enable_organization_extensions="y", SONIC_IMAGE_VERSION="A", PASSWORD="secret",
                   sonic_local_packages="docker-macsec.gz|target/docker-macsec.gz||y "
                                        "docker-dhcp-relay.gz|target/docker-dhcp-relay.gz||y")
        with patch.object(host, "stage_options"):  # The first stage has separate eligibility tests.
            cfg = host.options(self.repo, env)
            self.assertEqual(cfg["INCLUDE_FIPS"], "y")
            self.assertNotIn("PASSWORD", cfg)
            self.assertEqual(cfg, host.options(self.repo, dict(env, SONIC_IMAGE_VERSION="B")))
            with self.assertRaisesRegex(ValueError, "custom local image"):
                host.options(self.repo, dict(env, sonic_local_packages="custom.gz|target/custom.gz||y"))
            for template in ("constants.yml.overlay.j2", "init_cfg_organization.j2"):
                with self.subTest(template=template):
                    path = Path("files/build_templates") / template
                    self.put(path, "site data")
                    with self.assertRaisesRegex(ValueError, "site-specific"):
                        host.options(self.repo, env)
                    (self.repo / path).unlink()

    def test_input_closure_and_release_boundary(self):
        before = self.key()
        self.release("release-b")
        self.assertEqual(self.key(), before)
        self.put(self.debs + "/openssh-client_10_amd64.deb", "sibling globbed by the shell")
        self.assertNotEqual(self.key(), before)
        (self.repo / self.debs / "openssh-client_10_amd64.deb").unlink()
        self.put(self.wheel, "mutated wheel")
        self.assertNotEqual(self.key(), before)
        self.put(self.wheel, "local wheel")
        self.put(host.BASE, "changed base without receipt")
        with self.assertRaisesRegex(ValueError, "host-base receipt"):
            self.key()

    def test_bazel_device_tar_replaces_the_deb_in_the_host_identity(self):
        env = dict(self.cfg, RFS_SPLIT_FIRST_STAGE="n", RFS_SPLIT_LAST_STAGE="y", DOCKER_LOAD_ON_BOOT="y",
                   installer_device_data_tar=host.DEVICE_TAR)
        with patch.object(host, "stage_options"):
            cfg = host.options(self.repo, env)
        self.assertEqual(cfg["installer_device_data_tar"], host.DEVICE_TAR)
        self.put("sonic_debian_extension.sh",
                 'scripts/install_device_data.py --tar "$installer_device_data_tar"' + host.BOUNDARY)
        self.put(host.DEVICE_TAR, "selected device TAR")
        for name in host.DEVICE_REGISTRATION:
            self.put(name, name)

        def identity():
            return host.identity(self.repo, cfg, self.builder, [self.wheel])

        before = identity()
        self.assertIn(host.DEVICE_TAR, before["packages"])
        self.assertNotIn(self.package, before["packages"])
        (self.repo / self.package).unlink()
        self.assertEqual(before, identity())
        for name in (host.DEVICE_TAR, *host.DEVICE_REGISTRATION):
            with self.subTest(name=name):
                path = self.repo / name
                original = path.read_bytes()
                path.write_bytes(original + b"changed")
                self.assertNotEqual(before, identity())
                path.write_bytes(original)
        (self.repo / host.DEVICE_TAR).unlink()
        with self.assertRaises(FileNotFoundError):
            identity()

    def test_cache_is_keyed_on_package_bytes(self):
        try:
            binary = host.bazel_binary()
            if not shutil.which("mksquashfs") or not shutil.which("unsquashfs"):
                raise FileNotFoundError("squashfs-tools required")
        except (ValueError, FileNotFoundError) as error:
            if os.getenv("SONIC_REQUIRE_CONFIGURED_HOST_TEST") == "1":
                raise
            self.skipTest(str(error))

        def build():
            return host.cached_build(self.repo, self.cache, binary, self.key(), [self.package, self.wheel])

        with patch.dict(os.environ, {"PASSWORD": "private", "SONIC_IMAGE_VERSION": "release-a"}):
            cold = build()
        self.release("release-b")
        (self.repo / host.IMAGE).unlink()
        warm = build()
        self.assertFalse(cold["cache_hit"])
        self.assertTrue(warm["cache_hit"])
        self.assertEqual(cold["outputs"], warm["outputs"])
        self.assertEqual(cold["identity_sha256"], warm["identity_sha256"])
        self.put(self.package, "version-two")
        changed = build()
        self.assertFalse(changed["cache_hit"])
        self.assertNotEqual(changed["identity_sha256"], warm["identity_sha256"])
        self.assertNotEqual(changed["outputs"], warm["outputs"])
        restored = subprocess.check_output(["unsquashfs", "-processors", "1", "-cat", self.repo / host.IMAGE, "package"])
        self.assertEqual(restored, b"version-two")


if __name__ == "__main__":
    unittest.main()
