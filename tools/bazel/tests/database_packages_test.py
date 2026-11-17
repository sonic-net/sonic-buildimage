"""Check Database's installed source identities in runtime and debug status."""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from tools.bazel.ci.database_image import verify_database_packages


class DatabasePackageTest(unittest.TestCase):
    def setUp(self):
        self.owners = [
            dict(package="libdashapi", version="1.0", source_revision="dash-revision",
                 target="@dash//:libdashapi_pkg", payload_sha256="a" * 64),
            dict(package="libprotobuf32t64", version="3.21", source_revision="protobuf-revision",
                 target="@protobuf//:libprotobuf_pkg", payload_sha256="b" * 64),
        ]
        self.status = "\n\n".join(
            "Package: {package}\nArchitecture: amd64\nVersion: {version}\nStatus: install ok installed\n"
            "X-Sonic-Bazel-Target: {target}\nX-Sonic-Bazel-Payload-Sha256: {payload_sha256}\n"
            "X-Sonic-Source-Revision: {source_revision}".format(**record) for record in self.owners
        ).encode()

    def test_both_source_records(self):
        verify_database_packages(self.status, self.owners)

    def test_absent_owner(self):
        with self.assertRaisesRegex(ValueError, "missing Database source package"):
            verify_database_packages(self.status, self.owners[:1])

    def test_archive_or_owner_mismatch(self):
        for bad in (self.status.replace(b"a" * 64, b"0" * 64),
                    self.status.replace(b"protobuf-revision", b"different-source")):
            with self.subTest(status=bad), self.assertRaisesRegex(ValueError, "installed status differs"):
                verify_database_packages(bad, self.owners)


if __name__ == "__main__":
    unittest.main()
