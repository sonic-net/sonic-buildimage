"""Regression coverage for telemetry package cache flags."""

from pathlib import Path
import subprocess
import tarfile
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]

FIXTURE_MAKEFILE = """\
.SECONDEXPANSION:
include rules/telemetry.mk
$(SONIC_TELEMETRY)_BASE_PATH := /
include Makefile.cache

CACHE_DEP := $(DEBS_PATH)/$(SONIC_TELEMETRY).dep

.PHONY: identity
identity: $(CACHE_DEP)
\t@$(call GET_MOD_DEP_SHA,$(SONIC_TELEMETRY))
\t@$(call GET_MOD_SHA,$(SONIC_TELEMETRY))
\t@printf 'RESULT flags=%s\\n' "$$(cat $($(SONIC_TELEMETRY)_DEP_FLAGS_FILE))"
\t@printf 'RESULT cache=%s-%s-%s.tgz\\n' \
\t\t'$(SONIC_TELEMETRY)' \
\t\t'$($(SONIC_TELEMETRY)_DEP_MOD_SHA)' \
\t\t'$($(SONIC_TELEMETRY)_MOD_HASH)'

.PHONY: restore
restore: $(CACHE_DEP)
\t@$(call LOAD_FROM_CACHE,$(SONIC_TELEMETRY),$(DEBS_PATH)/$(SONIC_TELEMETRY))
\t@if test -f $(DEBS_PATH)/$(SONIC_TELEMETRY); then \
\t\tprintf 'RESULT restored=yes\\n'; \
\telse \
\t\tprintf 'RESULT restored=no\\n'; \
\tfi
"""


