import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import existing_oci
import equivalence_checker
from comparator import compare_artifacts
from context import Context
from diagnostics import ArtifactIdentifier, ArtifactIndex, ArtifactType, Codes, ComparableArtifact, Diagnostic, DiagnosticSink, Modifier
from extractor import DebugFiles, pair_debug_info


REVISION = "1" * 40


class ExistingOciTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.provenance = {}
        self.manifest = {"schema": 1, "required_images": ["docker-example"], "pairs": [], "tools": {}}
        for builder in ("make", "bazel"):
            self.provenance[builder] = {
                "schema": 1, "builder": builder, "buildimage_commit": REVISION,
                "clean": True, "command": [builder, "recorded-build"],
                "configuration": {"distribution": "trixie", "architecture": "amd64", "platform": "vs", "features": {"INCLUDE_FIPS": "y"}},
                "sources": {"component": {"revision": "2" * 40, "patches": []}},
                "locks": {"apt.lock.json": "3" * 64}, "artifacts": {},
            }
        self.provenance["make"]["BUILD_WITH_BAZEL_WHEN_AVAILABLE"] = "n"
        for name in ("docker-example.gz", "docker-example-dbg.gz"):
            pair = {"name": name}
            for builder in ("make", "bazel"):
                record = self.file(builder + "/" + name, (builder + name).encode())
                pair[builder] = record
                self.provenance[builder]["artifacts"][name] = record["sha256"]
            self.manifest["pairs"].append(pair)
        for name in ("readelf", "abidiff", "elfcompare"):
            record = self.file("tools/" + name, b"declared test-only tool")
            self.manifest["tools"][name] = dict(record, origin="synthetic pure-test fixture", closure=[record])
        self.path = self.root / "pairs.json"

    def file(self, name, data):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return {"path": name, "sha256": hashlib.sha256(data).hexdigest()}

    def save(self):
        for builder, provenance in self.provenance.items():
            self.manifest[builder + "_provenance"] = self.file(builder + ".json", json.dumps(provenance).encode())
        self.path.write_text(json.dumps(self.manifest))

    def load(self):
        self.save()
        return existing_oci.load(self.path, REVISION)

    def test_existing_pairs_need_no_process_query_or_build(self):
        with patch("subprocess.run", side_effect=AssertionError("process forbidden")), patch("subprocess.Popen", side_effect=AssertionError("process forbidden")):
            artifacts, required, tools, manifest_sha256 = self.load()
        self.assertEqual(required, {"docker-example.gz", "docker-example-dbg.gz"})
        self.assertEqual({str(a.type) for a in artifacts}, {"OCI_IMAGE"})
        self.assertIsNone(tools.dpkg_deb)
        self.assertEqual(manifest_sha256, hashlib.sha256(self.path.read_bytes()).hexdigest())

    def test_cli_uses_the_existing_comparison_without_resolving_or_building_tools(self):
        self.save()
        args = SimpleNamespace(existing_oci=self.path, source_checkout=self.root, report=self.root / "report.json", bldenv="trixie", jobs=1)
        tool_root = equivalence_checker.registry_lib.REPO_ROOT
        tool_revision = "9" * 40
        replies = [SimpleNamespace(stdout=str(self.root)), SimpleNamespace(stdout=""), SimpleNamespace(stdout=REVISION + "\n"), SimpleNamespace(stdout=str(tool_root)), SimpleNamespace(stdout=""), SimpleNamespace(stdout=tool_revision + "\n")]
        with patch("equivalence_checker.subprocess.run", side_effect=replies) as git, patch("equivalence_checker.Bazel", side_effect=AssertionError("Bazel forbidden")), patch("equivalence_checker.Tools.resolve", side_effect=AssertionError("tool builds forbidden")), patch("equivalence_checker.extract_all", side_effect=lambda ctx, sources, **kwargs: sources) as extract, patch("equivalence_checker.compare_artifacts") as compare:
            self.assertEqual(equivalence_checker.run_existing(args), 0)
        self.assertEqual([call.args[0] for call in git.call_args_list], [["git", "rev-parse", "--show-toplevel"], ["git", "status", "--porcelain"], ["git", "rev-parse", "HEAD"]] * 2)
        self.assertEqual([call.kwargs["cwd"] for call in git.call_args_list], [self.root] * 3 + [tool_root] * 3)
        self.assertEqual(extract.call_count, 1)
        self.assertEqual(extract.call_args.kwargs, {"scoped_oci": True})
        self.assertEqual(compare.call_count, 1)
        self.assertEqual(compare.call_args.kwargs, {"scoped_oci": True})
        self.assertFalse(extract.call_args.args[0].needs_build)
        self.assertIsNone(extract.call_args.args[0].bazel)
        self.assertEqual(json.loads(args.report.read_text())["scope"]["required_pairs"], ["docker-example-dbg.gz", "docker-example.gz"])
        self.assertEqual(json.loads(args.report.read_text())["source_checkout"]["revision"], REVISION)
        self.assertEqual(json.loads(args.report.read_text())["tool_checkout"]["revision"], tool_revision)
        self.assertEqual(json.loads(args.report.read_text())["provenance_manifest"]["sha256"], hashlib.sha256(self.path.read_bytes()).hexdigest())

    def test_report_cannot_overwrite_declared_inputs_or_their_aliases(self):
        self.save()
        inputs = [self.path, self.root / "make.json", self.root / self.manifest["pairs"][0]["make"]["path"], self.root / "tools/readelf"]
        for input_path in inputs:
            with self.subTest(input_path=input_path):
                before = input_path.read_bytes()
                with self.assertRaisesRegex(ValueError, "report path aliases an input"):
                    existing_oci.load(self.path, REVISION, report=input_path)
                self.assertEqual(input_path.read_bytes(), before)
        for kind in ("symlink", "hardlink"):
            alias = self.root / (kind + "-report.json")
            if kind == "symlink":
                alias.symlink_to(self.path)
            else:
                alias.hardlink_to(self.path)
            with self.subTest(kind=kind), self.assertRaisesRegex(ValueError, "report path aliases an input"):
                existing_oci.load(self.path, REVISION, report=alias)

    def test_cli_rejects_manifest_as_report_before_comparison(self):
        self.save()
        before = self.path.read_bytes()
        args = SimpleNamespace(existing_oci=self.path, source_checkout=self.root, report=self.path)
        with patch("equivalence_checker.checkout_revision", side_effect=[REVISION, "9" * 40]), patch("equivalence_checker.extract_all", side_effect=AssertionError("must reject before comparison")):
            with self.assertRaisesRegex(ValueError, "report path aliases an input"):
                equivalence_checker.run_existing(args)
        self.assertEqual(self.path.read_bytes(), before)

    def test_cli_rejects_dirty_checkout_before_artifacts_or_tools(self):
        args = SimpleNamespace(existing_oci=self.path, source_checkout=self.root, report=self.root / "report.json")
        replies = [SimpleNamespace(stdout=str(self.root)), SimpleNamespace(stdout=" M MODULE.bazel\n")]
        with patch("equivalence_checker.subprocess.run", side_effect=replies), patch("equivalence_checker.existing_oci.load", side_effect=AssertionError("must stop first")):
            with self.assertRaisesRegex(ValueError, "clean checkout"):
                equivalence_checker.run_existing(args)

    def test_wrong_producer_revision_is_rejected_independent_of_tool_revision(self):
        self.save()
        args = SimpleNamespace(existing_oci=self.path, source_checkout=self.root, report=self.root / "report.json")
        with patch("equivalence_checker.checkout_revision", side_effect=["4" * 40, "9" * 40]), patch("equivalence_checker.extract_all", side_effect=AssertionError("must reject before comparison")):
            with self.assertRaisesRegex(ValueError, "source checkout mismatch"):
                equivalence_checker.run_existing(args)

    def test_cli_argument_forms_and_build_opt_in_fail_closed(self):
        valid = [["--existing-oci", str(self.path), "--source-checkout", str(self.root)], ["--jobs", "1", "--source-checkout", str(self.root), "--existing-oci=" + str(self.path)]]
        invalid = [[], ["--skip-build"], ["--existing-oci"], ["--existing-oci=" + str(self.path)], valid[0] + ["--unknown"], valid[0] + ["--build-all-pairs"]]
        with patch("equivalence_checker.Bazel", side_effect=AssertionError("build forbidden")), patch("equivalence_checker.subprocess.run", side_effect=AssertionError("process forbidden")):
            for argv in valid:
                with self.subTest(argv=argv), patch.object(sys, "argv", ["checker"] + argv), patch("equivalence_checker.run_existing", return_value=0) as run:
                    self.assertEqual(equivalence_checker.main(), 0)
                    self.assertEqual(run.call_count, 1)
            for argv in invalid:
                with self.subTest(argv=argv), patch.object(sys, "argv", ["checker"] + argv), patch("equivalence_checker.run_existing", side_effect=AssertionError("comparison forbidden")):
                    with self.assertRaises(SystemExit) as failure:
                        equivalence_checker.main()
                    self.assertEqual(failure.exception.code, 2)

    def test_shell_routes_every_existing_form_before_legacy_actions(self):
        # Only /bin/bash and recording builtins run. PATH contains no Make/APT,
        # Python, Bazel, archive or native comparison executable.
        shell = Path(__file__).resolve().parent.parent / "test_equivalence.sh"
        self.assertTrue(shell.is_file(), "the existing shell entry must be present for this read-only routing check")
        with tempfile.TemporaryDirectory(prefix="oci-routing-") as tmp:
            root = Path(tmp)
            commands = root / "commands"
            commands.mkdir()
            (commands / "git").write_text('#!/bin/bash\nprintf "%s\\n" "$ROUTING_ROOT"\n')
            (commands / "env").write_text('#!/bin/bash\nprintf "%s\\n" "$@" > "$ROUTING_RECORD"\nexit 95\n')
            for path in commands.iterdir():
                path.chmod(0o755)
            record = root / "record"
            env = {"PATH": str(commands), "ROUTING_ROOT": str(root), "ROUTING_RECORD": str(record)}
            routed = [["--existing-oci", "pairs.json"], ["--existing-oci=pairs.json"], ["--jobs", "1", "--existing-oci", "pairs.json"], ["--existing-oci"], ["--unknown", "--existing-oci=pairs.json"]]
            rejected = [[], ["--unknown"], ["--build-all-pairs", "--unknown"]]
            for argv in routed + rejected:
                with self.subTest(argv=argv):
                    record.unlink(missing_ok=True)
                    result = subprocess.run(["/bin/bash", str(shell), *argv], env=env, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 95 if argv in routed else 2, result.stderr)
                    if argv in routed:
                        self.assertEqual(record.read_text().splitlines(), ["PYTHONPATH=tools/bazel/registry", "python3", "tools/bazel/equivalence_checker/equivalence_checker.py", *argv])
                    else:
                        self.assertFalse(record.exists())

    def test_missing_required_debug_pair_is_rejected(self):
        self.manifest["pairs"].pop()
        with self.assertRaisesRegex(ValueError, "missing required runtime/debug"):
            self.load()

    def test_missing_or_corrupt_required_artifacts_are_rejected(self):
        path = self.root / self.manifest["pairs"][0]["make"]["path"]
        original = path.read_bytes()
        for content in (None, b"corrupt"):
            with self.subTest(content=content):
                path.unlink(missing_ok=True)
                if content is not None:
                    path.write_bytes(content)
                with self.assertRaises(ValueError):
                    self.load()
                path.write_bytes(original)

    def test_missing_or_stale_provenance_is_rejected(self):
        for key, value in (("buildimage_commit", "4" * 40), ("clean", False), ("sources", {}), ("locks", {}), ("command", []), ("BUILD_WITH_BAZEL_WHEN_AVAILABLE", "y")):
            with self.subTest(key=key):
                before = copy.deepcopy(self.provenance)
                self.provenance["make"][key] = value
                with self.assertRaises(ValueError):
                    self.load()
                self.provenance = before
        self.save()
        (self.root / "make.json").unlink()
        with self.assertRaisesRegex(ValueError, "missing file"):
            existing_oci.load(self.path, REVISION)

    def test_source_patch_lock_and_configuration_drift_are_rejected(self):
        for key, value in (("sources", {"component": {"revision": "5" * 40, "patches": []}}), ("sources", {"component": {"revision": "2" * 40, "patches": ["6" * 64]}}), ("locks", {"apt.lock.json": "7" * 64}), ("configuration", {"distribution": "trixie", "architecture": "arm64", "platform": "vs", "features": {}})):
            with self.subTest(key=key, value=value):
                before = copy.deepcopy(self.provenance)
                self.provenance["bazel"][key] = value
                with self.assertRaisesRegex(ValueError, "mismatch"):
                    self.load()
                self.provenance = before

    def test_duplicate_or_deb_pairs_are_rejected(self):
        self.manifest["pairs"].append(copy.deepcopy(self.manifest["pairs"][0]))
        with self.assertRaisesRegex(ValueError, "duplicate"):
            self.load()
        self.manifest["pairs"].pop()
        self.manifest["pairs"][0]["name"] = "example.deb"
        with self.assertRaisesRegex(ValueError, "only named OCI"):
            self.load()

    def test_same_physical_artifact_is_rejected(self):
        pair = self.manifest["pairs"][0]
        pair["bazel"] = pair["make"]
        self.provenance["bazel"]["artifacts"][pair["name"]] = pair["make"]["sha256"]
        with self.assertRaisesRegex(ValueError, "same artifact file"):
            self.load()

    def test_producer_hash_and_tool_closure_are_checked(self):
        self.provenance["make"]["artifacts"]["docker-example.gz"] = "8" * 64
        with self.assertRaisesRegex(ValueError, "producer hash"):
            self.load()
        self.provenance["make"]["artifacts"]["docker-example.gz"] = self.manifest["pairs"][0]["make"]["sha256"]
        self.manifest["tools"]["abidiff"]["closure"] = []
        with self.assertRaisesRegex(ValueError, "tool closure"):
            self.load()

    def test_diagnostics_remain_visible_without_failing_required_pairs(self):
        required = {"docker-example.gz", "docker-example-dbg.gz"}
        other = ArtifactIdentifier("docker-other.gz", None, frozenset())
        child = ArtifactIdentifier("/usr/bin/example", other, frozenset({Modifier.RUNTIME}))
        result = existing_oci.scoped_result({None: [Diagnostic(child, Codes.FILE_CONTENT_MISMATCH, "different")]}, required, required | {other.name})
        self.assertEqual(result["required_status"], "PASS")
        self.assertEqual(result["all_compared_status"], "FAIL")
        self.assertEqual(result["diagnostic_pairs"], [other.name])

    def test_scoped_comparator_contains_readelf_errors_without_debug_files(self):
        make = self.root / self.file("elf/make", b"\x7fELFmake")["path"]
        bazel = self.root / self.file("elf/bazel", b"\x7fELFbazel")["path"]
        required = {"docker-required.gz"}
        for name in ("docker-diagnostic.gz", "docker-required.gz"):
            with self.subTest(name=name):
                source = ArtifactIdentifier(name, None, frozenset())
                identifier = ArtifactIdentifier("/usr/bin/truncated", source, frozenset({Modifier.RUNTIME}))
                artifact = ComparableArtifact(identifier, bazel, make, ArtifactType.ELF_EXECUTABLE)
                ctx = Context(sink=DiagnosticSink(), index=ArtifactIndex(), bazel=None,
                              tools=SimpleNamespace(readelf=object()), needs_build=False,
                              debian_release="trixie", jobs=1, workdir=self.root)
                error = subprocess.CalledProcessError(1, ["readelf", "-S"])
                with patch("extractor._build_id_of_binary", side_effect=AssertionError("needless readelf")), patch("comparator._has_symtab", side_effect=error):
                    self.assertEqual(pair_debug_info(ctx, [artifact], DebugFiles(), scoped_oci=True), ([], set()))
                    compare_artifacts(ctx, [artifact], scoped_oci=True)
                    with self.assertRaises(subprocess.CalledProcessError):
                        compare_artifacts(ctx, [artifact])
                self.assertEqual([(existing_oci.root_name(d.artifact), d.code) for d in ctx.sink.diagnostics],
                                 [(name, Codes.ELFCOMPARE_ERROR)])
                result = existing_oci.scoped_result({None: ctx.sink.diagnostics}, required, required | {name})
                self.assertEqual(result["required_status"], "FAIL" if name in required else "PASS")
                self.assertEqual(result["all_compared_status"], "FAIL")

    def test_scoped_comparator_contains_unreadable_plain_files(self):
        make = self.root / self.file("plain/make", b"make")["path"]
        bazel = self.root / self.file("plain/bazel", b"bazel")["path"]
        required = {"docker-required.gz"}
        for name in ("docker-diagnostic.gz", "docker-required.gz"):
            with self.subTest(name=name):
                source = ArtifactIdentifier(name, None, frozenset())
                identifier = ArtifactIdentifier("/usr/share/value", source, frozenset({Modifier.RUNTIME}))
                artifact = ComparableArtifact(identifier, bazel, make, ArtifactType.FILE)
                ctx = Context(sink=DiagnosticSink(), index=ArtifactIndex(), bazel=None,
                              tools=SimpleNamespace(), needs_build=False,
                              debian_release="trixie", jobs=1, workdir=self.root)
                with patch("comparator.filecmp.cmp", side_effect=PermissionError("unreadable file")):
                    compare_artifacts(ctx, [artifact], scoped_oci=True)
                    with self.assertRaises(PermissionError):
                        compare_artifacts(ctx, [artifact])
                self.assertEqual([(existing_oci.root_name(d.artifact), d.code) for d in ctx.sink.diagnostics],
                                 [(name, Codes.ELFCOMPARE_ERROR)])
                result = existing_oci.scoped_result({None: ctx.sink.diagnostics}, required, required | {name})
                self.assertEqual(result["required_status"], "FAIL" if name in required else "PASS")
                self.assertEqual(result["all_compared_status"], "FAIL")

    def test_missing_and_accepted_incomplete_required_pairs_fail(self):
        name = "docker-example.gz"
        parent = ArtifactIdentifier(name, None, frozenset())
        for classified, compared in (({}, set()), ({"future-exclusion": [Diagnostic(parent, Codes.COLLECTION_EXCLUDED_BY_TAG, "excluded")]}, {name}), ({None: [Diagnostic(parent, Codes.FILE_CONTENT_MISMATCH, "different")]}, {name})):
            with self.subTest(classified=classified):
                self.assertEqual(existing_oci.scoped_result(classified, {name}, compared)["required_status"], "FAIL")


if __name__ == "__main__":
    unittest.main()
