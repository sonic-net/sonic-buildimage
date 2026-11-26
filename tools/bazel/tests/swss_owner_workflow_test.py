#!/usr/bin/env python3
"""Exercise selected-owner CI transport and preserve direct runtime providers."""
import ast
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import yaml

ROOT = Path(__file__).resolve().parents[3]
CATALOG = "main"
JOBS = [("bazel-eventd.yml", "native"),
        ("bazel-supervisor-rust.yml", "native"), ("bazel-yang-models.yml", "package"),
        ("bazel-oci.yml", "archive"), ("bazel-oci.yml", "checks")]


def calls(path, function):
    return [{k.arg: ast.literal_eval(k.value) for k in node.keywords}
            for node in ast.walk(ast.parse(path.read_text()))
            if isinstance(node, ast.Call) and getattr(node.func, "id", "") == function]


class SwssOwnerWorkflowTest(unittest.TestCase):
    def test_native_closure_selects_one_configured_output(self):
        """A warm server can expose a second policy built in a Python test transition."""
        jobs = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]
        step = next(s for s in jobs["archive"]["steps"] if "native link closure" in s["name"])["run"]
        function = step[step.index("output() {"):step.index('runtime="$(output')]
        self.assertIn("bgp_eoiu_marker.py null)", step)
        with tempfile.TemporaryDirectory() as temporary:
            fake = Path(temporary) / "bazel"
            fake.write_text("#!/bin/sh\n" +
                            'case "$2" in "config(//:policy, target)") echo top-level ;; '
                            '"config(//:source.py, null)") echo source.py ;; '
                            '*) echo transitioned; echo ambiguous ;; esac\n')
            fake.chmod(0o755)
            shell = function + "output //:policy && output //:source.py null && ! output //:ambiguous"
            result = subprocess.run(["bash", "-ec", shell], env=dict(os.environ, BAZEL=str(fake)),
                                    check=True, capture_output=True, text=True)
            self.assertEqual(result.stdout.splitlines(), ["top-level", "source.py"])
            self.assertIn("Expected exactly one configured output", result.stderr)

    def test_filtered_workflows_include_local_downloader_input(self):
        filenames = {filename for filename, _job in JOBS} | {"bazel-libteam.yml"}
        for filename in filenames:
            # PyYAML's YAML 1.1 loader reads GitHub's `on` key as True.
            triggers = yaml.safe_load((ROOT / ".github/workflows" / filename).read_text())[True]
            for event in ("pull_request", "push"):
                paths = (triggers.get(event) or {}).get("paths")
                if paths is not None:
                    with self.subTest(filename=filename, event=event):
                        self.assertIn("tools/bazel/debian_mirrors.cfg", paths)

    def test_bootstrap_verifies_config_before_appending_to_existing_caches(self):
        from tools.bazel.ci import source_mirrors

        fixture = ROOT / "tools/bazel/tests/fixtures/source-mirrors-f9a9baae.cfg"
        self.assertEqual(hashlib.sha256(fixture.read_bytes()).hexdigest(), source_mirrors.SHA256)
        for filename, job in JOBS:
            steps = yaml.safe_load((ROOT / ".github/workflows" / filename).read_text())["jobs"][job]["steps"]
            mirror = next(s for s in steps if "tools/bazel/ci/source_mirrors.py" in s.get("run", ""))
            self.assertLess(next(i for i,s in enumerate(steps) if "preserve_setup_bazel_cache.py" in s.get("run", "")), steps.index(mirror))
            for corrupt in (False, True):
                with self.subTest(filename=filename, job=job, corrupt=corrupt), tempfile.TemporaryDirectory() as temporary:
                    root = Path(temporary)
                    (root / "tools").symlink_to(ROOT / "tools", target_is_directory=True)
                    config = root / ".bazelrc.user"
                    before = "common --repository_cache=/retained/repository\nbuild --disk_cache=/retained/disk\n"
                    config.write_text(before)
                    data = root / "response"
                    data.write_bytes(fixture.read_bytes() + (b"changed" if corrupt else b""))
                    # Run the actual shell and Python CLI, replacing only the HTTP response.
                    (root / "sitecustomize.py").write_text(
                        "import io, os, urllib.request\nfrom pathlib import Path\n"
                        "def fetch(url, timeout):\n"
                        "    assert url == os.environ['EXPECTED_URL'] and timeout == 60\n"
                        "    return io.BytesIO(Path('response').read_bytes())\n"
                        "urllib.request.urlopen = fetch\n")
                    env = dict(os.environ, PYTHONPATH=str(root), RUNNER_TEMP=str(root), EXPECTED_URL=source_mirrors.URL)
                    result = subprocess.run(["bash", "-ec", mirror["run"]], cwd=root, env=env,
                                            capture_output=True, text=True)
                    selected = root / "sonic-source-mirrors.cfg"
                    if corrupt:
                        self.assertNotEqual(result.returncode, 0)
                        self.assertIn("checksum mismatch", result.stderr)
                        self.assertFalse(selected.exists())
                        self.assertEqual(config.read_text(), before)
                    else:
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(config.read_text(), before + "common --downloader_config=" + str(selected) + "\n")
                        local = (ROOT / "tools/bazel/debian_mirrors.cfg").read_bytes()
                        self.assertEqual(selected.read_bytes(), local + b"\n" + fixture.read_bytes())

    def test_real_wrapper_selects_complete_catalog_and_respects_private_cache_overrides(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            recorder = root / "bazel"
            recorder.write_text("#!/usr/bin/python3\nimport json,sys\nprint(json.dumps(sys.argv[1:]))\n")
            recorder.chmod(0o755)
            wrapper = ROOT / "tools/bazel/ci/libteam_draft_bazel.sh"
            result = subprocess.run(["bash", str(wrapper), "test", "--disk_cache=/private/disk",
                                     "@sonic_swss//crates/countersyncd:common_rust_test"], cwd=root,
                                    env=dict(os.environ, PATH=str(root)+":/usr/bin:/bin"), capture_output=True, text=True, check=True)
            args = json.loads(result.stdout)
            self.assertIn("--nohome_rc", args)
            self.assertIn("--registry=https://raw.githubusercontent.com/sonic-net/sonic-bazel-registry/"+CATALOG, args)
            self.assertEqual(args[-2:], ["--disk_cache=/private/disk", "@sonic_swss//crates/countersyncd:common_rust_test"])

    def test_image_keeps_direct_debug_providers_and_library_only_metadata(self):
        document = ROOT / "dockers/docker-swss-layer-trixie/BUILD.bazel"
        # Parse only the literal rules whose provider edges are the contract.
        tree = ast.parse(document.read_text())
        selected = {}
        for node in tree.body:
            if isinstance(node, ast.Expr) and isinstance(node.value, ast.Call):
                fields = {k.arg: k.value for k in node.value.keywords}
                if "name" in fields:
                    selected[ast.literal_eval(fields["name"])] = fields
        runtime = ast.literal_eval(selected["source_runtime"]["tars"])
        self.assertIn("@libteam//:libteam_runtime", runtime)
        self.assertIn("@libteam//:libteamdctl_runtime", runtime)
        self.assertNotIn("@sonic_swss//dist:libteam_runtime", runtime)
        self.assertEqual(ast.literal_eval(selected["installed_packages"]["owner_metadata"]),
                         ["@libteam//:library_runtime_packages.json"])
        self.assertEqual(ast.literal_eval(selected["installed_packages"]["source_modules"]),
                         {"libteam": "@image_source_modules//:libteam.MODULE.bazel"})

    def test_runtime_adapter_matches_common_interface(self):
        # Evaluate the real Starlark macro with its rule/select interfaces stubbed,
        # then check the selected Common helper's positional argument contract.
        namespace = {"load": lambda *args: None, "select": lambda choices: choices["//conditions:default"]}
        observed = []
        namespace["sh_test"] = lambda **kwargs: observed.append(kwargs)
        exec(compile((ROOT / "tools/bazel/tests/runtime.bzl").read_text(), "runtime.bzl", "exec"), namespace)
        namespace["common_runtime_test"]("listener_tests", ":listener_tests_binary", test_args=["--test-threads=1"])
        test = observed[0]
        self.assertEqual(test["args"], ["$(rootpath @patchelf//:patchelf)",
            "$(rootpath @sonic_swss_common//tools/bazel/runtime:test_runtime)", "-", "disabled",
            "$(rootpath :listener_tests_binary)", "--test-threads=1"])
        self.assertIn("@patchelf//:patchelf", test["data"])
        dependencies = calls(ROOT / "MODULE.bazel", "bazel_dep")
        self.assertEqual([d["version"] for d in dependencies if d["name"] == "patchelf"], ["0.18.0.bcr.1"])


if __name__ == "__main__":
    unittest.main()
