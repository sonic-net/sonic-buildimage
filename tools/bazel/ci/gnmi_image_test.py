"""Reject undeclared OCI settings and account changes before loaded acceptance."""

import copy
import json
import unittest

from tools.bazel.ci import gnmi_image


class GnmiImageValidationTest(unittest.TestCase):
    def setUp(self):
        self.parent = {"Env": ["PATH=/usr/bin", "DEBIAN_FRONTEND=noninteractive"],
                       "Entrypoint": ["/parent"], "Cmd": ["parent-argument"],
                       "User": "", "WorkingDir": "/", "Labels": {"parent": "retained"}}
        self.manifest = {"name": "docker-sonic-gnmi"}
        self.runtime = copy.deepcopy(self.parent)
        self.runtime.pop("Cmd")
        self.runtime["Entrypoint"] = ["/usr/local/bin/supervisord"]
        self.runtime["Env"].append("IMAGE_VERSION=fixture")
        self.runtime["Labels"]["com.azure.sonic.manifest"] = json.dumps(self.manifest)
        self.accounts = {"etc/group": {"kind": "file", "uid": 0, "gid": 0, "mode": 0o644},
                         "etc/passwd": {"sha256": "original-passwd"},
                         "etc/gshadow": {"sha256": "original-gshadow"}}

    def verify(self, config):
        gnmi_image.verify_config(config, self.parent, self.manifest, "fixture", runtime=True)

    def test_declared_runtime_and_debug_configuration(self):
        self.verify(self.runtime)
        debug = copy.deepcopy(self.runtime)
        manifest = {"name": "docker-sonic-gnmi-dbg"}
        debug["Labels"]["com.azure.sonic.manifest"] = json.dumps(manifest)
        gnmi_image.verify_config(debug, self.runtime, manifest, "fixture", runtime=False)

    def test_reject_undeclared_settings(self):
        for key, value in {"User": "123", "WorkingDir": "/changed", "Cmd": ["extra"],
                           "ExposedPorts": {"9999/tcp": {}}, "StopSignal": "SIGKILL"}.items():
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.verify({**self.runtime, key: value})

    def test_reject_extra_duplicate_environment_and_labels(self):
        for extra in ("UNDECLARED=yes", "IMAGE_VERSION=fixture"):
            with self.subTest(extra=extra), self.assertRaises(ValueError):
                self.verify({**self.runtime, "Env": [*self.runtime["Env"], extra]})
        changed = copy.deepcopy(self.runtime)
        changed["Labels"]["extra"] = "unexpected"
        with self.assertRaises(ValueError):
            self.verify(changed)

    def test_group_addition_and_existing_group(self):
        rows = ["root:x:0:", "users:x:100:"]
        expected = [*rows, "_ssh:x:104:"]
        gnmi_image.verify_groups(self.accounts, self.accounts, rows, expected)
        gnmi_image.verify_groups(self.accounts, self.accounts, expected, expected)

    def test_reject_group_row_changes_and_duplicates(self):
        rows = ["root:x:0:", "users:x:100:"]
        for changed in (["users:x:100:", "root:x:0:", "_ssh:x:104:"],
                        ["root:x:0:member", "users:x:100:", "_ssh:x:104:"],
                        [*rows, "root:x:1:", "_ssh:x:104:"]):
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                gnmi_image.verify_groups(self.accounts, self.accounts, rows, changed)

    def test_reject_account_metadata_and_shadow_changes(self):
        rows = ["root:x:0:"]
        for key, value in (("mode", 0o600), ("uid", 1), ("gid", 1), ("kind", "link")):
            changed = copy.deepcopy(self.accounts)
            changed["etc/group"][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                gnmi_image.verify_groups(self.accounts, changed, rows, [*rows, "_ssh:x:104:"])
        for name in ("etc/passwd", "etc/gshadow"):
            changed = copy.deepcopy(self.accounts)
            changed[name]["sha256"] = "changed"
            with self.subTest(name=name), self.assertRaises(ValueError):
                gnmi_image.verify_groups(self.accounts, changed, rows, [*rows, "_ssh:x:104:"])


if __name__ == "__main__":
    unittest.main()
