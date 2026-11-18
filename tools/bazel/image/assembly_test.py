"""Contract tests: OCI metadata preservation and ONIE consumer formats.

Oracles are the OCI descriptor/diff-ID relationship, Docker layerdb contract,
TAR inode metadata, and the existing ONIE header's SHA1/byte-count protocol.
ZIPs are inspected by Python's independent standard ZIP reader.
"""
import hashlib
import argparse
import gzip
import subprocess
import sys
import io
import json
from pathlib import Path
import re
import tarfile
import tempfile
import unittest
from unittest.mock import patch
import zipfile

import onie
import overlay2_store
import disk_image
import merge_layers


_parser = argparse.ArgumentParser(add_help=False)
_parser.add_argument("--tar-split")
_parser.add_argument("--tar-split-json")
_parser.add_argument("--zstd")
_parser.add_argument("--openssl")
_parser.add_argument("--grub-editenv")
_parser.add_argument("--pigz")
TOOLS, _arguments = _parser.parse_known_args()
for _name, _value in vars(TOOLS).items():
    if _value:
        setattr(TOOLS, _name, str(Path(_value).absolute()))
sys.argv = [sys.argv[0]] + _arguments


class AssemblyTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def test_stream_layer_whiteout_targets(self):
        # OCI layer.md defines .wh.<basename> and the sole opaque marker.
        # A deletion must name a child, never the current/parent directory.
        class SplitterSink:
            def __init__(self):
                self.stdin = io.BytesIO()

            def wait(self):
                return 0

            def terminate(self):
                pass

        def stream(name):
            data = io.BytesIO()
            with tarfile.open(fileobj=data, mode="w") as layer:
                layer.addfile(tarfile.TarInfo(name))
            raw = data.getvalue()
            blob = self.write("whiteout-layer.tar", raw)
            output = io.BytesIO()
            with patch.object(overlay2_store.subprocess, "Popen", return_value=SplitterSink()):
                with tarfile.open(fileobj=output, mode="w") as archive:
                    overlay2_store.stream_layer(
                        archive, blob, "sha256:" + hashlib.sha256(raw).hexdigest(),
                        "diff", None, "splitter", self.root / "metadata.json")
            output.seek(0)
            return tarfile.open(fileobj=output, mode="r:")

        for directory in ("", "dir/"):
            for marker in (".wh.", ".wh..", ".wh..."):
                with self.subTest(name=directory + marker):
                    with self.assertRaises(ValueError):
                        stream(directory + marker)
        with stream("dir/.wh.deleted") as archive:
            removed = archive.getmember("diff/dir/deleted")
            self.assertTrue(removed.ischr())
            self.assertEqual((removed.devmajor, removed.devminor), (0, 0))
        with stream("dir/.wh..wh..opq") as archive:
            opaque = archive.getmember("diff/dir")
            self.assertTrue(opaque.isdir())
            self.assertEqual(opaque.pax_headers["SCHILY.xattr.trusted.overlay.opaque"], "y")
        with self.assertRaises(ValueError):
            stream("dir/.wh..wh.reserved")

    def write(self, name, data):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def blob(self, data):
        digest = hashlib.sha256(data).hexdigest()
        self.write("oci/blobs/sha256/" + digest, data)
        return {"digest": "sha256:" + digest, "size": len(data)}

    def layout(self, diff_id=None):
        data = io.BytesIO()
        with tarfile.open(fileobj=data, mode="w") as archive:
            directory = tarfile.TarInfo("data")
            directory.type, directory.mode, directory.uid, directory.gid = tarfile.DIRTYPE, 0o750, 1001, 42
            archive.addfile(directory)
            owned = tarfile.TarInfo("data/owned")
            owned.uid, owned.gid, owned.mode, owned.size = 1001, 42, 0o4750, 7
            archive.addfile(owned, io.BytesIO(b"payload"))
            hardlink = tarfile.TarInfo("data/alias")
            hardlink.type, hardlink.linkname = tarfile.LNKTYPE, "data/owned"
            hardlink.uid, hardlink.gid, hardlink.mode = 1001, 42, 0o4750
            archive.addfile(hardlink)
            archive.addfile(tarfile.TarInfo("data/.wh.deleted"))
            archive.addfile(tarfile.TarInfo("data/.wh..wh..opq"))
        layer_bytes = data.getvalue()
        layer = self.blob(layer_bytes)
        actual_diff = "sha256:" + hashlib.sha256(layer_bytes).hexdigest()
        config = self.blob(json.dumps({"os": "linux", "architecture": "amd64", "rootfs": {"type": "layers", "diff_ids": [diff_id or actual_diff]}}).encode())
        manifest = self.blob(json.dumps({"schemaVersion": 2, "config": config, "layers": [layer]}).encode())
        self.write("oci/index.json", json.dumps({"schemaVersion": 2, "manifests": [manifest]}).encode())
        self.write("oci/oci-layout", b'{"imageLayoutVersion":"1.0.0"}')
        return self.root / "oci", actual_diff[7:]

    @unittest.skipUnless(TOOLS.tar_split_json and TOOLS.pigz, "supply tar_split_json and pigz for Docker integration")
    def test_dockerfs_preserves_inode_semantics_and_deduplicates_shared_layers(self):
        layout, chain = self.layout()
        output = self.root / "dockerfs.tar"
        overlay2_store.build([(layout, "app:latest,app:{version}"), (layout, "other:latest")], "release", "linux/amd64", output, tar_split_json=TOOLS.tar_split_json, compression="none", pigz=TOOLS.pigz)
        with tarfile.open(output) as archive:
            owned = archive.getmember("overlay2/" + chain + "/diff/data/owned")
            self.assertEqual((owned.uid, owned.gid, owned.mode), (1001, 42, 0o4750))
            self.assertEqual(archive.extractfile(owned).read(), b"payload")
            alias = archive.getmember("overlay2/" + chain + "/diff/data/alias")
            self.assertTrue(alias.islnk())
            self.assertEqual(alias.linkname, owned.name)
            deleted = archive.getmember("overlay2/" + chain + "/diff/data/deleted")
            self.assertTrue(deleted.ischr())
            self.assertEqual((deleted.devmajor, deleted.devminor), (0, 0))
            directory = archive.getmember("overlay2/" + chain + "/diff/data")
            self.assertEqual(directory.pax_headers["SCHILY.xattr.trusted.overlay.opaque"], "y")
            self.assertEqual((directory.uid, directory.gid, directory.mode), (1001, 42, 0o750))
            self.assertEqual(sum(m.name == owned.name for m in archive), 1)
            repos = json.load(archive.extractfile("image/overlay2/repositories.json"))["Repositories"]
            self.assertEqual(repos["app"]["app:latest"], repos["app"]["app:release"])
            self.assertEqual(repos["app"]["app:latest"], repos["other"]["other:latest"])

    @unittest.skipUnless(TOOLS.tar_split_json and TOOLS.pigz, "supply tar_split_json and pigz for Docker integration")
    def test_dockerfs_rejects_uncompressed_digest_mismatch(self):
        layout, _ = self.layout("sha256:" + "0" * 64)
        with self.assertRaisesRegex(ValueError, "diff digest mismatch"):
            overlay2_store.build([(layout, "app:latest")], "", "linux/amd64", self.root / "bad.tar", tar_split_json=TOOLS.tar_split_json, compression="none", pigz=TOOLS.pigz)

    def onie_spec(self):
        kernel = self.write("boot/boot/vmlinuz-test-release", b"kernel bytes")
        records = {}
        for name, content in [("vmlinuz", b"kernel bytes"), ("config", b"CONFIG_BLK_DEV_INITRD=y\n"), ("System.map", b"symbol bytes")]:
            path = self.write("boot/boot/" + name + "-test-release", content)
            records[name] = {"path": "boot/" + path.name, "sha256": hashlib.sha256(content).hexdigest()}
        spec = {
            "pigz": TOOLS.pigz,
            "boot_files": str(kernel.parent.parent),
            "kernel_metadata": str(self.write("kernel.json", json.dumps({"schema": 1, "kernel_release": "test-release", "architecture": "amd64", "boot": records}).encode())),
            "initrd": str(self.write("initrd", b"initrd bytes")),
            "squashfs": str(self.write("root.squashfs", b"squashfs bytes")),
            "platform_archive": str(self.write("platform.tar.gz", gzip.compress(b"platform bytes", mtime=0))),
            "dockerfs": str(self.write("dockerfs.tar.gz", gzip.compress(b"docker bytes", mtime=0))),
            "install_sh": str(self.write("install.sh", b"#!/bin/sh\narch=%%ARCH%%\nversion=%%IMAGE_VERSION%%\nsize=%%ONIE_IMAGE_PART_SIZE%%\n")),
            "installer_variables": {"ARCH": "amd64", "IMAGE_VERSION": "test-image", "ONIE_IMAGE_PART_SIZE": "32768"},
            "efi_sbatlevel": str(self.write("efi_sbatlevel.py", b"# signed EFI downgrade comparison\n")),
            "default_platform_conf": str(self.write("default_platform.conf", b"# default functions\n")),
            "onie_image_conf": str(self.write("onie-image.conf", b"INSTALLER_PAYLOAD=fs.zip\n")),
            "arch_image_conf": str(self.write("onie-image-amd64.conf", b"# architecture config\n")),
            "platforms_asic": str(self.write("platforms_asic", b"x86_64-kvm_x86_64-r0\n")),
            "machine_conf": str(self.write("machine.conf", b"onie_machine=vs\n")),
            "boot_file_modes": {"efi/shimx64.efi": "0744", "device-tree.dtb": "0600"},
            "extra_files": [],
            "sharch_body": str(self.write("header", b"#!/bin/sh\npayload_image_size=%%PAYLOAD_IMAGE_SIZE%%\npayload_sha1=%%IMAGE_SHA1%%\nexit_marker\n")),
            "out": str(self.root / "sonic.bin"),
        }
        self.write("boot/boot/efi/shimx64.efi", b"MZsigned-shim")
        self.write("boot/boot/device-tree.dtb", b"device-tree")
        return spec, kernel

    def test_declared_image_version_file_reaches_installer_script(self):
        spec, _ = self.onie_spec()
        spec["version_file"] = str(self.write("declared-version", b"selected-build-version\n"))
        onie.build(spec)
        _, body = Path(spec['out']).read_bytes().split(b'exit_marker\n', 1)
        with tarfile.open(fileobj=io.BytesIO(body)) as archive:
            self.assertIn(b"version=selected-build-version", archive.extractfile("installer/install.sh").read())

    def test_amd64_installer_without_optional_arch_configuration(self):
        # installer/install.sh loads onie-image-[arch].conf only if present;
        # the mainline AMD64 source tree supplies no architecture override.
        spec, _ = self.onie_spec()
        del spec['arch_image_conf']
        onie.build(spec)
        _, body = Path(spec['out']).read_bytes().split(b'exit_marker\n', 1)
        with tarfile.open(fileobj=io.BytesIO(body)) as archive:
            self.assertNotIn('installer/onie-image-amd64.conf', archive.getnames())
            self.assertIn('installer/onie-image.conf', archive.getnames())

    def test_onie_nested_zip_and_self_extracting_digest(self):
        spec, kernel = self.onie_spec()
        self.write("extra/subdir/config", b"installer resource")
        spec["extra_files"] = [[str(self.root / "extra"), "resources"]]
        onie.build(spec)
        header, body = Path(spec["out"]).read_bytes().split(b"exit_marker\n", 1)
        self.assertEqual(int(re.search(rb"payload_image_size=(\d+)", header)[1]), len(body))
        self.assertEqual(re.search(rb"payload_sha1=([0-9a-f]+)", header)[1].decode(), hashlib.sha1(body).hexdigest())
        with tarfile.open(fileobj=io.BytesIO(body)) as archive:
            self.assertEqual(archive.getmember("installer/install.sh").mode, 0o755)
            script = archive.extractfile("installer/install.sh").read()
            self.assertIn(b"version=test-image", script)
            self.assertNotIn(b"%%", script)
            self.assertEqual(archive.extractfile("installer/default_platform.conf").read(), b"# default functions\n")
            self.assertEqual(archive.extractfile("installer/platforms_asic").read(), b"x86_64-kvm_x86_64-r0\n")
            self.assertEqual(archive.extractfile("installer/resources/subdir/config").read(), b"installer resource")
            with zipfile.ZipFile(io.BytesIO(archive.extractfile("installer/fs.zip").read())) as payload:
                self.assertIsNone(payload.testzip())
                self.assertEqual(payload.read("boot/efi/shimx64.efi"), b"MZsigned-shim")
                self.assertEqual(payload.read("boot/device-tree.dtb"), b"device-tree")
                self.assertEqual(payload.getinfo("boot/efi/shimx64.efi").external_attr >> 16 & 0o7777, 0o744)
                self.assertEqual(payload.getinfo("boot/device-tree.dtb").external_attr >> 16 & 0o7777, 0o600)
                self.assertEqual(payload.read("boot/vmlinuz-test-release"), b"kernel bytes")
                self.assertEqual(payload.read("boot/initrd.img-test-release"), b"initrd bytes")
                self.assertEqual(gzip.decompress(payload.read("dockerfs.tar.gz")), b"docker bytes")
                self.assertEqual(payload.getinfo("fs.squashfs").compress_type, zipfile.ZIP_STORED)
                self.assertEqual(payload.getinfo("boot/initrd.img-test-release").compress_type, zipfile.ZIP_STORED)
                self.assertEqual(payload.getinfo("boot/vmlinuz-test-release").compress_type, zipfile.ZIP_DEFLATED)
                # Real ONIE BusyBox 1.25.1 rejects data descriptors (flag 8).
                for info in payload.infolist():
                    self.assertEqual(info.flag_bits & 0x0009, 0)

        # Pairing comes from the source producer's SHA256 manifest. A different
        # build's same-named kernel must fail before an installer is written.
        kernel.write_bytes(b"different build")
        with self.assertRaisesRegex(ValueError, "SHA256 mismatch"):
            onie.build(spec)


    def test_onie_rejects_unrendered_script_and_mislabeled_gzip(self):
        spec, _ = self.onie_spec()
        spec["installer_variables"] = {}
        with self.assertRaisesRegex(ValueError, "unresolved installer"):
            onie.build(spec)
        spec["installer_variables"] = {"ARCH": "amd64", "IMAGE_VERSION": "another-image", "ONIE_IMAGE_PART_SIZE": "16384"}
        Path(spec["dockerfs"]).write_bytes(b"not gzip")
        with self.assertRaisesRegex(ValueError, "requires a gzip"):
            onie.build(spec)
        spec["dockerfs_compression"] = "none"
        if not TOOLS.pigz:
            with self.assertRaisesRegex(ValueError, "declared pigz executable"):
                onie.build(spec)
            return
        onie.build(spec)
        body = Path(spec["out"]).read_bytes().split(b"exit_marker\n", 1)[1]
        with tarfile.open(fileobj=io.BytesIO(body)) as outer:
            with zipfile.ZipFile(io.BytesIO(outer.extractfile("installer/fs.zip").read())) as payload:
                self.assertEqual(gzip.decompress(payload.read("dockerfs.tar.gz")), b"not gzip")

    def test_installer_reserved_resources_and_signing_requirements(self):
        spec, _ = self.onie_spec()
        spec["signing_mode"] = "dev"
        with self.assertRaisesRegex(ValueError, "requires openssl"):
            onie.build(spec)
        spec["signing_mode"] = "prod"
        with self.assertRaisesRegex(ValueError, "requires a declared"):
            onie.build(spec)
        spec["signing_mode"] = "no_sign"
        for destination in ("default_platform.conf", "./default_platform.conf"):
            spec["extra_files"] = [[spec["machine_conf"], destination]]
            with self.assertRaisesRegex(ValueError, "conflicts"):
                onie.build(spec)

    @unittest.skipUnless(TOOLS.tar_split_json and TOOLS.pigz, "supply tar_split_json and pigz for Docker integration")
    @unittest.skipUnless(TOOLS.tar_split, "supply the independent APT tar-split assembler")
    def test_tar_split_reassembles_original_layer_and_payload_size(self):
        layout, chain = self.layout()
        source_layer = next(p for p in (layout / "blobs/sha256").iterdir() if p.name == chain)
        for compression in ("none", "gzip"):
            output = self.root / (compression + ".tar")
            overlay2_store.build([(layout, "roundtrip:latest")], "", "linux/amd64", output,
                                 tar_split_json=TOOLS.tar_split_json, compression=compression, pigz=TOOLS.pigz)
            with tarfile.open(output) as archive:
                prefix = "image/overlay2/layerdb/sha256/" + chain + "/"
                self.assertEqual(archive.extractfile(prefix + "size").read(), b"7")
                metadata = self.write(compression + ".json.gz", archive.extractfile(prefix + "tar-split.json.gz").read())
                # Populate only regular data required by the independent assembler.
                files = self.root / (compression + "-files/data")
                files.mkdir(parents=True)
                (files / "owned").write_bytes(b"payload")
                (files / "alias").hardlink_to(files / "owned")
                reconstructed = self.root / (compression + "-restored.tar")
                subprocess.run([TOOLS.tar_split, "asm", "--input", str(metadata), "--path", str(files.parent), "--output", str(reconstructed)], check=True)
                self.assertEqual(reconstructed.read_bytes(), source_layer.read_bytes())

    @unittest.skipUnless(TOOLS.openssl, "pass a declared openssl tool for CMS integration")
    def test_dev_signature_verifies_exact_installer_and_prod_contract(self):
        spec, _ = self.onie_spec()
        key, cert = self.root / "key.pem", self.root / "cert.pem"
        subprocess.run([TOOLS.openssl, "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-subj", "/CN=image-rule-test",
                        "-keyout", str(key), "-out", str(cert), "-days", "1"], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        onie.build(spec)
        unsigned = Path(spec["out"]).read_bytes()
        spec.update(signing_mode="dev", openssl=TOOLS.openssl, signing_cert=str(cert), signing_key=str(key))
        onie.build(spec)
        signed = Path(spec["out"]).read_bytes()
        self.assertEqual(signed[:len(unsigned)], unsigned)
        signature = self.write("signature.pem", signed[len(unsigned):])
        content = self.write("unsigned.bin", unsigned)
        subprocess.run([TOOLS.openssl, "cms", "-verify", "-binary", "-inform", "PEM", "-in", str(signature),
                        "-content", str(content), "-noverify", "-out", str(self.root / "verified.bin")], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        self.assertEqual((self.root / "verified.bin").read_bytes(), unsigned)
        # Production tooling uses explicit placeholders, never an inherited env.
        spec.update(signing_mode="prod", signing_tool=TOOLS.openssl,
                    signing_tool_args=["cms", "-sign", "-nosmimecap", "-signer", str(cert), "-inkey", str(key),
                                       "-binary", "-in", "{input}", "-outform", "pem", "-out", "{output}"])
        onie.build(spec)
        self.assertTrue(Path(spec["out"]).read_bytes()[len(unsigned):].startswith(b"-----BEGIN CMS-----"))

    def test_installer_names_and_boot_paths_follow_configuration(self):
        for release, payload, squash, docker in [("vendor-7.2+custom", "custom.zip", "root.sqfs", "containers.gz"),
                                                 ("other-release", "payload.zip", "rootfs.sq", "images.gz")]:
            spec, _ = self.onie_spec()
            metadata = json.loads(Path(spec["kernel_metadata"]).read_text())
            previous = metadata["kernel_release"]
            metadata["kernel_release"] = release
            for record in metadata["boot"].values():
                source = Path(spec["boot_files"]) / record["path"]
                record["path"] = record["path"].replace(previous, release)
                source.rename(Path(spec["boot_files"]) / record["path"])
            Path(spec["kernel_metadata"]).write_text(json.dumps(metadata))
            Path(spec["default_platform_conf"]).write_text('linux /$image_dir/boot/vmlinuz-old-${arch} root=X\ninitrd /$image_dir/boot/initrd.img-old-${arch}\n')
            # A conflicting source setting must be overridden by the selected attrs.
            Path(spec["onie_image_conf"]).write_text('INSTALLER_PAYLOAD=old.zip\nFILESYSTEM_SQUASHFS=old.sq\n')
            Path(spec["arch_image_conf"]).write_text('FILESYSTEM_DOCKERFS=old.gz\n')
            spec.update(payload_name=payload, squashfs_name=squash, dockerfs_name=docker,
                        docker_directory="containers", docker_mode="ram")
            onie.build(spec)
            with tarfile.open(fileobj=io.BytesIO(Path(spec["out"]).read_bytes().split(b"exit_marker\n", 1)[1])) as archive:
                for field in ("onie-image.conf", "onie-image-amd64.conf"):
                    config = archive.extractfile("installer/" + field).read().decode()
                    self.assertIn("INSTALLER_PAYLOAD=" + payload + "\n", config)
                    self.assertIn("FILESYSTEM_SQUASHFS=" + squash + "\n", config)
                    self.assertIn("FILESYSTEM_DOCKERFS=" + docker + "\n", config)
                    self.assertIn("docker_inram=on\n", config)
                config = archive.extractfile("installer/default_platform.conf").read().decode()
                self.assertIn("/boot/vmlinuz-" + release + " root=X", config)
                self.assertIn("/boot/initrd.img-" + release + "\n", config)
                with zipfile.ZipFile(io.BytesIO(archive.extractfile("installer/" + payload).read())) as z:
                    self.assertEqual(z.read(squash), b"squashfs bytes")
                    self.assertEqual(gzip.decompress(z.read(docker)), b"docker bytes")
                    self.assertEqual(z.read("boot/initrd.img-" + release), b"initrd bytes")

    def test_grub_references_follow_version_root_and_squashfs(self):
        from image_config import boot_config, image_options
        text = "linux /image-old/boot/vmlinuz-obsolete root=UUID=old loop=image-old/old.squashfs\ninitrd /image-old/boot/initrd.img-obsolete\n"
        rendered = boot_config(text, {"kernel_release": "vendor-release"},
                               image_options({"squashfs_name": "root.sqfs"}), "new-version", "LABEL=SONiC-OS")
        self.assertEqual(rendered, "linux /image-new-version/boot/vmlinuz-vendor-release root=LABEL=SONiC-OS loop=image-new-version/root.sqfs\ninitrd /image-new-version/boot/initrd.img-vendor-release\n")

    def test_configured_names_reject_conflicts_and_arch_mismatch(self):
        for settings in ({"payload_name": "install.sh"}, {"squashfs_name": "boot"},
                         {"dockerfs_name": "../escape"}):
            spec, _ = self.onie_spec()
            spec.update(settings)
            with self.assertRaises(ValueError):
                onie.build(spec)
        spec, _ = self.onie_spec()
        spec["installer_variables"]["ARCH"] = "arm64"
        with self.assertRaisesRegex(ValueError, "ARCH differs"):
            onie.build(spec)

    @unittest.skipUnless(TOOLS.zstd and TOOLS.tar_split_json and TOOLS.pigz, "supply zstd, tar_split_json and pigz for codec interoperability")
    def test_zstd_docker_output_and_installer_ram_payload(self):
        layout, chain = self.layout()
        output = self.root / "docker.zst"
        overlay2_store.build([(layout, "zstd:latest")], "", "linux/amd64", output,
                             tar_split_json=TOOLS.tar_split_json, compression="zstd", zstd=TOOLS.zstd, pigz=TOOLS.pigz)
        decoded = subprocess.check_output([TOOLS.zstd, "-q", "-d", "-c", str(output)])
        with tarfile.open(fileobj=io.BytesIO(decoded)) as archive:
            self.assertEqual(archive.extractfile("overlay2/" + chain + "/diff/data/owned").read(), b"payload")
        spec, _ = self.onie_spec()
        spec.update(dockerfs=str(output), dockerfs_compression="zstd", docker_mode="ram")
        onie.build(spec)
        with tarfile.open(fileobj=io.BytesIO(Path(spec["out"]).read_bytes().split(b"exit_marker\n", 1)[1])) as archive:
            with zipfile.ZipFile(io.BytesIO(archive.extractfile("installer/fs.zip").read())) as z:
                self.assertEqual(z.read("dockerfs.tar.gz"), output.read_bytes())
        spec["docker_mode"] = "disk"
        with self.assertRaisesRegex(ValueError, "docker_mode=ram"):
            onie.build(spec)

    def test_signed_efi_mapping_preserves_loader_and_vendor_files(self):
        shim = self.write("signed-shim.efi", b"signed shim")
        grub = self.write("signed-grub.efi", b"signed grub")
        mok = self.write("signed-mok.efi", b"signed mok")
        layout = {"grub_partition_role": "sonic", "grub_directory": "grub", "efi_boot_filename": "BOOTX64.EFI"}
        spec = {"uefi_mode": "signed", "efi_bootloader": "EFI/SONiC-OS/shimx64.efi", "efi_files": {
            "EFI/SONiC-OS/shimx64.efi": str(shim), "EFI/SONiC-OS/grubx64.efi": str(grub), "EFI/SONiC-OS/mmx64.efi": str(mok)}}
        files = disk_image.efi_entries(spec, layout, self.root, [{"role": "sonic", "label": "SONiC-OS"}], None)
        self.assertEqual(files["EFI/BOOT/BOOTX64.EFI"].read_bytes(), shim.read_bytes())
        self.assertEqual(files["EFI/SONiC-OS/grubx64.efi"].read_bytes(), grub.read_bytes())
        self.assertIn("/grub", files["EFI/debian/grub.cfg"].read_text())
        for filename in ("../BOOTX64.EFI", "EFI/BOOTX64.EFI", r"..\BOOTX64.EFI"):
            invalid = dict(layout, efi_boot_filename=filename)
            with self.assertRaisesRegex(ValueError, "basename"):
                disk_image.efi_entries(spec, invalid, self.root, [{"role": "sonic", "label": "SONiC-OS"}], None)
        invalid_files = dict(spec["efi_files"])
        invalid_files[r"EFI/SONiC-OS\extra.efi"] = str(grub)
        with self.assertRaisesRegex(ValueError, "POSIX path"):
            disk_image.efi_entries(dict(spec, efi_files=invalid_files), layout, self.root,
                                   [{"role": "sonic", "label": "SONiC-OS"}], None)
        invalid = dict(layout, grub_platform="x86_64-efi", grub_prefix="/grub",
                       grub_modules=[], efi_grub_filename=r"grub\extra.efi")
        with patch.object(disk_image, "run"):
            with self.assertRaisesRegex(ValueError, "POSIX path"):
                disk_image.efi_entries({"uefi_mode": "grub", "grub_mkimage": "unused"}, invalid, self.root,
                                       [{"role": "sonic", "label": "SONiC-OS"}], self.root)
        spec["efi_bootloader"] = "missing.efi"
        with self.assertRaisesRegex(ValueError, "missing"):
            disk_image.efi_entries(spec, layout, self.root, [{"role": "sonic", "label": "SONiC-OS"}], None)

    @unittest.skipUnless(TOOLS.grub_editenv, "pass a declared grub-editenv tool for environment integration")
    def test_grub_environment_supports_saved_entry_and_next_entry(self):
        path = disk_image.prepare_grub_environment({"grub_editenv": TOOLS.grub_editenv}, self.root)
        result = subprocess.run([TOOLS.grub_editenv, str(path), "list"], check=True, capture_output=True)
        self.assertEqual(result.stdout, b"")
        subprocess.run([TOOLS.grub_editenv, str(path), "set", "saved_entry=SONiC-OS-test", "next_entry=ONIE"], check=True)
        result = subprocess.run([TOOLS.grub_editenv, str(path), "list"], check=True, capture_output=True)
        self.assertIn(b"saved_entry=SONiC-OS-test", result.stdout)
        self.assertIn(b"next_entry=ONIE", result.stdout)

    def test_rootfs_merged_usr_whiteout_and_hardlink_survival(self):
        first = self.root / "first.tar"
        second = self.root / "second.tar"
        with tarfile.open(first, "w") as archive:
            link = tarfile.TarInfo("bin")
            link.type, link.linkname = tarfile.SYMTYPE, "usr/bin"
            archive.addfile(link)
            directory = tarfile.TarInfo("usr/bin")
            directory.type = tarfile.DIRTYPE
            archive.addfile(directory)
            owned = tarfile.TarInfo("bin/program")
            owned.uid, owned.gid, owned.mode, owned.size = 1001, 42, 0o4750, 3
            archive.addfile(owned, io.BytesIO(b"old"))
            alias = tarfile.TarInfo("usr/bin/alias")
            alias.type, alias.linkname = tarfile.LNKTYPE, "bin/program"
            alias.uid, alias.gid, alias.mode = 1001, 42, 0o4750
            archive.addfile(alias)
        with tarfile.open(second, "w") as archive:
            archive.addfile(tarfile.TarInfo("usr/bin/.wh.program"))
        tree, opened = merge_layers.Tree(), []
        try:
            for path in (first, second):
                layer = tarfile.open(path)
                opened.append(layer)
                merge_layers.apply_layer(tree, layer, str(path))
            output = io.BytesIO()
            with tarfile.open(fileobj=output, mode="w", format=tarfile.PAX_FORMAT) as archive:
                merge_layers.emit(tree, archive, [])
            with tarfile.open(fileobj=io.BytesIO(output.getvalue())) as archive:
                self.assertNotIn("usr/bin/program", archive.getnames())
                self.assertTrue(archive.getmember("bin").issym())
                alias = archive.getmember("usr/bin/alias")
                self.assertEqual(archive.extractfile(alias).read(), b"old")
                self.assertEqual((alias.uid, alias.gid, alias.mode), (1001, 42, 0o4750))
        finally:
            for layer in opened:
                layer.close()

    def test_partition_layout_rejects_overlaps_and_disk_overflow(self):
        valid = {"partitions": [
            {"number": 2, "role": "onie", "start_mib": 2, "size_mib": 8},
            {"number": 3, "role": "sonic", "start_mib": 10},
        ]}
        partitions = disk_image.layout_partitions(valid, 64)
        self.assertEqual(partitions[1]["start_mib"] + partitions[1]["size_mib"], 63)
        for start in (9, 64):
            invalid = {"partitions": [dict(p) for p in valid["partitions"]]}
            invalid["partitions"][1]["start_mib"] = start
            with self.assertRaises(ValueError):
                disk_image.layout_partitions(invalid, 64)


if __name__ == "__main__":
    unittest.main()
