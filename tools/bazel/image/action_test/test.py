"""Inspect real filesystem and qcow2 outputs of the public assembly actions."""
import argparse
import hashlib
import io
import json
import os
import subprocess
from pathlib import Path
import tarfile
import tempfile
import zipfile


parser = argparse.ArgumentParser()
for name in ("squashfs", "docker-store", "installer", "architecture", "unsquashfs", "layer", "tar-split"):
    parser.add_argument("--" + name, required=True)
args = parser.parse_args()

unsquashfs = str(Path(args.unsquashfs).absolute())
content = subprocess.check_output([unsquashfs, '-cat', args.squashfs, 'usr/bin/action-probe'])
if content != b'probe':
    raise AssertionError('SquashFS lost the declared layer payload')
listing = subprocess.check_output([unsquashfs, '-lln', args.squashfs], text=True)
if not any('rwsr-x---' in line and '1001/42' in line and line.endswith('/usr/bin/action-probe') for line in listing.splitlines()):
    raise AssertionError('SquashFS lost numeric ownership or setuid permissions: ' + listing)

with tarfile.open(args.docker_store) as archive:
    repositories = json.load(archive.extractfile("image/overlay2/repositories.json"))["Repositories"]
    if "action-probe:action-probe" not in repositories["action-probe"]:
        raise AssertionError("Docker store action fixture lost its configured tag")
    member = next(member for member in archive if member.name.endswith("/diff/usr/bin/action-probe"))
    if archive.extractfile(member).read() != b"probe":
        raise AssertionError("Docker store action fixture lost its layer bytes")
    # Docker consumes the tar-split protocol, including the exact original TAR
    # headers/padding. Verify the public action using the independent APT CLI.
    original_layer = Path(args.layer).read_bytes()
    chain = hashlib.sha256(original_layer).hexdigest()
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        metadata = root / "metadata.json.gz"
        metadata.write_bytes(archive.extractfile("image/overlay2/layerdb/sha256/" + chain + "/tar-split.json.gz").read())
        files = root / "files"
        files.mkdir()
        with tarfile.open(args.layer) as source:
            for original in source:
                if original.isfile():
                    target = files / original.name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(archive.extractfile("overlay2/" + chain + "/diff/" + original.name).read())
        rebuilt = root / "rebuilt.tar"
        subprocess.run([str(Path(args.tar_split).absolute()), "asm", "--input", str(metadata),
                        "--path", str(files), "--output", str(rebuilt)], check=True)
        if rebuilt.read_bytes() != original_layer:
            raise AssertionError("Docker metadata cannot reassemble the original OCI layer TAR")
    config = next(member for member in archive if member.name.startswith("image/overlay2/imagedb/content/sha256/"))
    if json.load(archive.extractfile(config))["architecture"] != args.architecture:
        raise AssertionError("Docker store action fixture selected the wrong architecture")

body = Path(args.installer).read_bytes().split(b"exit_marker\n", 1)[1]
with tarfile.open(fileobj=io.BytesIO(body)) as archive:
    archive.getmember("installer/onie-image-" + args.architecture + ".conf")
    with zipfile.ZipFile(io.BytesIO(archive.extractfile("installer/fs.zip").read())) as payload:
        if payload.read("boot/vmlinuz-action-probe") != b"kernel" or payload.read("fs.squashfs") != Path(args.squashfs).read_bytes():
            raise AssertionError("ONIE action fixture lost its declared kernel or rootfs bytes")

outputs = {name: getattr(args, name) for name in ("squashfs", "docker_store", "installer")}
receipt = {
    "architecture": args.architecture,
    "outputs": {name: {"sha256": hashlib.sha256(Path(path).read_bytes()).hexdigest(), "size": Path(path).stat().st_size} for name, path in outputs.items()},
    "scope": "Public Bazel actions with pinned real tools: SquashFS data/metadata, OCI Docker store and ONIE ZIP. Kernel/initrd are synthetic format inputs; product boot is validated separately.",
}
Path(os.environ["TEST_UNDECLARED_OUTPUTS_DIR"], "action-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
