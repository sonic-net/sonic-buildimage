"""Keep source-TAR checks consistent with installed OCI link semantics."""

import copy
import io
from pathlib import Path
import struct
import sys
import tarfile
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.swss_layer import verify_source_runtime, verify_libteam_metadata
from tools.bazel.oci import image_validation


class SwssSourcePayloadTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.archive = Path(self.temporary.name) / "source.tar"
        self.binary = "usr/lib/x86_64-linux-gnu/libprotobuf.so.32.0.12"
        self.link = "usr/lib/x86_64-linux-gnu/libprotobuf.so.32"
        content = b"\x7fELF\x02\x01" + bytes(10) + struct.pack("<HH", 3, 62) + bytes(44)
        with tarfile.open(self.archive, "w") as tar:
            binary = tarfile.TarInfo(self.binary)
            binary.mode, binary.size = 0o644, len(content)
            tar.addfile(binary, io.BytesIO(content))
            link = tarfile.TarInfo(self.link)
            link.type, link.mode = tarfile.SYMTYPE, 0
            link.linkname = "./libprotobuf.so.32.0.12"
            tar.addfile(link)
        self.files = {}
        image_validation.apply_layer(self.archive, self.files)

    def test_owner_link_is_compared_with_the_same_installed_semantics(self):
        runtime, native = verify_source_runtime(self.files, self.archive)
        self.assertEqual(runtime[self.link]["mode"], 0)
        self.assertEqual(runtime[self.link]["linkname"], "libprotobuf.so.32.0.12")
        self.assertEqual(set(native), {self.binary})
        self.assertEqual(native[self.binary]["mode"], 0o644)

    def test_link_target_and_ownership_changes_are_rejected(self):
        for key, value in (("linkname", "libprotobuf.so.999"), ("uid", 1000), ("gid", 1000)):
            with self.subTest(key=key):
                files = copy.deepcopy(self.files)
                files[self.link][key] = value
                with self.assertRaisesRegex(ValueError, "OCI changed source-owned payload"):
                    verify_source_runtime(files, self.archive)

    def test_file_bytes_permissions_and_missing_payload_are_rejected(self):
        for key, value in (("sha256", "0" * 64), ("mode", 0o755), ("uid", 1000)):
            with self.subTest(key=key):
                files = copy.deepcopy(self.files)
                files[self.binary][key] = value
                with self.assertRaisesRegex(ValueError, "OCI changed source-owned payload"):
                    verify_source_runtime(files, self.archive)
        files = copy.deepcopy(self.files)
        del files[self.link]
        with self.assertRaisesRegex(ValueError, "OCI changed source-owned payload"):
            verify_source_runtime(files, self.archive)


