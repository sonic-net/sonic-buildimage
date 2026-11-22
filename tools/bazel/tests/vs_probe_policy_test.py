"""Prove that VS service probes honor the build selection and independent owner bytes."""

import hashlib
import io
import json
from pathlib import Path
import tarfile
import tempfile
import unittest

from tools.bazel.ci import vs_container_probe as probe
from tools.bazel.ci import vs_probe_policy as policy


class VSProbePolicyTest(unittest.TestCase):
    def test_debug_dash_engine_validates_the_runtime_manifest_used_by_its_labels_target(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            runtime_manifest = root / "manifests/docker-dash-engine/manifest.json"
            runtime_manifest.parent.mkdir(parents=True)
            runtime_manifest.write_text("{}")
            products = policy.prepare("docker-dash-engine", "test", "debug", None, {}, root,
                                      make_manifests=root / "manifests")
            self.assertEqual(products["debug.make.manifest.json"], runtime_manifest)

    def test_teamd_rejects_apt_packages_already_supplied_by_the_swss_parent(self):
        from tools.bazel.ci.teamd_image import verify_parent_runtime_packages
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            selection = directory / "runtime-apt.json"
            owner = "//dockers/docker-teamd:runtime_apt_selection"
            for debug in (False, True):
                self.assertEqual(policy.preparation_targets("docker-teamd", debug)[selection.name], owner)
            selection.write_text(json.dumps({"selected": []}))
            exported = policy.prepare("docker-teamd", "test", "runtime", None,
                                      {selection.name: selection}, directory)
            self.assertEqual(exported[selection.name], selection)
            verify_parent_runtime_packages(directory)
            selection.write_text(json.dumps({"selected": [{"package": "libnl-3-200"}]}))
            with self.assertRaisesRegex(ValueError, "already owned by its parent"):
                probe.runtime_report(directory, {"image": "docker-teamd"})

    def test_supervised_services_bind_selected_owner_outputs_to_the_installer_build(self):
        from tools.bazel.ci import vs_standalone_probes as standalone
        from tools.bazel.ci import vs_validation_bundles as bundles
        for image in standalone.SUPERVISED:
            with self.subTest(image=image), tempfile.TemporaryDirectory() as temporary:
                runtime, debug = image + ".gz", image + "-dbg.gz"
                label = "//dockers/" + image + ":"
                for enabled in (False, True):
                    archives = {runtime: label + runtime}
                    if enabled:
                        archives[debug] = label + debug
                    family = bundles.validation_targets(archives, [debug], enabled)[image]
                    self.assertEqual(set(family["members"]), {"runtime", "debug"} if enabled else {"runtime"})
                    targets = family["sources"]
                    self.assertEqual(targets["service.files.tar"], label + "service_files")
                    self.assertEqual("all-debug.tar" in targets, enabled)
                    if enabled:
                        self.assertEqual(targets["all-debug.tar"], label + image + ".debug_symbols")
                    if image in standalone.WATCHDOGS:
                        self.assertEqual(targets["watchdog.source.tar"], "//dockers/" + image + "/watchdog:runtime")
                    artifacts = {name: Path(temporary) / name for name in targets}
                    artifacts["runtime.owners.json"] = Path(temporary) / "runtime.owners.json"
                    projected = bundles.stage_probe_fixtures(image, artifacts, Path(temporary))
                    expected = {"runtime.owners.json"}
                    if enabled and image in standalone.WATCHDOGS:
                        self.assertEqual(targets["watchdog-symbol-tools.tar.gz"], "//tools/bazel/ci:watchdog_symbol_tools")
                        expected.add("watchdog-symbol-tools.tar.gz")
                    self.assertEqual(set(projected), expected)

    def test_supervised_runtime_rejects_detached_symbols_from_its_debug_companion(self):
        from tools.bazel.ci.vs_standalone_probes import SUPERVISED
        native = {"usr/bin/watchdog": {"build_id": "abcd"}}
        symbols = "usr/lib/debug/.build-id/ab/cd.debug"
        for image in SUPERVISED:
            with self.subTest(image=image):
                probe.verify_debug_separation(image, "runtime", native, {}, {"usr/bin/watchdog"})
                with self.assertRaisesRegex(ValueError, "runtime contains debug-only files"):
                    probe.verify_debug_separation(image, "runtime", native, {}, {symbols})
                with self.assertRaisesRegex(ValueError, "runtime contains debug-only files"):
                    probe.verify_debug_separation(image, "debug", native, {"all-debug.tar": {symbols: {}}}, {symbols})

    def test_disabled_debug_does_not_request_owner_symbols(self):
        for image in set(policy.FAMILIES) | {"docker-gnmi-sidecar", "docker-restapi-sidecar",
                                                     "docker-gnmi-watchdog", "docker-bmp-watchdog"}:
            with self.subTest(image=image):
                targets = policy.preparation_targets(image, False)
                self.assertFalse([name for name in targets if "debug" in name or "symbols" in name])
        self.assertIn("p4rt.debug.tar", policy.preparation_targets("docker-sonic-p4rt", True))

    def test_p4rt_projects_only_selected_owner_packages_and_records_whole_tar_hashes(self):
        image = "docker-sonic-p4rt"
        self.assertEqual(policy.preparation_targets(image, False)["p4rt.source.tar"],
                         "@sonic_pins//p4rt_app:p4rt_runtime")
        self.assertEqual(policy.preparation_targets(image, True)["p4rt.debug.tar"],
                         "//dockers/docker-sonic-p4rt:all_debug_symbols")
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            inputs = {}
            files = {"p4rt.source.tar": "usr/share/p4rt/main.json", "sairedis.source.tar": "usr/lib/libsairedis.txt"}
            for name, member in files.items():
                inputs[name] = root / name
                with tarfile.open(inputs[name], "w") as output:
                    info = tarfile.TarInfo(member)
                    info.size = 1
                    output.addfile(info, io.BytesIO(b"x"))
            for variant, expected in (("runtime", {"p4rt.source.tar"}), ("debug", set(inputs))):
                result = policy.prepare(image, "test", variant, None, inputs, root)
                self.assertEqual(result.keys() & inputs.keys(), {"p4rt.source.tar"} if variant == "debug" else set())
                report = json.loads(result[variant + ".owners.json"].read_bytes())
                self.assertEqual(set(report["owners"]), expected)
                for name in expected:
                    self.assertEqual(report["source_sha256"][name], hashlib.sha256(inputs[name].read_bytes()).hexdigest())
                    self.assertEqual(report["owners"][name][files[name]]["sha256"], hashlib.sha256(b"x").hexdigest())

    def test_p4rt_exports_the_pinned_owner_test_inputs_only_with_the_debug_image(self):
        image = "docker-sonic-p4rt"
        targets = {
            "p4rt.source.tar": "@sonic_pins//p4rt_app:p4rt_runtime",
            "pins-debug-symbols.tar": "@sonic_pins//p4rt_app:p4rt_runtime.debug_symbols",
            "pins-metadata.json": "@sonic_pins//p4rt_app:runtime_packages.json",
            "pins-binutils.tar.gz": "//dockers/docker-sonic-p4rt/bazel:owner_binutils_tar",
        }
        self.assertEqual({k: policy.preparation_targets(image, True)[k] for k in targets}, targets)
        self.assertEqual(targets.keys() & policy.preparation_targets(image, False).keys(), {"p4rt.source.tar"})
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            paths = {name: directory / name for name in targets}
            with tarfile.open(paths["p4rt.source.tar"], "w"):
                pass
            runtime = policy.prepare(image, "test", "runtime", None, paths, directory)
            self.assertFalse(paths.keys() & runtime.keys())
            debug = policy.prepare(image, "test", "debug", None, paths, directory)
            self.assertEqual({name: debug[name] for name in targets}, paths)

    def test_runtime_only_bundle_never_runs_a_companion_probe(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            variants = ("docker-mux", "docker-mux-dbg")
            bundle = {"archives": {"runtime": {"file": "docker-mux.gz"}}, "debug_enabled": False,
                      "supports_debug": True}
            (directory / "bundle.json").write_text(json.dumps(bundle))
            self.assertEqual(policy.selected_images(directory, variants), variants[:1])
            bundle["debug_enabled"] = True
            (directory / "bundle.json").write_text(json.dumps(bundle))
            with self.assertRaisesRegex(ValueError, "selected service variants"):
                policy.selected_images(directory, variants)

    def test_gearbox_debug_launcher_follows_its_declared_implementation_provider(self):
        launcher, library, real = "usr/bin/pi_CLI_dummy", "usr/lib/libpicli.so.0", "usr/lib/libpicli.so.0.1"
        symbol_dir = "usr/lib/debug/.build-id/"
        base = {"mode": 0o755, "uid": 0, "gid": 0, "has_debuglink": True, "needed": [], "sha256": "123"}
        native = {launcher: {**base, "build_id": "0011"}, real: {**base, "build_id": "0022"}}
        files = {path: {**item, "elf_machine": 62} for path, item in native.items()}
        files.update({library: {"kind": "symlink", "linkname": "libpicli.so.0.1"},
                      symbol_dir + "00/11.debug": {}, symbol_dir + "00/22.debug": {}})
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            spec = {"provider": library, "symbol": "main", "source": "main.c"}
            (directory / "policy.json").write_text(json.dumps({"native": {"debug_forwarding": {launcher: spec}}}))
            _, pairs = probe.native_expectations(directory, {"fixtures": {}, "image": "docker-gbsyncd-vs"},
                                                 "debug", files, {"native": native, "owners": {}})
            record = next(row for row in pairs if row["path"] == launcher)
            self.assertEqual(record["debug_forwarding"], {**spec, "debug_path": symbol_dir + "00/22.debug"})

    def test_missing_native_source_or_requested_symbols_fails_before_docker(self):
        path = "usr/bin/service"
        owner = {"sha256": "abc", "mode": 0o755, "uid": 0, "gid": 0, "build_id": "aa11",
                 "has_debuglink": True, "needed": ["libc.so.6"]}
        installed = {path: {**owner, "elf_machine": 62}}
        evidence = {"native": {path: owner}, "owners": {}}
        bundle = {"fixtures": {}, "image": "docker-mux"}
        native, pairs = probe.native_expectations(Path("."), bundle, "runtime", installed, evidence)
        self.assertEqual(list(native), [path])
        self.assertEqual(pairs, [])
        with self.assertRaisesRegex(ValueError, "matching detached symbols"):
            probe.native_expectations(Path("."), bundle, "debug", installed, evidence)
        installed[path]["sha256"] = "wrong"
        with self.assertRaisesRegex(ValueError, "source owner ELF"):
            probe.native_expectations(Path("."), bundle, "runtime", installed, evidence)


if __name__ == "__main__":
    unittest.main()
