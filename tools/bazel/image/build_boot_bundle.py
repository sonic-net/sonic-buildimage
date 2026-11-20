#!/usr/bin/env python3
"""Build the installer-selected offline OCI bundle with Bazel."""
import argparse
from collections import Counter, defaultdict
from contextlib import contextmanager
from dataclasses import dataclass
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.build_helpers import cache_options, export_archive, remote_cache_options
from tools.bazel.ci.build import execution_root_from_bep, resolve_output
from tools.bazel.ci.cache_reuse import json_records
from tools.bazel.oci.oci_layout import validate_layout
from tools.bazel.image.boot_image_bundle import DIGEST, blob_name, json_bytes

REPO = "sonic_boot_bundle_inputs"
RULES = "@@//tools/bazel/image:boot_bundle.bzl"


@dataclass
class Image:
    source: Path
    tags: list[str]
    check_label: bool
    layout: Path | None = None
    archive: str = ""
    info: dict | None = None


def starlark_call(rule, **kwargs):
    # Every attribute here is a string, integer or list thereof.
    attributes = [f"    {name} = {json.dumps(value)},\n" for name, value in kwargs.items()]
    return rule + "(\n" + "".join(attributes) + ")\n"


def link_input(directory, name, source):
    destination = directory / name
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.symlink_to(source.resolve(strict=True))
    return name


def sidecar_for_archive(source, platform):
    # Make may symlink its archive into target/. Receipts live alongside that
    # logical filename, even when the archive itself is held in another cache.
    store = source.parent / ".boot-image-layouts"
    receipt = store / (source.name + ".json")
    if receipt.is_file():
        try:
            record = json.loads(receipt.read_bytes())
            index = record["index"]
            if DIGEST.fullmatch("sha256:" + index):
                candidate = store / index
                # A pruned sidecar must still build from the authoritative archive.
                # Check its small metadata and presence of all the referenced blobs.
                validate_layout(candidate, platform, verify_layer_digest=False)
                return receipt, candidate
        except (OSError, ValueError, KeyError, TypeError):
            pass
    return None, None


def write_archive_package(package, images, platform):
    package.mkdir()
    rules = [f'load("{RULES}", "boot_archive")', 'package(default_visibility=["//visibility:public"])']
    sources = {}
    filenames = Counter()
    archives = []
    for image in images:
        if image.source.is_dir():
            image.layout = image.source
            continue
        logical_path = image.source.absolute()
        if logical_path in sources:
            image.archive, image.layout = sources[logical_path]
            continue
        # Stable target and exec paths preserve each archive's action across tag
        # changes, optional images, checkout locations and different CI runners.
        key = hashlib.sha256(image.source.name.encode()).hexdigest()
        occurrence = filenames[key]
        filenames[key] += 1
        image.archive = "source_" + key + (f"_{occurrence}" if occurrence else "")
        prefix = "inputs/" + image.archive
        attrs = {"name": image.archive, "src": link_input(package, prefix + "/archive", image.source)}
        receipt, candidate = sidecar_for_archive(image.source, platform)
        image.layout = candidate
        if candidate:
            attrs["receipt"] = link_input(package, prefix + "/receipt", receipt)
            attrs["index"] = link_input(package, prefix + "/index", candidate / "index.json")
        sources[logical_path] = (image.archive, image.layout)
        rules.append(starlark_call("boot_archive", **attrs))
        archives.append(":" + image.archive)
    rules.append(starlark_call("filegroup", name="metadata", srcs=archives, output_group="metadata"))
    (package / "BUILD.bazel").write_text("\n".join(rules))
    return bool(archives)


