"""Hand off each configured component once to checks outside its installed containers."""

import argparse
from importlib import import_module
import json
import os
from pathlib import Path
import re

from tools.bazel.ci.artifact_validation import require, sha


COMPONENTS = Path(__file__).with_name("components")


def components():
    return {p.stem: import_module("tools.bazel.ci.components." + p.stem)
            for p in sorted(COMPONENTS.glob("*.py")) if not p.name.startswith("_")}


def selected(families):
    result = {}
    for name, module in components().items():
        images = sorted(families.keys() & set(module.IMAGES))
        if images:
            debug = any("debug" in families[image]["members"] for image in images)
            result[name] = (module, images, debug)
    return result


def labels(module, debug):
    return {**module.RUNTIME, **(module.DEBUG if debug else {})}


def targets(families):
    return {label for module, _, debug in selected(families).values()
            for label in labels(module, debug).values()}


def publish(root, families, outputs, revision, architecture, settings):
    """Copy files from the same configured outputs that yielded the installed OCI layouts."""
    from tools.bazel.ci.vs_validation_bundles import copy_output

    for name, (module, images, debug) in selected(families).items():
        bundle = root / name
        bundle.mkdir()
        files = {}
        for filename, label in labels(module, debug).items():
            source = outputs[label]
            copy_output(source, bundle / filename)
            files[filename] = {"target": label, "sha256": sha(source), "size": source.stat().st_size}
        manifest = {"component": name, "revision": revision, "architecture": architecture, "debug": debug,
                    "images": images, "build_settings": settings, "files": files}
        (bundle / "manifest.json").write_text(json.dumps(manifest, sort_keys=True) + "\n")


def verify_inputs(name, directory, revision, debug):
    module = components()[name]
    manifest = json.loads((directory / "manifest.json").read_bytes())
    expected = labels(module, debug)
    require(manifest["component"] == name and manifest["revision"] == revision and
            re.fullmatch(r"[0-9a-f]{40}", revision) and manifest["architecture"] == "amd64" and
            manifest["debug"] is debug, "VS component revision or configuration differs: " + name)
    require(bool(manifest["images"]) and set(manifest["images"]) <= set(module.IMAGES) and
            bool(manifest["build_settings"].get("//tools/bazel/oci:image_version")),
            "Missing VS configuration or consuming image: " + name)
    require(set(manifest["files"]) == expected.keys() and
            {p.name for p in directory.iterdir()} == {"manifest.json", *expected},
            "Unexpected or missing VS component files: " + name)
    for filename, label in expected.items():
        path = directory / filename
        require(path.is_file() and not path.is_symlink() and path.stat().st_size > 0 and
                manifest["files"][filename] == {"target": label, "size": path.stat().st_size, "sha256": sha(path)},
                "VS component output differs: " + filename)
    return module


def matrix(directory, steps):
    result = []
    selection = json.loads((directory.parent / "selection.json").read_bytes())
    require({p.name for p in directory.iterdir()} == set(selection["components"]), "Incomplete VS component export")
    registered = components()
    for name in selection["components"]:
        module = registered[name]
        bundle = directory / name
        artifact_id = steps["component_" + name]["outputs"]["artifact-id"]
        require(re.fullmatch(r"[1-9][0-9]*", artifact_id), "Missing VS artifact ID: " + name)
        manifest = json.loads((bundle / "manifest.json").read_bytes())
        if not getattr(module, "REUSABLE_WORKFLOW", False):
            result.append({"component": name, "artifact_id": artifact_id, "debug": "y" if manifest["debug"] else "n"})
    return {"include": result}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for mode in ("matrix", "verify", "test"):
        cmd = commands.add_parser(mode)
        cmd.add_argument("--directory", type=Path, required=True)
        if mode != "matrix":
            cmd.add_argument("--component", required=True)
            cmd.add_argument("--revision", required=True)
            cmd.add_argument("--debug", choices=("y", "n"), required=True)
    args = parser.parse_args()
    if args.command == "matrix":
        print(json.dumps(matrix(args.directory, json.loads(os.environ["UPLOAD_STEPS"])), separators=(",", ":")))
    else:
        debug = args.debug == "y"
        module = verify_inputs(args.component, args.directory, args.revision, debug)
        if args.command == "test":
            module.verify(args.directory.resolve(), debug=debug)


if __name__ == "__main__":
    main()
