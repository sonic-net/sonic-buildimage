"""Run the real image-verification workflow shell against recording commands."""

import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[3]


class VerificationWorkflowTest(unittest.TestCase):
    def run_sysmgr_public_evidence(self, directory):
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        step = next(step for step in workflow["jobs"]["archive"]["steps"]
                    if step.get("name") == "Prepare reviewed public evidence")
        # The existing guard owns when the step runs. Exercise its Python
        # publication boundary after an attempted build, including early failure.
        script = step["run"].split("python3 - <<'PYCODE'\n", 1)[1].split("\nPYCODE", 1)[0]
        previous = Path.cwd()
        sys.path.insert(0, str(ROOT))
        try:
            os.chdir(directory)
            exec(compile(script, "<sysmgr-public-evidence>", "exec"), {})
        finally:
            os.chdir(previous)
            sys.path.pop(0)

    def test_sysmgr_public_evidence_handles_failure_before_build_directory_exists(self):
        with tempfile.TemporaryDirectory(prefix="sysmgr early failure ") as temporary:
            directory = Path(temporary)
            self.run_sysmgr_public_evidence(directory)
            root = directory / "artifacts/sysmgr"
            self.assertEqual(json.loads((root / "build-summary.json").read_text()), {})
            self.assertEqual([path.name for path in root.iterdir()], ["build-summary.json"])

    def test_sysmgr_public_evidence_preserves_failed_receipt_without_private_commands(self):
        with tempfile.TemporaryDirectory(prefix="sysmgr failed receipt ") as temporary:
            directory = Path(temporary)
            root = directory / "artifacts/sysmgr"
            root.mkdir(parents=True)
            receipt = {"status": "failed", "architecture": "amd64", "revision": "a" * 40,
                       "artifacts": {}, "commands": [["private diagnostic"]]}
            (root / "build-receipt.json").write_text(json.dumps(receipt))
            self.run_sysmgr_public_evidence(directory)
            self.assertEqual(json.loads((root / "public-receipt.json").read_text()),
                             {key: value for key, value in receipt.items() if key != "commands"})
            self.assertEqual(json.loads((root / "build-summary.json").read_text()), {})

    def test_sysmgr_public_evidence_rejects_malformed_or_private_present_evidence(self):
        for name, contents in (("validation.json", "{malformed"),
                               ("validation.json", '{"environment": {"PRIVATE": "value"}}'),
                               ("build-receipt.json", "{malformed")):
            with self.subTest(name=name, contents=contents):
                with tempfile.TemporaryDirectory(prefix="sysmgr rejected evidence ") as temporary:
                    directory = Path(temporary)
                    root = directory / "artifacts/sysmgr"
                    root.mkdir(parents=True)
                    (root / name).write_text(contents)
                    with self.assertRaises(ValueError):
                        self.run_sysmgr_public_evidence(directory)
                    self.assertFalse((root / "build-summary.json").exists())

    def test_required_amd64_archive_check_runs_installed_eventd(self):
        jobs = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]
        archive = jobs["archive"]
        gate = jobs["eventd_runtime"]
        self.assertEqual(gate["name"], "Container archive (AMD64)")
        self.assertEqual(gate["needs"], [
            "archive", "database_acceptance", "frr_runtime", "teamd_runtime",
            "radv_acceptance", "lldp_runtime", "mux_runtime", "gearbox_runtime", "macsec_runtime",
        ])
        self.assertEqual(jobs["macsec_runtime"]["needs"], "archive")
        self.assertTrue(any("macsec_loaded.py" in step.get("run", "")
                            for step in jobs["macsec_runtime"]["steps"]))
        self.assertEqual(gate["if"], "always() && (" + archive["if"] + ")")
        build = next(step for step in archive["steps"] if step.get("id") == "eventd_build")
        self.assertEqual(build["if"], "matrix.architecture == 'amd64'")
        producer_names = {archive["name"].replace("${{ matrix.name }}", row["name"])
                          for row in archive["strategy"]["matrix"]["include"]}
        self.assertNotIn(gate["name"], producer_names)
        self.assertIn("Container archive (ARM64)", producer_names)
        upload = next(step for step in archive["steps"]
                      if "eventd/public/*.gz" in step.get("with", {}).get("path", ""))
        download = next(step for step in gate["steps"] if "download-artifact@" in step.get("uses", ""))
        # Paths within the checkout resolve the same way for container shell
        # steps and upload-artifact. Downloaded files must share one flat root.
        patterns = upload["with"]["path"].splitlines()
        root = Path("artifacts/eventd/public")
        self.assertEqual({Path(pattern).parent for pattern in patterns}, {root})
        for name in ("docker-eventd.gz", "docker-eventd-dbg.gz", "validation.json",
                     "test-redis.tar", "config-engine-source.tar", "eventd-runtime.tar"):
            self.assertTrue(any((root / name).match(pattern) for pattern in patterns), name)
        self.assertEqual(upload["with"]["name"], download["with"]["name"])
        self.assertTrue(any("eventd_loaded.py" in step.get("run", "") for step in gate["steps"]))
        guard = gate["steps"][0]
        self.assertEqual(guard["env"]["JOB_RESULTS"], "${{ toJSON(needs.*.result) }}")
        self.assertNotIn("continue-on-error", guard)
        self.assertNotIn("if", guard)
        for malformed_or_empty in ("[]", "null", "not-json"):
            with self.subTest(results=malformed_or_empty):
                result = subprocess.run(["bash", "-ec", guard["run"]], capture_output=True,
                                        env=dict(os.environ, JOB_RESULTS=malformed_or_empty))
                self.assertNotEqual(result.returncode, 0)
        for position in range(len(gate["needs"])):
            for status in ("success", "failure", "skipped", "cancelled"):
                with self.subTest(job=gate["needs"][position], status=status):
                    results = ["success"] * len(gate["needs"])
                    results[position] = status
                    result = subprocess.run(["bash", "-ec", guard["run"]], capture_output=True,
                                            env=dict(os.environ, JOB_RESULTS=json.dumps(results)))
                    self.assertEqual(result.returncode == 0, status == "success")

    def test_archive_evidence_preservation_is_unique_and_keeps_previous_runs(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        steps = workflow["jobs"]["archive"]["steps"]
        preserve = next(
            step for step in steps
            if step.get("name") == "Preserve completed archive evidence before Sysmgr acceptance"
        )
        upload = next(step for step in steps if step.get("name") == "Upload public archive evidence")
        build = next(step for step in steps if step.get("name") == "Build Sysmgr runtime and debug images")
        self.assertLess(steps.index(upload), steps.index(preserve))
        self.assertLess(steps.index(preserve), steps.index(build))
        self.assertEqual(preserve["if"], "matrix.architecture == 'amd64'")
        with tempfile.TemporaryDirectory(prefix="archive evidence ") as temporary:
            root = Path(temporary)
            scratch = root / "scratch"
            scratch.mkdir()
            old = scratch / "pre-sysmgr-artifacts"
            old.mkdir()
            (old / "receipt").write_text("retained")
            destinations = []
            for index in range(2):
                artifacts = root / "artifacts"
                artifacts.mkdir()
                (artifacts / "receipt").write_text(str(index))
                result = subprocess.run(
                    ["bash", "-e", "-o", "pipefail", "-c", preserve["run"]],
                    cwd=root, capture_output=True, text=True,
                    env=dict(os.environ, RUNNER_TEMP=str(scratch)),
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertFalse(artifacts.exists())
                candidates = set(scratch.glob("pre-sysmgr-artifacts.*")) - set(destinations)
                self.assertEqual(len(candidates), 1)
                destination = candidates.pop()
                self.assertEqual(destination.stat().st_mode & 0o777, 0o700)
                self.assertEqual((destination / "artifacts/receipt").read_text(), str(index))
                destinations.append(destination)
            self.assertNotEqual(*destinations)
            self.assertEqual((old / "receipt").read_text(), "retained")
            result = subprocess.run(
                ["bash", "-e", "-o", "pipefail", "-c", preserve["run"]],
                cwd=root, capture_output=True, text=True,
                env=dict(os.environ, RUNNER_TEMP=str(scratch)),
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(set(scratch.glob("pre-sysmgr-artifacts.*")), set(destinations))
            artifacts = root / "artifacts"
            artifacts.mkdir()
            (artifacts / "receipt").write_text("keep on failure")
            result = subprocess.run(
                ["bash", "-e", "-o", "pipefail", "-c", preserve["run"]],
                cwd=root, capture_output=True, text=True,
                env=dict(os.environ, RUNNER_TEMP=str(root / "missing")),
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((artifacts / "receipt").read_text(), "keep on failure")

    def test_sysmgr_evidence_preserves_previous_and_repeated_runs(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        preserve = next(
            step for step in workflow["jobs"]["archive"]["steps"]
            if step.get("name") == "Preserve completed Sysmgr evidence before SWSS layer acceptance"
        )
        with tempfile.TemporaryDirectory(prefix="sysmgr evidence ") as temporary:
            root = Path(temporary)
            scratch = root / "scratch"
            old = scratch / "pre-swss-layer-artifacts"
            old.mkdir(parents=True)
            (old / "receipt").write_text("previous")

            def run(directory=scratch):
                return subprocess.run(
                    ["bash", "-e", "-o", "pipefail", "-c", preserve["run"]],
                    cwd=root, capture_output=True, text=True,
                    env=dict(os.environ, RUNNER_TEMP=str(directory)),
                )

            for value in ("first", "second"):
                artifacts = root / "artifacts"
                artifacts.mkdir()
                (artifacts / "receipt").write_text(value)
                result = run()
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertFalse(artifacts.exists())
            receipts = {p: p.read_text() for p in scratch.rglob("receipt")}
            self.assertCountEqual(receipts.values(), ["previous", "first", "second"])
            self.assertEqual((old / "receipt").read_text(), "previous")
            result = run()
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual({p: p.read_text() for p in scratch.rglob("receipt")}, receipts)
            artifacts.mkdir()
            (artifacts / "receipt").write_text("keep on failure")
            result = run(root / "missing")
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((artifacts / "receipt").read_text(), "keep on failure")

    def test_sflow_evidence_preserves_previous_and_repeated_runs(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        steps = workflow["jobs"]["archive"]["steps"]
        upload = next(step for step in steps if step.get("name") == "Retain sFlow archives and source evidence")
        preserve = next(
            step for step in steps
            if step.get("name") == "Preserve completed sFlow evidence before Orchagent acceptance"
        )
        build = next(step for step in steps if step.get("name") == "Build Orchagent variants from source owners")
        self.assertLess(steps.index(upload), steps.index(preserve))
        self.assertLess(steps.index(preserve), steps.index(build))
        self.assertEqual(preserve["if"], "matrix.architecture == 'amd64'")
        with tempfile.TemporaryDirectory(prefix="sflow evidence ") as temporary:
            root = Path(temporary)
            scratch = root / "scratch"
            old = scratch / "pre-orchagent-artifacts"
            old.mkdir(parents=True)
            (old / "receipt").write_text("previous")

            def run(directory=scratch):
                return subprocess.run(
                    ["bash", "-e", "-o", "pipefail", "-c", preserve["run"]],
                    cwd=root, capture_output=True, text=True,
                    env=dict(os.environ, RUNNER_TEMP=str(directory)),
                )

            destinations = []
            for value in ("first", "second"):
                artifacts = root / "artifacts"
                artifacts.mkdir()
                (artifacts / "receipt").write_text(value)
                result = run()
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertFalse(artifacts.exists())
                candidates = set(scratch.glob("pre-orchagent-artifacts.*")) - set(destinations)
                self.assertEqual(len(candidates), 1)
                destination = candidates.pop()
                self.assertEqual(destination.stat().st_mode & 0o777, 0o700)
                destinations.append(destination)
            receipts = {p: p.read_text() for p in scratch.rglob("receipt")}
            self.assertCountEqual(receipts.values(), ["previous", "first", "second"])
            self.assertEqual((old / "receipt").read_text(), "previous")
            result = run()
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual({p: p.read_text() for p in scratch.rglob("receipt")}, receipts)
            artifacts.mkdir()
            (artifacts / "receipt").write_text("keep on failure")
            result = run(root / "missing")
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((artifacts / "receipt").read_text(), "keep on failure")

    def test_sysmgr_evidence_requires_a_build_attempt_but_retains_failure_evidence(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        steps = workflow["jobs"]["archive"]["steps"]
        build = next(step for step in steps if step.get("name") == "Build Sysmgr runtime and debug images")
        public = next(step for step in steps if step.get("name") == "Prepare reviewed public evidence")
        self.assertEqual(build["id"], "sysmgr_build")
        self.assertEqual(
            public["if"],
            "always() && matrix.architecture == 'amd64' && "
            "(steps.sysmgr_build.outcome == 'success' || steps.sysmgr_build.outcome == 'failure')",
        )

    def test_swss_layer_evidence_preservation_is_unique_and_keeps_previous_runs(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        steps = workflow["jobs"]["archive"]["steps"]
        upload = next(step for step in steps if step.get("name") == "Retain reviewed artifacts and receipts")
        preserve = next(
            step for step in steps
            if step.get("name") == "Preserve completed SWSS layer evidence before sFlow acceptance"
        )
        build = next(step for step in steps if step.get("name") == "Build sFlow runtime and debug images from source owners")
        self.assertLess(steps.index(upload), steps.index(preserve))
        self.assertLess(steps.index(preserve), steps.index(build))
        self.assertEqual(preserve["if"], "matrix.architecture == 'amd64'")
        with tempfile.TemporaryDirectory(prefix="swss layer evidence ") as temporary:
            root = Path(temporary)
            scratch = root / "scratch"
            scratch.mkdir()
            old = scratch / "pre-sflow-artifacts"
            old.mkdir()
            (old / "receipt").write_text("retained")
            destinations = []
            for index in range(2):
                artifacts = root / "artifacts"
                artifacts.mkdir()
                (artifacts / "receipt").write_text(str(index))
                result = subprocess.run(
                    ["bash", "-e", "-o", "pipefail", "-c", preserve["run"]],
                    cwd=root, capture_output=True, text=True,
                    env=dict(os.environ, RUNNER_TEMP=str(scratch)),
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertFalse(artifacts.exists())
                candidates = set(scratch.glob("pre-sflow-artifacts.*")) - set(destinations)
                self.assertEqual(len(candidates), 1)
                destination = candidates.pop()
                self.assertEqual(destination.stat().st_mode & 0o777, 0o700)
                self.assertEqual((destination / "artifacts/receipt").read_text(), str(index))
                destinations.append(destination)
            self.assertNotEqual(*destinations)
            self.assertEqual((old / "receipt").read_text(), "retained")
            artifacts = root / "artifacts"
            artifacts.mkdir()
            (artifacts / "receipt").write_text("keep on failure")
            result = subprocess.run(
                ["bash", "-e", "-o", "pipefail", "-c", preserve["run"]],
                cwd=root, capture_output=True, text=True,
                env=dict(os.environ, RUNNER_TEMP=str(root / "missing")),
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((artifacts / "receipt").read_text(), "keep on failure")

    def test_sflow_evidence_handles_early_failure_and_binds_checkout_identity(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        archive_steps = workflow["jobs"]["archive"]["steps"]
        build = next(step for step in archive_steps if step.get("name") == "Build sFlow runtime and debug images from source owners")
        validate = next(step for step in archive_steps if step.get("name") == "Validate sFlow installed payloads and matching symbols")
        incremental = next(step for step in archive_steps if step.get("name") == "Verify sFlow no-op, Python source edit and restored archives")
        public = next(step for step in archive_steps if step.get("name") == "Review public sFlow evidence")
        self.assertEqual(build["id"], "sflow_build")
        self.assertEqual(
            public["if"],
            "always() && matrix.architecture == 'amd64' && "
            "(steps.sflow_build.outcome == 'success' || steps.sflow_build.outcome == 'failure')",
        )
        for step in (build, validate, incremental):
            self.assertEqual(step["env"]["CI_HEAD_SHA"], "${{ steps.checkout_identity.outputs.head_sha }}")
            self.assertEqual(step["env"]["CI_BASE_SHA"], "${{ steps.checkout_identity.outputs.base_sha }}")
        runtime_steps = workflow["jobs"]["swss_layer_runtime"]["steps"]
        identity = next(step for step in runtime_steps if step.get("name") == "Bind the tested checkout identity")
        retain_swss = next(step for step in runtime_steps if step.get("name") == "Retain installed-image evidence")
        preserve_runtime = next(
            step for step in runtime_steps
            if step.get("name") == "Preserve completed SWSS runtime evidence before sFlow acceptance"
        )
        download_sflow = next(
            step for step in runtime_steps
            if (step.get("with") or {}).get("name") == "sflow-source-amd64"
        )
        loaded = next(step for step in runtime_steps if step.get("name") == "Exercise installed sFlow manager, mapper and exporter variants")
        self.assertEqual(identity["id"], "checkout_identity")
        self.assertLess(runtime_steps.index(identity), runtime_steps.index(loaded))
        self.assertLess(runtime_steps.index(retain_swss), runtime_steps.index(preserve_runtime))
        self.assertLess(runtime_steps.index(preserve_runtime), runtime_steps.index(download_sflow))
        self.assertLess(runtime_steps.index(download_sflow), runtime_steps.index(loaded))
        self.assertEqual(loaded["env"]["CI_HEAD_SHA"], "${{ steps.checkout_identity.outputs.head_sha }}")
        self.assertEqual(loaded["env"]["CI_BASE_SHA"], "${{ steps.checkout_identity.outputs.base_sha }}")
        with tempfile.TemporaryDirectory(prefix="swss runtime evidence ") as temporary:
            root = Path(temporary)
            scratch = root / "scratch"
            scratch.mkdir()
            artifacts = root / "artifacts"
            (artifacts / "swss-layer").mkdir(parents=True)
            (artifacts / "swss-layer/runtime-smoke.json").write_text('{"status": "passed"}\n')
            result = subprocess.run(
                ["bash", "-e", "-o", "pipefail", "-c", preserve_runtime["run"]],
                cwd=root, capture_output=True, text=True,
                env=dict(os.environ, RUNNER_TEMP=str(scratch)),
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertFalse(artifacts.exists())
            candidates = list(scratch.glob("pre-sflow-runtime-artifacts.*"))
            self.assertEqual(len(candidates), 1)
            destination = candidates[0]
            self.assertEqual(destination.stat().st_mode & 0o777, 0o700)
            self.assertEqual(
                (destination / "artifacts/swss-layer/runtime-smoke.json").read_text(),
                '{"status": "passed"}\n',
            )
        script = public["run"].split("python3 - <<'PYCODE'\n", 1)[1].split("\nPYCODE", 1)[0]
        with tempfile.TemporaryDirectory(prefix="sflow early failure ") as temporary:
            directory = Path(temporary)
            previous = Path.cwd()
            sys.path.insert(0, str(ROOT))
            try:
                os.chdir(directory)
                exec(compile(script, "<sflow-public-evidence>", "exec"), {})
            finally:
                os.chdir(previous)
                sys.path.pop(0)
            root = directory / "artifacts/sflow"
            self.assertEqual(json.loads((root / "build-summary.json").read_text()), {})
            self.assertEqual([path.name for path in root.iterdir()], ["build-summary.json"])

    def test_base_image_runtime_uses_guarded_kernel_workspace_repair(self):
        workflow = yaml.safe_load(
            (ROOT / ".github/workflows/bazel-base-images.yml").read_text()
        )
        kernel = yaml.safe_load(
            (ROOT / ".github/workflows/bazel-kernel-bundle.yml").read_text()
        )
        repair = next(
            step for step in kernel["jobs"]["kernel-bundle"]["steps"]
            if step.get("name") == "Restore workspace access before checkout"
        )
        runtime = workflow["jobs"]["runtime"]
        self.assertNotIn("container", runtime)
        self.assertEqual(runtime["steps"][0], repair)
        self.assertTrue(runtime["steps"][1]["uses"].startswith("actions/checkout@"))
        self.assertTrue(
            runtime["steps"][2]["uses"].startswith("actions/download-artifact@")
        )

    def test_eventd_startup_diagnostics_probe_writes_and_preserve_bazel_failure(self):
        workflow = yaml.safe_load(
            (ROOT / ".github/workflows/bazel-eventd.yml").read_text()
        )
        steps = workflow["jobs"]["native"]["steps"]
        diagnostic = next(
            step for step in steps
            if step.get("name") == "Diagnose Bazel startup permissions"
        )
        build = next(
            step for step in steps
            if step.get("name") == "Audit, build and run all original suites with private Redis"
        )
        self.assertLess(steps.index(diagnostic), steps.index(build))
        for status in (0, 36):
            with self.subTest(status=status), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                home = root / "home"
                cache = home / ".cache/bazel"
                cache.mkdir(parents=True)
                retained = cache / "retained-cache"
                retained.write_text("keep")
                wrapper = root / "tools/bazel/ci/libteam_draft_bazel.sh"
                wrapper.parent.mkdir(parents=True)
                wrapper.write_text(
                    "#!/bin/bash\n"
                    'test "$*" = "info output_base" || exit 99\n'
                    'echo "fixture Bazel startup probe"\n'
                    f"exit {status}\n"
                )
                wrapper.chmod(0o755)
                result = subprocess.run(
                    ["bash", "-e", "-o", "pipefail", "-c", diagnostic["run"]],
                    cwd=root, env=dict(os.environ, HOME=str(home), RUNNER_TEMP=str(root)),
                    capture_output=True, text=True,
                )
                self.assertEqual(result.returncode, status, result.stdout + result.stderr)
                self.assertIn("Bazel cache-parent write probe passed.", result.stdout)
                self.assertIn("fixture Bazel startup probe", result.stdout)
                self.assertIn(str(cache), result.stdout)
                self.assertEqual(list(cache.iterdir()), [retained])
                self.assertEqual(retained.read_text(), "keep")

    def test_self_hosted_base_images_do_not_delete_host_sdks(self):
        workflow = yaml.safe_load(
            (ROOT / ".github/workflows/bazel-base-images.yml").read_text()
        )
        for name in ("native", "runtime"):
            with self.subTest(job=name):
                job = workflow["jobs"][name]
                matrix = job["strategy"]["matrix"]
                self.assertEqual(matrix["architecture"], ["amd64", "arm64"])
                self.assertEqual(matrix["selection"], ["${{ inputs.architecture || 'all' }}"])
                self.assertEqual(matrix["exclude"], [{"architecture": "arm64", "selection": "amd64"}])
                self.assertEqual(job["runs-on"], [
                    "self-hosted", "linux",
                    "${{ matrix.architecture == 'amd64' && 'bazel-x64' || 'bazel-arm64' }}",
                ])
        native = workflow["jobs"]["native"]
        self.assertFalse(native["container"].get("volumes"))
        self.assertNotIn(
            "Allocate source-build space on this disposable runner",
            [step.get("name") for step in native["steps"]],
        )
        for step in native["steps"]:
            self.assertNotIn("/runner-unused/", step.get("run", ""))

    def test_archive_container_reaps_child_processes_and_preserves_host_sdks(self):
        workflow = yaml.safe_load(
            (ROOT / ".github/workflows/bazel-oci.yml").read_text()
        )
        archive = workflow["jobs"]["archive"]
        self.assertEqual(archive["runs-on"], "${{ matrix.runner }}")
        self.assertEqual(
            {entry["architecture"]: entry["runner"]
             for entry in archive["strategy"]["matrix"]["include"]},
            {"amd64": ["self-hosted", "linux", "bazel-x64"],
             "arm64": ["self-hosted", "linux", "bazel-arm64"]},
        )
        self.assertFalse(archive["container"].get("volumes"))
        self.assertIn("--init", archive["container"]["options"].split())
        self.assertNotIn(
            "Allocate source-build space on this disposable runner",
            [step.get("name") for step in archive["steps"]],
        )
        for step in archive["steps"]:
            script = step.get("run", "")
            self.assertNotIn('test "$RUNNER_ENVIRONMENT" = github-hosted', script)
            for path in ("/runner-unused/", "/usr/local/lib/android", "/usr/share/dotnet", "/opt/ghc"):
                with self.subTest(step=step.get("name"), path=path):
                    self.assertNotIn(path, script)

    def test_native_package_workflows_use_cache_reachable_runners(self):
        expected = {
            "amd64": "self-hosted, linux, bazel-x64",
            "arm64": "self-hosted, linux, bazel-arm64",
        }
        for name in ("bazel-eventd.yml", "bazel-supervisor-rust.yml", "bazel-yang-models.yml"):
            with self.subTest(workflow=name):
                workflow = (ROOT / ".github/workflows" / name).read_text()
                runners = re.findall(
                    r"^          - (?:architecture|arch): (\S+)\n"
                    r"            runner: \[([^\n]+)\]$",
                    workflow, re.MULTILINE,
                )
                self.assertEqual(len(runners), 2)
                self.assertEqual({arch.lower(): labels for arch, labels in runners}, expected)
                self.assertIn("runs-on: ${{ matrix.runner }}", workflow)

    def test_static_checks_match_make_configs_and_stop_publication_on_failure(self):
        verifier = (ROOT / "tools/bazel/ci/verify_vs_oci.sh").read_text()
        script = "set -euo pipefail\n" + verifier[verifier.index("# Match the archive recipes"):].split(
            "\npython3 -B tools/bazel/ci/verify_p4rt_debug.py", 1)[0]
        with tempfile.TemporaryDirectory(prefix="image configs ") as temporary:
            directory = Path(temporary)
            (directory / "artifacts/vs").mkdir(parents=True)
            for source in ("tools/bazel/build_helpers.py", "tools/bazel/cache/credentials.py"):
                destination = directory / source
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / source, destination)
            calls = directory / "calls.jsonl"
            # A stale convenience link must never supply validation results.
            (directory / "bazel-testlogs").symlink_to(directory / "stale")
            bazel = directory / "bazel"
            bazel.write_text(f"#!{sys.executable}\n" + textwrap.dedent('''\
                import json, os, sys
                from pathlib import Path
                args = sys.argv[1:]
                with Path(os.environ['CALLS']).open('a') as output:
                    output.write(json.dumps(args) + '\\n')
                command = args[0]
                config = 'dash' if '--config=syncd-dash' in args else 'default'
                logs = Path('testlogs') / config
                logs.mkdir(parents=True, exist_ok=True)
                link = Path('bazel-testlogs')
                link.unlink(missing_ok=True)
                link.symlink_to(logs, target_is_directory=True)
                if command == 'build':
                    Path('policy.json').write_text(config)
                if command == 'cquery':
                    print('policy.json')
                if command == 'test':
                    target = next(arg.split(':')[1] for arg in args if arg.startswith('//'))
                    status = int(os.environ['SYNCD_EXIT' if config == 'dash' else 'SWSS_EXIT'])
                    report = logs / 'tools/bazel/tests' / target
                    report.mkdir(parents=True)
                    contents = json.dumps({'target': target, 'config': config, 'exit': status}) + '\\n'
                    for filename in ('test.log', 'test.xml'):
                        (report / filename).write_text(contents)
                    if not status:
                        (report / 'test.outputs').mkdir()
                        (report / 'test.outputs/validation.json').write_text(contents)
                    sys.exit(status)
                '''))
            bazel.chmod(0o755)
            for swss_exit, syncd_exit in ((0, 0), (13, 0), (0, 17), (13, 17)):
                with self.subTest(swss_exit=swss_exit, syncd_exit=syncd_exit):
                    self.check_static_workflow(directory, script, bazel, calls, swss_exit, syncd_exit)

    def check_static_workflow(self, directory, script, bazel, calls, swss_exit, syncd_exit):
        calls.unlink(missing_ok=True)
        for name in ("testlogs", "artifacts/vs/swss-images", "artifacts/vs/syncd-p4-images"):
            shutil.rmtree(directory / name, ignore_errors=True)
        outputs = [directory / "artifacts/vs" / (family + "-installed-images-policy.json")
                   for family in ("swss", "syncd")]
        for output in outputs:
            output.unlink(missing_ok=True)
        result = subprocess.run(["bash", "-c", script], cwd=directory,
                                env=dict(os.environ, BAZEL=str(bazel), CALLS=str(calls),
                                         SWSS_EXIT=str(swss_exit), SYNCD_EXIT=str(syncd_exit)),
                                capture_output=True, text=True)
        failure = syncd_exit or swss_exit
        self.assertEqual(result.returncode, failure, result.stderr)
        arguments = [json.loads(line) for line in calls.read_text().splitlines()]
        target_calls = [args for args in arguments if any(arg.startswith("//") for arg in args)]
        self.assertEqual(len(target_calls), 2 if failure else 6)
        for invocation in target_calls:
            target = next(arg for arg in invocation if arg.startswith("//"))
            self.assertEqual("--config=syncd-dash" in invocation, "syncd" in target)
        for output in outputs:
            self.assertEqual(output.exists(), failure == 0)
        for family, target, config, status in (
                ("swss-images", "swss_images_test", "default", swss_exit),
                ("syncd-p4-images", "syncd_p4_images_test", "dash", syncd_exit)):
            expected = {"target": target, "config": config, "exit": status}
            report = directory / "artifacts/vs" / family
            for filename in ("test.log", "test.xml"):
                self.assertEqual(json.loads((report / filename).read_text()), expected)
            receipt = report / "validation.json"
            self.assertEqual(receipt.is_file(), status == 0)
            if not status:
                self.assertEqual(json.loads(receipt.read_text()), expected)

    def test_private_verification_keeps_build_store_and_prepares_cgroups_first(self):
        workflow = (ROOT / ".github/workflows/bazel-oci.yml").read_text()
        with tempfile.TemporaryDirectory(prefix="swss workflow ") as temporary:
            directory = Path(temporary)
            (directory / "artifacts/vs").mkdir(parents=True)
            commands = directory / "commands"
            commands.mkdir()
            captured = directory / "arguments.json"
            make = commands / "make"
            make.write_text(f"#!{sys.executable}\nimport json, os, sys\n"
                            "from pathlib import Path\n"
                            "Path(os.environ['CAPTURE']).write_text(json.dumps(sys.argv[1:]))\n")
            make.chmod(0o755)
            environment = dict(os.environ, PATH=str(commands) + os.pathsep + os.environ["PATH"],
                               CAPTURE=str(captured), SONIC_CI_CONTAINER_LABEL="test-job=swss")
            block = workflow.split("      - name: Verify installed base, SWSS and Syncd images\n", 1)[1].split(
                "\n      - ", 1)[0]
            script = textwrap.dedent(block.split("        run: |\n", 1)[1])
            subprocess.run(["bash", "-e", "-o", "pipefail", "-c", script],
                           cwd=directory, env=environment, check=True, capture_output=True)
            arguments = json.loads(captured.read_text())
            self.assertIn("sonic-slave-run", arguments)
            common = dict(argument.split("=", 1) for argument in arguments if "=" in argument)
            root = Path(common["DOCKER_ROOT"])
            self.assertEqual(root.parent, directory)
            self.assertNotEqual(root.name, "fsroot.docker.trixie")
            self.assertIn("--cgroupns=private", shlex.split(common["SONIC_BUILDER_EXTRA_CMDLINE"]))
            self.assertEqual(common["SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD"], "n")

            # Execute the actual nested shell chain with recording commands. A
            # failed cgroup setup must prevent both daemon startup and probing.
            calls = directory / "calls"
            record = (f"#!{sys.executable}\nimport os, sys\nfrom pathlib import Path\n"
                      "name = Path(sys.argv[0]).name\n"
                      "stage = 'daemon' if name == 'make' else ('prepare' if any("
                      "arg.endswith('/prepare_nested_docker.py') for arg in sys.argv) else 'probe')\n"
                      "with Path(os.environ['CALLS']).open('a') as output: output.write(stage + '\\n')\n"
                      "sys.exit(int(os.environ['PREPARE_EXIT']) if stage == 'prepare' else 0)\n")
            make.write_text(record)
            python = commands / "python3"
            python.write_text(record)
            python.chmod(0o755)
            sudo = commands / "sudo"
            sudo.write_text('#!/bin/sh\nexec "$@"\n')
            sudo.chmod(0o755)
            for failure, expected in (("0", ["prepare", "daemon", "probe"]), ("23", ["prepare"])):
                calls.unlink(missing_ok=True)
                result = subprocess.run(["bash", "-c", common["SONIC_RUN_CMDS"]], cwd=directory,
                                        env=dict(environment, CALLS=str(calls), PREPARE_EXIT=failure),
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, int(failure), result.stderr)
                self.assertEqual(calls.read_text().splitlines(), expected)

    def test_active_slave_runs_shared_validation_and_retains_both_static_results(self):
        """Use the private daemon and reject failed probes or unresolved runtime libraries."""
        workflow = (ROOT / ".github/workflows/bazel-oci.yml").read_text()
        legacy, active = workflow.split("\n  vs_container:\n", 1)
        self.assertIn("if: ${{ false }}", legacy.split("\n  vs:\n", 1)[1])
        self.assertIn("runs-on: [self-hosted, linux, bazel-x64, vs]", active)
        names = [line.split(": ", 1)[1] for line in active.splitlines()
                 if line.startswith("      - name: ")]
        self.assertLess(names.index("Validate the installer and retained container pairs"),
                        names.index("Verify installed base, SWSS and Syncd images"))
        self.assertLess(names.index("Verify installed base, SWSS and Syncd images"),
                        names.index("Publish the installer and matching container archives"))
        self.assertIn("bash tools/bazel/ci/validate_vs.sh", active)
        installed = active.split("      - name: Verify installed base, SWSS and Syncd images\n", 1)[1].split(
            "\n      - ", 1)[0]
        script = textwrap.dedent(installed.split("        run: |\n", 1)[1])
        self.assertIn("sonic-slave-run", script)
        self.assertIn("SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD=n", script)
        self.assertIn('--cgroupns=private', script)
        with tempfile.TemporaryDirectory(prefix="active-image-validation-") as temporary:
            directory = Path(temporary)
            (directory / "artifacts/vs").mkdir(parents=True)
            make = directory / "make"
            make.write_text(f"#!{sys.executable}\n" + textwrap.dedent('''\
                import os, subprocess, sys
                args = sys.argv[1:]
                if "sonic-slave-run" in args:
                    command = next(arg.split("=", 1)[1] for arg in args
                                   if arg.startswith("SONIC_RUN_CMDS="))
                    environment = dict(os.environ, DOCKER_HOST="unix:///worker-private.sock")
                    sys.exit(subprocess.run(["bash", "-c", command], env=environment).returncode)
                assert "docker-start" in args, args
                '''))
            make.chmod(0o755)
            sudo = directory / "sudo"
            sudo.write_text("#!/bin/sh\nexit 0\n")
            sudo.chmod(0o755)
            python = directory / "python3"
            python.write_text(f"#!{sys.executable}\nimport json, os, sys\nfrom pathlib import Path\n"
                              "Path('docker-call.json').write_text(json.dumps({'args': sys.argv[1:], "
                              "'docker_host': os.environ.get('DOCKER_HOST')}))\n"
                              "sys.exit(int(os.environ['PROBE_EXIT']))\n")
            python.chmod(0o755)
            docker_arguments = [
                "run", "--rm", "--network", "none", "--read-only", "--cap-drop", "ALL",
                "--security-opt", "no-new-privileges", "--env", "LD_BIND_NOW=1",
                "--entrypoint", "/usr/bin/ldd", "docker-orchagent:latest",
                "-r", "/usr/bin/tlm_teamd",
            ]
            docker = directory / "docker"
            docker.write_text(f"#!{sys.executable}\n" + textwrap.dedent('''\
                import json, os, sys
                from pathlib import Path
                args = sys.argv[1:]
                if args != json.loads(os.environ['EXPECTED_DOCKER_ARGUMENTS']):
                    print('Unexpected Docker invocation', file=sys.stderr)
                    sys.exit(125)
                Path('linkage-call.json').write_text(json.dumps({
                    'args': args, 'docker_host': os.environ.get('DOCKER_HOST')}))
                sys.stdout.write(os.environ['LINKAGE_OUTPUT'])
                sys.exit(int(os.environ['DOCKER_EXIT']))
                '''))
            docker.chmod(0o755)
            shell = script
            cases = (
                ("resolved", 0, 0, "libnl-3.so.200 => /usr/lib/libnl-3.so.200\n", 0),
                ("probe failed", 19, 0, "", 19),
                ("docker failed", 0, 23, "Docker launch failed\n", 23),
                ("missing library", 0, 0, "libnl-3.so.200 => not found\n", 1),
                ("unresolved symbol", 0, 0,
                 "undefined symbol: nl_msg_parse, version libnl_3 (/usr/lib/libswsscommon.so.0)\n", 1),
                ("loader failed", 0, 0, "symbol lookup error: /usr/bin/tlm_teamd\n", 1),
            )
            linkage_call = directory / "linkage-call.json"
            linkage_report = directory / "artifacts/vs/tlm-teamd-linkage.txt"
            for name, probe_exit, docker_exit, output, expected_status in cases:
                with self.subTest(name=name):
                    for path in (directory / "docker-call.json", linkage_call, linkage_report):
                        path.unlink(missing_ok=True)
                    result = subprocess.run(["bash", "-e", "-o", "pipefail", "-c", shell], cwd=directory,
                        env=dict(os.environ, PATH=str(directory) + os.pathsep + os.environ["PATH"],
                                 RUNNER_TEMP=str(directory), TEST_WORKSPACE=str(directory),
                                 SONIC_CI_CONTAINER_LABEL="fixture-vs",
                                 PROBE_EXIT=str(probe_exit), DOCKER_EXIT=str(docker_exit),
                                 EXPECTED_DOCKER_ARGUMENTS=json.dumps(docker_arguments), LINKAGE_OUTPUT=output),
                        capture_output=True, text=True)
                    self.assertEqual(result.returncode, expected_status, result.stdout + result.stderr)
                    called = json.loads((directory / "docker-call.json").read_text())
                    self.assertEqual(called["docker_host"], "unix:///worker-private.sock")
                    self.assertEqual(called["args"], ["-B", "tools/bazel/ci/validate_runtime.py",
                        "--images-policy", "artifacts/vs/swss-installed-images-policy.json",
                        "--images-policy", "artifacts/vs/syncd-installed-images-policy.json", "--target", "target",
                        "--output", "artifacts/vs/installed-images.json"])
                    if probe_exit:
                        self.assertFalse(linkage_call.exists())
                        self.assertFalse(linkage_report.exists())
                    else:
                        linkage = json.loads(linkage_call.read_text())
                        self.assertEqual(linkage["args"], docker_arguments)
                        self.assertEqual(linkage["docker_host"], "unix:///worker-private.sock")
                        self.assertEqual(linkage_report.read_text(), output)
        retained = active.split("      - name: Retain shared image validation evidence\n", 1)[1].split(
            "\n      - ", 1)[0]
        self.assertIn("if: always()", retained)
        for path in ("artifacts/vs/swss-images/", "artifacts/vs/syncd-p4-images/",
                     "artifacts/vs/*-installed-images-policy.json", "artifacts/vs/installed-images.json"):
            self.assertIn(path, retained)


if __name__ == "__main__":
    unittest.main()