class BundlePackage:
    def __init__(self, directory):
        self.directory = directory
        directory.mkdir()
        self.rules = [f'load("{RULES}", "boot_archive_members", "boot_bundle", "boot_layer")']
        self.files = {}
        self.aliases = Counter()
        self.members = defaultdict(set)
        self.layer_targets = {}
        self.verified = set()
        self.configs = set()
        self.metadata = set()
        self.spec = []

    def file(self, source, base):
        # Shared OCI layouts normally reference the same blob pool. A separately
        # supplied file claiming the same digest must pass its own content check.
        stat = source.stat()
        key = (base, stat.st_dev, stat.st_ino)
        if key not in self.files:
            alias = self.aliases[base]
            self.aliases[base] += 1
            self.files[key] = link_input(self.directory, base + (f"_{alias}" if alias else ""), source)
        return self.files[key]

    def member(self, image, kind, digest):
        blob_name(digest)
        member = f"{image.archive}_files/{kind}/" + digest[7:]
        self.members[image.archive].add(member)
        return member

    def image(self, image, platform):
        if image.layout:
            # Layer SHA and DiffID verification run in declared boot_layer actions.
            native = validate_layout(image.layout, platform, verify_layer_digest=False)
            cfg_digest = native.manifest["config"]["digest"]
            cfg = self.file(image.layout / blob_name(cfg_digest), "configs/" + cfg_digest[7:])
            self.metadata.update(self.file(path, "metadata/" + name) for path, name in (
                (image.layout / "oci-layout", "oci-layout"),
                (image.layout / "index.json", "index"),
                (image.layout / blob_name(native.descriptor["digest"]), native.descriptor["digest"][7:])))
            return self.declare_image(image, cfg_digest, cfg, native.manifest["layers"],
                                 native.config["rootfs"]["diff_ids"], [], native.layers)
        info = image.info
        cfg_digest = info["config"]
        cfg = self.member(image, "configs", cfg_digest)
        return self.declare_image(image, cfg_digest, cfg, info["layers"], info["diff_ids"], info["original_tags"], None)

    def declare_image(self, image, cfg_digest, cfg, layers, diff_ids, original_tags, paths):
        self.configs.add(cfg)
        for i, (desc, diff) in enumerate(zip(layers, diff_ids, strict=True)):
            digest = desc["digest"]
            blob_name(digest)
            blob_name(diff)
            key = (digest, desc["size"], desc["mediaType"], diff)
            if paths is None and key in self.verified:
                # Normalization derived the encoded digest from the archive bytes.
                # A matching native blob or another normalized copy is sufficient.
                continue
            src = (self.file(paths[i], "blobs/" + digest[7:]) if paths is not None
                   else self.member(image, "layers", digest))
            if (key, src) not in self.layer_targets:
                prefix = "layer_" + digest[7:]
                number = self.aliases[prefix]
                self.aliases[prefix] += 1
                name = prefix + (f"_{number}" if number else "")
                self.layer_targets[(key, src)] = name
                self.rules.append(starlark_call("boot_layer", name=name, src=src, source_digest=digest,
                                       source_size=desc["size"], diff_id=diff, media_type=desc["mediaType"]))
            self.verified.add(key)
        return {"config": cfg_digest, "tags": image.tags, "layers": layers,
                "original_tags": original_tags, "check_label": image.check_label}

    def select(self, images, platform):
        # Prefer native blobs for shared layers. Preserve the caller's order for
        # identical tag lists, as the stock packager chooses the first matching gzip.
        self.spec = [None] * len(images)
        for index, image in sorted(enumerate(images), key=lambda item: item[1].layout is None):
            self.spec[index] = self.image(image, platform)

    def finish(self, platform, has_archives):
        for owner, files in sorted(self.members.items()):
            self.rules.append(starlark_call("boot_archive_members", name="project_" + owner,
                                   src="//archives:" + owner, outs=sorted(files)))
        if has_archives:
            self.metadata.add("//archives:metadata")
        self.rules.append(starlark_call("boot_bundle", name="boot", platform=platform, spec="images.json",
                               metadata=sorted(self.metadata), configs=sorted(self.configs),
                               layers=sorted(self.layer_targets.values())))
        images = sorted(self.spec, key=lambda image: image["tags"])
        (self.directory / "images.json").write_bytes(json_bytes({"images": images}) + b"\n")
        (self.directory / "BUILD.bazel").write_text("\n".join(self.rules))


