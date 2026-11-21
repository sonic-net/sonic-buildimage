"""Reject account loss while retaining the declared gearbox OpenSSH group."""

import ast
import contextlib
import io
import copy
import json
import sys
from types import SimpleNamespace
from pathlib import Path
import tempfile
import unittest

from unittest.mock import patch

from tools.bazel.ci import gbsyncd_image
from tools.bazel.ci.public_artifacts import PublicArtifactError, safe_json


BASE = "root:x:0:\nusers:x:100:sonic\nredis:x:103:\n"
RUNTIME = BASE + "_ssh:x:104:\n"


class GearboxGroups(unittest.TestCase):
    def validate(self, base=BASE, runtime=RUNTIME, debug=RUNTIME, mode=0o644):
        layouts = [(base, 0o644), (runtime, mode), (debug, 0o644)]

        def inventory(layout):
            return {"etc/group": {"kind": "file", "mode": layout[1], "uid": 0, "gid": 0}}, {}

        def extract(layout, files, origins, selected, root):
            self.assertEqual(selected, {"etc/group"})
            (root / "etc").mkdir()
            (root / "etc/group").write_text(layout[0])

        with patch.object(gbsyncd_image.image_validation, "image_inventory", inventory), \
                patch.object(gbsyncd_image.source_base_validation, "extract_selected", extract):
            return gbsyncd_image.validate_groups(*layouts)

    def test_declared_addition_preserves_parent_and_debug_groups(self):
        self.assertEqual(self.validate(), {"base_groups": 3, "runtime_groups": 4,
                                          "debug_groups": 4, "declared_addition": "_ssh:x:104:"})

    def test_missing_or_modified_parent_rows_are_rejected(self):
        for changed in (RUNTIME.replace("redis:x:103:\n", ""),
                        RUNTIME.replace("users:x:100:sonic", "users:x:100:")):
            with self.subTest(changed=changed), self.assertRaisesRegex(ValueError, "existing groups"):
                self.validate(runtime=changed, debug=changed)

    def test_extra_or_wrong_ssh_group_is_rejected(self):
        for changed in (RUNTIME + "unexpected:x:105:\n", RUNTIME.replace("_ssh:x:104:", "_ssh:x:105:")):
            with self.subTest(changed=changed), self.assertRaisesRegex(ValueError, "existing groups"):
                self.validate(runtime=changed, debug=changed)

    def test_debug_cannot_drop_or_add_runtime_group(self):
        for changed in (BASE, RUNTIME + "unexpected:x:105:\n"):
            with self.subTest(changed=changed), self.assertRaisesRegex(ValueError, "runtime groups"):
                self.validate(debug=changed)

    def test_duplicate_names_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "duplicate group names"):
            self.validate(runtime=RUNTIME + "users:x:100:sonic\n")

    def test_permissions_are_preserved(self):
        with self.assertRaisesRegex(ValueError, "ownership or permissions"):
            self.validate(mode=0o666)


