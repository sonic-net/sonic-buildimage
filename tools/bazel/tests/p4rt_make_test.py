"""Check source download policy on the P4Runtime package's actual Make recipe."""

import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest


SOURCE = Path(__file__).resolve().parents[3] / "src/sonic-p4rt"
POLICY = SOURCE / "source_mirrors.cfg"
FEDORA = (
    "src.fedoraproject.org/lookaside/extras/xz/xz-5.4.6.tar.gz/sha512/"
    "b08a61d8d478d3b4675cb1ddacdbbd98dc6941a55bcdd81a28679e54e9367d3a"
    "595fa123ac97874a17da571c1b712e2a3e901c2737099a9d268616a1ba3de497/xz-5.4.6.tar.gz"
)


class P4rtMakeTest(unittest.TestCase):
    def test_nested_bazel_commands_and_package_export_are_timed_on_a_cache_miss(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "tools").symlink_to(SOURCE.parents[1] / "tools")
            source = root / "src/sonic-p4rt"
            workspace = source / "sonic-pins"
            workspace.mkdir(parents=True)
            other_bin = root / "other-build"
            (other_bin / "p4rt_app").mkdir(parents=True)
            (other_bin / "p4rt_app/p4rt.deb").write_text("other build")
            (workspace / "bazel-bin").symlink_to(other_bin)
            (source / "Makefile").symlink_to(SOURCE / "Makefile")
            dest = root / "debs"
            dest.mkdir()
            bazel = root / "bazel"
            bazel.write_text("""#!/usr/bin/env python3
import json, os, sys
from pathlib import Path
args = sys.argv[1:]
command = next(arg for arg in args if arg in {'build', 'test', 'info', 'shutdown'})
start, options = args[:args.index(command)], args[args.index(command) + 1:]
base, = [Path(arg.split('=', 1)[1]) for arg in start if arg.startswith('--output_base=')]
with open(os.environ['BAZEL_RECORD'], 'a') as record:
    print(json.dumps([command, str(base), start, options]), file=record)
print('fixture Bazel ' + command, file=sys.stderr)
if command in ('build', 'test'):
    assert options[0].startswith('--profile=') and options[1] == '--noslim_profile'
    Path(options[0].split('=', 1)[1]).touch()
out = base / 'execroot/pins/bazel-out/k8-opt/bin'
if command == 'build':
    (out / 'p4rt_app').mkdir(parents=True)
    for name in ('p4rt', 'p4rt-dbg'):
        (out / 'p4rt_app' / (name + '.deb')).write_text(name)
if command == 'test' and os.environ.get('FAIL_TEST'):
    sys.exit(12)
if command == 'info':
    assert options[-1] == 'bazel-bin'
    print(out)
""")
            bazel.chmod(0o755)
            sdk_builder = root / "sdk_builder.py"
            sdk_builder.write_text("")
            command = ["make", "--no-print-directory", f"DEST={dest}", f"BAZEL_CACHE={root}",
                       f"COMMON_SDK_BUILDER={sdk_builder}",
                       "SONIC_P4RT=p4rt.deb", "SONIC_P4RT_DBG=p4rt-dbg.deb", str(dest / "p4rt.deb")]
            record = root / "calls.jsonl"
            env = dict(os.environ, SONIC_VS_IMAGE_TIMINGS="1", BAZEL="root-workspace-only-wrapper",
                       PATH=f"{root}:{os.environ['PATH']}", BAZEL_RECORD=str(record),
                       SONIC_BAZEL_REMOTE_CACHE="grpc://cache.example", SONIC_BAZEL_REMOTE_UPLOAD="n")
            for _ in range(2):  # The second call reuses the packages and must leave no new rows.
                subprocess.run(command, env=env, cwd=source, check=True, capture_output=True)
            directory, = (root / "artifacts/vs/image-timings/sonic-p4rt").iterdir()
            self.assertEqual(json.loads((directory / "recipe.json").read_text()),
                             {"bazel": ["build", "test", "info", "shutdown"], "profiles": ["build", "test"],
                              "exports": ["stage-debs"]})
            for phase in ("build", "test", "info", "shutdown", "stage-debs"):
                self.assertEqual(json.loads((directory / f"{phase}.timing.json").read_text())["exit_code"], 0)
            self.assertCountEqual([p.name for p in directory.glob("*.profile.json.gz")],
                                  ["build.profile.json.gz", "test.profile.json.gz"])
            self.assertIn("fixture Bazel test", (directory / "make.log").read_text())
            self.assertEqual((dest / "p4rt.deb").read_text(), "p4rt")
            self.assertEqual((dest / "p4rt-dbg.deb").read_text(), "p4rt-dbg")
            calls = [json.loads(line) for line in record.read_text().splitlines()]
            self.assertEqual([c[0] for c in calls], ["build", "test", "info", "shutdown"])
            base, = {Path(c[1]) for c in calls}
            self.assertFalse(base.exists())
            for name, _, startup, options in calls:
                self.assertIn(f"--output_user_root={root}/cache", startup)
                if name != "shutdown":
                    for option in ("--symlink_prefix=/", f"--disk_cache={root}/disk_cache",
                                   "--remote_cache=grpc://cache.example", "--remote_upload_local_results=false"):
                        self.assertIn(option, options)

            # A fresh recipe must clean only its own server, including after a failed test.
            (dest / "p4rt.deb").unlink()
            failed = subprocess.run(command, env=dict(env, FAIL_TEST="1"), cwd=source, capture_output=True)
            self.assertNotEqual(failed.returncode, 0)
            retry = [json.loads(line) for line in record.read_text().splitlines()][len(calls):]
            self.assertEqual([c[0] for c in retry], ["build", "test", "shutdown"])
            retry_base, = {Path(c[1]) for c in retry}
            self.assertNotEqual(retry_base, base)
            self.assertFalse(retry_base.exists())
            self.assertEqual((workspace / "bazel-bin").resolve(strict=True), other_bin)
            self.assertEqual((other_bin / "p4rt_app/p4rt.deb").read_text(), "other build")

    def test_mirror_policy_is_passed_to_both_nested_bazel_commands(self):
        with tempfile.TemporaryDirectory() as dest:
            result = subprocess.check_output([
                "make", "--no-print-directory", "--dry-run", "DEST=" + dest,
                "SONIC_P4RT=p4rt.deb", "SONIC_P4RT_DBG=p4rt-dbg.deb",
                "BAZEL_CACHE=" + dest + "/absent", dest + "/p4rt.deb",
            ], cwd=SOURCE, text=True)
        lines = result.splitlines()
        options, = [line for line in lines if line.startswith("build_options=(")]
        parsed_options = shlex.split(options.removeprefix("build_options=(").removesuffix(")"))
        self.assertIn("--experimental_downloader_config=" + str(POLICY), parsed_options)
        self.assertIn("--override_repository=sonic_swss_common=" + str(SOURCE / ".common-sdk"), parsed_options)
        build, test = [shlex.split(line) for line in lines if line.startswith('"${private_bazel[@]}"')]
        self.assertEqual(build[1:3], ["build", "${build_options[@]}"])
        self.assertEqual(build[-2:], ["//p4rt_app:p4rt_deb", "//p4rt_app:p4rt_dbg_deb"])
        self.assertEqual(test[1:3], ["test", "${build_options[@]}"])
        self.assertEqual(test[-1], "//p4rt_app/...")

    def test_only_the_pinned_xz_archive_adds_an_official_mirror(self):
        rewrites = [line.split()[1:] for line in POLICY.read_text().splitlines() if line.startswith("rewrite ")]
        self.assertEqual([url for pattern, url in rewrites if re.fullmatch(pattern, FEDORA)], [
            "https://github.com/tukaani-project/xz/releases/download/v5.4.6/xz-5.4.6.tar.gz", "https://$1",
        ])
        for pattern, _ in rewrites:
            self.assertEqual(re.fullmatch(pattern, FEDORA).group(1), FEDORA)
            self.assertIsNone(re.fullmatch(pattern, FEDORA.replace("sha512/b08", "sha512/000")))
            self.assertIsNone(re.fullmatch(pattern, FEDORA.replace("5.4.6", "5.4.5")))


if __name__ == "__main__":
    unittest.main()