class TelemetryCacheFlagsTest(unittest.TestCase):
    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tempdir.cleanup)
        self.fixture = Path(self.tempdir.name)

        for directory in (
            "cache",
            "files",
            "fsroot",
            "out",
            "platform",
            "python-debs",
            "python-wheels",
            "rules",
            "src/sonic-gnmi",
            "target",
        ):
            (self.fixture / directory).mkdir(parents=True, exist_ok=True)

        (self.fixture / "rules/telemetry.dep").symlink_to(
            ROOT / "rules/telemetry.dep"
        )
        (self.fixture / "platform/rules.dep").symlink_to("/dev/null")
        (self.fixture / "Makefile").write_text(
            FIXTURE_MAKEFILE, encoding="utf-8", newline="\n"
        )

        source = self.fixture / "src/sonic-gnmi"
        subprocess.run(["git", "init", "-q"], cwd=source, check=True)
        subprocess.run(
            ["git", "config", "user.email", "fixture@example.invalid"],
            cwd=source,
            check=True,
        )
        subprocess.run(
            ["git", "config", "user.name", "fixture"],
            cwd=source,
            check=True,
        )
        (source / "source.txt").write_text(
            "fixture\n", encoding="utf-8", newline="\n"
        )
        subprocess.run(["git", "add", "source.txt"], cwd=source, check=True)
        subprocess.run(
            ["git", "commit", "-qm", "fixture"], cwd=source, check=True
        )

    def run_make(self, target, translib_write):
        command = [
            "make",
            "--no-print-directory",
            "-f",
            str(self.fixture / "Makefile"),
            target,
            f"RULES_PATH={self.fixture / 'rules'}",
            f"PLATFORM_PDDF_PATH={self.fixture / 'platform'}",
            f"PLATFORM_PATH={self.fixture / 'platform'}",
            f"SRC_PATH={self.fixture / 'src'}",
            f"DEBS_PATH={self.fixture / 'out'}",
            f"FILES_PATH={self.fixture / 'files'}",
            f"PYTHON_DEBS_PATH={self.fixture / 'python-debs'}",
            f"PYTHON_WHEELS_PATH={self.fixture / 'python-wheels'}",
            f"FSROOT_PATH={self.fixture / 'fsroot'}",
            f"TARGET_PATH={self.fixture / 'target'}",
            f"SONIC_DPKG_CACHE_DIR={self.fixture / 'cache'}",
            f"SONIC_DPKG_CACHE_SOURCE={self.fixture / 'cache'}",
            "SONIC_COMMON_FILES_LIST=",
            "SONIC_COMMON_BASE_FILES_LIST=",
            "SONIC_COMMON_FLAGS_LIST=common",
            "SONIC_DPKG_CACHE_METHOD=cache",
            "SLAVE_BASE_TAG=fixture",
            "CONFIGURED_ARCH=amd64",
            "BLDENV=bookworm",
            "ENABLE_NATIVE_WRITE=y",
        ]
        if translib_write is not None:
            command.append(f"ENABLE_TRANSLIB_WRITE={translib_write}")

        result = subprocess.run(
            command,
            cwd=ROOT,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        values = {}
        for line in result.stdout.splitlines():
            if line.startswith("RESULT "):
                key, value = line.removeprefix("RESULT ").split("=", 1)
                values[key] = value
        return values, result

    def cache_identity(self, translib_write):
        values, result = self.run_make("identity", translib_write)

        self.assertEqual(values.keys(), {"flags", "cache"}, result.stdout)
        package = "sonic-gnmi_0.1_amd64.deb"
        flags_path = self.fixture / "out" / f"{package}.flags"
        return values, flags_path.stat().st_mtime_ns

    def prime_cache(self, translib_write, payload):
        identity, _ = self.cache_identity(translib_write)
        package = self.fixture / "out/sonic-gnmi_0.1_amd64.deb"
        package.write_text(payload, encoding="utf-8", newline="\n")
        cache_file = self.fixture / "cache" / identity["cache"]
        with tarfile.open(cache_file, "w:gz") as archive:
            archive.add(package, arcname=str(package).lstrip("/"))
        package.unlink()
        return cache_file

    def restore_from_cache(self, translib_write):
        package = self.fixture / "out/sonic-gnmi_0.1_amd64.deb"
        package.unlink(missing_ok=True)
        values, result = self.run_make("restore", translib_write)
        self.assertEqual(values.keys(), {"restored"}, result.stdout)
        payload = package.read_text(encoding="utf-8") if package.exists() else None
        return values["restored"] == "yes", payload

    def test_translib_write_changes_real_package_cache_identity(self):
        enabled, enabled_mtime = self.cache_identity("y")
        enabled_again, enabled_again_mtime = self.cache_identity("y")
        self.assertEqual(enabled_again, enabled)
        self.assertEqual(enabled_again_mtime, enabled_mtime)

        empty, empty_mtime = self.cache_identity("")
        self.assertNotEqual(empty["cache"], enabled["cache"])
        self.assertIn("ENABLE_TRANSLIB_WRITE=", empty["flags"])

        unset, unset_mtime = self.cache_identity(None)
        self.assertEqual(unset, empty)
        self.assertEqual(unset_mtime, empty_mtime)

        disabled, _ = self.cache_identity("n")
        self.assertNotEqual(disabled["cache"], enabled["cache"])
        self.assertNotEqual(disabled["cache"], empty["cache"])
        self.assertIn("ENABLE_TRANSLIB_WRITE=n", disabled["flags"])

        enabled_after_disabled, _ = self.cache_identity("y")
        self.assertEqual(enabled_after_disabled["cache"], enabled["cache"])

    def test_cache_lookup_does_not_restore_opposite_flag_artifact(self):
        enabled_cache = self.prime_cache("y", "enabled")
        self.assertEqual(self.restore_from_cache("y"), (True, "enabled"))
        self.assertEqual(self.restore_from_cache(""), (False, None))

        enabled_cache.unlink()
        self.prime_cache("", "disabled")
        self.assertEqual(self.restore_from_cache(""), (True, "disabled"))
        self.assertEqual(self.restore_from_cache("y"), (False, None))


if __name__ == "__main__":
    unittest.main()
