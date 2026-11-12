"""Regression coverage for the rootfs inline-environment cache audit."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]

FILE_PRESENCE = (
    '$(if $(and $(filter dev prod,$(subst ",,$(SECURE_UPGRADE_MODE))),'
    '$(SECURE_BOOT_DB_CERT)),y,n)'
)

FILE_TRACKING = (
    "SECURE_BOOT_DB_CERT_ENABLED := " + FILE_PRESENCE + "\n"
) + """
ifeq ($(SECURE_BOOT_DB_CERT_ENABLED),y)
SECURE_BOOT_DB_CERT_CACHE_FILE := $(TARGET_PATH)/secure-boot/DB.auth
RFS_DEP_FILES += $(SECURE_BOOT_DB_CERT_CACHE_FILE)
endif

ifeq ($(SECURE_BOOT_DB_CERT_ENABLED),y)
# Recheck the source without changing an identical copy's mtime.
.PHONY: secure-boot-db-cert-force
secure-boot-db-cert-force:

$(SECURE_BOOT_DB_CERT_CACHE_FILE): secure-boot-db-cert-force
\t@mkdir -p '$(@D)'
\tcmp -s -- '$(subst ','"'"',$(SECURE_BOOT_DB_CERT))' '$@' || \\
\t\tcp -- '$(subst ','"'"',$(SECURE_BOOT_DB_CERT))' '$@'
endif
"""

FILE_CONSUMER = """
file ./build_debian.sh
if [[ $SECURE_UPGRADE_MODE == 'dev' || $SECURE_UPGRADE_MODE == "prod" ]]; then
    if [ -n "$SECURE_BOOT_DB_CERT" ]; then
        echo "Embedding $SECURE_BOOT_DB_CERT"
        if [ ! -f "$SECURE_BOOT_DB_CERT" ]; then
            echo "Error: SECURE_BOOT_DB_CERT=$SECURE_BOOT_DB_CERT is missing"
            exit 1
        fi
        sudo cp "$SECURE_BOOT_DB_CERT" "$FILESYSTEM_ROOT/boot/DB.auth"
    fi