def build_target(repository, package, target, expected, evidence, options):
    events = repository.parent / (package + ".bep")
    log = repository.parent / (package + ".spawns")
    command = [os.environ.get("BAZEL", "bazel"), "build", *options,
               "--remote_download_outputs=toplevel", "--inject_repository=" + REPO + "=" + str(repository),
               "--build_event_json_file=" + str(events)]
    if evidence is not None:
        command.append("--execution_log_json_file=" + str(log))
    command.append("@" + REPO + "//" + package + ":" + target)
    start = time.monotonic()
    result = subprocess.run(command)
    if result.returncode:
        # Caller options may include private cache headers. Bazel already printed
        # the phase diagnostics, so avoid repeating credentials in a traceback.
        raise SystemExit(f"Bazel {package} build failed (exit {result.returncode})")
    if evidence is not None:
        actions = defaultdict(Counter)
        for spawn in json_records(log):
            name = spawn.get("mnemonic", "")
            if name.startswith("SonicBoot"):
                actions[name]["hits" if spawn.get("cacheHit") else "executed"] += 1
        evidence[package] = {"seconds": time.monotonic() - start, "actions": actions}
    root = execution_root_from_bep(events)
    outputs = {}
    with events.open() as stream:
        for line in stream:
            for f in json.loads(line).get("namedSetOfFiles", {}).get("files", []):
                # Match the injected repository, accounting for the module
                # extension prefix that Bazel assigns to its canonical name.
                parts = Path(f["name"]).parts
                if (len(parts) < 4 or parts[0] != "external" or
                        parts[1].split("+")[-1] != REPO or parts[2] != package):
                    continue
                name = "/".join(parts[3:])
                if name in expected:
                    relative = str(Path(*f.get("pathPrefix", []), f["name"]))
                    outputs[name] = resolve_output(root, relative)
    if outputs.keys() != expected:
        raise ValueError("Bazel did not report the selected outputs: " + str(expected - outputs.keys()))
    return outputs


@contextmanager
def lock_checkout():
    # Two recipes in the same checkout may share a Bazel output base. Keep the
    # phase-one metadata and phase-two files from being replaced before export.
    checkout = os.open(".", os.O_RDONLY | os.O_DIRECTORY)
    try:
        fcntl.flock(checkout, fcntl.LOCK_EX)
        yield
    finally:
        os.close(checkout)


def build(images, platform, output, inventory, labels, evidence_dir=None):
    options = (cache_options(os.environ.get("BAZEL_CONTAINER_CACHE_DIR") or os.environ.get("BAZEL_SWSS_CACHE_DIR"))
               + shlex.split(os.environ.get("BAZEL_CONTAINER_ARGS", ""))
               + remote_cache_options())
    evidence = {} if evidence_dir else None
    with lock_checkout(), tempfile.TemporaryDirectory(prefix="sonic-boot-inputs-") as tmp:
        repo = Path(tmp).resolve() / "inputs"
        repo.mkdir()
        (repo / "MODULE.bazel").write_text(f'module(name="{REPO}")\n')
        archives = write_archive_package(repo / "archives", images, platform)
        if archives:
            expected = {im.archive + "/image.json" for im in images if im.archive}
            results = build_target(repo, "archives", "metadata", expected, evidence, options)
            for im in images:
                if im.archive:
                    im.info = json.loads(results[im.archive + "/image.json"].read_bytes())
                    if not im.info["use_sidecar"]:
                        im.layout = None
        package = BundlePackage(repo / "bundle")
        package.select(images, platform)
        package.finish(platform, archives)
        expected = {"boot/bundle.tar.gz", "boot/inventory.json", "boot/labels.json"}
        result = build_target(repo, "bundle", "boot", expected, evidence, options)
        publish_start = time.monotonic()
        for dest, name in ((inventory, "inventory.json"), (labels, "labels.json"), (output, "bundle.tar.gz")):
            if dest is not None:
                export_archive(result["boot/" + name], dest)
        if evidence is not None:
            evidence["bundle"]["publish_seconds"] = time.monotonic() - publish_start
    if evidence is not None:
        evidence_dir.mkdir(exist_ok=True, parents=True)
        with tempfile.NamedTemporaryFile(mode="w", dir=evidence_dir, prefix="boot-", suffix=".json", delete=False) as receipt:
            json.dump(evidence, receipt, sort_keys=True)
            receipt.write("\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--inventory", type=Path)
    parser.add_argument("--labels", type=Path)
    parser.add_argument("--image", nargs=2, action="append", default=[])
    parser.add_argument("--package-image", nargs=2, action="append", default=[])
    args = parser.parse_args()
    images = []
    for selections, check_label in ((args.image, True), (args.package_image, False)):
        for path, tags in selections:
            images.append(Image(Path(path), tags.split(","), check_label))
    if not images:
        parser.error("at least one image is required")
    evidence = os.environ.get("SONIC_BOOT_BUNDLE_EVIDENCE_DIR")
    build(images, args.platform, args.output, args.inventory, args.labels, Path(evidence) if evidence else None)


if __name__ == "__main__":
    main()
