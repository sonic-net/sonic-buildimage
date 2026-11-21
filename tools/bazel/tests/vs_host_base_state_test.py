#!/usr/bin/env python3
"""Exercise the host-base admission, sealing and personalization shell paths."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / "scripts/vs_host_base_state.sh"
TARGET = "sonic-vs.bin__vs__rfs.squashfs"
ABI = "6.12.41+deb13-sonic-amd64"
MARKER = Path("etc/sonic/.vs-host-base-v1")


class HostBaseStateTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        self.buildinfo = self.work / "src/sonic-build-hooks/buildinfo"
        self.write(self.buildinfo / "config/buildinfo.config", "export DISTRO=trixie\n")
        self.write(self.buildinfo / "sonic-build-hooks_1.0_all.deb", "build hooks")
        self.fs = self.work / "fsroot-vs"
        self.env = dict(
            PATH=os.defpath, USERNAME="admin", TARGET_MACHINE="vs", IMAGE_TYPE="onie",
            CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64", IMAGE_DISTRO="trixie",
            TARGET_BOOTLOADER="grub", RFS_SPLIT_FIRST_STAGE="y", RFS_SPLIT_LAST_STAGE="n",
            RFS_SQUASHFS_NAME=TARGET, MIRROR_SNAPSHOT="y",
            BUILD_SNAPSHOT_URL="http://packages.trafficmanager.net/snapshot",
            BUILD_PUBLIC_URL="https://packages.trafficmanager.net/public",
            BUILD_PACKAGES_URL="https://packages.trafficmanager.net/packages",
            TRUSTED_GPG_URLS="https://packages.microsoft.com/keys/microsoft.asc",
        )

    @staticmethod
    def write(path, contents):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents)
        return path

    def run_state(self, *args, **settings):
        return subprocess.run(["bash", SCRIPT, *args], cwd=self.work,
                              env=self.env | settings, capture_output=True, text=True)

    def test_only_stock_vs_inputs_are_eligible(self):
        result = self.run_state("check-inputs")
        self.assertEqual(result.returncode, 0, result.stderr)
        for name, value in [
            ("PASSWORD", "fixture-secret"), ("DEFAULT_PASSWORD", "fixture-secret"),
            ("BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD", "fixture-secret"),
            ("http_proxy", "http://fixture-secret@example.test"),
            ("USERNAME", "operator"), ("IMAGE_TYPE", "kvm"), ("MIRROR_SNAPSHOT", "n"),
            ("MIRROR_URLS", "https://site.example"), ("INCLUDE_KUBERNETES", "y"),
        ]:
            with self.subTest(name=name):
                result = self.run_state("check-inputs", **{name: value})
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn("fixture-secret", result.stdout + result.stderr)

    def test_site_files_exclude_the_root_from_sharing(self):
        for path in ("rules/config.user", "files/build_templates/build_debian.organization.sh",
                     "files/image_config/sonic_release", "src/sonic-build-hooks/buildinfo/private"):
            with self.subTest(path=path):
                extra = self.write(self.work / path, "fixture-secret")
                result = self.run_state("check-inputs")
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn("fixture-secret", result.stdout + result.stderr)
                extra.unlink()
        self.write(self.buildinfo / "config/buildinfo.config", "export PRIVATE=fixture-secret\n")
        result = self.run_state("check-inputs")
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("fixture-secret", result.stdout + result.stderr)

    def test_make_routes_only_the_vs_installer_base_through_the_cache(self):
        slave = (ROOT / "slave.mk").read_text()
        start = slave.index("ifneq ($(strip $(SONIC_HOSTBASE_CACHE_DIR)),)")
        gate = slave[start:slave.index("\nendif", start) + len("\nendif")]
        start = slave.index("\t\tRFS_SQUASHFS_NAME=$* ")
        launch = slave[start:slave.index(" $(LOG)", start)]
        (self.work / "Makefile").write_text(
            "SHELL=/bin/bash\n.ONESHELL:\nTARGET_PATH=target\n"
            "USERNAME=admin\nCHANGE_DEFAULT_PASSWORD=y\nBMC_NOS_ACCOUNT_USERNAME=site-bmc\n"
            + gate + "\n\ntarget/%:\n" + launch + "\n")
        # The original account settings reach either entry point. The Python
        # driver needs them only to run an ineligible build through native Make.
        stub = """#!/usr/bin/env python3
