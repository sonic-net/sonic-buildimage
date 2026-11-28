"""Exercise inherited file, account, stage and owner checks independently from service startup."""

import copy
import gzip
import hashlib
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.bazel.ci import vs_family_parent as parent
from tools.bazel.ci import vs_probe_policy as policy


def regular(digest="original"):
    return {"kind": "file", "uid": 0, "gid": 0, "mode": 0o644, "sha256": digest}


class VSFamilyParentTest(unittest.TestCase):
    def test_gnmi_accepts_declared_make_labels_and_rejects_an_undeclared_environment_change(self):
        with TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / "runtime.make.manifest.json").write_text("{}")
            (directory / "runtime.make.components.labels").write_text("libswsscommon=1.0\n")
            fixtures = {"runtime.make.components.labels", "runtime.make.manifest.json"}
            base = {"config": {"config": {"Env": ["LANG=C"], "Labels": {"parent": "yes"}}}}
            actual = {"config": {"config": {
                "Entrypoint": ["/usr/local/bin/supervisord"],
                "Env": ["LANG=C", "DEBIAN_FRONTEND=noninteractive", "IMAGE_VERSION=r1"],
                "Labels": {"parent": "yes", "libswsscommon": "1.0", "Tag": "r1", "com.azure.sonic.manifest": "{}"},
            }}}
            parent.verify_service_config(directory, "gnmi", "runtime", base, actual, fixtures)
            actual["config"]["config"]["Env"].append("UNDECLARED=1")
            with self.assertRaisesRegex(ValueError, "undeclared image environment"):
                parent.verify_service_config(directory, "gnmi", "runtime", base, actual, fixtures)

    def test_unlisted_make_inputs_cannot_change_the_framework_parent_contract(self):
        """A leftover environment file for another service cannot authorize extra runtime or debug values."""
        with TemporaryDirectory() as temporary:
            directory = Path(temporary)
            base = {"config": {"config": {"Env": ["PATH=/usr/bin"], "WorkingDir": "/"}}}
            for variant in ("runtime", "debug"):
                with self.subTest(variant=variant):
                    env = variant + ".make.image.env"
                    (directory / env).write_text("DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=HEAD.0-c079810a\n")
                    fixtures = {variant + ".make.manifest.json"}
                    current = copy.deepcopy(base)
                    rows = current["config"]["config"]["Env"]
                    rows.append("DEBIAN_FRONTEND=noninteractive")
                    parent.verify_service_config(directory, "mgmt_framework", variant, base, current, fixtures)
                    rows.append("IMAGE_VERSION=HEAD.0-c079810a")
                    with self.assertRaisesRegex(ValueError, "inherited configuration"):
                        parent.verify_service_config(directory, "mgmt_framework", variant, base, current, fixtures)

    def test_p4rt_debug_inherits_runtime_and_adds_its_symbols_as_a_layer(self):
        # The producer records only parent, runtime and debug OCI layouts.
        group = b"root:x:0:\n_ssh:x:104:\n"
        runtime = {
            "files": {"etc/group": regular(hashlib.sha256(group).hexdigest()), "usr/local/bin/p4rt": regular()},
            "layers": ["base", "p4rt"],
            "config": {"rootfs": {"diff_ids": ["base", "p4rt"]}},
        }
        runtime["files"].update({
            path: {"kind": "symlink", "linkname": dest}
            for path, dest in (("bin", "usr/bin"), ("sbin", "usr/sbin"), ("lib", "usr/lib"), ("var/run", "/run"))
        })
        base = copy.deepcopy(runtime)
        base["layers"].pop()
        base["config"]["rootfs"]["diff_ids"].pop()
        debug = copy.deepcopy(runtime)
        debug["layers"].append("symbols")
        debug["config"]["rootfs"]["diff_ids"].append("symbols")
        debug["files"]["usr/bin/gdb"] = regular("tool")
        bundle = {"image": "docker-sonic-p4rt", "fixtures": {}}
        owners = {"owners": {"p4rt.debug-tools.tar": {"usr/bin/gdb": regular("tool")}}}
        with TemporaryDirectory() as temporary:
            directory = Path(temporary)
            for stage, image in (("parent", base), ("runtime", runtime)):
                (directory / (stage + ".oci.json.gz")).write_bytes(gzip.compress(json.dumps(image).encode()))
            for stage in ("parent", "debug"):
                (directory / (stage + ".group")).write_bytes(group)
            (directory / "parent.json").write_text(json.dumps({"image": "docker-config-engine-trixie", "diff_ids": ["base"]}))
            parent.verify_parent(directory, bundle, "p4rt", "debug", debug, owners)
            debug["layers"][1] = "other-runtime"
            with self.assertRaisesRegex(ValueError, "does not inherit"):
                parent.verify_parent(directory, bundle, "p4rt", "debug", debug, owners)
            debug["layers"][1] = "p4rt"
            debug["files"]["usr/bin/gdb"] = regular("changed")
            with self.assertRaisesRegex(ValueError, "debug tools changed"):
                parent.verify_parent(directory, bundle, "p4rt", "debug", debug, owners)

    def test_only_the_service_owners_declared_replacement_may_override_its_parent(self):
        original = {"files": {"etc/debian_version": regular(), "etc/rsyslog.conf": regular()}}
        replacement = regular("owner")
        image = {"files": {**original["files"], "etc/rsyslog.conf": replacement}}
        owners = {"config-files.tar": {"etc/rsyslog.conf": replacement}}
        parent.verify_files("gnmi", "runtime", original, image, owners)
        image["files"]["etc/debian_version"] = regular("modified")
        with self.assertRaisesRegex(ValueError, "etc/debian_version"):
            parent.verify_files("gnmi", "runtime", original, image, owners)
        image["files"] = {**original["files"], "etc/rsyslog.conf": regular("unowned")}
        with self.assertRaisesRegex(ValueError, "etc/rsyslog.conf"):
            parent.verify_files("gnmi", "runtime", original, image, owners)

    def test_debug_replaces_parent_vim_links_only_with_the_declared_tool_payload(self):
        before = {"files": {"etc/alternatives/vim": {"kind": "symlink", "linkname": "/usr/bin/vim.tiny"}}}
        expected = {"kind": "symlink", "uid": 0, "gid": 0, "mode": 0o777, "linkname": "/usr/bin/vim.basic"}
        after = {"files": {"etc/alternatives/vim": expected.copy()}}
        owners = {"config-files.tar": {}, "debug-tools.tar": {"etc/alternatives/vim": expected}}
        parent.verify_files("gnmi", "debug", before, after, owners)
        with self.assertRaisesRegex(ValueError, "alternatives/vim"):
            parent.verify_files("gnmi", "runtime", before, after, owners)
        after["files"]["etc/alternatives/vim"]["linkname"] = "/bin/false"
        with self.assertRaisesRegex(ValueError, "alternatives/vim"):
            parent.verify_files("gnmi", "debug", before, after, owners)

    def test_database_refuses_existing_parent_dpkg_overrides(self):
        original = {"files": {parent.STATOVERRIDE: regular()}}
        with self.assertRaisesRegex(ValueError, "statoverride"):
            parent.verify_files("database", "runtime", original, original, {})

    def test_the_expected_parent_blob_and_diff_id_must_both_be_preserved(self):
        before = {"layers": [{"digest": "a", "size": 1}], "config": {"rootfs": {"diff_ids": ["x"]}}}
        after = copy.deepcopy(before)
        after["layers"].append({"digest": "b", "size": 1})
        after["config"]["rootfs"]["diff_ids"].append("y")
        parent.ancestry(before, after, "service")
        for key in ("blob", "diff_id"):
            mutation = copy.deepcopy(after)
            if key == "blob":
                mutation["layers"][0]["digest"] = "z"
            else:
                mutation["config"]["rootfs"]["diff_ids"][0] = "z"
            with self.subTest(key=key), self.assertRaisesRegex(ValueError, "inherit"):
                parent.ancestry(before, mutation, "service")

    def test_gearbox_adds_ssh_in_runtime_and_rejects_a_lost_parent_group(self):
        with TemporaryDirectory() as temporary:
            directory = Path(temporary)
            def account(variant, text):
                path = directory / (variant + ".group")
                path.write_text(text)
                return {"etc/group": regular(hashlib.sha256(path.read_bytes()).hexdigest())}
            base = account("parent", "root:x:0:\nusers:x:100:\n")
            runtime = account("runtime", "root:x:0:\n_ssh:x:104:\nusers:x:100:\n")
            parent.check_accounts(directory, "gbsyncd", "runtime", base, runtime)
            debug = account("debug", "root:x:0:\n_ssh:x:104:\n")
            with self.assertRaisesRegex(ValueError, "inherited groups"):
                parent.check_accounts(directory, "gbsyncd", "debug", base, debug)

    def test_debug_parent_tools_are_collected_only_for_enabled_companions(self):
        for image, family in policy.FAMILIES.items():
            if family in {"frr", "teamd"}:
                continue
            with self.subTest(image=image):
                selected = policy.preparation_targets(image, True)
                original = policy.preparation_targets(image, False)
                tool = parent.TOOL_TAR.get(family, "debug-tools.tar")
                if tool in selected:
                    self.assertNotIn(tool, original)
                if family in parent.CONFIG_TAR:
                    self.assertIn(parent.CONFIG_TAR[family], original)


if __name__ == "__main__":
    unittest.main()
