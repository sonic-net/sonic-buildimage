"""Render declared SONiC package manifests and one shared image version."""

import argparse
import copy
import json
from pathlib import Path

from tools.bazel.oci.manifest_labels import component_labels


def render(spec, image, version, debug=False):
    if spec.get("schema") != 1:
        raise ValueError("unsupported VS metadata schema")
    if not version or any(c in version for c in "\r\n\0$"):
        raise ValueError("image version must be nonempty and contain no shell substitutions")
    manifest = copy.deepcopy(spec["containers"][image]["manifest"])
    if debug:
        package_version = manifest["package"]["version"]
        manifest["package"]["version"] = package_version + ("." if "+" in package_version else "+") + "dbg"
    return manifest, "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=" + version + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--spec", type=Path)
    parser.add_argument("--image")
    parser.add_argument("--image-version")
    parser.add_argument("--component-receipt", type=Path)
    parser.add_argument("--component-package", action="append", default=[])
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--include-system-eventd", choices=("y", "n"))
    for name in ("manifest", "environment", "version"):
        parser.add_argument("--" + name, type=Path)
    args = parser.parse_args()
    if args.component_receipt:
        args.manifest.write_text(component_labels(args.component_receipt.read_text(), args.component_package))
        return
    manifest, environment = render(json.loads(args.spec.read_text()), args.image, args.image_version, args.debug)
    if args.include_system_eventd is not None:
        environment = "DEBIAN_FRONTEND=noninteractive\nINCLUDE_SYSTEM_EVENTD=" + args.include_system_eventd + "\n"
    args.manifest.write_text(json.dumps(manifest, indent=2) + "\n")
    args.environment.write_text(environment)
    args.version.write_text(args.image_version + "\n")


if __name__ == "__main__":
    main()