import os
from pathlib import Path
assert os.environ['PASSWORD'] == 'user-fixture'
assert os.environ['BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD'] == 'bmc-fixture'
assert os.environ['BMC_NOS_ACCOUNT_USERNAME'] == 'site-bmc'
assert os.environ['CHANGE_DEFAULT_PASSWORD'] == 'y'
(Path('target') / os.environ['RFS_SQUASHFS_NAME']).touch()
print(Path(__file__).name)
"""
        self.write(self.work / "tools/bazel/ci/vs_host_base.py", stub)
        self.write(self.work / "build_debian.sh", stub).chmod(0o755)
        (self.work / "target").mkdir()
        for target, cache, expected in [
            (TARGET, "", "build_debian.sh"),
            (TARGET, "/vs_host_base_cache", "vs_host_base.py"),
            (TARGET, "/vs_host_base_cache", "vs_host_base.py"),
            ("sonic-vs.img.gz__vs__rfs.squashfs", "/vs_host_base_cache", "build_debian.sh"),
        ]:
            with self.subTest(target=target, cache=cache):
                # Leave the target in place. Even a previous, personalized
                # SquashFS must pass through the cache driver when enabled.
                result = subprocess.run(
                    ["make", "-s", "target/" + target, "SONIC_HOSTBASE_CACHE_DIR=" + cache],
                    cwd=self.work, capture_output=True, text=True,
                    env=self.env | {"PASSWORD": "user-fixture", "BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD": "bmc-fixture"})
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.strip(), expected)

    def prepare_root(self):
        for path in ("boot/vmlinuz-" + ABI, "boot/initrd.img-" + ABI,
                     "usr/lib/modules/" + ABI + "/modules.dep"):
            self.write(self.fs / path, "fixture\n")
        self.write(self.fs / "etc/shadow", "root:*:::::::\nadmin:!:::::::\n")
        (self.fs / "etc/sonic").mkdir(parents=True)
        # The test runner has no chroot or chown privilege. Exercise the real state
        # script and replace only the calls that require those privileges.
        binary = self.work / "bin"
        commands = {
            "chroot": """
root=$1
shift
case "$1" in
    modinfo) test "$2" = -k && test "$3" = "$EXPECTED_ABI" ;;
    systemd-id128) test "$2" = new && echo 00000000000000000000000000000002 ;;
    make-ssl-cert)
        test "$2" = generate-default-snakeoil && test "$3" = --force-overwrite || exit 1
        echo new-fixture-key > "$root/etc/ssl/private/ssl-cert-snakeoil.key"
        echo new-fixture-cert > "$root/etc/ssl/certs/ssl-cert-snakeoil.pem" ;;
    *) exit 1 ;;
esac
""",
            "install": 'if [ "$1" = "-o" ]; then shift 4; fi\nexec /usr/bin/install "$@"\n',
        }
        for name, script in commands.items():
            self.write(binary / name, "#!/bin/sh\n" + script).chmod(0o755)
        self.env.update(PATH=str(binary) + ":" + os.defpath, EXPECTED_ABI=ABI)

    def test_seal_requires_boot_files_and_locked_accounts(self):
        self.fs.mkdir()
        result = self.run_state("seal", self.fs, ABI)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("incomplete kernel", result.stderr)
        self.prepare_root()
        self.write(self.fs / "etc/shadow", "root:*:::::::\nadmin:$fixture-secret:::::::\n")
        result = self.run_state("seal", self.fs, ABI)
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("fixture-secret", result.stdout + result.stderr)

    def test_seal_rejects_credential_files(self):
        self.prepare_root()
        for path in ("etc/sonic/bmc_config.json", "etc/apt/auth.conf",
                     "etc/ssh/ssh_host_ed25519_key", "root/.ssh/id_ed25519"):
            with self.subTest(path=path):
                extra = self.write(self.fs / path, "fixture-secret")
                result = self.run_state("seal", self.fs, ABI)
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn("fixture-secret", result.stdout + result.stderr)
                extra.unlink()
                (self.fs / MARKER).unlink(missing_ok=True)

    def test_restore_requires_a_sealed_root(self):
        self.prepare_root()
        for marker in ("absent", "symlink"):
            with self.subTest(marker=marker):
                if marker == "symlink":
                    (self.fs / MARKER).symlink_to(self.fs / "etc/shadow")
                result = self.run_state("restore", self.fs, ABI)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse((self.fs / "etc/resolv.conf").exists())

    def test_seal_and_restore_worker_state(self):
        self.prepare_root()
        machine_id = self.write(self.fs / "etc/machine-id", "0" * 31 + "1\n")
        dbus_id = self.write(self.fs / "var/lib/dbus/machine-id", machine_id.read_text())
        resolver = self.write(self.fs / "etc/resolv.conf", "nameserver 192.0.2.1\n")
        seed = self.write(self.fs / "var/lib/systemd/random-seed", "old-fixture-seed")
        key = self.write(self.fs / "etc/ssl/private/ssl-cert-snakeoil.key", "old-fixture-key")
        cert = self.write(self.fs / "etc/ssl/certs/ssl-cert-snakeoil.pem", "old-fixture-cert")
        self.write(self.fs / "usr/sbin/make-ssl-cert", "fixture").chmod(0o755)

        seal = self.run_state("seal", self.fs, ABI)
        self.assertEqual(seal.returncode, 0, seal.stderr)
        self.assertEqual(machine_id.read_bytes(), b"")
        for path in (dbus_id, resolver, seed, cert, key):
            self.assertFalse(path.exists(), path)
        self.assertTrue((self.fs / MARKER).is_file())

        # The real stage runs as root and can overwrite the sealed read-only file.
        machine_id.chmod(0o644)
        restore = self.run_state("restore", self.fs, ABI)
        self.assertEqual(restore.returncode, 0, restore.stderr)
        self.assertEqual(machine_id.read_text(), "0" * 31 + "2\n")
        self.assertEqual(dbus_id.read_text(), machine_id.read_text())
        self.assertEqual(resolver.read_bytes(), Path("/etc/resolv.conf").read_bytes())
        self.assertEqual(key.read_text(), "new-fixture-key\n")
        self.assertFalse(seed.exists())
        self.assertFalse((self.fs / MARKER).exists())


if __name__ == "__main__":
    unittest.main()
