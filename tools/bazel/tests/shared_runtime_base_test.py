"""Check the base path evidence used to decide whether APT files can move."""

import io
import json
from pathlib import Path
import tarfile
import tempfile
import unittest

from tools.bazel.oci.shared_runtime_base import non_directory_paths
from tools.bazel.tests.oci_base_fixture import digest, oci_files, write_layout


class SharedRuntimeBaseTest(unittest.TestCase):
    def witness(self, *layers):
        payloads = []
        for entries in layers:
            data = io.BytesIO()
            with tarfile.open(fileobj=data, mode="w") as archive:
                for name, kind, *target in entries:
                    member = tarfile.TarInfo(name)
                    member.type = kind
                    if target:
                        member.linkname = target[0]
                    archive.addfile(member)
            payloads.append(data.getvalue())
        config = {"os": "linux", "architecture": "amd64",
                  "rootfs": {"type": "layers", "diff_ids": [digest(layer) for layer in payloads]}}
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            write_layout(base, oci_files(json.dumps(config).encode(), payloads))
            return non_directory_paths(base)

    def test_merged_usr_records_links_and_files_across_layers(self):
        self.assertEqual(self.witness(
            [("./usr/", tarfile.DIRTYPE), ("./usr/lib", tarfile.DIRTYPE),
             ("lib", tarfile.SYMTYPE, "usr/lib"), ("var/cache", tarfile.REGTYPE)],
            [("usr/lib/a.so", tarfile.REGTYPE), ("var/cache", tarfile.DIRTYPE),
             ("usr/lib/b.so", tarfile.LNKTYPE, "usr/lib/a.so")]),
            ["lib", "usr/lib/a.so", "usr/lib/b.so", "var/cache"])

    def test_links_traversed_in_one_or_multiple_layers_disable_the_witness(self):
        link = ("lib", tarfile.SYMTYPE, "usr/lib")
        alias = ("alias", tarfile.LNKTYPE, "lib")
        for path in ("lib/foo", "alias/foo"):
            entry = (path, tarfile.REGTYPE)
            for layers in (([link, alias, entry],), ([link, alias], [entry])):
                with self.subTest(path=path, layers=len(layers)):
                    self.assertIsNone(self.witness(*layers))

    def test_whiteouts_and_unresolved_paths_disable_the_witness(self):
        for path in ("lib/.wh.a", "lib/.wh..wh..opq", "dir/../a", "/root/file"):
            with self.subTest(path=path):
                self.assertIsNone(self.witness([(path, tarfile.REGTYPE)]))


if __name__ == "__main__":
    unittest.main()
