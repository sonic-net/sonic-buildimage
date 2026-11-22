"""Bazel actions for archive selection, reusable layers, and the offline boot bundle."""

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from boot_image_bundle import (
    CHUNK, LAYER, archive_image, blob_name, checked_rootfs, decode, descriptor,
    hash_file, json_bytes, layer_media, register_tags, write_bundle,
)
from file_copy import copy_file


def archive_input(src, receipt, index, dest, info_out, pigz):
    dest.mkdir(exist_ok=True)
    use_sidecar = False
    if receipt and index:
        try:
            attestation = json.loads(receipt.read_bytes())
            use_sidecar = (attestation["index"] == hash_file(index)
                           and attestation["archive"] == hash_file(src))
        except (KeyError, TypeError, ValueError):
            pass
    info = {"use_sidecar": use_sidecar}
    if not use_sidecar:
        # Scratch and declared outputs share a filesystem. Link each unique blob
        # into the result without copying it back out of the extracted archive.
        with tempfile.TemporaryDirectory(dir=dest) as temporary:
            config, layers, original_tags = archive_image(src, Path(temporary), pigz)
            (dest / "configs").mkdir()
            (dest / "layers").mkdir()
            os.link(config, dest / "configs" / config.name)
            for digest, path in {desc["digest"]: path for desc, path in layers}.items():
                os.link(path, dest / "layers" / digest[7:])
            info.update(config="sha256:" + config.name, layers=[desc for desc, _ in layers],
                        diff_ids=json.loads(config.read_bytes())["rootfs"]["diff_ids"], original_tags=original_tags)
    data = json_bytes(info) + b"\n"
    (dest / "image.json").write_bytes(data)
    info_out.write_bytes(data)


def project_members(archive, outputs):
    for out in outputs:
        kind, hex_digest = out.parts[-2:]
        if kind not in ("configs", "layers"):
            raise ValueError("unexpected archive member group: " + kind)
        blob_name("sha256:" + hex_digest)
        copy_file(archive / kind / hex_digest, out)


def normalize_layer(src, source, diff_id, receipt, out, pigz, zstd, gzip_level=6):
    if type(gzip_level) is not int or not 1 <= gzip_level <= 9:
        raise ValueError("gzip level must be between 1 and 9")
    digest = source["digest"]
    blob_name(digest)
    blob_name(diff_id)
    if src.stat().st_size != source["size"]:
        raise ValueError("incorrect source layer size: " + digest)
    media = source["mediaType"]
    if media.endswith(("+gzip", ".gzip")):
        kind = LAYER + "+gzip"
    elif media.endswith("+zstd"):
        kind = LAYER + "+zstd"
    elif media in (LAYER, "application/vnd.docker.image.rootfs.diff.tar"):
        kind = LAYER
    else:
        raise ValueError("unsupported OCI layer type: " + media)
    if layer_media(src) != kind:
        raise ValueError("layer encoding does not match source descriptor: " + digest)

    if kind == LAYER + "+gzip":
        if "sha256:" + hash_file(src) != digest:
            raise ValueError("incorrect source layer digest: " + digest)
        with gzip.open(src, "rb") as raw:
            observed = hashlib.file_digest(raw, "sha256").hexdigest()
        # The declared source is already the exact normalized blob.
        normalized = descriptor(LAYER + "+gzip", digest, source["size"])
    else:
        if kind == LAYER + "+zstd" and "sha256:" + hash_file(src) != digest:
            raise ValueError("incorrect source layer digest: " + digest)
        checksum = hashlib.sha256()
        with (decode([zstd, "-dc", str(src)]) if kind == LAYER + "+zstd" else src.open("rb")) as raw:
            with out.open("wb") as encoded, subprocess.Popen(
                    [pigz, "-n", "-" + str(gzip_level), "-p", "2", "-c"],
                    stdin=subprocess.PIPE, stdout=encoded) as process:
                with process.stdin:
                    for chunk in iter(lambda: raw.read(CHUNK), b""):
                        checksum.update(chunk)
                        process.stdin.write(chunk)
                if process.wait():
                    raise ValueError("failed to compress layer: " + digest)
        observed = checksum.hexdigest()
        if kind == LAYER and "sha256:" + observed != digest:
            raise ValueError("incorrect source layer digest: " + digest)
        normalized = descriptor(LAYER + "+gzip", "sha256:" + hash_file(out), out.stat().st_size)
    if "sha256:" + observed != diff_id:
        raise ValueError("incorrect uncompressed layer digest: " + diff_id)
    receipt.write_bytes(json_bytes({"source": source, "diff_id": diff_id, "gzip_source": kind == LAYER + "+gzip",
                                    "normalized": normalized}) + b"\n")


