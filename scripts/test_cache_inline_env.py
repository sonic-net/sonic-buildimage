"""Regression coverage for the rootfs inline-environment cache audit."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class InlineEnvironmentAuditTest(unittest.TestCase):
    def run_audit(self, cache):
        source = (ROOT / "scripts/audit_dep_completeness.sh").read_text(encoding="utf-8")
        function = source.split("check_inline_recipe_env_vars() {", 1)[1]
        function = "check_inline_recipe_env_vars() {" + function.split("\n}\n", 1)[0] + "\n}\n"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "Makefile.cache").write_text(cache, encoding="utf-8", newline="\n")
            (root / "slave.mk").write_text(
                '\tUSERNAME="$(USERNAME)" \\\n'
                '\tPASSWORD="$(PASSWORD)" \\\n'
                '\tIMAGE_TYPE=$($(installer)_IMAGE_TYPE) \\\n'
                '\tUNKEYED="$(UNKEYED)" \\\n'
                '\t\t./build_debian.sh\n'
                '\t$(call SAVE_CACHE,$*,$@)\n',
                encoding="utf-8", newline="\n",
            )
            (root / "build_debian.sh").write_text(
                'printf "%s" "$USERNAME" "$PASSWORD" "$IMAGE_TYPE" "$UNKEYED"\n',
                encoding="utf-8", newline="\n",
            )
            harness = """
REPO_ROOT=$(pwd)
MAKEFILE_CACHE="$REPO_ROOT/Makefile.cache"
COMMON_FLAGS_LIST_CACHE=""
VERBOSE=false
flag_is_exported() { return 1; }
waiver_reason() { :; }
log_verbose() { :; }
add_finding() { printf 'FINDING:%s:%s\\n' "$1" "$2"; }
"""
            result = subprocess.run(
                [os.environ.get("BASH", "bash")],
                input=harness + function + "\ncheck_inline_recipe_env_vars\ntrue\n",
                cwd=root, text=True, encoding="utf-8",
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        return {line for line in result.stdout.splitlines() if line.startswith("FINDING:")}

    def test_existing_rootfs_key_covers_values_fingerprints_and_image_type(self):
        cache = (ROOT / "Makefile.cache").read_text(encoding="utf-8")
        self.assertEqual(self.run_audit(cache), {"FINDING:P1:UNKEYED"})

    def test_removing_rootfs_helper_from_key_reports_its_inputs(self):
        cache = (ROOT / "Makefile.cache").read_text(encoding="utf-8").replace(
            "$(SONIC_COMMON_FLAGS_LIST) $(RFS_INLINE_ENV_FLAGS)",
            "$(SONIC_COMMON_FLAGS_LIST)",
        )
        self.assertEqual(
            self.run_audit(cache),
            {"FINDING:P1:USERNAME", "FINDING:P1:PASSWORD", "FINDING:P1:UNKEYED"},
        )

    def test_removing_image_type_from_key_reports_it(self):
        cache = (ROOT / "Makefile.cache").read_text(encoding="utf-8").replace(
            "$($($(pkg)_INSTALLER)_IMAGE_TYPE)", "",
        )
        self.assertEqual(
            self.run_audit(cache),
            {"FINDING:P1:IMAGE_TYPE", "FINDING:P1:UNKEYED"},
        )

    def test_missing_rootfs_key_reports_all_consumed_inputs(self):
        self.assertEqual(
            self.run_audit(""),
            {"FINDING:P1:USERNAME", "FINDING:P1:PASSWORD",
             "FINDING:P1:IMAGE_TYPE", "FINDING:P1:UNKEYED"},
        )


if __name__ == "__main__":
    unittest.main()
