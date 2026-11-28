"""Sysmgr owner inputs shared by the VS image and its native namespace tests."""
import hashlib
import json

IMAGES = ("docker-sysmgr",)
REUSABLE_WORKFLOW = True
RUNTIME = {
    "sysmgr.tar": "@sonic_sysmgr//:sysmgr_pkg",
    "sysmgr-packages.json": "@sonic_sysmgr//:runtime_packages.json",
    "libswsscommon.tar": "@sonic_swss_common//dist:libswsscommon_pkg",
    # Test fixture includes the VS libnl and libyang runtime libraries alongside Debian helpers.
    "swss-test-runtime.tar": "@sonic_swss_common//tools/bazel/runtime:test_runtime",
    "libprotobuf.tar": "@sonic_protobuf//:libprotobuf_pkg",
}
DEBUG = {"sysmgr-symbols.tar": "@sonic_sysmgr//:sysmgr_debug_pkg"}


def verify(directory, *, debug):
    # The reusable job runs the package, native and isolated service tests with these inputs.
    data = json.loads((directory / "sysmgr-packages.json").read_bytes())
    digest = hashlib.sha256((directory / "sysmgr.tar").read_bytes()).hexdigest()
    if data.get("architecture") != "amd64" or data["packages"]["sysmgr"]["payload_sha256"] != digest:
        raise ValueError("VS Sysmgr runtime TAR differs from its source package metadata")
    return {"runtime_sha256": digest}
