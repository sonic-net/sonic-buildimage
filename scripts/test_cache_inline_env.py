"""Regression coverage for cache-key environment and flag-flow auditing."""

import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
SCANNER_SPEC = importlib.util.spec_from_file_location(
    "cache_key_scan", ROOT / "scripts/cache_key_scan.py")
SCANNER = importlib.util.module_from_spec(SCANNER_SPEC)
SCANNER_SPEC.loader.exec_module(SCANNER)


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


class DerivedExportAuditTest(unittest.TestCase):
    def classify(self, updates=None):
        files = {
            "Makefile.cache": "SONIC_COMMON_FLAGS_LIST := $(CONFIGURED_PLATFORM)\n",
            "slave.mk": "PLATFORM_PATH = platform/$(CONFIGURED_PLATFORM)\n",
            "platform/vendor/package.mk": "export PLATFORM_PATH UNKEYED\n",
            "src/package/Makefile": "build:\n\techo $(PLATFORM_PATH) $(UNKEYED)\n",
        }
        files.update(updates or {})
        with patch.object(SCANNER, "read", side_effect=lambda path: files.get(path, "")), \
             patch.object(SCANNER, "git_files", return_value=list(files)):
            return {name: (kind, evidence) for name, kind, evidence in SCANNER.classify()}

    def test_slave_selector_derived_from_common_key_is_covered(self):
        result = self.classify()
        self.assertEqual(result["PLATFORM_PATH"][0], "derived")
        self.assertIn("CONFIGURED_PLATFORM", result["PLATFORM_PATH"][1])
        self.assertEqual(result["UNKEYED"][0], "gap")

    def test_derived_coverage_does_not_depend_on_variable_names(self):
        result = self.classify({
            "Makefile.cache": "SONIC_COMMON_FLAGS_LIST := $(BOARD_VARIANT)\n",
            "slave.mk": "BOARD_SOURCE := boards/$(BOARD_VARIANT)\n",
            "platform/vendor/package.mk": "export BOARD_SOURCE\n",
            "src/package/Makefile": "build:\n\techo $(BOARD_SOURCE)\n",
        })
        self.assertEqual(result["BOARD_SOURCE"][0], "derived")

    def test_ambiguous_or_overrideable_derivations_remain_findings(self):
        definitions = (
            "PLATFORM_PATH ?= platform/$(CONFIGURED_PLATFORM)\n",
            "ifeq ($(EXTRA),y)\nPLATFORM_PATH = platform/$(CONFIGURED_PLATFORM)\nendif\n",
            "PLATFORM_PATH = $(shell echo $(CONFIGURED_PLATFORM))\n",
            "PLATFORM_PATH = platform/$(CONFIGURED_PLATFORM)/$(UNKEYED)\n",
            "PLATFORM_PATH = platform/$(CONFIGURED_PLATFORM)\nPLATFORM_PATH = $(UNKEYED)\n",
        )
        for definition in definitions:
            with self.subTest(definition=definition):
                self.assertEqual(self.classify({"slave.mk": definition})["PLATFORM_PATH"][0], "gap")

    def test_reassignment_in_platform_rule_invalidates_derivation(self):
        result = self.classify({
            "platform/vendor/override.mk": "override PLATFORM_PATH = $(UNKEYED)\n",
        })
        self.assertEqual(result["PLATFORM_PATH"][0], "gap")

    def test_configuration_override_invalidates_derivation(self):
        result = self.classify({"rules/config": "override PLATFORM_PATH = $(UNKEYED)\n"})
        self.assertEqual(result["PLATFORM_PATH"][0], "gap")

    def test_forwarded_selector_override_is_not_derived(self):
        result = self.classify({
            "Makefile.work": "COMMAND := $(MAKE) -f slave.mk PLATFORM_PATH=$(UNKEYED)\n",
        })
        self.assertEqual(result["PLATFORM_PATH"][0], "gap")

    def test_dynamic_and_target_specific_assignments_remain_findings(self):
        for assignment in (
            "all: PLATFORM_PATH = $(UNKEYED)\n",
            "$(eval PLATFORM_PATH = $(UNKEYED))\n",
        ):
            with self.subTest(assignment=assignment):
                result = self.classify({"platform/vendor/override.mk": assignment})
                self.assertEqual(result["PLATFORM_PATH"][0], "gap")

    def test_escaped_hash_does_not_hide_an_unkeyed_input(self):
        result = self.classify({
            "slave.mk": "PLATFORM_PATH = platform/$(CONFIGURED_PLATFORM)\\#$(UNKEYED)\n",
        })
        self.assertEqual(result["PLATFORM_PATH"][0], "gap")

    def test_continued_comment_does_not_supply_a_common_key(self):
        result = self.classify({
            "Makefile.cache": "# disabled key \\\nSONIC_COMMON_FLAGS_LIST := $(CONFIGURED_PLATFORM)\n",
        })
        self.assertEqual(result["PLATFORM_PATH"][0], "gap")

    def test_non_top_level_common_key_reset_invalidates_coverage(self):
        for reset in (
            "$(eval SONIC_COMMON_FLAGS_LIST := fixed)\n",
            "all: SONIC_COMMON_FLAGS_LIST := fixed\n",
        ):
            with self.subTest(reset=reset):
                result = self.classify({
                    "Makefile.cache": "SONIC_COMMON_FLAGS_LIST := $(CONFIGURED_PLATFORM)\n" + reset,
                })
                self.assertEqual(result["PLATFORM_PATH"][0], "gap")

    def test_even_backslashes_do_not_hide_common_key_reset(self):
        result = self.classify({
            "Makefile.cache": (
                "SONIC_COMMON_FLAGS_LIST := $(CONFIGURED_PLATFORM)\n"
                + "# comment " + "\\" * 2 + "\n"
                + "SONIC_COMMON_FLAGS_LIST := fixed\n"
            ),
        })
        self.assertEqual(result["PLATFORM_PATH"][0], "gap")

    def test_package_specific_input_key_is_not_global_coverage(self):
        result = self.classify({
            "Makefile.cache": "SONIC_COMMON_FLAGS_LIST := $(CONFIGURED_ARCH)\n",
            "rules/other.dep": "$(OTHER)_DEP_FLAGS := $(CONFIGURED_PLATFORM)\n",
        })
        self.assertEqual(result["PLATFORM_PATH"][0], "gap")


