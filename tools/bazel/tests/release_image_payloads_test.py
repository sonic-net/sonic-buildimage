import json
from pathlib import Path
import tempfile
import unittest


from tools.bazel.ci.release_image_payloads import SWSS_UPLOAD, release


ROOT = Path(__file__).resolve().parents[3]


class ReleaseImagePayloadsTest(unittest.TestCase):
    def test_release_preserves_receipts_inputs_other_families_and_unlisted_exports(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            artifacts = root / "artifacts"
            stage = artifacts / "eventd"
            public = stage / "public"
            public.mkdir(parents=True)
            source = root / "bazel-output.tar"
            source.write_bytes(b"immutable source output")
            (stage / "source.tar").write_bytes(source.read_bytes())
            (public / "source.tar").hardlink_to(stage / "source.tar")
            kept = [stage / "runtime.json", stage / "unlisted.tar", public / "report.json",
                    artifacts / "other-job.gz", source]
            for path in kept[:-1]:
                path.write_text("keep")
            receipt = stage / "build-receipt.json"
            receipt.write_text(json.dumps({"artifacts": {"source.tar": {}, "runtime.json": {}}}))
            release(artifacts, "eventd")
            release(artifacts, "eventd")
            self.assertFalse((stage / "source.tar").exists())
            self.assertFalse((public / "source.tar").exists())
            self.assertTrue(all(path.is_file() for path in [*kept, receipt]))
            self.assertEqual(source.read_bytes(), b"immutable source output")

    def test_unsafe_paths_fail_before_removing_any_payload(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            artifacts = root / "artifacts"
            stage = artifacts / "sysmgr"
            stage.mkdir(parents=True)
            retained = stage / "good.tar"
            retained.write_text("keep on rejection")
            outside = root / "outside.tar"
            outside.write_text("outside")
            (stage / "symlink.gz").symlink_to(outside)
            for name in ("../outside.tar", "symlink.gz"):
                with self.subTest(name=name):
                    (stage / "build-receipt.json").write_text(json.dumps({"artifacts": {"good.tar": {}, name: {}}}))
                    with self.assertRaises(ValueError):
                        release(artifacts, "sysmgr")
                    self.assertTrue(retained.exists())
                    self.assertEqual(outside.read_text(), "outside")
            (artifacts / "redirect").symlink_to(stage, target_is_directory=True)
            with self.assertRaises(ValueError):
                release(artifacts, "redirect")

    def test_swss_private_comparison_tars_are_retained(self):
        with tempfile.TemporaryDirectory() as tmp:
            artifacts = Path(tmp) / "artifacts"
            stage = artifacts / "swss-layer"
            stage.mkdir(parents=True)
            private = {"swss-runtime.tar", "config-engine-debug.tar"}
            for name in SWSS_UPLOAD | private:
                (stage / name).write_text(name)
            (stage / "receipt.json").write_text(json.dumps({"artifacts": dict.fromkeys(SWSS_UPLOAD | private)}))
            release(artifacts, "swss-layer")
            self.assertEqual({path.name for path in stage.iterdir()}, private | {"receipt.json"})
