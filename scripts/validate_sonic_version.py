#!/usr/bin/env python3
"""Reject unusable SONiC version metadata before installation or publication."""

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import zipfile

import yaml


def validate_boot_bundle(archive, payload, squashfs):
    name = "sonic-images.tar.gz"
    if "dockerfs.tar.gz" in payload.namelist() or "installer/dockerfs.tar.gz" in archive.getnames():
        raise ValueError("load-on-boot installer also contains a classic Docker store")
    if name in payload.namelist():
        source = payload.open(name)
    else:
        source = archive.extractfile("installer/" + name)
    expected = json.loads(subprocess.check_output(
        ["unsquashfs", "-cat", str(squashfs), "usr/share/sonic/docker-image-inventory.json"]
    ))
    with source, tarfile.open(fileobj=source, mode="r|gz") as bundle:
        first = bundle.next()
        if first is None or first.name != "manifest.json":
            raise ValueError("boot bundle must start with a Docker load manifest")
        images = json.load(bundle.extractfile(first))
    actual = {}
    for image in images:
        config = image["Config"]
        if not config.startswith("blobs/sha256/") or not image["RepoTags"]:
            raise ValueError("invalid image config or missing tags in boot bundle")
        for tag in image["RepoTags"]:
            if tag in actual:
                raise ValueError("duplicate tag in boot bundle: " + tag)
            actual[tag] = "sha256:" + config.split("/")[-1]
    if not actual or actual != expected:
        raise ValueError("boot bundle tags or image IDs disagree with the installed inventory")
    print(f"Verified {len(images)} packaged images and {len(expected)} boot-time tags")


def read_installer_version(path, expect_boot_bundle=False):
    """Read the packaged filesystem without executing the installer wrapper."""
    with path.open("rb") as stream:
        marker = b"\nexit_marker\n"
        offset = stream.read(65536).find(marker)
        if offset < 0:
            raise ValueError("installer shell header has no exit_marker")
        stream.seek(offset + len(marker))
        with tarfile.open(fileobj=stream, mode="r:") as archive:
            payload = archive.extractfile("installer/fs.zip")
            if payload is None:
                raise ValueError("installer/fs.zip must be a regular payload file")
            with payload, zipfile.ZipFile(payload) as filesystem_zip:
                with tempfile.TemporaryDirectory(prefix="sonic-version-") as temporary:
                    squashfs = Path(temporary) / "fs.squashfs"
                    with filesystem_zip.open("fs.squashfs") as source, squashfs.open("wb") as target:
                        shutil.copyfileobj(source, target)
                    if expect_boot_bundle:
                        validate_boot_bundle(archive, filesystem_zip, squashfs)
                    result = subprocess.run(
                        ["unsquashfs", "-cat", str(squashfs), "etc/sonic/sonic_version.yml"],
                        check=True, capture_output=True, text=True,
                    )
                    return yaml.safe_load(result.stdout)


def validate_version(data, secure_upgrade_mode):
    if not isinstance(data, dict) or not data:
        raise ValueError("sonic_version.yml must contain a nonempty YAML mapping")
    for name in ("build_version", "asic_type", "kernel_version", "commit_id"):
        value = data.get(name)
        if not isinstance(value, str) or not value.strip():
            raise ValueError(f"sonic_version.yml requires a nonempty string {name}")
    if secure_upgrade_mode not in ("no_sign", "dev", "prod"):
        raise ValueError(f"unsupported secure upgrade mode: {secure_upgrade_mode!r}")
    expected = "no" if secure_upgrade_mode == "no_sign" else "yes"
    if data.get("secure_boot_image") != expected:
        raise ValueError(
            f"secure_boot_image must be the string {expected!r} "
            f"for secure upgrade mode {secure_upgrade_mode!r}"
        )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version_file", type=Path)
    parser.add_argument(
        "--secure-upgrade-mode", required=True, choices=("no_sign", "dev", "prod")
    )
    parser.add_argument("--installer", action="store_true", help="read version_file as a sonic-vs.bin installer")
    parser.add_argument("--expect-boot-bundle", action="store_true", help="verify the offline bundle against the installed inventory")
    parser.add_argument("--expected-commit-id", help="require the packaged commit_id to match this build")
    parser.add_argument("--json-output", type=Path, help="save validated metadata as a CI receipt")
    args = parser.parse_args()
    try:
        data = (read_installer_version(args.version_file, args.expect_boot_bundle) if args.installer
                else yaml.safe_load(args.version_file.read_text()))
        validate_version(data, args.secure_upgrade_mode)
        if args.expected_commit_id is not None and data["commit_id"] != args.expected_commit_id:
            raise ValueError("sonic_version.yml commit_id does not match the build checkout")
        if args.json_output:
            args.json_output.write_text(json.dumps(data, indent=2) + "\n")
    except (OSError, yaml.YAMLError, ValueError, KeyError, tarfile.TarError,
            zipfile.BadZipFile, subprocess.CalledProcessError) as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