class ForwardedFlagAuditTest(unittest.TestCase):
    def covered(self, updates=None, flag="ENABLE_FEATURE", forwarded="RPC_SELECTOR", extra_block=""):
        block = f"ifeq ($({flag}),y)\n$(SYNCD)_DEPENDS += $(RPC_LIBRARY)\n{extra_block}endif\n"
        files = {
            "Makefile.cache": "SONIC_COMMON_FLAGS_LIST := $(CONFIGURED_PLATFORM)\n",
            "Makefile.work": (
                "SONIC_BUILD_INSTRUCTION := $(MAKE) \\\n"
                f"    -f slave.mk \\\n    {forwarded}=$({flag})\n"
            ),
            "slave.mk": f"ifeq ($({forwarded}),y)\n{flag} = y\nendif\n",
            "platform/vendor/rules.mk": block,
            "platform/vendor/rules.dep": "include $(PLATFORM_PATH)/package.dep\n",
            "rules/syncd.dep": f"$(SYNCD)_DEP_FLAGS := $(SONIC_COMMON_FLAGS_LIST) $({forwarded})\n",
        }
        files.update(updates or {})
        with patch.object(SCANNER, "read", side_effect=lambda path: files.get(path, "")), \
             patch.object(SCANNER, "git_files", return_value=list(files)):
            return SCANNER.forwarded_flag_is_tracked(flag, "platform/vendor/rules.mk", block)

    def test_exact_forwarding_and_controller_use_affected_package_key(self):
        self.assertTrue(self.covered())
        self.assertTrue(self.covered(flag="BUILD_OPTION", forwarded="PASSED_OPTION"))

    def test_similar_names_without_forwarding_are_not_evidence(self):
        self.assertFalse(self.covered({"Makefile.work": ""}, forwarded="SONIC_ENABLE_FEATURE"))

    def test_unrelated_package_key_does_not_cover_the_affected_package(self):
        self.assertFalse(self.covered({
            "rules/syncd.dep": "$(OTHER)_DEP_FLAGS := $(RPC_SELECTOR)\n",
        }))

    def test_every_affected_package_requires_the_forwarded_key(self):
        self.assertFalse(self.covered(extra_block="$(OTHER)_DEPENDS += $(RPC_LIBRARY)\n"))

    def test_lossy_forwarding_and_non_slave_commands_are_rejected(self):
        commands = (
            "$(MAKE) -f slave.mk RPC_SELECTOR=$(filter y,$(ENABLE_FEATURE))\n",
            "$(MAKE) -f slave.mk RPC_SELECTOR=prefix$(ENABLE_FEATURE)\n",
            "$(MAKE) -f other.mk RPC_SELECTOR=$(ENABLE_FEATURE)\n",
            "ifeq ($(EXTRA),y)\n$(MAKE) -f slave.mk RPC_SELECTOR=$(ENABLE_FEATURE)\nendif\n",
        )
        for command in commands:
            with self.subTest(command=command):
                self.assertFalse(self.covered({"Makefile.work": command}))

    def test_extra_slave_assignment_rejects_the_alias(self):
        self.assertFalse(self.covered({
            "slave.mk": "ifeq ($(RPC_SELECTOR),y)\nENABLE_FEATURE = y\nendif\nENABLE_FEATURE = $(OTHER)\n",
        }))

    def test_reassigned_forwarded_input_is_not_evidence(self):
        self.assertFalse(self.covered({
            "slave.mk": "ifeq ($(RPC_SELECTOR),y)\nENABLE_FEATURE = y\nendif\nRPC_SELECTOR = fixed\n",
        }))

    def test_conflicting_forwarding_commands_are_not_evidence(self):
        self.assertFalse(self.covered({
            "Makefile.work": (
                "COMMAND := $(MAKE) -f slave.mk RPC_SELECTOR=$(ENABLE_FEATURE)\n"
                "COMMAND := $(MAKE) -f slave.mk RPC_SELECTOR=$(OTHER)\n"
            ),
        }))

    def test_independent_source_argument_is_not_covered_by_forwarded_key(self):
        self.assertFalse(self.covered({
            "Makefile.work": "COMMAND := $(MAKE) -f slave.mk RPC_SELECTOR=$(ENABLE_FEATURE) ENABLE_FEATURE=$(UNKEYED)\n",
        }))

    def test_other_assignment_operators_are_not_forwarding_evidence(self):
        for operator in (":=", "?=", "+=", "!=", "::="):
            with self.subTest(operator=operator):
                self.assertFalse(self.covered({
                    "Makefile.work": "COMMAND := $(MAKE) -f slave.mk RPC_SELECTOR=$(ENABLE_FEATURE) ENABLE_FEATURE" + operator + "$(UNKEYED)\n",
                }))

    def test_transformed_key_value_is_not_lossless_coverage(self):
        self.assertFalse(self.covered({
            "rules/syncd.dep": "$(SYNCD)_DEP_FLAGS := $(filter y,$(RPC_SELECTOR))\n",
        }))

    def test_conditional_only_package_key_is_not_guaranteed(self):
        self.assertFalse(self.covered({
            "rules/syncd.dep": "ifeq ($(EXTRA),y)\n$(SYNCD)_DEP_FLAGS := $(RPC_SELECTOR)\nendif\n",
        }))

    def test_platform_guarded_package_key_is_available_for_its_platform(self):
        self.assertTrue(self.covered({
            "rules/syncd.dep": "ifneq ($(CONFIGURED_PLATFORM),vs)\n$(SYNCD)_DEP_FLAGS := $(RPC_SELECTOR)\nendif\n",
        }))
        self.assertFalse(self.covered({
            "rules/syncd.dep": "ifeq ($(CONFIGURED_PLATFORM),vs)\n$(SYNCD)_DEP_FLAGS := $(RPC_SELECTOR)\nendif\n",
        }))

    def test_prefixed_reset_does_not_preserve_an_earlier_key(self):
        self.assertFalse(self.covered({
            "rules/syncd.dep": "$(SYNCD)_DEP_FLAGS := $(RPC_SELECTOR)\noverride $(SYNCD)_DEP_FLAGS := fixed\n",
        }))

    def test_continued_comment_does_not_supply_a_package_key(self):
        self.assertFalse(self.covered({
            "rules/syncd.dep": "# disabled key \\\n$(SYNCD)_DEP_FLAGS := $(RPC_SELECTOR)\n",
        }))

    def test_non_top_level_package_key_reset_invalidates_coverage(self):
        for reset in (
            "$(eval $(SYNCD)_DEP_FLAGS := fixed)\n",
            "all: $(SYNCD)_DEP_FLAGS := fixed\n",
        ):
            with self.subTest(reset=reset):
                self.assertFalse(self.covered({
                    "rules/syncd.dep": "$(SYNCD)_DEP_FLAGS := $(RPC_SELECTOR)\n" + reset,
                }))

    def test_even_backslashes_do_not_hide_package_key_reset(self):
        self.assertFalse(self.covered({
            "rules/syncd.dep": (
                "$(SYNCD)_DEP_FLAGS := $(RPC_SELECTOR)\n"
                + "# comment " + "\\" * 2 + "\n"
                + "$(SYNCD)_DEP_FLAGS := fixed\n"
            ),
        }))

    def test_similar_key_name_does_not_cover_forwarded_value(self):
        self.assertFalse(self.covered({
            "rules/syncd.dep": "$(SYNCD)_DEP_FLAGS := $(RPC_SELECTOR_EXTRA)\n",
        }))

    def test_owning_platform_reassignment_invalidates_key_coverage(self):
        self.assertFalse(self.covered({
            "platform/vendor/rules.dep": "$(SYNCD)_DEP_FLAGS := $(UNRELATED)\n",
        }))


if __name__ == "__main__":
    unittest.main()
