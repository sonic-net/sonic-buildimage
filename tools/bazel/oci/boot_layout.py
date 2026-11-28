"""Publish native Bazel OCI layouts and identify the outputs selected by Make."""

import argparse
import errno
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import tempfile


def sha256(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def archive_identity(status):
    return [status.st_dev, status.st_ino, status.st_size, status.st_mtime_ns, status.st_ctime_ns]


def installer_receipt(archive, handoff):
    if not handoff or not re.fullmatch(r"[A-Za-z0-9_-]+", handoff):
        raise ValueError("Invalid VS boot image handoff")
    return archive.parent / ".boot-image-layouts" / "installers" / handoff / (archive.name + ".json")


def record_installer_input(archive, handoff, selection):
    """Bind this invocation to its output and update Make's content-stable prerequisite."""
    receipt = installer_receipt(archive, handoff)
    marker = archive.parent / ".vs-boot-inputs" / archive.name
    receipt.parent.mkdir(parents=True, exist_ok=True)
    marker.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=receipt.parent) as temporary:
        source = Path(temporary) / "selection"
        source.write_text(json.dumps({"handoff": handoff, "name": archive.name, **selection}) + "\n")
        source.replace(receipt)
        contents = json.dumps(selection) + "\n"
        if not marker.is_file() or marker.read_text() != contents:
            source.write_text(contents)
            source.replace(marker)


def publish_layout(source, archive_source, archive_destination, *, handoff=None, published=None):
    """Preserve the configured OCI output independently of the Bazel output base."""
    store = archive_destination.parent / ".boot-image-layouts"
    pool = store / "blobs/sha256"
    pool.mkdir(parents=True, exist_ok=True)
    index = (source / "index.json").read_bytes()
    index_id = hashlib.sha256(index).hexdigest()
    generation = store / index_id
    with tempfile.TemporaryDirectory(dir=store) as tmp:
        staging = Path(tmp)
        for blob in (source / "blobs/sha256").iterdir():
            if not re.fullmatch("[0-9a-f]{64}", blob.name):
                raise ValueError(f"invalid OCI blob name in {source}: {blob.name}")
            destination = pool / blob.name
            if destination.is_file():
                continue
            try:
                os.link(blob.resolve(), destination)
            except FileExistsError:
                pass
            except OSError as error:
                if error.errno not in (errno.EXDEV, errno.EPERM, errno.EACCES):
                    raise
                copy = staging / "copy"
                shutil.copyfile(blob, copy)
                try:
                    os.link(copy, destination)
                except FileExistsError:
                    pass
                copy.unlink()
        if not generation.is_dir():
            layout = staging / "layout"
            layout.mkdir()
            (layout / "index.json").write_bytes(index)
            shutil.copyfile(source / "oci-layout", layout / "oci-layout")
            (layout / "blobs").symlink_to("../blobs", target_is_directory=True)
            try:
                layout.rename(generation)
            except OSError as error:
                if error.errno not in (errno.EEXIST, errno.ENOTEMPTY) or not generation.is_dir():
                    raise
        receipt = staging / "receipt"
        if archive_source is None:
            record_installer_input(archive_destination, handoff, {"index": index_id})
            return generation
        if handoff and published is not None:
            # A current Make invocation owns the paired configured outputs. Record
            # the inode from export_archive's open descriptor, never a later stat
            # of the path, which may already name another producer's archive.
            item = {"handoff": handoff, "name": archive_destination.name,
                    "identity": archive_identity(published), "index": index_id}
            receipt_path = store / (archive_destination.name + ".direct.json")
        else:
            item = {"archive": sha256(archive_source), "index": index_id}
            receipt_path = store / (archive_destination.name + ".json")
        receipt.write_text(json.dumps(item) + "\n")
        receipt.replace(receipt_path)
    return generation


def installer_input(archive, handoff):
    """Resolve the selected layout or the exact fallback archive from this invocation."""
    receipt = installer_receipt(archive, handoff)
    item = json.loads(receipt.read_bytes())
    if item.get("handoff") != handoff or item.get("name") != archive.name:
        raise ValueError(f"Invalid installer OCI receipt: {receipt}")
    if "archive_sha256" in item:
        if item["archive_sha256"] != sha256(archive):
            raise ValueError(f"Installer fallback archive changed after publication: {archive}")
        return archive
    if not re.fullmatch(r"[0-9a-f]{64}", str(item.get("index", ""))):
        raise ValueError(f"Invalid installer OCI index: {receipt}")
    layout = archive.parent / ".boot-image-layouts" / item["index"]
    if sha256(layout / "index.json") != item["index"]:
        raise ValueError(f"Installer OCI index does not match its receipt: {receipt}")
    return layout


def direct_layout_for_archive(archive, handoff):
    """Accept only the exact Make-selected output from this producer invocation."""
    if not handoff:
        return None
    store = archive.parent / ".boot-image-layouts"
    try:
        item = json.loads((store / (archive.name + ".direct.json")).read_bytes())
        if (item["handoff"] != handoff or item["name"] != archive.name
                or item["identity"] != archive_identity(archive.stat())):
            return None
        digest = item["index"]
        if re.fullmatch("[0-9a-f]{64}", digest):
            layout = store / digest
            if sha256(layout / "index.json") == digest:
                return layout
    except (OSError, KeyError, TypeError, ValueError):
        pass
    return None


def layout_for_archive(archive):
    """An old export or pruned sidecar falls back to its authoritative Docker archive."""
    store = archive.parent / ".boot-image-layouts"
    receipt = store / (archive.name + ".json")
    if not receipt.is_file():
        return archive
    try:
        item = json.loads(receipt.read_bytes())
        digest = item["index"]
        if not re.fullmatch("[0-9a-f]{64}", digest) or item["archive"] != sha256(archive):
            return archive
        layout = store / digest
        if sha256(layout / "index.json") == digest:
            return layout
    except (OSError, KeyError, TypeError, ValueError):
        pass
    return archive


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Resolve a VS installer input from the current build.")
    parser.add_argument("archive", type=Path)
    parser.add_argument("handoff")
    args = parser.parse_args()
    print(installer_input(args.archive, args.handoff))