class GearboxEnvironment(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.path = Path(self.temporary.name) / "image.env"
        self.lines = "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=review-version\n"
        self.policy = {"image_validation": {"environment": {"DEBIAN_FRONTEND": "noninteractive"},
                                            "other_guard": "retained"}}

    def validate(self, lines=None, version="review-version", policy=None):
        self.path.write_text(self.lines if lines is None else lines)
        return gbsyncd_image.declared_image_policy(
            self.policy if policy is None else policy, version, self.path)

    def test_actual_declared_values_keep_policy_immutable(self):
        prior = copy.deepcopy(self.policy)
        result = self.validate()
        self.assertEqual(result["image_validation"]["environment"],
                         {"DEBIAN_FRONTEND": "noninteractive", "IMAGE_VERSION": "review-version"})
        self.assertEqual(result["image_validation"]["other_guard"], "retained")
        self.assertEqual(self.policy, prior)

    def test_wrong_extra_missing_and_duplicate_environment_are_rejected(self):
        for lines in (self.lines.replace("review-version", "other"),
                      self.lines.replace("noninteractive", "interactive"),
                      self.lines + "UNDECLARED=value\n",
                      self.lines + "IMAGE_VERSION=review-version\n",
                      "DEBIAN_FRONTEND=noninteractive\n"):
            with self.subTest(lines=lines), self.assertRaises(ValueError):
                self.validate(lines=lines)

    def test_wrong_or_missing_receipt_version_is_rejected(self):
        for version in ("different", "", None, "review-version\nextra"):
            with self.subTest(version=version), self.assertRaises(ValueError):
                self.validate(version=version)

    def test_changed_public_policy_is_rejected(self):
        changed = copy.deepcopy(self.policy)
        changed["image_validation"]["environment"]["UNDECLARED"] = "value"
        with self.assertRaises(ValueError):
            self.validate(policy=changed)


class GearboxVimReceipt(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Execute the actual pure receipt functions without importing the
        # container-only Redis/Supervisor dependencies or running the fixture.
        path = Path(__file__).resolve().parents[3] / "platform/vs/docker-gbsyncd-vs/bazel/runtime_probe.py"
        tree = ast.parse(path.read_text())
        functions = [node for node in tree.body if isinstance(node, ast.FunctionDef)
                     and node.name in ("require", "full_vim_receipt")]
        assert len(functions) == 2
        namespace = {}
        exec(compile(ast.Module(body=functions, type_ignores=[]), str(path), "exec"), namespace)
        cls.receipt = staticmethod(namespace["full_vim_receipt"])

    def test_actual_command_result_becomes_safe_public_evidence(self):
        result = {"returncode": 0, "stdout": "VIM - Vi IMproved 9.1\nHuge version without GUI",
                  "stderr": "", "argv": ["/usr/bin/vim", "--version"]}
        with self.assertRaises(PublicArtifactError):
            safe_json(result)
        public = self.receipt(result)
        self.assertEqual(public, {"returncode": 0, "huge_version": True})
        safe_json(public)

    def test_wrong_variant_and_failed_command_are_rejected(self):
        for code, stdout in ((0, "Tiny version"), (0, "Small version"), (0, ""), (1, "Huge version")):
            with self.subTest(code=code, stdout=stdout), self.assertRaises(AssertionError):
                self.receipt({"returncode": code, "stdout": stdout})


class GearboxBuildEvidenceTest(unittest.TestCase):
    def test_installer_compares_dash_data_against_the_selected_source_receipt(self):
        """A second DASH generation can change pipeline JSON even if the same libraries are installed."""
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            pipeline = "etc/dash/dash_pipeline_p4rt.json"
            license_file = "usr/share/doc/dash-sai/LICENSE"
            changelog = "usr/share/doc/p4lang-bmv2/changelog.Debian.gz"
            library = "usr/lib/x86_64-linux-gnu/libsai.so"
            soname = "usr/lib/x86_64-linux-gnu/libsai.so.0"
            regular = {"kind": "file", "mode": 0o644, "uid": 0, "gid": 0, "sha256": "1" * 64, "size": 173}
            source = {pipeline: regular, license_file: regular, changelog: regular,
                      library: dict(regular, elf_machine=62), soname: {"kind": "symlink", "mode": 0, "linkname": library}}
            installed = dict(source)
            del installed[changelog]
            installed[soname] = dict(source[soname], mode=0o777)
            contract = directory / "source-contract.json"
            contract.write_text(json.dumps({"packages": [{"required_paths": {pipeline: "file"}}]}))
            receipt = {"contract_sha256": gbsyncd_image.check.sha(contract), "payload": {"sha256": "2" * 64},
                       "packages": [{"files": source}]}
            (directory / "source-receipt.json").write_text(json.dumps(receipt))
            (directory / "source-excludes.txt").write_text("usr/share/doc/*\n")
            (directory / "source-includes.txt").write_text("usr/share/doc/*/LICENSE\n")
            with patch.object(gbsyncd_image, "check_p4_inputs"):
                for variant in ("runtime", "debug"):
                    with self.subTest(variant=variant):
                        gbsyncd_image.verify_installer_source_payload(directory, variant, installed, "2" * 64)
                        changed = installed | {pipeline: dict(regular, sha256="3" * 64)}
                        with self.assertRaisesRegex(ValueError, pipeline):
                            gbsyncd_image.verify_installer_source_payload(directory, variant, changed, "2" * 64)
                with self.assertRaisesRegex(ValueError, "built runtime TAR"):
                    gbsyncd_image.verify_installer_source_payload(directory, "runtime", installed, "4" * 64)

    def test_exported_packages_and_archives_match_the_validated_layouts(self):
        """Validation must inspect the same build's payload as the published archives."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            workspace = root / "checkout"
            workspace.mkdir()
            directory = root / "artifacts"
            directory.mkdir()
            outputs = {target: "outputs/" + name for name, target in gbsyncd_image.TARGETS.items()}
            layouts = [gbsyncd_image.BASE, *[gbsyncd_image.OWNER + name for name in gbsyncd_image.IMAGES]]
            outputs.update({target: "outputs/layout-" + str(index) for index, target in enumerate(layouts)})
            bazel = root / "bazel"
            bazel.write_text("#!" + sys.executable + "\n" + f"outputs = {outputs!r}\n" + r"""
from pathlib import Path
import sys
if sys.argv[1] == 'build':
    counter = Path('generation')
    generation = int(counter.read_text()) + 1 if counter.exists() else 1
    counter.write_text(str(generation))
    # Re-execution may change generated payloads even for unchanged sources.
    # Image archives depend on their layouts, which are built transitively.
    for output in outputs.values():
        path = Path(output)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('payload from build ' + str(generation))
elif sys.argv[1] == 'cquery':
    path = outputs[sys.argv[-1]]
    if not Path(path).is_file():
        raise SystemExit('output was not built')
    print(path)
else:
    raise SystemExit('unexpected operation')
""")
            bazel.chmod(0o755)
            execute = gbsyncd_image.command_log.execute

            def command(argv, *args, **kwargs):
                if argv[0] == "make":
                    return ""
                return execute(argv, *args, **kwargs)

            with patch.object(gbsyncd_image, "ROOT", workspace), \
                    patch.object(gbsyncd_image, "clean_source"), \
                    patch.object(gbsyncd_image, "identity", return_value={"revision": "fixture"}), \
                    patch.object(gbsyncd_image.platform, "machine", return_value="x86_64"), \
                    patch.object(gbsyncd_image.platform, "freedesktop_os_release", return_value={"VERSION_CODENAME": "trixie"}), \
                    patch.object(gbsyncd_image.command_log, "execute", side_effect=command), \
                    patch.object(gbsyncd_image.bazel_commands, "audit_targets", return_value={}), \
                    patch.object(gbsyncd_image.resolution, "collect", return_value={}), \
                    contextlib.redirect_stdout(io.StringIO()):
                gbsyncd_image.build_images(SimpleNamespace(bazel=str(bazel), image_version="fixture"), directory)
            receipt = json.loads((directory / "build-receipt.json").read_text())
            source_payload = (directory / "source-runtime.tar").read_bytes()
            for image in gbsyncd_image.IMAGES:
                with self.subTest(image=image):
                    self.assertEqual((directory / (image + ".gz")).read_bytes(), source_payload)
                    self.assertEqual((workspace / receipt["layouts"][image]).read_bytes(), source_payload)
            self.assertEqual((workspace / receipt["base_layout"]).read_bytes(), source_payload)


if __name__ == "__main__":
    unittest.main()
