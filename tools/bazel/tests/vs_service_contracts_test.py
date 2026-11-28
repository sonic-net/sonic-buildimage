"""Verify installed package labels and permissions hidden by Docker probe mounts."""

import io
import json
from pathlib import Path
import tarfile
import tempfile
import zipfile
import unittest

from tools.bazel.ci import artifact_validation as check, gnmi_image, snmp_image, vs_container_probe, vs_probe_policy
from tools.bazel.ci.vs_probe_policy import preparation_targets


class VSServiceContractsTest(unittest.TestCase):
    def setUp(self):
        work = tempfile.TemporaryDirectory()
        self.addCleanup(work.cleanup)
        self.directory = Path(work.name)
        self.packages = {name: {"Version": version} for name, version in
                         (("libswsscommon", "1.0"), ("sonic-supervisord-utilities-rs", "2.0"))}
        self.labels = {"com.azure.sonic.versions." + name: item["Version"]
                       for name, item in self.packages.items()}
        self.files = {
            path: {"kind": kind, "uid": uid, "gid": gid, "mode": mode}
            for path, kind, uid, gid, mode in (
                ("etc/redis", "directory", 100, 101, 0o2770),
                ("etc/redis/redis.conf", "file", 100, 101, 0o640),
                ("var/lib/redis", "directory", 100, 101, 0o750),
                ("var/log/redis", "directory", 100, 4, 0o2750),
                ("var/lib/snmp", "directory", 101, 102, 0o755),
            )
        }

    def test_labels_use_the_original_package_report_for_each_selected_variant(self):
        for image, filenames in {
            "docker-database": ("runtime-packages.json", "debug-packages.json"),
            "docker-lldp": ("runtime-packages.json",) * 2,
            "docker-mux": ("runtime-packages.json",) * 2,
            "docker-router-advertiser": ("config-engine-packages.json",) * 2,
            "docker-snmp": ("config-engine.packages.json",) * 2,
        }.items():
            for variant, filename in zip(("runtime", "debug"), filenames):
                with self.subTest(image=image, variant=variant):
                    self.assertIn(filename, preparation_targets(image, variant == "debug"))
                    fixture = self.directory / filename
                    fixture.write_text(json.dumps({"dependency_check": {"packages": self.packages}}))
                    vs_container_probe.verify_service_installation(image, variant, self.directory,
                                                                   {"Labels": self.labels}, self.files)
                    for label in self.labels:
                        with self.assertRaisesRegex(ValueError, "component label differs"):
                            vs_container_probe.verify_service_installation(image, variant, self.directory,
                                {"Labels": {**self.labels, label: "stale"}}, self.files)
                    fixture.unlink()

    def test_service_storage_checks_reject_wrong_owner_group_and_mode(self):
        for path, original in self.files.items():
            image, document = (("docker-snmp", "config-engine.packages.json") if path == "var/lib/snmp"
                               else ("docker-database", "runtime-packages.json"))
            (self.directory / document).write_text(json.dumps({"dependency_check": {"packages": self.packages}}))
            for field in ("uid", "gid", "mode"):
                with self.subTest(path=path, field=field), self.assertRaisesRegex(ValueError, "ownership|permissions"):
                    files = {**self.files, path: {**original, field: 0}}
                    vs_container_probe.verify_service_installation(image, "runtime", self.directory,
                                                                   {"Labels": self.labels}, files)


    def test_gnmi_requires_the_make_selected_launcher_and_client(self):
        launcher = {"kind": "file", "uid": 0, "gid": 0, "mode": 0o755,
                    "sha256": check.sha(gnmi_image.ROOT / "dockers/docker-sonic-gnmi/dialout.sh")}
        enabled = {"usr/bin/dialout.sh": launcher, "usr/sbin/dialout_client_cli": {}}
        gnmi_image.verify_dialout(enabled, True)
        gnmi_image.verify_dialout({}, False)
        for files, requested in ((enabled, False), ({}, True), ({"usr/bin/dialout.sh": launcher}, True)):
            with self.subTest(requested=requested), self.assertRaisesRegex(ValueError, "differs from Make"):
                gnmi_image.verify_dialout(files, requested)
        with self.assertRaisesRegex(ValueError, "launcher differs"):
            gnmi_image.verify_dialout({**enabled, "usr/bin/dialout.sh": {**launcher, "mode": 0o644}}, True)

    def test_runtime_excludes_selected_debug_owners_with_or_without_a_debug_image(self):
        symbol = "usr/lib/debug/.build-id/12/34.debug"
        owners = {"all-debug.tar": {symbol: {}}}
        for image, variant, native, products, leaked in (
            ("docker-database", "runtime", {"usr/bin/test": {"build_id": "1234"}}, {}, symbol),
            ("docker-lldp", "debug", {}, owners, symbol),
            ("docker-orchagent", "runtime", {}, {}, "usr/bin/ssh"),
            ("docker-orchagent", "debug", {}, {"openssh.source.tar": ["usr/bin/sftp"]}, "usr/bin/sftp"),
            ("docker-sonic-p4rt", "runtime", {}, {}, "usr/lib/x86_64-linux-gnu/libsaimeta.so.0.0.0"),
        ):
            with self.subTest(image=image, variant=variant):
                vs_container_probe.verify_debug_separation(image, variant, native, products, set())
                with self.assertRaisesRegex(ValueError, "runtime contains debug-only"):
                    vs_container_probe.verify_debug_separation(image, variant, native, products, {leaked})

    def test_full_owner_tars_and_snmp_wheels_verify_non_native_files(self):
        module = "usr/share/sonic/example.py"
        for image, names in vs_probe_policy.FULL_INSTALL_TARS.items():
            if image == "docker-sonic-p4rt":  # Its SAI owner payloads belong exclusively to debug.
                continue
            with self.subTest(image=image):
                inputs = {}
                for name in names:
                    path = self.directory / name
                    self.assertIn(name, preparation_targets(image, False))
                    with tarfile.open(path, "w") as archive:
                        member = tarfile.TarInfo(module)
                        member.size = 3
                        archive.addfile(member, io.BytesIO(b"123"))
                    inputs[name] = path
                vs_probe_policy.prepare(image, "test", "runtime", None, inputs, self.directory)
                evidence = json.loads((self.directory / "runtime.owners.json").read_text())
                self.assertEqual(set(evidence["owners"]), names)
                expected = next(iter(evidence["owners"].values()))[module]
                bundle = {"image": image, "fixtures": {}}
                vs_container_probe.native_expectations(self.directory, bundle, "runtime", {module: expected}, evidence)
                with self.assertRaisesRegex(ValueError, "owner payload differs"):
                    vs_container_probe.native_expectations(self.directory, bundle, "runtime",
                                                            {module: {**expected, "sha256": "changed"}}, evidence)
        for name in snmp_image.TARGETS:
            if name.endswith(".whl"):
                self.assertIn(name, preparation_targets("docker-snmp", False))
        wheel = self.directory / "sample.whl"
        with zipfile.ZipFile(wheel, "w") as source:
            source.writestr("asyncsnmp/unused.py", b"# module")
        with self.assertRaisesRegex(ValueError, "differs from its source wheel"):
            snmp_image.wheel_payload({}, wheel)


if __name__ == "__main__":
    unittest.main()
