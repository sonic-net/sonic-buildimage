"""Exercise APT provenance checks on the data a container validation job downloads."""

import gzip
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

from tools.bazel.ci import artifact_validation, vs_package_stages


class PackageStagesTest(unittest.TestCase):
    def setUp(self):
        workspace = TemporaryDirectory()
        self.addCleanup(workspace.cleanup)
        self.directory = Path(workspace.name)
        for stage in ("parent", "runtime", "symbols"):
            with gzip.open(self.directory / (stage + ".oci.json.gz"), "wt") as stream:
                json.dump({"manifest_digest": stage, "layers": [{"digest": stage}]}, stream)

    def write(self, name, value):
        path = self.directory / name
        path.write_text(json.dumps(value))
        return path

    def test_gnmi_debug_apt_must_describe_the_built_runtime(self):
        for variant, parent, name in (("runtime", "parent", "packages.json"), ("debug", "runtime", "debug-packages.json")):
            selection = {"base_manifest_digest": parent, "dependency_check": {"status": "satisfied"}}
            apt = self.write(variant + "-apt.json", selection)
            metadata = dict(selection, apt_selection_sha256=artifact_validation.sha(apt))
            self.write(name, metadata)
            bundle = {"image": "docker-sonic-gnmi"}
            vs_package_stages.verify(self.directory, bundle, variant, {}, {})
            self.write(name, dict(metadata, apt_selection_sha256="different APT selection"))
            with self.assertRaisesRegex(ValueError, "checked APT selection"):
                vs_package_stages.verify(self.directory, bundle, variant, {}, {})

    def test_dash_rejects_a_replaced_apt_layer_even_when_metadata_matches(self):
        selection = {"schema": 1, "image": "docker-dash-engine", "group": "runtime", "apt_lock_sha256": "lock",
                     "base_manifest_digest": "parent", "dependency_check": {"status": "satisfied"}}
        apt = self.write("runtime-apt.json", selection)
        self.write("engine.packages.json", dict(selection, apt_selection_sha256=artifact_validation.sha(apt),
                                                source_packages=[]))
        layer = {"digest": "apt-layer", "size": 15}
        bundle = {"image": "docker-dash-engine", "apt_lock_sha256": "lock", "apt_layers": {"runtime": layer}}
        current = {"layers": [{"digest": "parent"}, layer.copy()]}
        vs_package_stages.verify(self.directory, bundle, "runtime", current, {})
        current["layers"][1]["size"] += 1
        with self.assertRaisesRegex(ValueError, "first declared stage layer"):
            vs_package_stages.verify(self.directory, bundle, "runtime", current, {})

    def test_p4rt_validates_runtime_and_debug_against_their_direct_bases(self):
        bundle = {"image": "docker-sonic-p4rt"}
        with patch.object(vs_package_stages, "verify_p4rt_stage") as verify:
            vs_package_stages.verify(self.directory, bundle, "runtime", {}, {})
            self.assertEqual([(call.args[1], call.args[2]["manifest_digest"]) for call in verify.call_args_list],
                             [("runtime", "parent")])
            verify.reset_mock()
            vs_package_stages.verify(self.directory, bundle, "debug", {}, {})
            self.assertEqual([(call.args[1], call.args[2]["manifest_digest"]) for call in verify.call_args_list],
                             [("debug", "runtime")])
