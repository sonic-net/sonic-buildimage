"""Exercise VS bundle selection at the producer/consumer boundary."""

import gzip
import hashlib
import io
import json
import sys
from pathlib import Path
import tarfile
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import vs_validation_bundles as bundles
from tools.bazel.ci.vs_container_validation import PMON, static_i386_files, verify_amd64_files


class BundleSelectionTest(unittest.TestCase):
    def test_feature_evidence_uses_the_selected_bazel_flags(self):
        for image, flag, field in (
            (PMON, "//dockers/docker-platform-monitor:pddf", "pddf_enabled"),
            ("docker-sonic-gnmi", "@sonic_gnmi//tools/bazel:enable_dialout", "dialout_enabled"),
        ):
            for value, expected in (("true", True), ("false", False)):
                self.assertEqual(bundles.make_features(image, {flag: value}), {field: expected})
            for invalid in ({}, {flag: "unknown"}):
                with self.assertRaisesRegex(ValueError, "missing Make-selected"):
                    bundles.make_features(image, invalid)

    def test_only_pmon_can_ship_the_original_static_smartcmd(self):
        path = "usr/bin/SmartCmd"
        source = Path(__file__).resolve().parents[3] / "dockers" / PMON / "ssd_tools/SmartCmd"
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            layer = root / "layer.tar"
            with tarfile.open(layer, "w") as archive:
                archive.add(source, arcname=path)
            descriptor = {"digest": "sha256:" + bundles.sha(layer), "size": layer.stat().st_size}
            manifest = {"config": descriptor, "layers": [descriptor]}
            with patch.object(bundles, "layout_metadata", return_value=(descriptor, manifest, {}, [layer])):
                inventory, *_ = bundles.write_inventory(root, "runtime", root, {}, account_layers={},
                                                        static_i386=static_i386_files(PMON))
                files = json.loads(gzip.decompress(inventory.read_bytes()))["files"]
                with self.assertRaisesRegex(ValueError, "expected little-endian ELF64"):
                    bundles.write_inventory(root, "runtime", root, {}, account_layers={})
        verify_amd64_files(PMON, files)
        with self.assertRaisesRegex(ValueError, "non-AMD64"):
            verify_amd64_files("docker-nat", files)
        with self.assertRaisesRegex(ValueError, "non-AMD64"):
            verify_amd64_files(PMON, {**files, "usr/bin/unrelated": files[path]})
        for replacement in ({}, {path: {**files[path], "sha256": "0" * 64}},
                            {path: {**files[path], "elf_machine": 62}}):
            with self.subTest(replacement=replacement), self.assertRaisesRegex(ValueError, "Missing or changed PMON"):
                verify_amd64_files(PMON, replacement)

    def test_private_owner_fixture_uses_its_exact_module_mapping(self):
        """A private target is selected directly and matched to its owner's canonical output."""
        label = "@sonic_sysmgr//tests:reboot_dbus_mock"
        canonical = "@@sonic-sysmgr+//tests:reboot_dbus_mock"
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "mock").touch()
            events = root / "events"
            events.write_text(json.dumps({"workspaceInfo": {"localExecRoot": str(root)}}))
            with patch.object(bundles.subprocess, "check_output", side_effect=[
                json.dumps({"sonic_sysmgr": "sonic-sysmgr+"}), canonical + "\tmock\n",
            ]) as bazel:
                self.assertEqual(bundles.configured_outputs("bazel", [], [label], events), {label: root / "mock"})
                self.assertIn('let roots = set(' + label + ') in config($roots, target) + kind("source file", $roots)',
                              bazel.call_args.args[0])

    def test_cross_repository_alias_binds_its_single_requested_output(self):
        """Bazel may print a rule from an entirely different repository for an owner alias."""
        alias = "@lldpd//:runtime"
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "lldpd.tar").touch()
            events = root / "events"
            events.write_text(json.dumps({"workspaceInfo": {"localExecRoot": str(root)}}))
            with patch.object(bundles.subprocess, "check_output", side_effect=[
                '{"lldpd":"lldpd+"}', "@@lldpd++_repo_rules+lldpd_source//:runtime\tlldpd.tar\n" * 2, "lldpd.tar\n",
            ]) as bazel:
                self.assertEqual(bundles.configured_outputs("bazel", [], [alias], events), {alias: root / "lldpd.tar"})
                self.assertEqual(bazel.call_args.args[0], ["bazel", "cquery", f"config({alias}, target)", "--output=files"])

    def test_inherited_file_checks_reuse_the_full_matching_parent_inventory(self):
        """Every inherited path, including a file outside the sampled files, is available to the selected children."""
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            accounts = {"etc/group": b"root:x:0:\nusers:x:100:\n", "etc/passwd": b"root:x:0:0:root:/root:/bin/sh\n"}
            digests = {name: hashlib.sha256(data).hexdigest() for name, data in accounts.items()}
            account_paths = {name: {"sha256": digest} for name, digest in digests.items()}
            layer = root / "parent.tar"
            with tarfile.open(layer, "w") as archive:
                for name, data in accounts.items():
                    member = tarfile.TarInfo(name)
                    member.size = len(data)
                    archive.addfile(member, io.BytesIO(data))
            origins = {(name, digest): layer for name, digest in digests.items()}
            for name in set(bundles.PARENT_IMAGES.values()):
                owner = root / name
                owner.mkdir()
                inventory = owner / "runtime.oci.json.gz"
                inventory.write_bytes(gzip.compress(json.dumps({
                    "files": {"usr/share/doc/README": {"sha256": "a" * 64}, **account_paths},
                }).encode()))
                digest = bundles.sha(inventory)
                (owner / "bundle.json").write_text(json.dumps({"fixtures": {inventory.name: digest}}))
            for image, parent in bundles.PARENT_IMAGES.items():
                with self.subTest(image=image):
                    child = root / image
                    child.mkdir(exist_ok=True)
                    reference = {"image": parent, "files": account_paths}
                    fixtures = bundles.stage_parent(child, image, {parent: reference}, origins)
                    self.assertEqual(json.loads((child / "parent.json").read_text())["image"], parent)
                    if image in bundles.GROUP_CHECKS | {"docker-teamd"}:
                        self.assertEqual((child / "parent.group").read_bytes(), accounts["etc/group"])
                        self.assertEqual(fixtures["parent.group"], digests["etc/group"])
                    else:
                        self.assertNotIn("parent.group", fixtures)
                    if image == "docker-sonic-otel":
                        self.assertEqual((child / "parent.passwd").read_bytes(), accounts["etc/passwd"])
                    else:
                        self.assertNotIn("parent.passwd", fixtures)
                    if image in bundles.FULL_PARENT_INVENTORY:
                        inherited = child / "parent.oci.json.gz"
                        self.assertTrue(inherited.samefile(root / parent / "runtime.oci.json.gz"))
                        self.assertEqual(fixtures[inherited.name], bundles.sha(inherited))
                    else:
                        self.assertNotIn("parent.oci.json.gz", fixtures)

    def test_apt_evidence_comes_from_each_selected_stage_and_the_original_owner(self):
        """Runtime-only builds omit the debug APT graph."""
        for image in sorted(bundles.APT_REPORT_FAMILIES):
            with self.subTest(image=image), patch("tools.bazel.ci.vs_probe_policy.preparation_targets", return_value={}):
                package = "//dockers/" + image + ":"
                runtime, debug = image + ".gz", image + "-dbg.gz"
                images = {runtime: package + runtime}
                plain = bundles.validation_targets(images, {debug}, False)[image]
                self.assertEqual(plain["sources"]["runtime-apt.json"], package + "runtime_apt_selection")
                self.assertNotIn("debug-apt.json", plain["sources"])
                paired = bundles.validation_targets({**images, debug: package + debug}, {debug}, True)[image]
                self.assertEqual(paired["sources"]["debug-apt.json"], package + "debug_apt_selection")
                if image == "docker-orchagent":
                    self.assertEqual(plain["sources"]["libteam-source-receipt.json"], package + "libteam_source_receipt")
                if image == "docker-dash-engine":
                    self.assertEqual(plain["apt_lock"], bundles.DASH_LOCK)

    def test_service_only_installer_stages_shared_parents_in_dependency_order(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive = PMON + ".gz"
            with tarfile.open(root / archive, "w:gz"):
                pass
            installer = {archive: f"//dockers/{PMON}:{archive}"}
            args = SimpleNamespace(target_path=root, archive=installer, validation_directory=root / "bundles",
                                   architecture="amd64", validation_debug="n", registered_debug=[], validation_only_debug=[])
            with patch("tools.bazel.ci.vs_probe_policy.preparation_targets", return_value={}):
                families = bundles.validation_targets(installer, set(), False)
            targets = {label: root / archive for label in bundles.extra_targets(families)}
            layer = {"digest": "sha256:" + bundles.sha(root / archive), "size": (root / archive).stat().st_size}
            config = {"rootfs": {"type": "layers", "diff_ids": [layer["digest"]]}}
            metadata = (layer, {"config": layer, "layers": [layer]}, config, [root / archive])
            with patch.object(bundles, "layout_metadata", return_value=metadata):
                bundles.write_bundles(args, families, targets, {"//tools/bazel/oci:image_version": "test",
                                                               "//dockers/docker-platform-monitor:pddf": "false"})

            selection = json.loads((args.validation_directory / "selection.json").read_bytes())
            self.assertEqual(selection["installer_archives"], [archive])
            self.assertEqual(selection["auxiliary_parents"], [bundles.BASE, bundles.CONFIG, bundles.SWSS])
            for child, parent in ((bundles.CONFIG, bundles.BASE), (bundles.SWSS, bundles.CONFIG), (PMON, bundles.CONFIG)):
                staged = json.loads((args.validation_directory / child / "parent.json").read_bytes())
                self.assertEqual(staged["image"], parent)

    def test_registered_companion_only_requested_when_debug_is_enabled(self):
        runtime = {"docker-unit.gz": "//unit:docker-unit.gz"}
        debug = {"docker-unit-dbg.gz": "//unit:docker-unit-dbg.gz"}
        registered = set(debug)
        with patch("tools.bazel.ci.vs_probe_policy.preparation_targets", return_value={}) as inputs:
            families = bundles.validation_targets(runtime, registered, False)
            self.assertEqual(set(families), {"docker-unit", *bundles.PARENTS})
            self.assertEqual(set(families["docker-unit"]["members"]), {"runtime"})
            self.assertTrue(families["docker-unit"]["supports_debug"])
            self.assertTrue(all(not call.args[1] for call in inputs.call_args_list))
            self.assertNotIn("//unit:docker-unit-dbg.gz", bundles.extra_targets(families))
            with self.assertRaisesRegex(ValueError, "batch differs from debug selection"):
                bundles.validation_targets(runtime, registered, True)
            paired = bundles.validation_targets({**runtime, **debug}, registered, True)
            self.assertEqual(set(paired["docker-unit"]["members"]), {"runtime", "debug"})
            for parent in bundles.PARENTS:
                self.assertEqual(set(paired[parent]["members"]), {"runtime"})

    def test_validation_only_companion_follows_debug_without_entering_the_installer(self):
        """DASH engine's debug archive is an auxiliary output only in a debug VS run."""
        installer = {"docker-dash-engine.gz": "//platform/vs/docker-dash-engine:docker-dash-engine.gz"}
        debug = "docker-dash-engine-dbg.gz"
        with patch("tools.bazel.ci.vs_probe_policy.preparation_targets", return_value={}):
            runtime = bundles.validation_targets(installer, set(), False, [debug])["docker-dash-engine"]
            paired = bundles.validation_targets(installer, set(), True, [debug])["docker-dash-engine"]
            self.assertTrue(runtime["supports_debug"])
            self.assertEqual(set(runtime["members"]), {"runtime"})
            self.assertEqual(set(paired["members"]), {"runtime", "debug"})
            self.assertEqual(paired["members"]["debug"][1], "//platform/vs/docker-dash-engine:" + debug)
            self.assertNotIn(debug, installer)

    def test_matrix_comes_from_selected_targets_even_if_a_directory_is_lost(self):
        with TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / "selection.json").write_text(json.dumps({
                "installer_archives": ["docker-unit.gz"], "registered_debug": [], "validation_only_debug": [], "debug_enabled": False,
            }))
            for image in ("docker-unit", *bundles.PARENTS):
                (directory / image).mkdir()
                (directory / image / "bundle.json").write_text(json.dumps({"archives": {"runtime": {}}}))
            self.assertEqual([entry["image"] for entry in bundles.matrix(directory, "n")["include"]],
                             sorted(["docker-unit", *bundles.PARENTS]))
            (directory / "docker-unit").rename(directory / "unexpected")
            with self.assertRaisesRegex(ValueError, "differs from the VS build"):
                bundles.matrix(directory, "n")


if __name__ == "__main__":
    unittest.main()
