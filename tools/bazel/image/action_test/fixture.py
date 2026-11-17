"""Write small declared inputs that let Bazel execute all four assembly helpers."""
import hashlib
import io
import json
from pathlib import Path
import sys
import tarfile


request = json.loads(Path(sys.argv[1]).read_text())
architecture = request["architecture"]
paths = {name: Path(value) for name, value in request["outputs"].items()}
for path in paths.values():
    path.parent.mkdir(parents=True, exist_ok=True)

layer = io.BytesIO()
with tarfile.open(fileobj=layer, mode="w", format=tarfile.PAX_FORMAT) as archive:
    member = tarfile.TarInfo("usr/bin/action-probe")
    member.uid, member.gid, member.mode, member.size = 1001, 42, 0o4750, 5
    archive.addfile(member, io.BytesIO(b"probe"))
layer_bytes = layer.getvalue()
paths["layer"].write_bytes(layer_bytes)
paths["onie_boot"].write_bytes(layer_bytes)
paths["platform_archive"].write_bytes(layer_bytes)

boot = paths["boot_files"] / "boot"
boot.mkdir(parents=True)
records = {}
for name, content in [("vmlinuz", b"kernel"), ("config", b"CONFIG_BLK_DEV_INITRD=y\n"), ("System.map", b"symbols")]:
    file = boot / (name + "-action-probe")
    file.write_bytes(content)
    records[name] = {"path": "boot/" + file.name, "sha256": hashlib.sha256(content).hexdigest()}
paths["kernel_metadata"].write_text(json.dumps({"schema": 1, "architecture": architecture, "kernel_release": "action-probe", "boot": records}))
paths["initrd"].write_bytes(b"initrd")
paths["version"].write_text("action-probe\n")

contents = {
    "install_sh": "#!/bin/sh\narch=%%ARCH%%\nversion=%%IMAGE_VERSION%%\n",
    "sharch_body": "#!/bin/sh\npayload_image_size=%%PAYLOAD_IMAGE_SIZE%%\npayload_sha1=%%IMAGE_SHA1%%\nexit_marker\n",
    "machine_conf": "onie_machine=vs\n",
    "default_platform_conf": "# fixture\n",
    "onie_image_conf": "INSTALLER_PAYLOAD=fs.zip\n",
    "arch_image_conf": "# fixture\n",
    "platforms_asic": "x86_64-kvm_x86_64-r0\n",
    "efi_sbatlevel": "# fixture\n",
    "grub_config": "linux /image-old/boot/vmlinuz-old root=UUID=old loop=image-old/fs.squashfs\ninitrd /image-old/boot/initrd.img-old\n",
}
for name, value in contents.items():
    paths[name].write_text(value)
paths["partition_layout"].write_text(json.dumps({
    "firmware": "bios",
    "partitions": [
        {"number": 1, "role": "bios", "start_mib": 1, "size_mib": 1, "type": "EF02", "label": "BIOS-BOOT"},
        {"number": 2, "role": "onie", "start_mib": 2, "size_mib": 32, "type": "8300", "label": "ONIE-BOOT"},
        {"number": 3, "role": "sonic", "start_mib": 34, "type": "8300", "label": "SONiC-OS"},
    ],
    "grub_partition_role": "onie", "grub_directory": "grub", "grub_platform": "i386-pc",
    "grub_prefix": "(hd0,gpt2)/grub", "grub_modules": [],
}))

oci = paths["oci"]
(oci / "blobs/sha256").mkdir(parents=True)
def blob(data):
    digest = hashlib.sha256(data).hexdigest()
    (oci / "blobs/sha256" / digest).write_bytes(data)
    return {"digest": "sha256:" + digest, "size": len(data)}
layer_record = blob(layer_bytes)
config = blob(json.dumps({"os": "linux", "architecture": architecture, "rootfs": {"type": "layers", "diff_ids": [layer_record["digest"]]}}).encode())
manifest = blob(json.dumps({"schemaVersion": 2, "config": config, "layers": [layer_record]}).encode())
(oci / "index.json").write_text(json.dumps({"schemaVersion": 2, "manifests": [manifest]}))
(oci / "oci-layout").write_text('{"imageLayoutVersion":"1.0.0"}')
