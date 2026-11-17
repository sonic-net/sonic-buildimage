"""Exercise the native sandbox probe without Bazel downloads or namespaces."""

from __future__ import annotations

import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap
import unittest
from contextlib import redirect_stderr
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[3]
HELPER = ROOT / "tools/bazel/ci/check_kernel_sandbox.py"
PINNED_VERSION = (ROOT / "tools/bazel/kernel/.bazelversion").read_text().strip()
sys.path.insert(0, str(ROOT))
from tools.bazel.tests.source_workflow_test import jobs, shell, steps


def load_probe():
    spec = importlib.util.spec_from_file_location("kernel_sandbox_probe", HELPER)
    probe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(probe)
    return probe


class KernelSandboxReadinessTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.events = self.root / "events.jsonl"
        self.install = self.root / "install"
        self.install.mkdir()
        self.cache = self.root / "persistent-cache"
        self.cache.mkdir()
        self.sentinel = self.cache / "existing-cache-content"
        self.sentinel.write_text("keep this existing cache\n")
        self.bazel = self.root / "configured bazel"
        self.write_executable(self.bazel, """
            import json, os, pathlib, sys
            args = sys.argv[1:]
            event = {
                "kind": "bazel", "argv": args, "cwd": os.getcwd(),
                "uid": os.getuid(), "gid": os.getgid(),
                "version": os.environ.get("USE_BAZEL_VERSION"),
                "workspace_exists": pathlib.Path("WORKSPACE").is_file(),
                "workspace_contents": pathlib.Path("WORKSPACE").read_text()
                    if pathlib.Path("WORKSPACE").is_file() else None,
            }
            with open(os.environ["PROBE_EVENTS"], "a") as out:
                out.write(json.dumps(event) + "\\n")
            if args == ["--version"]:
                print(os.environ["PROBE_VERSION"])
            elif "info" in args and args[-1] == "install_base":
                if os.environ.get("PROBE_INFO_STATUS"):
                    print("controlled install-base lookup failure", file=sys.stderr)
                    sys.exit(int(os.environ["PROBE_INFO_STATUS"]))
                print(os.environ.get("PROBE_INSTALL_OUTPUT", os.environ["PROBE_INSTALL"]))
            else:
                sys.exit("Unexpected Bazel command")
        """)
        self.write_executable(self.install / "linux-sandbox", """
            import json, os, sys
            with open(os.environ["PROBE_EVENTS"], "a") as out:
                out.write(json.dumps({
                    "kind": "sandbox", "argv": sys.argv[1:],
                    "uid": os.getuid(), "gid": os.getgid(),
                    "euid": os.geteuid(), "egid": os.getegid(),
                    "cwd": os.getcwd(),
                }) + "\\n")
            status = int(os.environ.get("PROBE_SANDBOX_STATUS", "0"))
            if status:
                print("controlled sandbox failure", file=sys.stderr)
            sys.exit(status)
        """)
        diagnostic_commands = self.root / "diagnostic-commands"
        diagnostic_commands.mkdir()
        self.write_executable(diagnostic_commands / "unshare", """
            import json, os, sys
            with open(os.environ["PROBE_EVENTS"], "a") as out:
                out.write(json.dumps({
                    "kind": "unshare", "argv": sys.argv[1:],
                    "uid": os.getuid(), "gid": os.getgid(),
                }) + "\\n")
            status = int(os.environ.get("PROBE_UNSHARE_STATUS", "0"))
            if status:
                print("controlled user-namespace diagnostic failure", file=sys.stderr)
            else:
                print("fixture-userns-profile (enforce)")
            sys.exit(status)
        """)
        self.env = {
            "PATH": str(diagnostic_commands) + os.pathsep + os.defpath,
            "HOME": str(self.root),
            "BAZEL": str(self.bazel),
            "PROBE_EVENTS": str(self.events),
            "PROBE_INSTALL": str(self.install),
            "PROBE_VERSION": f"bazel {PINNED_VERSION}",
            "BAZEL_CONTAINER_CACHE_DIR": str(self.cache),
            "BAZEL_DISK_CACHE": str(self.cache),
            "BAZEL_REPOSITORY_CACHE": str(self.cache),
            "USE_BAZEL_VERSION": "unexpected-inherited-version",
            "UNRELATED_SECRET": "this-value-must-not-be-diagnosed",
        }

    def write_executable(self, path: Path, source: str):
        path.write_text(f"#!{sys.executable}\n" + textwrap.dedent(source).lstrip())
        path.chmod(0o755)

    def run_probe(self, **changes):
        if os.getuid() == 0 or os.geteuid() == 0:
            self.skipTest("Executable identity probe requires an unprivileged test process")
        return subprocess.run(
            [sys.executable, str(HELPER)], cwd=self.root,
            env={**self.env, **changes}, text=True, capture_output=True,
            timeout=15,
        )

    def recorded(self):
        if not self.events.exists():
            return []
        return [json.loads(line) for line in self.events.read_text().splitlines()]

    def assert_cache_untouched(self):
        self.assertEqual(self.sentinel.read_text(), "keep this existing cache\n")
        self.assertEqual(list(self.cache.iterdir()), [self.sentinel])

    def test_probe_uses_pinned_bazel_isolated_workspace_and_current_identity(self):
        result = self.run_probe()
        self.assertEqual(result.returncode, 0, result.stderr)
        version, info, sandbox = self.recorded()
        self.assertEqual(version["argv"], ["--version"])
        self.assertEqual(version["version"], PINNED_VERSION)
        self.assertEqual(info["version"], PINNED_VERSION)
        self.assertEqual(info["kind"], "bazel")
        self.assertEqual(info["argv"][-4:], [
            "info", "--enable_bzlmod=false", "--enable_workspace=true", "install_base",
        ])
        self.assertEqual(info["argv"][:4], [
            "--batch", "--nosystem_rc", "--nohome_rc", "--noworkspace_rc",
        ])
        output_options = [arg for arg in info["argv"] if arg.startswith("--output_user_root=")]
        self.assertEqual(len(output_options), 1)
        output_root = Path(output_options[0].split("=", 1)[1])
        probe_directory = Path(info["cwd"])
        self.assertTrue(output_root.is_relative_to(probe_directory))
        self.assertNotEqual(probe_directory, self.root)
        self.assertTrue(info["workspace_exists"])
        self.assertEqual(info["workspace_contents"], "")
        self.assertEqual(sandbox["kind"], "sandbox")
        self.assertEqual(sandbox["argv"], ["-W", str(probe_directory), "--", "/bin/true"])
        self.assertEqual(sandbox["uid"], os.getuid())
        self.assertEqual(sandbox["euid"], os.geteuid())
        self.assertEqual(sandbox["gid"], os.getgid())
        self.assertEqual(sandbox["egid"], os.getegid())
        self.assertFalse(probe_directory.exists(), "The private probe directory must be removed")
        self.assert_cache_untouched()

    def test_sandbox_failure_reports_allowlisted_diagnostics_and_fails(self):
        result = self.run_probe(PROBE_SANDBOX_STATUS="23")
        self.assertEqual(result.returncode, 1)
        self.assertIn("controlled sandbox failure", result.stderr)
        for label in ("uid", "gid", "AppArmor", "CapEff", "NoNewPrivs", "Seccomp"):
            self.assertIn(label.lower(), result.stderr.lower())
        self.assertNotIn(self.env["UNRELATED_SECRET"], result.stderr)
        self.assertNotIn("UNRELATED_SECRET", result.stderr)
        self.assertIn("linux-sandbox failed (23)", result.stderr)
        self.assertIn("fixture-userns-profile (enforce)", result.stderr)
        self.assertEqual([event["kind"] for event in self.recorded()], ["bazel", "bazel", "sandbox", "unshare"])
        diagnostic = self.recorded()[-1]
        self.assertEqual(diagnostic["argv"], ["--user", "--", "/bin/cat", "/proc/self/attr/current"])
        self.assertEqual(diagnostic["uid"], os.getuid())
        self.assertEqual(diagnostic["gid"], os.getgid())
        self.assertFalse(Path(self.recorded()[1]["cwd"]).exists())
        self.assert_cache_untouched()

    def test_failed_user_namespace_diagnostic_preserves_sandbox_failure(self):
        result = self.run_probe(PROBE_SANDBOX_STATUS="23", PROBE_UNSHARE_STATUS="19")
        self.assertEqual(result.returncode, 1)
        self.assertIn("linux-sandbox failed (23)", result.stderr)
        self.assertIn("controlled sandbox failure", result.stderr)
        self.assertIn("controlled user-namespace diagnostic failure", result.stderr)
        self.assertEqual([event["kind"] for event in self.recorded()], ["bazel", "bazel", "sandbox", "unshare"])
        self.assert_cache_untouched()

    def test_wrong_actual_bazel_version_stops_before_install_or_sandbox(self):
        result = self.run_probe(PROBE_VERSION="bazel 0.0.0")
        self.assertEqual(result.returncode, 1)
        self.assertIn(PINNED_VERSION, result.stderr)
        self.assertEqual(len(self.recorded()), 1)
        self.assert_cache_untouched()

    def test_invalid_install_base_stops_before_sandbox(self):
        for install in ("", "relative/install", str(self.root / "missing"), str(self.install) + "\nextra-line"):
            with self.subTest(install=install):
                self.events.unlink(missing_ok=True)
                result = self.run_probe(PROBE_INSTALL_OUTPUT=install)
                self.assertEqual(result.returncode, 1, result.stdout)
                self.assertEqual([event["kind"] for event in self.recorded()], ["bazel", "bazel"])
                self.assert_cache_untouched()

    def test_failed_install_base_query_stops_before_sandbox(self):
        result = self.run_probe(PROBE_INFO_STATUS="17")
        self.assertEqual(result.returncode, 1)
        self.assertIn("controlled install-base lookup failure", result.stderr)
        self.assertEqual([event["kind"] for event in self.recorded()], ["bazel", "bazel"])
        self.assert_cache_untouched()

    def test_missing_or_nonexecutable_sandbox_stops_before_invocation(self):
        sandbox = self.install / "linux-sandbox"
        sandbox.chmod(0o644)
        result = self.run_probe()
        self.assertEqual(result.returncode, 1)
        self.assertIn("linux-sandbox", result.stderr)
        self.assertEqual(len(self.recorded()), 2)
        self.events.unlink()
        sandbox.unlink()
        result = self.run_probe()
        self.assertEqual(result.returncode, 1)
        self.assertIn("linux-sandbox", result.stderr)
        self.assertEqual(len(self.recorded()), 2)
        self.assert_cache_untouched()

    def test_root_rejected_before_any_command(self):
        probe = load_probe()
        for real_uid, effective_uid in ((0, 1000), (1000, 0), (0, 0)):
            with self.subTest(real_uid=real_uid, effective_uid=effective_uid):
                with patch.object(probe.os, "getuid", return_value=real_uid), \
                     patch.object(probe.os, "geteuid", return_value=effective_uid), \
                     patch.object(probe.subprocess, "run") as run:
                    with self.assertRaisesRegex(RuntimeError, "(?i)root|unprivileged|non-root"):
                        probe.check_sandbox(str(self.bazel))
                    run.assert_not_called()
        self.assertEqual(self.recorded(), [])
        self.assert_cache_untouched()

    def test_invalid_pinned_version_rejected_before_any_command(self):
        probe = load_probe()
        version_file = self.root / "invalid.bazelversion"
        version_file.write_text("latest\n")
        with patch.object(probe, "VERSION_FILE", version_file), \
             patch.object(probe.os, "getuid", return_value=1000), \
             patch.object(probe.os, "geteuid", return_value=1000), \
             patch.object(probe.subprocess, "run") as run:
            with self.assertRaisesRegex(RuntimeError, "(?i)invalid.*version"):
                probe.check_sandbox(str(self.bazel))
            run.assert_not_called()

    def test_diagnostics_reads_only_explicit_security_fields(self):
        probe = load_probe()
        allowed_files = {
            "/proc/self/attr/current": "fixture-profile (enforce)\n",
            "/proc/sys/kernel/unprivileged_userns_clone": "1\n",
            "/proc/sys/kernel/apparmor_restrict_unprivileged_userns": "1\n",
            "/proc/sys/kernel/apparmor_restrict_unprivileged_unconfined": "1\n",
            "/proc/sys/user/max_user_namespaces": "1024\n",
            "/proc/self/status": (
                "Name:\tprivate-process-name\nCapEff:\t0000000000000000\n"
                "NoNewPrivs:\t1\nSeccomp:\t2\nVmRSS:\tprivate-memory-status\n"
            ),
        }
        read_paths = []

        def read_text(path):
            read_paths.append(str(path))
            return allowed_files[str(path)]

        output = io.StringIO()
        with patch.object(probe.Path, "read_text", read_text), redirect_stderr(output):
            probe.diagnostics()
        self.assertEqual(set(read_paths), set(allowed_files))
        self.assertEqual(len(read_paths), len(allowed_files))
        self.assertIn("fixture-profile (enforce)", output.getvalue())
        self.assertIn("apparmor_restrict_unprivileged_unconfined: 1", output.getvalue())
        for field in ("uid=", "euid=", "gid=", "egid=", "CapEff:", "NoNewPrivs:", "Seccomp:"):
            self.assertIn(field, output.getvalue())
        self.assertNotIn("private-process-name", output.getvalue())
        self.assertNotIn("private-memory-status", output.getvalue())

    def test_workflow_runs_native_probe_after_nested_docker_and_before_build(self):
        job = jobs()["vs"]
        job_steps = steps(job)
        nested = "Check nested Docker in both build environments"
        readiness = "Check native kernel sandbox in Trixie"
        build = next(name for name, step in job_steps.items() if "id: vs_build" in step)
        names = list(job_steps)
        self.assertLess(names.index(nested), names.index(readiness))
        self.assertLess(names.index(readiness), names.index(build))
        self.assertIn("for build_env in bookworm trixie", shell(job_steps[nested]))
        self.assertIn("docker-start", shell(job_steps[nested]))
        script = shell(job_steps[readiness])
        self.assertIn("set -o pipefail", script)
        self.assertNotIn("continue-on-error", job_steps[readiness])
        command_dir = self.root / "commands"
        command_dir.mkdir()
        self.write_executable(command_dir / "make", """
            import json, os, sys
            with open(os.environ["PROBE_EVENTS"], "a") as out:
                out.write(json.dumps({"kind": "make", "argv": sys.argv[1:]}) + "\\n")
            print("controlled native probe failure", file=sys.stderr)
            sys.exit(23)
        """)
        (self.root / "artifacts/vs").mkdir(parents=True)
        result = subprocess.run(
            ["bash", "-e", "-c", script], cwd=self.root,
            env={**self.env, "PATH": str(command_dir) + os.pathsep + os.defpath,
                 "SONIC_CI_CONTAINER_LABEL": "sandbox-readiness=fixture"},
            capture_output=True, text=True, timeout=15,
        )
        self.assertEqual(result.returncode, 23, "tee must not hide a failed readiness check")
        [make] = self.recorded()
        self.assertEqual(make["argv"], [
            "-f", "Makefile.work", "BLDENV=trixie", "sonic-slave-run",
            "SONIC_BUILDER_EXTRA_CMDLINE=--label sandbox-readiness=fixture -e BAZEL=" + str(self.bazel),
            "SONIC_RUN_CMDS=python3 -B tools/bazel/ci/check_kernel_sandbox.py",
        ])
        self.assertIn("controlled native probe failure", (self.root / "artifacts/vs/kernel-sandbox.log").read_text())


if __name__ == "__main__":
    unittest.main()
