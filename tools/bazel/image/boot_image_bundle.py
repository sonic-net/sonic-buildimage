#!/usr/bin/env python3
"""Package the selected SONiC images for one offline docker load at first boot."""

import argparse
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager
from functools import lru_cache
import gzip
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import tarfile
import tempfile

# Make also invokes this script directly with PYTHONSAFEPATH enabled.
HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE), str(HERE.parent / "oci")]
from boot_layout import layout_for_archive
from image_config import compressed_output
from oci_layout import validate_layout


OCI = "application/vnd.oci.image."
LAYER = OCI + "layer.v1.tar"
BLOB = re.compile(r"blobs/sha256/([0-9a-f]{64})")
DIGEST = re.compile(r"sha256:[0-9a-f]{64}")
CHUNK = 1024 * 1024


def json_bytes(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode()


def descriptor(media_type, digest, size):
    return {"mediaType": media_type, "digest": digest, "size": size}


def blob_name(digest):
    if not DIGEST.fullmatch(digest):
        raise ValueError("invalid OCI digest: " + digest)
    return "blobs/sha256/" + digest[7:]


def member_name(name):
    path = PurePosixPath(name)
    if path.is_absolute() or ".." in path.parts or "\\" in name:
        raise ValueError("unsafe archive member: " + name)
    return path.as_posix()


def hash_file(path):
    with path.open("rb") as data:
        return hashlib.file_digest(data, "sha256").hexdigest()


@contextmanager
def decode(command):
    with subprocess.Popen(command, stdout=subprocess.PIPE) as process:
        try:
            yield process.stdout
            # A tar reader may stop at the end marker before the decoder's trailer.
            while process.stdout.read(CHUNK):
                pass
            if process.wait():
                raise ValueError("failed to decompress " + str(command[-1]))
        finally:
            process.stdout.close()


def archive_members(path, scratch, pigz):
    """Read a docker-save archive in one pass, spooling each distinct blob once."""
    with path.open("rb") as stream:
        gzipped = stream.read(2) == b"\x1f\x8b"
    files = {}
    links = {}
    with decode([pigz, "-dc", str(path)]) if gzipped else path.open("rb") as raw:
        with tarfile.open(fileobj=raw, mode="r|", bufsize=CHUNK) as archive:
            for member in archive:
                name = member_name(member.name)
                if member.isdir():
                    continue
                if name in files or name in links:
                    raise ValueError("duplicate archive member: " + name)
                if member.islnk() or member.issym():
                    target = member.linkname
                    if member.issym():
                        target = str(PurePosixPath(name).parent / target)
                    links[name] = member_name(target)
                    continue
                if not member.isfile():
                    raise ValueError("unexpected Docker archive member: " + name)
                match = BLOB.fullmatch(name)
                if match and (scratch / match[1]).exists():
                    files[name] = scratch / match[1]
                    if files[name].stat().st_size != member.size:
                        raise ValueError("conflicting sizes for shared blob: " + name)
                    continue
                checksum = hashlib.sha256()
                partial = scratch / "member.tmp"
                with partial.open("wb") as output:
                    data = archive.extractfile(member)
                    for chunk in iter(lambda: data.read(CHUNK), b""):
                        checksum.update(chunk)
                        output.write(chunk)
                digest = checksum.hexdigest()
                if match and match[1] != digest:
                    raise ValueError("Docker archive blob checksum mismatch: " + name)
                files[name] = scratch / digest
                if files[name].exists():
                    partial.unlink()
                else:
                    partial.rename(files[name])
    for name, target in links.items():
        # Docker-save tar hard links reference regular layer files.
        files[name] = files[target]
    return files


def qualified_tag(tag):
    first = tag.split("/", 1)[0]
    if "/" not in tag:
        return "docker.io/library/" + tag
    if first == "localhost" or "." in first or ":" in first:
        return tag
    return "docker.io/" + tag


def layer_media(path):
    with path.open("rb") as stream:
        magic = stream.read(4)
    if magic.startswith(b"\x1f\x8b"):
        return LAYER + "+gzip"
    if magic == b"\x28\xb5\x2f\xfd":
        return LAYER + "+zstd"
    return LAYER


def archive_image(path, scratch, pigz):
    files = archive_members(path, scratch, pigz)
    if "manifest.json" not in files:
        raise ValueError(f"expected a single docker-save image with manifest.json in {path}")
    entries = json.loads(files["manifest.json"].read_bytes())
    if len(entries) != 1:
        raise ValueError("expected one image in " + str(path))
    entry = entries[0]
    config_path = files[member_name(entry["Config"])]
    layers = []
    for name in entry["Layers"]:
        blob = files[member_name(name)]
        layers.append((descriptor(layer_media(blob), "sha256:" + blob.name, blob.stat().st_size), blob))
    return config_path, layers, entry.get("RepoTags") or []


def emit(archive, name, content):
    header = tarfile.TarInfo(name)
    if isinstance(content, Path):
        header.size = content.stat().st_size
        with content.open("rb") as data:
            archive.addfile(header, data)
    else:
        header.size = len(content)
        archive.addfile(header, io.BytesIO(content))


def compressed_layers(layer_by_diff, scratch, pigz, zstd):
    """Keep Docker load's temporary extraction bounded by the compressed payload size."""
    cpus = min(8, len(os.sched_getaffinity(0)))
    workers = min(4, cpus)
    threads = max(1, cpus // workers)

    def compress(item):
        diff, (desc, source) = item
        if desc["mediaType"] == LAYER + "+gzip":
            return diff, (desc, source)
        dest = scratch / (diff[7:] + ".gz")
        # Normalize uncommon zstd inputs too: classic Docker can load gzip layers.
        with (decode([zstd, "-dc", str(source)]) if desc["mediaType"] == LAYER + "+zstd"
              else source.open("rb")) as raw, dest.open("wb") as out:
            subprocess.run([pigz, "-n", "-6", "-p", str(threads), "-c"], stdin=raw, stdout=out, check=True)
        return diff, (descriptor(LAYER + "+gzip", "sha256:" + hash_file(dest), dest.stat().st_size), dest)

    with ThreadPoolExecutor(max_workers=workers) as pool:
        return dict(pool.map(compress, sorted(layer_by_diff.items(), key=lambda item: -item[1][0]["size"])))


def build(images, platform, output, inventory, pigz, zstd, labels=None):
    blobs, images_by_config, tags, layer_by_diff = {}, {}, {}, {}

    @lru_cache(maxsize=None)
    def file_digest(path):
        return hash_file(path)

    @lru_cache(maxsize=None)
    def layer_diff(digest, media, path):
        if media.endswith("+gzip") or media.endswith(".gzip"):
            with gzip.open(path, "rb") as data:
                return "sha256:" + hashlib.file_digest(data, "sha256").hexdigest()
        if media.endswith("+zstd"):
            with decode([zstd, "-dc", str(path)]) as data:
                return "sha256:" + hashlib.file_digest(data, "sha256").hexdigest()
        if media not in (LAYER, "application/vnd.docker.image.rootfs.diff.tar"):
            raise ValueError("unsupported OCI layer type: " + media)
        return digest

    with tempfile.TemporaryDirectory(dir=output.parent, prefix=".boot-images-") as temporary:
        scratch = Path(temporary)
        for source, requested_tags, check_label in sorted(images, key=lambda image: image[1]):
            if source.is_file():
                source = layout_for_archive(source)
            if source.is_dir():
                layout = validate_layout(source, platform, digest_file=lambda p: file_digest(p.resolve()))
                config_path = source / blob_name(layout.manifest["config"]["digest"])
                layers = zip(layout.manifest["layers"], layout.layers)
                original_tags = []
            else:
                config_path, layers, original_tags = archive_image(source, scratch, pigz)
            config_data = config_path.read_bytes()
            config = json.loads(config_data)
            image_id = "sha256:" + hashlib.sha256(config_data).hexdigest()
            if config["os"] + "/" + config["architecture"] != platform:
                raise ValueError(f"{source}: image does not match platform {platform}")
            if check_label:
                value = (config["config"].get("Labels") or {}).get("com.azure.sonic.manifest")
                parsed = json.loads(value) if value else None
                if parsed is None or parsed is False:
                    raise ValueError(f"{source}: expected valid com.azure.sonic.manifest")
            rootfs = config["rootfs"]
            if rootfs["type"] != "layers":
                raise ValueError(f"{source}: expected layered image")
            blobs[image_id] = config_data
            for diff, (desc, path) in zip(rootfs["diff_ids"], layers, strict=True):
                encoded = desc["digest"]
                blob_name(diff)
                known = layer_by_diff.get(diff)
                if not known or known[0]["digest"] != encoded:
                    if layer_diff(encoded, desc["mediaType"], path.resolve()) != diff:
                        raise ValueError(f"{source}: incorrect uncompressed layer digest {diff}")
                    media = layer_media(path)
                    if not known or (media == LAYER + "+gzip" and known[0]["mediaType"] != media):
                        layer_by_diff[diff] = (descriptor(media, encoded, path.stat().st_size), path)
            images_by_config[image_id] = rootfs["diff_ids"]
            for tag in [*requested_tags, *original_tags]:
                if not tag or ":" not in tag.rsplit("/", 1)[-1] or "@" in tag:
                    raise ValueError("expected a tagged Docker reference: " + tag)
                if tags.setdefault(tag, image_id) != image_id:
                    raise ValueError("multiple images claim tag " + tag)

        encoded_layers = compressed_layers(layer_by_diff, scratch, pigz, zstd)
        for desc, path in encoded_layers.values():
            blobs[desc["digest"]] = path

        docker, index = [], []
        for image_id, diffs in sorted(images_by_config.items()):
            layers = [encoded_layers[diff][0] for diff in diffs]
            repo_tags = sorted(tag for tag, config in tags.items() if config == image_id)
            manifest = json_bytes({
                "schemaVersion": 2, "mediaType": OCI + "manifest.v1+json",
                "config": descriptor(OCI + "config.v1+json", image_id, len(blobs[image_id])),
                "layers": layers,
            })
            digest = "sha256:" + hashlib.sha256(manifest).hexdigest()
            blobs[digest] = manifest
            item = descriptor(OCI + "manifest.v1+json", digest, len(manifest))
            for tag in repo_tags:
                name = qualified_tag(tag)
                index.append({**item, "annotations": {"org.opencontainers.image.ref.name": name,
                                                       "io.containerd.image.name": name}})
            docker.append({"Config": blob_name(image_id), "RepoTags": repo_tags,
                           "Layers": [blob_name(layer["digest"]) for layer in layers]})

        pending = scratch / "bundle.tar.gz"
        with compressed_output(pending, "gzip", None, pigz, gzip_level=0) as compressed:
            with tarfile.open(fileobj=compressed, mode="w|", format=tarfile.USTAR_FORMAT,
                              bufsize=CHUNK, copybufsize=CHUNK) as archive:
                emit(archive, "manifest.json", json_bytes(docker))
                emit(archive, "oci-layout", json_bytes({"imageLayoutVersion": "1.0.0"}))
                emit(archive, "index.json", json_bytes({"schemaVersion": 2, "mediaType": OCI + "index.v1+json",
                                                        "manifests": index}))
                for digest, content in sorted(blobs.items()):
                    emit(archive, blob_name(digest), content)
        if labels:
            labels.write_bytes(json_bytes({tag: json.loads(blobs[digest])["config"].get("Labels") or {}
                                           for tag, digest in tags.items()}) + b"\n")
        if inventory:
            inventory.write_bytes(json_bytes(tags) + b"\n")
        pending.replace(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", nargs=2, action="append", default=[], metavar=("SOURCE", "TAGS"))
    parser.add_argument("--package-image", nargs=2, action="append", default=[], metavar=("SOURCE", "TAGS"))
    parser.add_argument("--platform", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--inventory", type=Path)
    parser.add_argument("--labels", type=Path)
    parser.add_argument("--pigz", default="pigz")
    parser.add_argument("--zstd", default="zstd")
    args = parser.parse_args()
    if not args.image and not args.package_image:
        parser.error("at least one image is required")
    images = [(Path(path), tags.split(","), check_label)
              for entries, check_label in ((args.image, True), (args.package_image, False))
              for path, tags in entries]
    build(images, args.platform, args.output, args.inventory, args.pigz, args.zstd, args.labels)


if __name__ == "__main__":
    main()