class LibteamMetadataTest(unittest.TestCase):
    def setUp(self):
        self.native = {}
        self.metadata = {"schema": 1, "architecture": "amd64",
                         "source_modules": {"libteam": {"version": "1.31.0.sonic.1", "module_sha256": "a" * 64}},
                         "source_packages": []}
        for package, target, filename in (
                ("libteam5", "libteam_runtime", "libteam.so.5.6.1"),
                ("libteamdctl0", "libteamdctl_runtime", "libteamdctl.so.0.1.5")):
            path = "usr/lib/x86_64-linux-gnu/" + filename
            item = {"sha256": "b" * 64, "mode": 0o755, "uid": 0, "gid": 0}
            self.native[path] = item
            self.metadata["source_packages"].append({
                "package": package, "target": "@libteam//:" + target,
                "source_revision": "1.31.0.sonic.1", "payload_sha256": "c" * 64,
                "native_files": [{"path": path, **item}]})

    def test_two_library_records_match_selected_owner_and_installed_native_bytes(self):
        report = verify_libteam_metadata(self.metadata, self.native)
        self.assertEqual(report["owner"], self.metadata["source_modules"]["libteam"])
        self.assertEqual(len(report["packages"]), 2)

    def test_captured_canonical_library_records_match_selected_owner(self):
        # PR195 job127364237079 packages.json (SHA256 c0c2043e104b471c0df1bf3f427f8221de04b801cafffea6ed8b9bed6147071c).
        # These are metadata fields only; this pure test does not read an ELF.
        metadata = {
            "schema": 1, "architecture": "amd64",
            "source_modules": {"libteam": {
                "version": "1.31.0.sonic.1",
                "module_sha256": "33b616806dea29faf2b69a338e4df53cd96ac0f0ced328c8de4b95e6a2c016e3",
            }},
            "source_packages": [],
        }
        native = {}
        for package, target, filename, payload_sha, native_sha in (
                ("libteam5", "libteam_runtime", "libteam.so.5.6.1",
                 "029d4fa4cacc7fdddc36c73fec089f06f6155238bdcc484feea16ab8ae8e7884",
                 "df6213bccca57b4c9ec45acbab2e6c7a648f29e84bf7d8cc1ff5675c99f3e237"),
                ("libteamdctl0", "libteamdctl_runtime", "libteamdctl.so.0.1.5",
                 "1fcc1b0d885522897443e4d9f1b101640e34348d8367c98e7963cecfff9aa152",
                 "e2cc923fc6b03c35af3ab328dd602ea11fda65a46b43fcc04347aea8a5abce31")):
            path = "usr/lib/x86_64-linux-gnu/" + filename
            native[path] = {"sha256": native_sha, "mode": 0o644, "uid": 0, "gid": 0}
            metadata["source_packages"].append({
                "package": package, "version": "1.31-1", "replaced_version": None,
                "target": "@@libteam+//:" + target, "source_revision": "1.31.0.sonic.1",
                "payload_sha256": payload_sha, "native_files": [{"path": path, **native[path]}],
            })
        self.assertEqual(verify_libteam_metadata(metadata, native)["packages"],
                         metadata["source_packages"])

    def test_canonical_target_does_not_relax_package_identity(self):
        metadata = copy.deepcopy(self.metadata)
        for item in metadata["source_packages"]:
            item["target"] = item["target"].replace("@libteam//", "@@libteam+//")
        for field, value in (
                ("target", "@@other+//:libteam_runtime"),
                ("target", "@@libteam++other//:libteam_runtime"),
                ("target", "@@libteam+1.31.0.sonic.1//:libteam_runtime"),
                ("target", "@libteam+//:libteam_runtime"),
                ("target", "@@libteam+//:libteam_runtime_amd64"),
                ("target", "@@libteam+//:libteamdctl_runtime"),
                ("target", "@@libteam+//:libteam_runtime-extra"),
                ("source_revision", "1.31.0.sonic.2"),
                ("payload_sha256", "not-a-sha256")):
            with self.subTest(field=field, value=value):
                changed = copy.deepcopy(metadata)
                changed["source_packages"][0][field] = value
                with self.assertRaisesRegex(ValueError, "libteam package provenance differs"):
                    verify_libteam_metadata(changed, self.native)
        metadata["source_modules"]["libteam"]["module_sha256"] = "not-a-sha256"
        with self.assertRaisesRegex(ValueError, "selected owner identity"):
            verify_libteam_metadata(metadata, self.native)

    def test_old_owner_missing_or_extra_utils_records_are_rejected(self):
        for change in ("owner", "missing", "utils", "duplicate"):
            with self.subTest(change=change):
                metadata = copy.deepcopy(self.metadata)
                if change == "owner":
                    metadata["source_modules"] = {"sonic-buildimage": {"version": "0.0.1"}}
                elif change == "missing":
                    metadata["source_packages"].pop()
                elif change == "utils":
                    metadata["source_packages"].append({"package": "libteam-utils"})
                else:
                    metadata["source_packages"][1] = metadata["source_packages"][0]
                with self.assertRaises(ValueError):
                    verify_libteam_metadata(metadata, self.native)

    def test_wrong_target_revision_and_native_bytes_are_rejected(self):
        for field, value in (("target", "@sonic_swss//dist:libteam_runtime"),
                             ("source_revision", "0.0.1"), ("payload_sha256", "")):
            with self.subTest(field=field):
                metadata = copy.deepcopy(self.metadata)
                metadata["source_packages"][0][field] = value
                with self.assertRaises(ValueError):
                    verify_libteam_metadata(metadata, self.native)
        for field, value in (("sha256", "d" * 64), ("mode", 0o644), ("uid", 1000), ("gid", 1000)):
            with self.subTest(field=field):
                metadata = copy.deepcopy(self.metadata)
                metadata["source_packages"][0]["native_files"][0][field] = value
                with self.assertRaises(ValueError):
                    verify_libteam_metadata(metadata, self.native)


if __name__ == "__main__":
    unittest.main()
