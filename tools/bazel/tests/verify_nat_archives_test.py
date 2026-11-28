"""Exercise NAT archive selection and failure on a missing selected export."""

from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import verify_nat_archives


class VerifyNatArchivesTest(unittest.TestCase):
    def test_build_modes_select_required_archives(self):
        for layer, names in (("n", ["docker-nat"]), ("y", ["docker-nat", "docker-nat-dbg"])):
            with self.subTest(layer=layer), mock.patch.object(verify_nat_archives, "verify") as verify:
                verify_nat_archives.main(["--debug-layer", layer])
                self.assertEqual(verify.call_args_list, [mock.call(Path.cwd(), name) for name in names])

    def test_missing_required_export_fails_even_when_bazel_receipt_exists(self):
        with tempfile.TemporaryDirectory() as tmp:
            workspace = Path(tmp)
            target = workspace / "target"
            (workspace / ".platform").write_text("vs")
            (workspace / ".arch").write_text("amd64")
            (target / ".container-build-method").mkdir(parents=True)
            for name in ("docker-nat", "docker-nat-dbg"):
                with self.subTest(name=name):
                    filename = name + ".gz"
                    (target / ".container-build-method" / filename).write_text("bazel")
                    manifest_dir = target / "bazel-manifests" / name
                    manifest_dir.mkdir(parents=True)
                    (manifest_dir / "manifest.json").write_text("{}")
                    with self.assertRaises(FileNotFoundError) as failure:
                        verify_nat_archives.verify(workspace, name)
                    self.assertEqual(Path(failure.exception.filename), target / filename)