def bundle_input(spec, configs, layers, platform, out, inventory, labels, pigz):
    blobs = {}
    for config in configs:
        data = config.read_bytes()
        blobs["sha256:" + hashlib.sha256(data).hexdigest()] = data

    # Match every image descriptor and original config DiffID to an action
    # receipt. Only the small receipts are read before writing the final tar.
    sources = {}
    for receipt, blob in layers:
        report = json.loads(receipt.read_bytes())
        source = report["source"]
        key = (source["digest"], source["size"], source["mediaType"], report["diff_id"])
        sources[key] = (report["gzip_source"], report["normalized"], blob)
    by_config, tags, selected = {}, {}, {}
    images = json.loads(spec.read_bytes())["images"]
    if not images:
        raise ValueError("at least one boot image is required")
    for image in sorted(images, key=lambda item: item["tags"]):
        image_id = image["config"]
        config = json.loads(blobs[image_id])
        rootfs = checked_rootfs(config, image_id, platform, image["check_label"])
        by_config[image_id] = rootfs["diff_ids"]
        for diff, desc in zip(rootfs["diff_ids"], image["layers"], strict=True):
            layer = sources[(desc["digest"], desc["size"], desc["mediaType"], diff)]
            if diff not in selected or (layer[0] and not selected[diff][0]):
                selected[diff] = layer
        register_tags(tags, image_id, [*image["tags"], *image.get("original_tags", [])])
    encoded = {diff: (desc, path) for diff, (_, desc, path) in selected.items()}
    with tempfile.TemporaryDirectory(dir=out.parent, prefix=".boot-images-") as temporary:
        write_bundle(blobs, by_config, tags, encoded, out, inventory, labels, pigz, Path(temporary))


def main():
    parser = argparse.ArgumentParser(description=__doc__, fromfile_prefix_chars="@")
    sub = parser.add_subparsers(dest="action", required=True)
    archive = sub.add_parser("archive")
    archive.add_argument("--src", type=Path, required=True)
    archive.add_argument("--receipt", type=Path)
    archive.add_argument("--index", type=Path)
    archive.add_argument("--dest", type=Path, required=True)
    archive.add_argument("--metadata", type=Path, required=True)
    archive.add_argument("--pigz", required=True)
    project = sub.add_parser("project")
    project.add_argument("--archive", type=Path, required=True)
    project.add_argument("--out", type=Path, action="append", required=True)
    layer = sub.add_parser("layer")
    layer.add_argument("--src", type=Path, required=True)
    layer.add_argument("--source-digest", required=True)
    layer.add_argument("--source-size", type=int, required=True)
    layer.add_argument("--media-type", required=True)
    layer.add_argument("--diff-id", required=True)
    layer.add_argument("--receipt", type=Path, required=True)
    layer.add_argument("--out", type=Path)
    layer.add_argument("--pigz")
    layer.add_argument("--zstd")
    layer.add_argument("--gzip-level", type=int, choices=range(1, 10), default=6)
    final = sub.add_parser("bundle")
    final.add_argument("--spec", type=Path, required=True)
    final.add_argument("--platform", required=True)
    final.add_argument("--config", type=Path, action="append", default=[])
    final.add_argument("--layer", type=Path, nargs=2, action="append", default=[])
    final.add_argument("--out", type=Path, required=True)
    final.add_argument("--inventory", type=Path, required=True)
    final.add_argument("--labels", type=Path, required=True)
    final.add_argument("--pigz", required=True)
    args = parser.parse_args()
    if args.action == "archive":
        archive_input(args.src, args.receipt, args.index, args.dest, args.metadata, args.pigz)
    elif args.action == "project":
        project_members(args.archive, args.out)
    elif args.action == "layer":
        source = descriptor(args.media_type, args.source_digest, args.source_size)
        normalize_layer(args.src, source, args.diff_id, args.receipt, args.out, args.pigz, args.zstd,
                        args.gzip_level)
    else:
        bundle_input(args.spec, args.config, args.layer, args.platform, args.out,
                     args.inventory, args.labels, args.pigz)


if __name__ == "__main__":
    main()
