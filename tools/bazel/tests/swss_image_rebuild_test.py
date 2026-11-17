#!/usr/bin/env python3
"""Record the image probe's Bazel interface without executing a build."""

import importlib.util
import json
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location("swss_image_rebuild", ROOT / "tools/bazel/ci/swss_image_rebuild.py")
probe = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(probe)


class SwssImageRebuildTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.output_base = self.root / "output base with spaces"
        self.execution_root = self.output_base / "execroot/_main"
        (self.execution_root / "external").mkdir(parents=True)
        self.owner = self.output_base / "external/sonic-swss+"
        (self.owner / "fpmsyncd").mkdir(parents=True)
        (self.execution_root / "external/sonic-swss+").symlink_to(self.owner, target_is_directory=True)
        self.source = self.owner / probe.SOURCE_PATH
        self.original = probe.SHEBANG + b"\nprint('fixture')\n"
        self.source.write_bytes(self.original)
        self.source.chmod(0o640)
        self.output = self.root / "artifacts"
        self.runtime = ["//image:runtime", "//image:runtime.gz", "//image:debug.gz"]
        self.unchanged = ["//image:symbols", "//base:runtime.gz"]
        self.targets = self.runtime + self.unchanged
        self.paths = {target: Path("bazel-out/fixture") / (str(index) + ".tar")
                      for index, target in enumerate(self.targets)}
        self.flags = ["--host_platform=@infra//:amd64", "--platforms=@infra//:amd64",
                      "--experimental_downloader_config=/cache/config with spaces", "--disk_cache=/cache/disk"]
        self.bazel = "/selected wrapper with spaces/review_bazel.sh"
        self.calls, self.phase_sources = [], {}
        self.fault = {}
        self.phase = None

    def command(self, argv, **kwargs):
        self.calls.append(list(argv))
        self.assertEqual(argv[0], self.bazel)
        self.assertTrue(kwargs["check"])
        self.assertEqual(argv[2:2 + len(self.flags)], self.flags)
        command = argv[1]
        if command == "info":
            self.assertEqual(argv[-1], "output_base")
            value = self.fault.get("info", str(self.output_base) + "\n")
            return subprocess.CompletedProcess(argv, 0, stdout=value)
        if command == "cquery":
            target = argv[-2]
            self.assertEqual(argv[-1], "--output=files")
            if target == probe.SOURCE_LABEL:
                value = self.fault.get("source", "external/sonic-swss+/fpmsyncd/bgp_eoiu_marker.py\n")
            else:
                self.assertIn(target, self.targets)
                value = str(self.paths[target]) + "\n"
                if self.fault.get("missing_output") == self.phase:
                    value = "bazel-out/missing.tar\n"
                if self.fault.get("multiple_outputs") == self.phase:
                    value += value
            return subprocess.CompletedProcess(argv, 0, stdout=value)
        self.assertEqual(command, "build")
        self.assertIn("--jobs=2", argv)
        event = next(arg.split("=", 1)[1] for arg in argv if arg.startswith("--build_event_json_file="))
        self.phase = Path(event).name.removesuffix("-events.json")
        self.phase_sources[self.phase] = self.source.read_bytes()
        self.assertEqual(argv[-len(self.targets):], self.targets)
        if self.fault.get("fail_phase") == self.phase:
            raise subprocess.CalledProcessError(1, argv)
        for target, relative in self.paths.items():
            data = self.source.read_bytes() if target in self.runtime else b"unchanged " + target.encode()
            if self.phase == "source-edit" and self.fault.get("inert_edit"):
                data = self.original if target in self.runtime else data
            if ((self.phase == "source-edit" and self.fault.get("changed_stable") == target) or
                    (self.phase == self.fault.get("changed_phase") and target == self.runtime[0])):
                data += b"unexpected change"
            path = self.execution_root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        return subprocess.CompletedProcess(argv, 0)

    def invoke(self):
        argv = ["probe", "--bazel", self.bazel, "--output", str(self.output),
                "--runtime-target", self.runtime[0], "--symbols-target", self.unchanged[0]]
        argv += ["--bazel-arg=" + value for value in self.flags]
        for target in self.runtime[1:]:
            argv += ["--additional-runtime-target", target]
        argv += ["--unchanged-target", self.unchanged[1]]
        with patch.object(sys, "argv", argv), patch.object(probe.subprocess, "run", side_effect=self.command):
            probe.main()

    def assert_restored(self):
        self.assertEqual(self.source.read_bytes(), self.original)
        self.assertEqual(stat.S_IMODE(self.source.stat().st_mode), 0o640)
        receipt = json.loads((self.output / "rebuild.json").read_text())
        self.assertTrue(receipt["source_restored"])
        self.assertEqual(receipt["source_label"], probe.SOURCE_LABEL)
        self.assertEqual(receipt["source_path"], str(probe.SOURCE_PATH))
        self.assertNotIn(str(self.root), json.dumps(receipt))
        return receipt

    def test_four_phases_resolve_actual_owner_and_preserve_all_options_and_outputs(self):
        self.invoke()
        self.assertEqual(self.calls[0][1], "info")
        self.assertEqual(self.calls[1][-2], probe.SOURCE_LABEL)
        self.assertEqual(list(self.phase_sources), ["baseline", "no-op", "source-edit", "restored"])
        for phase, data in self.phase_sources.items():
            expected = probe.SHEBANG + probe.PROBE + self.original[len(probe.SHEBANG):] if phase == "source-edit" else self.original
            self.assertEqual(data, expected)
        receipt = self.assert_restored()
        self.assertEqual(receipt["status"], "passed")
        self.assertEqual([row["phase"] for row in receipt["builds"]], list(self.phase_sources))
        self.assertEqual(len(self.calls), 2 + 4 * (1 + len(self.targets)))

    def test_each_failed_build_restores_exact_owner_bytes_and_mode(self):
        for phase in ("baseline", "no-op", "source-edit", "restored"):
            with self.subTest(phase=phase):
                self.fault = {"fail_phase": phase}
                with self.assertRaises(subprocess.CalledProcessError): self.invoke()
                self.assertEqual(self.assert_restored()["status"], "failed")

    def test_output_and_invalidation_failures_restore_source(self):
        faults = [{"inert_edit": True}, {"changed_phase": "no-op"}, {"changed_phase": "restored"}]
        faults += [{"changed_stable": target} for target in self.unchanged]
        faults += [{name: phase} for name in ("missing_output", "multiple_outputs")
                   for phase in ("baseline", "source-edit", "restored")]
        for fault in faults:
            with self.subTest(fault=fault):
                self.fault = fault
                with self.assertRaises(ValueError): self.invoke()
                self.assertEqual(self.assert_restored()["status"], "failed")

    def test_missing_or_ambiguous_execution_root_never_builds(self):
        for value in ("", "relative/root\n", str(self.root / "missing") + "\n",
                      str(self.execution_root) + "\n" + str(self.execution_root) + "\n"):
            with self.subTest(value=value):
                self.calls.clear(); self.fault = {"info": value}
                with self.assertRaises(ValueError): self.invoke()
                self.assertFalse(any(call[1] == "build" for call in self.calls))
                self.assertEqual(self.source.read_bytes(), self.original)

    def test_source_query_rejects_missing_multiple_local_and_escaping_paths(self):
        source = "external/sonic-swss+/fpmsyncd/bgp_eoiu_marker.py"
        for value in ("", source + "\n" + source + "\n", str(self.source) + "\n",
                      "external/sonic-swss+/fpmsyncd/../fpmsyncd/bgp_eoiu_marker.py\n"):
            with self.subTest(value=value):
                self.calls.clear(); self.fault = {"source": value}
                with self.assertRaises(ValueError): self.invoke()
                self.assertFalse(any(call[1] == "build" for call in self.calls))
                self.assertEqual(self.source.read_bytes(), self.original)

    def test_source_file_symlink_and_unexpected_content_are_rejected_before_build(self):
        outside = self.root / "unrelated.py"; outside.write_bytes(self.original)
        self.source.unlink(); self.source.symlink_to(outside)
        with self.assertRaises(ValueError): self.invoke()
        self.assertEqual(outside.read_bytes(), self.original)
        self.source.unlink()
        for data in (b"unexpected source\n", probe.SHEBANG + probe.PROBE + b"content\n"):
            self.source.write_bytes(data); self.calls.clear()
            with self.assertRaises(ValueError): self.invoke()
            self.assertEqual(self.source.read_bytes(), data)
            self.assertFalse(any(call[1] == "build" for call in self.calls))

    def test_source_parent_symlink_cannot_redirect_the_edit_outside_owner(self):
        outside = self.root / "unrelated-owner/fpmsyncd"; outside.mkdir(parents=True)
        outside_source = outside / self.source.name; outside_source.write_bytes(self.original)
        self.source.parent.rename(self.owner / "retained-fpmsyncd")
        (self.owner / "fpmsyncd").symlink_to(outside, target_is_directory=True)
        with self.assertRaises(ValueError): self.invoke()
        self.assertEqual(outside_source.read_bytes(), self.original)
        self.assertFalse(any(call[1] == "build" for call in self.calls))


if __name__ == "__main__":
    unittest.main()
