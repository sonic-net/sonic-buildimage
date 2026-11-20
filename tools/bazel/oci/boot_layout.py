"""Preserve native Bazel OCI blobs for a Make installer, tied to its exported archive."""

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


def publish_layout(source, archive_source, archive_destination):
    """Make the OCI sidecar available even to a later build slave with a new Bazel output base."""
    store = archive_destination.parent / ".boot-image-layouts"
    pool = store / "blobs/sha256"
    pool.mkdir(parents=True, exist_ok=True)
    index = (source / "index.json").read_bytes()
    index_id = hashlib.sha256(index).hexdigest()
    generation = store / index_id
    with tempfile.TemporaryDirectory(dir=store) as tmp:
        staging = Path(tmp)
        if not generation.is_dir():
            layout = staging / "layout"
            layout.mkdir()
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
            (layout / "index.json").write_bytes(index)
            shutil.copyfile(source / "oci-layout", layout / "oci-layout")
            (layout / "blobs").symlink_to("../blobs", target_is_directory=True)
            try:
                layout.rename(generation)
            except OSError as error:
                if error.errno not in (errno.EEXIST, errno.ENOTEMPTY) or not generation.is_dir():
                    raise
        receipt = staging / "receipt"
        receipt.write_text(json.dumps({"archive": sha256(archive_source), "index": index_id}) + "\n")
        receipt.replace(store / (archive_destination.name + ".json"))


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
