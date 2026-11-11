"""Regression coverage for what the cache audit's PR gate blames on a PR."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
AUDIT = (ROOT / "scripts/audit_dep_completeness.sh").read_text(encoding="utf-8")


def diff(*files):
    """A `git diff --unified=0` of (path, removed lines, added lines) files."""
    out = []
    for path, removed, added in files:
        out += [
            f"diff --git a/{path} b/{path}",
            "index 0000000..1111111 100644",
            f"--- a/{path}",
            f"+++ b/{path}",
            "@@ -1 +1 @@",
        ]
        out += ["-" + line for line in removed]
        out += ["+" + line for line in added]
    return "\n".join(out) + "\n"


class DiffVarsTest(unittest.TestCase):
    """The variables a PR touches, which Check 10 escalates to P0."""

    def touched(self, text):
        program = AUDIT.split("DIFF_VARS_PY='", 1)[1].split("\n'\n", 1)[0]
        result = subprocess.run(
            [sys.executable, "-c", program], input=text,
            text=True, encoding="utf-8",
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        return set(result.stdout.split())

    def test_rewritten_line_keeping_a_reference_touches_it(self):
        self.assertEqual(self.touched(diff(("slave.mk",
            ['run = V="$(SONIC_IMAGE_VERSION)" ./a.py'],
            ['run = V="$(SONIC_IMAGE_VERSION)" ./a.py --strict'],
        ))), {"SONIC_IMAGE_VERSION"})

    def test_reference_dropped_from_a_cache_key_touches_it(self):
        self.assertEqual(self.touched(diff(("Makefile.cache",
            ["SONIC_COMMON_FLAGS_LIST := $(CONFIGURED_ARCH) $(FOO)"],
            ["SONIC_COMMON_FLAGS_LIST := $(CONFIGURED_ARCH)"],
        ))), {"CONFIGURED_ARCH", "FOO"})

    def test_reference_moved_within_a_file_touches_it(self):
        self.assertEqual(self.touched(diff(("slave.mk",
            ["\tINSTALLER_ONLY=$(FOO) ./build_image.sh"],
            ["\tFOO=$(FOO) dpkg-buildpackage -b"],
        ))), {"FOO"})

    def test_new_reference_touches_it(self):
        self.assertEqual(self.touched(diff(("slave.mk",
            ["\tcp a b"],
            ['\tcp a b DEST="$(IMAGE_DISTRO_DEBS_PATH)"'],
        ))), {"IMAGE_DISTRO_DEBS_PATH"})

    def test_reference_moved_to_another_file_touches_it(self):
        self.assertEqual(self.touched(diff(
            ("slave.mk", ["X = $(FOO)"], []),
            ("rules/foo.mk", [], ["FOO_BUILD_ENV += $(FOO)"]),
        )), {"FOO"})

    def test_added_or_removed_export_touches_it(self):
        self.assertEqual(self.touched(diff(("slave.mk",
            ["export OLD_FLAG"],
            ["export NEW_FLAG", "export A=1 B=$(C)"],
        ))), {"OLD_FLAG", "NEW_FLAG", "A", "B", "C"})

    def test_removed_line_starting_with_dashes_is_content(self):
        self.assertEqual(self.touched(diff(("slave.mk",
            [], ["-- $(FOO)"],
        )) + diff(("Makefile", ["-- $(BAR)"], ["++ $(BAR) $(BAR)"]))),
            {"FOO", "BAR"})


class RecipeInvokedScriptsTest(unittest.TestCase):
    """Check 28: scripts a slave.mk recipe runs that no cache key hashes."""

    def findings(self, scripts):
        function = AUDIT.split("check_recipe_invoked_scripts() {", 1)[1]
        function = "check_recipe_invoked_scripts() {" + function.split("\n}\n", 1)[0] + "\n}\n"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "scripts").mkdir()
            for script in scripts:
                (root / script).write_text("", encoding="utf-8")
            (root / "Makefile.cache").write_text("", encoding="utf-8")
            (root / "slave.mk").write_text(
                "".join(f"\t./{script}\n" for script in scripts),
                encoding="utf-8", newline="\n",
            )
            harness = """
REPO_ROOT=$(pwd)
MAKEFILE_CACHE="$REPO_ROOT/Makefile.cache"
add_finding() { printf 'FINDING:%s:%s\\n' "$1" "$2"; }
"""
            result = subprocess.run(
                [os.environ.get("BASH", "bash")],
                input=harness + function + "\ncheck_recipe_invoked_scripts\ntrue\n",
                cwd=root, text=True, encoding="utf-8",
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        return {line for line in result.stdout.splitlines() if line.startswith("FINDING:")}

    def test_sbom_scripts_are_not_reported(self):
        self.assertEqual(self.findings([
            "scripts/build_sbom.sh", "scripts/build_sbom.py",
            "scripts/install_sbom_tool.sh", "scripts/sbom_fragment.py",
            "scripts/sbom_cve_refs.py", "scripts/sbom_purl.py",
            "scripts/sbom_parse_lockfiles.py",
            "scripts/sbom_extract_vex_from_patches.py",
            "scripts/wait_for_docker.sh",
        ]), set())

    def test_other_unhashed_scripts_are_still_reported(self):
        self.assertEqual(
            self.findings(["scripts/prepare_docker_buildinfo.sh", "scripts/sbom_purl.py"]),
            {"FINDING:P2:scripts/prepare_docker_buildinfo.sh"},
        )


if __name__ == "__main__":
    unittest.main()