fi
"""


class InlineEnvironmentAuditTest(unittest.TestCase):
    def run_audit(self, cache, extra_consumer="", extra_env=""):
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
                + extra_env +
                '\t\t./build_debian.sh\n'
                '\t$(call SAVE_CACHE,$*,$@)\n',
                encoding="utf-8", newline="\n",
            )
            (root / "build_debian.sh").write_text(
                'printf "%s" "$USERNAME" "$PASSWORD" "$IMAGE_TYPE" "$UNKEYED"\n'
                + extra_consumer,
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
                input=f'SCRIPT_DIR="{ROOT / "scripts"}"\n' + harness + function
                + "\ncheck_inline_recipe_env_vars\ntrue\n",
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

    def file_cache(self):
        cache = (ROOT / "Makefile.cache").read_text(encoding="utf-8")
        return cache.replace(
            "$(call rfs_secret_fp,$(BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD))",
            "$(call rfs_secret_fp,$(BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD)) \\\n"
            "    SECURE_BOOT_DB_CERT_ENABLED=$(SECURE_BOOT_DB_CERT_ENABLED)",
        ) + FILE_TRACKING

    def file_findings(self, cache, consumer=FILE_CONSUMER):
        return self.run_audit(
            cache, consumer,
            '\tSECURE_BOOT_DB_CERT=\'$(subst \',\'"\'"\',$(SECURE_BOOT_DB_CERT))\' \\\n',
        )

    def test_file_bytes_and_enable_state_cover_the_source_path(self):
        self.assertEqual(self.file_findings(self.file_cache()), {"FINDING:P1:UNKEYED"})

    def test_incomplete_file_tracking_is_still_reported(self):
        stage = "\nifeq (" + FILE_TRACKING.rsplit("\nifeq (", 1)[1]
        enable_flag = (
            "SECURE_BOOT_DB_CERT_ENABLED=$(SECURE_BOOT_DB_CERT_ENABLED)"
        )
        changes = [
            ("missing enable bit", enable_flag, ""),
            ("constant enable bit", enable_flag,
             "SECURE_BOOT_DB_CERT_ENABLED=y"),
            ("unkeyed enable bit", "$(RFS_INLINE_ENV_FLAGS)", ""),
            ("missing file", "RFS_DEP_FILES += $(SECURE_BOOT_DB_CERT_CACHE_FILE)", ""),
            ("file list disconnected", "$(SONIC_COMMON_BASE_FILES_LIST) $(RFS_DEP_FILES)",
             "$(SONIC_COMMON_BASE_FILES_LIST)"),
            ("non-content key", "$(pkg)_CACHE_MODE := GIT_CONTENT_SHA",
             "$(pkg)_CACHE_MODE := GIT_COMMIT_SHA"),
            ("different source", "cp -- '$(subst ','\"'\"',$(SECURE_BOOT_DB_CERT))'",
             "cp -- '$(OTHER_CERT)'"),
            ("no copy", "\t\tcp --", "\t\ttrue # cp --"),
            ("no forced refresh", ".PHONY: secure-boot-db-cert-force", ""),
            ("different stage guard",
             "$(SECURE_BOOT_DB_CERT_CACHE_FILE): secure-boot-db-cert-force",
             "$(OTHER_FILE): secure-boot-db-cert-force"),
            ("overwritten file list", FILE_TRACKING,
             FILE_TRACKING + "\nRFS_DEP_FILES := other\n"),
            ("presence bit without mode", FILE_PRESENCE,
             "$(if $(SECURE_BOOT_DB_CERT),y,n)"),
            ("nested stage guard", stage,
             "\nifeq ($(OTHER_FLAG),y)" + stage + "\nendif\n"),
            ("duplicate staging recipe", FILE_TRACKING,
             FILE_TRACKING + "\n$(SECURE_BOOT_DB_CERT_CACHE_FILE):\n\t@true\n"),
        ]
        for label, old, new in changes:
            with self.subTest(label=label):
                cache = self.file_cache()
                self.assertIn(old, cache)
                self.assertIn(
                    "FINDING:P1:SECURE_BOOT_DB_CERT",
                    self.file_findings(cache.replace(old, new)),
                )

    def test_extra_or_differently_gated_content_uses_are_reported(self):
        for consumer in [
            FILE_CONSUMER + 'printf "%s" "$SECURE_BOOT_DB_CERT" > rootfs/path\n',
            FILE_CONSUMER.replace('== "prod"', '== "test"'),
            FILE_CONSUMER.replace('if [ -n "$SECURE_BOOT_DB_CERT" ]; then', "if true; then"),
            FILE_CONSUMER.replace("if [[ $SECURE_UPGRADE_MODE", "if [[ $OTHER_MODE"),
            FILE_CONSUMER + 'SECURE_BOOT_DB_CERT=other\n',
            FILE_CONSUMER.replace(
                'file ./build_debian.sh',
                'SECURE_UPGRADE_MODE=prod',
            ),
        ]:
            with self.subTest(consumer=consumer):
                self.assertIn(
                    "FINDING:P1:SECURE_BOOT_DB_CERT",
                    self.file_findings(self.file_cache(), consumer),
                )

    def test_recognition_is_not_tied_to_certificate_variable_names(self):
        cache = self.file_cache().replace("SECURE_BOOT_DB_CERT", "ROOTFS_DATA")
        consumer = FILE_CONSUMER.replace("SECURE_BOOT_DB_CERT", "ROOTFS_DATA")
        self.assertEqual(
            self.run_audit(cache, consumer, '\tROOTFS_DATA="$(ROOTFS_DATA)" \\\n'),
            {"FINDING:P1:UNKEYED"},
        )

    def test_recipe_source_alias_does_not_clear_file_input(self):
        self.assertIn(
            "FINDING:P1:SECURE_BOOT_DB_CERT",
            self.run_audit(
                self.file_cache(), FILE_CONSUMER,
                '\tSECURE_BOOT_DB_CERT="$(OTHER_CERT)" \\\n',
            ),
        )


if __name__ == "__main__":
    unittest.main()
