"""Validate the kernel producer's schema-1 boot artifact pairing."""
import hashlib
import json
import re
from pathlib import Path, PurePosixPath


def read_kernel_metadata(metadata_path):
    metadata = json.loads(Path(metadata_path).read_text())
    if not isinstance(metadata, dict) or metadata.get("schema") != 1:
        raise ValueError("unsupported kernel metadata schema")
    release = metadata.get("kernel_release")
    if not isinstance(release, str) or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._+-]*", release):
        raise ValueError("invalid kernel release")
    boot = metadata.get("boot")
    if not isinstance(boot, dict):
        raise ValueError("missing kernel boot records")
    for kind in ("vmlinuz", "config", "System.map"):
        record = boot.get(kind)
        if not isinstance(record, dict) or not isinstance(record.get("path"), str):
            raise ValueError("missing kernel boot record: " + kind)
        relative = PurePosixPath(record["path"])
        if relative != PurePosixPath("boot", kind + "-" + release):
            raise ValueError("kernel boot path/release mismatch: " + str(relative))
        if not isinstance(record.get("sha256"), str) or not re.fullmatch(r"[0-9a-f]{64}", record["sha256"]):
            raise ValueError("invalid kernel boot SHA256: " + kind)
    return metadata


def boot_inputs(metadata_path, boot_tree):
    metadata = read_kernel_metadata(metadata_path)
    for kind in ("vmlinuz", "config", "System.map"):
        record = metadata["boot"][kind]
        relative = PurePosixPath(record["path"])
        source = Path(boot_tree) / str(relative)
        # Bazel may materialize declared tree inputs as symlinks inside the
        # action sandbox. The source producer checks regular output files;
        # this consumer verifies the declared bytes against that manifest.
        if not source.is_file():
            raise ValueError("missing kernel boot artifact: " + str(source))
        digest = hashlib.sha256()
        with source.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
        if digest.hexdigest() != record["sha256"]:
            raise ValueError("kernel boot SHA256 mismatch: " + str(relative))
    # Preserve the complete prepared boot directory, including signed EFI and DTB
    # resources. The three manifest records above remain the kernel identity check.
    return metadata, sorted((Path(boot_tree) / "boot").rglob("*"))


def boot_mode(spec, relative, directory=False):
    """Explicit logical permissions, independent of Bazel artifact host modes."""
    default = "0744" if directory else ("0600" if Path(relative).name.startswith("System.map-") else "0644")
    return int(spec.get("boot_file_modes", {}).get(relative, default), 8)
