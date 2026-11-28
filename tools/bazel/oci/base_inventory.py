"""Generate a checked OCI base inventory for package selection and assembly."""

import argparse
import json
from pathlib import Path

from tools.bazel.ci.artifact_validation import require
from tools.bazel.oci.oci_inventory import apply_layer
from tools.bazel.oci.oci_layout import validate_layout


def read(base, architecture, *, inventory=None, static_i386=None):
    platform = "linux/" + architecture
    layout = validate_layout(base, platform, verify_layer_digest=inventory is None)
    digest = layout.descriptor["digest"]
    if inventory is not None:
        value = json.loads(inventory.read_text())
        require(value["manifest_digest"] == digest and value["platform"] == platform
                and value["static_i386"] == (static_i386 or {}), "OCI base inventory differs from its input")
        return value
    files = {}
    status_path = "var/lib/dpkg/status"
    for layer in layout.layers:
        apply_layer(layer, files, preserve_symlink_mode=True, static_i386=static_i386,
                    capture_contents=(status_path,))
    status = files.get(status_path, {}).pop("contents", None)
    return {
        "manifest_digest": digest,
        "platform": platform,
        "static_i386": static_i386 or {},
        "dpkg_status": status,
        "files": files,
    }


def installed_files(files):
    return {path: {**item, "mode": 0o777} if item["kind"] == "symlink" else item
            for path, item in files.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True, type=Path)
    parser.add_argument("--architecture", required=True)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    args.out.write_text(json.dumps(read(args.base, args.architecture), sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
