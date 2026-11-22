"""Run the real image-verification workflow shell against recording commands."""

import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest

import yaml

from tools.bazel.ci.vs_probe_policy import FAMILIES
from tools.bazel.ci.vs_standalone_probes import STANDALONE_IMAGES

ROOT = Path(__file__).resolve().parents[3]


class VerificationWorkflowTest(unittest.TestCase):
    def test_container_matrix_uses_only_artifacts_from_the_current_installer(self):
        """Every selected image gets its own runner and downloads the archive for that matrix row."""
        jobs = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]
        producer, validation = jobs["vs_container"], jobs["validate_container"]
        self.assertEqual(validation["strategy"]["matrix"], "${{ fromJSON(needs.vs_container.outputs.containers) }}")
        self.assertFalse(validation["strategy"]["fail-fast"])
        self.assertEqual(validation["runs-on"], ["self-hosted", "linux", "bazel-x64"])
        self.assertEqual(set(validation["needs"]), {"vs_container", "installer_validation"})
        self.assertEqual(producer["outputs"]["containers"], "${{ steps.bundles.outputs.matrix }}")
        uploads = {}
        for step in producer["steps"]:
            artifact = step.get("with", {}).get("name", "")
            if artifact.startswith("${{ steps.bundles.outputs.prefix }}-"):
                image = artifact.split("}}-", 1)[1]
                self.assertNotIn(image, uploads)
                self.assertEqual(step["if"], f"contains(fromJSON(steps.bundles.outputs.matrix).include.*.image, '{image}')")
                self.assertTrue(step["with"]["path"].rstrip("/").endswith("/containers/" + image))
                self.assertEqual(step["with"]["if-no-files-found"], "error")
                uploads[image] = artifact
        # The publisher has one conditional upload per service family plus both parents.
        self.assertEqual(set(uploads), set(FAMILIES) | STANDALONE_IMAGES)
        download, = [step["with"] for step in validation["steps"] if "download-artifact@" in step.get("uses", "")]
        self.assertEqual(download["name"], "${{ needs.vs_container.outputs.artifact_prefix }}-${{ matrix.image }}")
        self.assertNotIn("run-id", download)
        probe, = [step for step in validation["steps"] if "vs_container_probe.py" in step.get("run", "")]
        self.assertEqual(probe["env"]["BUILD_VERSION"], "${{ needs.installer_validation.outputs.build_version }}")
        self.assertEqual(probe["env"]["DEBUG_LAYER"], "${{ needs.vs_container.outputs.debug_layer }}")
        self.assertIn('--debug-layer "$DEBUG_LAYER"', probe["run"])
        self.assertIn('--build-version "$BUILD_VERSION"', probe["run"])
        self.assertIn('--revision "$GITHUB_SHA"', probe["run"])
        for job in (validation, jobs["installer_validation"]):
            for step in job["steps"]:
                self.assertNotIn("setup-bazel@", step.get("uses", ""))
                self.assertNotRegex(step.get("run", ""), r"(?m)^\s*(?:make|bazel(?:isk)?)\s")

    def test_container_artifacts_are_isolated_on_reused_runners_and_removed_after_upload(self):
        """Two jobs sharing RUNNER_TEMP must never read, upload or remove another family's files."""
        jobs = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]
        steps = jobs["validate_container"]["steps"]
        allocate, = [step for step in steps if step.get("id") == "bundle"]
        download, = [step for step in steps if "download-artifact@" in step.get("uses", "")]
        probe, = [step for step in steps if "vs_container_probe.py" in step.get("run", "")]
        publish, = [step for step in steps if "upload-artifact@" in step.get("uses", "")]
        cleanup = steps[-1]
        selected = "${{ steps.bundle.outputs.directory }}"
        self.assertEqual(download["with"]["path"], selected)
        self.assertEqual(probe["env"]["BUNDLE_DIR"], selected)
        self.assertIn('--artifacts "$BUNDLE_DIR"', probe["run"])
        self.assertEqual(publish["with"]["path"].splitlines(),
                         [selected + "/vs-validation.json", selected + "/*.runtime.json"])
        self.assertEqual(cleanup["env"]["BUNDLE_DIR"], selected)
        self.assertEqual(cleanup["if"], "always() && steps.bundle.outcome == 'success'")
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "output"
            env = dict(os.environ, RUNNER_TEMP=temporary, GITHUB_OUTPUT=str(output))
            def run_allocation():
                subprocess.run(["bash", "-eu", "-c", allocate["run"]], env=env, check=True)
                return Path(output.read_text().splitlines()[-1].removeprefix("directory="))
            previous = run_allocation()
            (previous / "runtime.make.image.env").write_text("IMAGE_VERSION=previous-job\n")
            current = run_allocation()
            self.assertEqual(list(current.iterdir()), [])
            subprocess.run(["bash", "-eu", "-c", cleanup["run"]], env=env | {"BUNDLE_DIR": str(current)}, check=True)
            self.assertFalse(current.exists())
            self.assertTrue((previous / "runtime.make.image.env").is_file())

    def test_installer_validation_reads_the_published_binary_before_container_checks(self):
        """Get the release from the shipped installer so a stale container bundle cannot choose its own version."""
        jobs = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]
        producer, inspection = jobs["vs_container"], jobs["installer_validation"]
        self.assertEqual(inspection["needs"], "vs_container")
        upload, = [step for step in producer["steps"] if step.get("id") == "vs_installer_upload"]
        fetch, = [step for step in inspection["steps"] if "download-artifact@" in step.get("uses", "")]
        self.assertEqual(upload["with"]["name"], "sonic-vs-slave-amd64")
        self.assertEqual(producer["outputs"]["installer_artifact_id"], "${{ steps.vs_installer_upload.outputs.artifact-id }}")
        self.assertEqual(fetch["with"]["artifact-ids"], "${{ needs.vs_container.outputs.installer_artifact_id }}")
        self.assertTrue(fetch["with"]["merge-multiple"])
        self.assertIn("/target/sonic-vs.bin", upload["with"]["path"].splitlines()[0])
        validate, = [step for step in inspection["steps"] if step.get("id") == "installer"]
        self.assertLess(inspection["steps"].index(fetch), inspection["steps"].index(validate))
        self.assertIn("sha256sum --check", validate["run"])
        self.assertIn("--expected-commit-id", validate["run"])
        self.assertIn("verify_vs_selection", validate["run"])
        self.assertEqual(inspection["outputs"]["build_version"], "${{ steps.installer.outputs.build_version }}")
        producer_commands = "\n".join(step.get("run", "") for step in producer["steps"])
        for tool in ("validate_vs.sh", "verify_vs_selection", "vs_container_probe.py"):
            self.assertNotIn(tool, producer_commands)

    def test_amd64_package_checks_consume_the_vs_uploads(self):
        """A rerun must inspect the producer's original wheels and kernel packages for this run."""
        jobs = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]
        vs = jobs["vs_container"]
        outputs = vs["outputs"]
        for name, artifact, upload in (
            ("kernel_packages_amd64", "kernel_artifact_id", "vs_kernel_upload"),
            ("frr_python_amd64", "python_packages_artifact_id", "vs_python_packages_upload"),
        ):
            job = jobs[name]
            self.assertEqual(job["needs"], "vs_container")
            self.assertEqual(job["with"]["vs_artifact_id"], "${{ needs.vs_container.outputs." + artifact + " }}")
            self.assertEqual(outputs[artifact], "${{ steps." + upload + ".outputs.artifact-id }}")
            publish, = (s for s in vs["steps"] if s.get("id") == upload)
            self.assertEqual(publish["with"]["if-no-files-found"], "error")

        amd64 = jobs["package_amd64"]
        self.assertIn("vs_container", amd64["needs"])
        for name, required in (("archive", "Container archive (ARM64)"), ("package_amd64", "Container package tests (AMD64)")):
            job = jobs[name]
            row, = job["strategy"]["matrix"]["include"]
            self.assertIn("${{ matrix.name }}", job["name"])  # A literal name makes Actions append every matrix value.
            self.assertEqual(job["name"].replace("${{ matrix.name }}", row["name"]), required)
        arm64 = jobs["archive"]
        self.assertIn("vs_container", arm64["needs"])
        self.assertEqual(arm64["env"]["VS_WHEELS_ARTIFACT_ID"], amd64["env"]["VS_WHEELS_ARTIFACT_ID"])
        steps = {s["name"]: s for s in amd64["steps"]}
        download = steps["Download wheels used by VS"]["with"]
        self.assertEqual(download["artifact-ids"], "${{ env.VS_WHEELS_ARTIFACT_ID }}")
        self.assertEqual(amd64["env"]["VS_WHEELS_ARTIFACT_ID"], jobs["frr_python_amd64"]["with"]["vs_artifact_id"])
        installed = steps["Test Python packages and check startup generation"]["run"]
        self.assertIn('vs_python_packages stage', installed)
        self.assertIn('"$BAZEL" test "${wheel_flags[@]}"', installed)
        self.assertIn('--vs-packages "$VS_WHEELS"', steps["Retain wheels and required Python test evidence"]["run"])

    def test_dhcp_runtime_waits_for_the_amd64_incremental_test_artifact(self):
        """The ARM64 archive job cannot publish the images for the installed DHCP relay probe."""
        jobs = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]
        runtime = jobs["dhcp_relay_runtime"]
        self.assertEqual(set(runtime["needs"]), {"package_amd64", "dhcp_relay_source_image"})
        self.assertIn("needs.package_amd64.result == 'success'", runtime["if"])
        download, = (step["with"]["name"] for step in runtime["steps"]
                     if step.get("uses", "").startswith("actions/download-artifact@"))
        for producer in runtime["needs"]:
            upload, = (step for step in jobs[producer]["steps"] if step.get("id") == "bazel_upload_dhcp_relay_public")
            self.assertEqual(upload["with"]["name"], download)

    def test_required_archive_status_propagates_each_validation_failure(self):
        """A bad installer, an incomplete production job or any failed image row must block acceptance."""
        job = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]["archive_acceptance"]
        self.assertEqual(job["name"], "Container archive (AMD64)")
        self.assertEqual(set(job["needs"]), {
            "archive", "package_amd64", "kernel_packages_amd64", "frr_python_amd64",
            "vs_container", "installer_validation", "validate_container", "validate_component",
        })
        gate, = job["steps"]
        self.assertEqual(set(gate["env"].values()), {"${{ needs." + need + ".result }}" for need in job["needs"]})
        success = dict.fromkeys(gate["env"], "success")
        self.assertEqual(subprocess.run(["bash", "-eu", "-c", gate["run"]], env=success).returncode, 0)
        for dependency in success:
            for outcome in ("failure", "skipped", "cancelled"):
                with self.subTest(dependency=dependency, outcome=outcome):
                    env = dict(success, **{dependency: outcome})
                    self.assertNotEqual(subprocess.run(["bash", "-eu", "-c", gate["run"]], env=env).returncode, 0)

    def test_runtime_pruning_commands_stop_on_errors_without_using_a_real_daemon(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        cleanup = next(step for step in workflow["jobs"]["vs_container"]["steps"]
                       if step.get("name") == "Reclaim unused images and build cache")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            docker = root / "docker"
            docker.write_text("#!" + sys.executable + "\n" + textwrap.dedent("""
                import json, os, sys
                with open(os.environ["CALLS"], "a") as out:
                    out.write(json.dumps(sys.argv[1:]) + "\\n")
                if sys.argv[1:] == os.environ.get("FAIL_COMMAND", "").split():
                    sys.exit(42)
            """))
            docker.chmod(0o755)
            calls = root / "calls"
            commands = [["system", "df"], ["image", "prune", "--all", "--force"],
                        ["builder", "prune", "--all", "--force"], ["system", "df"]]
            for failure in ("", "system df", "image prune --all --force",
                            "builder prune --all --force"):
                with self.subTest(failure=failure):
                    calls.unlink(missing_ok=True)
                    environment = dict(os.environ, PATH=str(root) + os.pathsep + os.environ["PATH"],
                                       CALLS=str(calls), FAIL_COMMAND=failure)
                    result = subprocess.run(["bash", "-c", cleanup["run"]], env=environment,
                                            capture_output=True, text=True)
                    self.assertEqual(result.returncode, 42 if failure else 0, result.stderr)
                    expected = commands[:commands.index(failure.split()) + 1] if failure else commands
                    self.assertEqual([json.loads(line) for line in calls.read_text().splitlines()],
                                     expected)

    def test_image_jobs_restore_populated_submodules_on_reused_checkouts(self):
        script = Path("tools/bazel/ci/prepare_image_submodules.sh")
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        self.assertIn(f"bash {script}", [step.get("run") for step in workflow["jobs"]["archive"]["steps"]])
        matrix_steps = workflow["jobs"]["validate_container"]["steps"]
        restore = next(i for i, step in enumerate(matrix_steps) if step.get("run") == f"bash {script} --populated-only")
        self.assertTrue(matrix_steps[restore - 1].get("uses", "").startswith("actions/checkout@"))
        self.assertNotIn("if", matrix_steps[restore])
        self.assertEqual([step["if"] for step in matrix_steps[restore + 1:]
                          if "git submodule update" in step.get("run", "")],
                         ["matrix.image == 'docker-sonic-p4rt'", "matrix.image == 'docker-sysmgr'"])
        python_frr = yaml.safe_load((ROOT / ".github/workflows/bazel-frr-python.yml").read_text())["jobs"]["packages"]
        steps = python_frr["steps"]
        register = next(i for i, step in enumerate(steps) if step.get("id") == "runner_cleanup")
        restore_python_frr = steps[register + 1]["run"]

        frr = yaml.safe_load((ROOT / ".github/workflows/bazel-frr.yml").read_text())["jobs"]["native"]
        self.assertIn(f"bash {script} --populated-only", [step.get("run") for step in frr["steps"]])

        env = dict(os.environ, GIT_ALLOW_PROTOCOL="file")

        def git(repo, *args):
            return subprocess.check_output(
                ["git", "-c", "user.name=Fixture", "-c", "user.email=ci@example.org", *args],
                cwd=repo, env=env, text=True, stderr=subprocess.PIPE,
            ).strip()

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, parent, checkout = (root / name for name in ("source", "parent", "checkout"))
            git(root, "init", "-q", str(source))
            git(root, "init", "-q", str(parent))
            helper = parent / script
            helper.parent.mkdir(parents=True)
            shutil.copyfile(ROOT / script, helper)
            git(parent, "add", str(script))
            (source / "input").write_text("old\n")
            git(source, "add", "input")
            git(source, "commit", "-qm", "old")
            old = git(source, "rev-parse", "HEAD")
            (source / "input").write_text("pinned\n")
            git(source, "commit", "-qam", "pinned")
            pinned = git(source, "rev-parse", "HEAD")

            # Pin a nested copy of the original repository before the top-level
            # source gains a submodule of its own.
            nested = root / "nested source"
            git(root, "clone", "-q", str(source), str(nested))
            git(source, "submodule", "add", "-q", str(nested), "nested module")
            git(source, "commit", "-qm", "pin nested module")
            nested_pin = pinned
            pinned = git(source, "rev-parse", "HEAD")

            gnoi, pins, unused = "src/sonic-sysmgr/gnoi", "src/sonic-p4rt/sonic-pins", "unused module"
            kernel = "src/sonic-linux-kernel"
            for path in (gnoi, pins, unused, kernel):
                git(parent, "submodule", "add", "-q", str(source), path)
            git(parent, "commit", "-qm", "pin submodules")
            git(root, "clone", "-q", str(parent), str(checkout))
            git(checkout, "submodule", "init", "--", unused)

            def prepare(*args, check=True, protocol="file"):
                return subprocess.run(["bash", ROOT / script, *args], cwd=checkout,
                                      env=dict(env, GITHUB_WORKSPACE=str(checkout), GIT_ALLOW_PROTOCOL=protocol),
                                      capture_output=True, text=True, check=check)

            # A fresh artifact validator needs no unrelated clones, even with all transports disabled.
            prepare("--populated-only", protocol="")
            for submodule in (kernel, pins, gnoi, unused):
                self.assertFalse((checkout / submodule / ".git").exists())
            prepare("--kernel-only", protocol="")
            self.assertFalse((checkout / kernel / ".git").exists())
            self.assertFalse((checkout / gnoi / ".git").exists())
            git(checkout, "submodule", "update", "--init", "--", kernel)
            prepare("--kernel-only", protocol="")
            retained_kernel = checkout / kernel
            git(retained_kernel, "checkout", "-q", old)
            git(checkout, "config", "--remove-section", f"submodule.{kernel}")
            (retained_kernel / "new.txt").write_text("keep untracked work\n")
            self.assertNotEqual(prepare("--kernel-only", check=False).returncode, 0)
            self.assertEqual((retained_kernel / "new.txt").read_text(), "keep untracked work\n")
            (retained_kernel / "new.txt").unlink()
            (retained_kernel / "input").write_text("keep local edits\n")
            git(retained_kernel, "add", "input")
            self.assertNotEqual(prepare("--kernel-only", check=False).returncode, 0)
            self.assertEqual(git(retained_kernel, "show", ":input"), "keep local edits")
            self.assertEqual(git(retained_kernel, "rev-parse", "HEAD"), old)
            git(retained_kernel, "restore", "--staged", "--worktree", "input")
            git(checkout, "add", kernel)
            self.assertNotEqual(prepare("--kernel-only", check=False).returncode, 0)
            self.assertEqual(git(checkout, "rev-parse", f":{kernel}"), old)
            git(checkout, "restore", "--staged", kernel)
            prepare("--kernel-only", protocol="")
            self.assertEqual(git(retained_kernel, "rev-parse", "HEAD"), pinned)
            self.assertFalse((checkout / gnoi / ".git").exists())

            # A newly pinned revision may only be available from the kernel remote.
            (source / "input").write_text("new kernel\n")
            git(source, "commit", "-qam", "next kernel")
            next_kernel = git(source, "rev-parse", "HEAD")
            git(checkout, "update-index", "--cacheinfo", "160000", next_kernel, kernel)
            git(checkout, "commit", "-qm", "pin new kernel")
            prepare("--kernel-only")
            self.assertEqual(git(retained_kernel, "rev-parse", "HEAD"), next_kernel)

            # Run the real FRR Python workflow step on a stale retained PINS worktree.
            git(checkout, "submodule", "update", "--init", "--", pins)
            git(checkout / pins, "checkout", "-q", old)
            git(checkout, "config", "--remove-section", f"submodule.{pins}")
            subprocess.run(["bash", "-e", "-o", "pipefail", "-c", restore_python_frr],
                           cwd=checkout, env=dict(env, GITHUB_WORKSPACE=str(checkout), GIT_ALLOW_PROTOCOL=""),
                           check=True, capture_output=True, text=True)
            self.assertEqual(git(checkout / pins, "rev-parse", "HEAD"), pinned)
            self.assertEqual(git(checkout, "diff", "--ignore-submodules=none", "HEAD"), "")
            self.assertFalse((checkout / gnoi / ".git").exists())
            self.assertFalse((checkout / unused / ".git").exists())
            git(checkout, "submodule", "deinit", "-f", "--", pins)

            prepare()
            self.assertEqual(git(checkout / gnoi, "rev-parse", "HEAD"), pinned)
            self.assertFalse((checkout / pins / ".git").exists())
            self.assertFalse((checkout / unused / ".git").exists())

            git(checkout, "submodule", "update", "--init", "--", pins)
            git(checkout / pins, "checkout", "-q", old)
            git(checkout, "reset", "--hard", "HEAD")
            self.assertIn(pins, git(checkout, "diff", "--name-only", "--ignore-submodules=none", "HEAD"))
            # A populated worktree can also lose its local registration.
            git(checkout, "config", "--remove-section", f"submodule.{pins}")
            prepare("--populated-only", protocol="")
            self.assertEqual(git(checkout / pins, "rev-parse", "HEAD"), pinned)
            self.assertEqual(git(checkout, "diff", "--ignore-submodules=none", "HEAD"), "")
            self.assertFalse((checkout / unused / ".git").exists())

            git(checkout / pins, "submodule", "update", "--init", "--", "nested module")
            retained = checkout / pins / "nested module"
            git(retained, "checkout", "-q", old)
            git(checkout / pins, "config", "--remove-section", "submodule.nested module")
            prepare()
            self.assertEqual(git(retained, "rev-parse", "HEAD"), nested_pin)
            self.assertEqual(git(checkout, "diff", "--ignore-submodules=none", "HEAD"), "")

            # Already-pinned local edits remain visible to the strict build check.
            (checkout / gnoi / "input").write_text("local edit\n")
            prepare()
            self.assertIn(gnoi, git(checkout, "diff", "--name-only", "--ignore-submodules=none", "HEAD"))
            git(checkout / pins, "checkout", "-q", old)
            (checkout / pins / "input").write_text("conflicting edit\n")
            self.assertNotEqual(prepare(check=False).returncode, 0)
            self.assertEqual((checkout / pins / "input").read_text(), "conflicting edit\n")

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

    def test_archive_container_reaps_child_processes_and_preserves_host_sdks(self):
        workflow = yaml.safe_load(
            (ROOT / ".github/workflows/bazel-oci.yml").read_text()
        )
        archive = workflow["jobs"]["archive"]
        self.assertEqual(archive["runs-on"], "${{ matrix.runner }}")
        self.assertEqual(
            {entry["architecture"]: entry["runner"]
             for name in ("archive", "package_amd64")
             for entry in workflow["jobs"][name]["strategy"]["matrix"]["include"]},
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
        # This excerpt starts after the real verifier has queried slave.mk.
        make_config = "declare -A config=([SONIC_IMAGE_VERSION]=release-from-make)\nruntime_only=false\n"
        script = "set -euo pipefail\n" + make_config + verifier[verifier.index("# Match the archive recipes"):].split(
            "\npython3 -B tools/bazel/ci/verify_p4rt_debug.py", 1)[0]
        with tempfile.TemporaryDirectory(prefix="image configs ") as temporary:
            directory = Path(temporary)
            (directory / "artifacts/vs").mkdir(parents=True)
            for source in ("tools/bazel/build_helpers.py", "tools/bazel/cache/credentials.py",
                           "tools/bazel/image/file_copy.py"):
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
            self.assertIn("--//tools/bazel/oci:image_version=release-from-make", invocation)
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


if __name__ == "__main__":
    unittest.main()
