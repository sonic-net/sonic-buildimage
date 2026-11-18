"""Write an ONIE self-extracting installer, streaming the nested payload ZIP.

Only deflated boot members are temporarily compressed to determine their size.
The large SquashFS and Docker members are streamed after a CRC-only read pass.
ONIE BusyBox 1.25.1 rejects ZIP data descriptors, so CRCs must precede data.
No fs.zip, installer staging tree or outer sharch.tar is materialized.
"""
import argparse
import binascii
import hashlib
import json
from contextlib import ExitStack
from functools import partial
import re
import shutil
import subprocess
from pathlib import Path, PurePosixPath
import stat
import struct
import tarfile
import tempfile
import zlib

from kernel_inputs import boot_inputs, boot_mode
from image_config import image_options, installer_config, boot_config, compressed_output


class Member:
    def __init__(self, name, path=None, mode=0o644, deflate=False, cleanup=None):
        self.name, self.path, self.mode = name, Path(path) if path else None, mode
        self.method, self.temporary = (8 if deflate else 0), None
        self.size = self.original_size = self.path.stat().st_size if self.path else 0
        self.crc = 0
        if deflate:
            self.temporary = cleanup.enter_context(tempfile.TemporaryFile()) if cleanup else tempfile.TemporaryFile()
            compressor = zlib.compressobj(6, zlib.DEFLATED, -15)
            with self.path.open("rb") as source:
                for data in iter(lambda: source.read(1024 * 1024), b""):
                    self.crc = binascii.crc32(data, self.crc)
                    self.temporary.write(compressor.compress(data))
            self.temporary.write(compressor.flush())
            self.size = self.temporary.tell()
            self.temporary.seek(0)
        elif self.path:
            with self.path.open("rb") as source:
                for data in iter(lambda: source.read(1024 * 1024), b""):
                    self.crc = binascii.crc32(data, self.crc)

    def chunks(self):
        if self.path is None:
            return
        source = self.temporary or self.path.open("rb")
        try:
            yield from iter(lambda: source.read(1024 * 1024), b"")
        finally:
            source.close()


def zip_size(members):
    return 22 + sum(30 + len(m.name.encode()) + m.size + 46 + len(m.name.encode()) for m in members)


def zip_chunks(members):
    """Non-ZIP64 archive with CRCs in local headers and Unix file modes."""
    central, offset = [], 0
    for member in members:
        name = member.name.encode()
        flags = 0x800  # UTF-8; ONIE's unzip rejects the data-descriptor flag.
        local = struct.pack("<I5H3I2H", 0x04034B50, 20, flags, member.method, 0, 33,
                            member.crc, member.size, member.original_size, len(name), 0) + name
        start = offset
        yield local
        offset += len(local)
        for chunk in member.chunks():
            offset += len(chunk)
            yield chunk
        central.append(struct.pack("<I6H3I5H2I", 0x02014B50, (3 << 8) | 20, 20, flags, member.method, 0, 33,
                                   member.crc, member.size, member.original_size, len(name), 0, 0, 0, 0,
                                   ((stat.S_IFDIR if member.name.endswith("/") else stat.S_IFREG) | member.mode) << 16, start) + name)
    central_start = offset
    for record in central:
        yield record
        offset += len(record)
    yield struct.pack("<I4H2IH", 0x06054B50, 0, 0, len(central), len(central), offset - central_start, central_start, 0)


class ChunksReader:
    def __init__(self, chunks):
        self.chunks, self.buffer = iter(chunks), b""

    def read(self, size):
        while len(self.buffer) < size:
            chunk = next(self.chunks, b"")
            if not chunk:
                break
            self.buffer += chunk
        result, self.buffer = self.buffer[:size], self.buffer[size:]
        return result


class HashWriter:
    def __init__(self, output):
        self.output, self.digest, self.size = output, hashlib.sha1(), 0

    def write(self, data):
        self.digest.update(data)
        self.size += len(data)
        return self.output.write(data)


def add_file(archive, name, path, mode=0o644):
    path = Path(path)
    if path.is_dir():
        directory = tarfile.TarInfo(name)
        directory.type, directory.mode = tarfile.DIRTYPE, mode
        archive.addfile(directory)
        for child in sorted(path.iterdir()):
            add_file(archive, name + "/" + child.name, child, stat.S_IMODE(child.stat().st_mode))
    else:
        info = tarfile.TarInfo(name)
        info.mode, info.size = mode, path.stat().st_size
        with path.open("rb") as source:
            archive.addfile(info, source)


def add_resource(archive, destination, source, modes):
    """Installer resources use explicit modes, independent of sandbox links."""
    source = Path(source)
    if source.is_dir():
        info = tarfile.TarInfo("installer/" + destination)
        info.type = tarfile.DIRTYPE
        info.mode = int(modes.get(destination, "0755"), 8)
        archive.addfile(info)
        for child in sorted(source.iterdir()):
            add_resource(archive, destination + "/" + child.name, child, modes)
    elif source.is_file():
        add_file(archive, "installer/" + destination, source, int(modes.get(destination, "0644"), 8))
    else:
        raise ValueError("installer resource is not a regular file/directory: " + str(source))


def _build(spec):
    with ExitStack() as cleanup:
        _write(spec, cleanup)


def _write(spec, cleanup):
    member = partial(Member, cleanup=cleanup)
    metadata, boot_files = boot_inputs(spec["kernel_metadata"], spec["boot_files"])
    release = metadata["kernel_release"]
    names = image_options(spec)
    reserved = {"install.sh", "machine.conf", "default_platform.conf", "onie-image.conf", "platform.conf",
                "onie-image-" + metadata["architecture"] + ".conf", "platforms_asic", "efi_sbatlevel.py"}
    if names["payload_name"] in reserved or names["dockerfs_name"] in reserved or names["payload_name"] == names["dockerfs_name"]:
        raise ValueError("image name conflicts with installer resource")
    members = [member("boot/", mode=0o744)]
    boot_root = Path(spec["boot_files"]) / "boot"
    for path in boot_files:
        relative = path.relative_to(boot_root).as_posix()
        if relative == "initrd.img-" + release:
            continue
        name = "boot/" + relative
        if path.is_dir():
            members.append(member(name + "/", mode=boot_mode(spec, relative, True)))
        else:
            members.append(member(name, path, boot_mode(spec, relative), not path.name.endswith(".gz")))
    members += [member("boot/initrd.img-" + release, spec["initrd"]),
                member("platform.tar.gz", spec["platform_archive"]),
                member(names["squashfs_name"], spec["squashfs"])]
    external = None
    if spec.get("dockerfs"):
        path = Path(spec["dockerfs"])
        size = path.stat().st_size
        overhead = struct.calcsize("<I5H3I2H") + struct.calcsize("<I6H3I5H2I") + 2 * len(names["dockerfs_name"].encode())
        if size < 2**32 and zip_size(members) + size + overhead < 2**32:
            members.append(member(names["dockerfs_name"], path))
        else:
            external = path
    if len(members) >= 65535 or zip_size(members) >= 2**32 or any(max(m.size, m.original_size) >= 2**32 for m in members):
        raise ValueError("payload exceeds target ONIE non-ZIP64 capability")
    template = Path(spec["sharch_body"]).read_bytes()
    def header(digest, size):
        return template.replace(b"%%IMAGE_SHA1%%", digest.encode()).replace(b"%%PAYLOAD_IMAGE_SIZE%%", f"{size:020d}".encode())
    placeholder = header("0" * 40, 0)
    with Path(spec["out"]).open("w+b") as output:
        output.write(placeholder)
        writer = HashWriter(output)
        with tarfile.open(fileobj=writer, mode="w|", format=tarfile.PAX_FORMAT) as archive:
            root = tarfile.TarInfo("installer")
            root.type, root.mode = tarfile.DIRTYPE, 0o755
            archive.addfile(root)
            add_file(archive, "installer/install.sh", spec["install_sh"], 0o755)
            add_file(archive, "installer/machine.conf", spec["machine_conf"])
            resources = {"default_platform.conf": spec["default_platform_conf"], "onie-image.conf": spec["onie_image_conf"], "platforms_asic": spec["platforms_asic"], "efi_sbatlevel.py": spec["efi_sbatlevel"]}
            if spec.get("arch_image_conf"):
                resources["onie-image-" + metadata["architecture"] + ".conf"] = spec["arch_image_conf"]
            if spec.get("platform_conf"):
                resources["platform.conf"] = spec["platform_conf"]
            for destination, path in resources.items():
                add_file(archive, "installer/" + destination, path)
            for path, destination in spec["extra_files"]:
                normalized = PurePosixPath(destination)
                if normalized.is_absolute() or ".." in normalized.parts or str(normalized) == ".":
                    raise ValueError("unsafe installer destination " + destination)
                if normalized.parts[0] in set(resources) | {"install.sh", "machine.conf", names["payload_name"], names["dockerfs_name"]}:
                    raise ValueError("installer resource conflicts with reserved path: " + destination)
                add_resource(archive, destination, path, spec.get("extra_file_modes", {}))
            if external:
                add_file(archive, "installer/" + names["dockerfs_name"], external)
            info = tarfile.TarInfo("installer/" + names["payload_name"])
            info.mode, info.size = 0o644, zip_size(members)
            archive.addfile(info, ChunksReader(zip_chunks(members)))
        completed = header(writer.digest.hexdigest(), writer.size)
        if len(completed) != len(placeholder):
            raise ValueError("self-extracting header size changed")
        output.seek(0)
        output.write(completed)
    Path(spec["out"]).chmod(0o755)


def build(spec):
    """Prepare declared template/resources, normalize Docker input, append CMS."""
    mode = spec.get("signing_mode", "no_sign")
    if mode not in ("no_sign", "dev", "prod"):
        raise ValueError("unsupported signing mode " + mode)
    if mode == "dev" and not all(spec.get(k) for k in ("openssl", "signing_cert", "signing_key")):
        raise ValueError("dev signing requires openssl, signing_cert and signing_key")
    if mode == "prod" and not spec.get("signing_tool"):
        raise ValueError("prod signing requires a declared signing_tool")
    with tempfile.TemporaryDirectory(prefix="onie-prepare-") as temporary:
        work = Path(temporary)
        prepared = dict(spec)
        names = image_options(spec)
        kernel, _ = boot_inputs(spec["kernel_metadata"], spec["boot_files"])
        for field in ("onie_image_conf", "arch_image_conf", "default_platform_conf"):
            if not spec.get(field):
                continue
            text = Path(spec[field]).read_text()
            text = boot_config(text, kernel, names) if field == "default_platform_conf" else installer_config(text, names)
            destination = work / field
            destination.write_text(text)
            prepared[field] = str(destination)
        if spec.get("installer_variables", {}).get("ARCH", kernel["architecture"]) != kernel["architecture"]:
            raise ValueError("installer ARCH differs from kernel metadata")
        script = Path(spec["install_sh"]).read_text()
        variables = dict(spec.get("installer_variables", {}))
        if spec.get("version_file"):
            version = Path(spec["version_file"]).read_text().strip()
            if not version or any(character in version for character in "\r\n\0"):
                raise ValueError("installer image version must be one nonempty line")
            variables["IMAGE_VERSION"] = version
        for key, value in variables.items():
            script = script.replace("%%" + key + "%%", value)
        unresolved = sorted(set(re.findall(r"%%[A-Z_]+%%", script)))
        if unresolved:
            raise ValueError("unresolved installer variables: " + ", ".join(unresolved))
        installed = work / "install.sh"
        installed.write_text(script)
        prepared["install_sh"] = str(installed)
        for field, name, setting in [("dockerfs", "dockerfs.tar.gz", "dockerfs_compression"),
                                     ("platform_archive", "platform.tar.gz", "platform_compression")]:
            if not spec.get(field):
                continue
            source = Path(spec[field])
            compression = spec.get(setting, "gzip")
            with source.open("rb") as stream:
                magic = stream.read(2)
            if compression == "gzip":
                if magic != b"\x1f\x8b":
                    raise ValueError(setting + "=gzip requires a gzip stream")
            elif compression == "zstd" and field == "dockerfs":
                if magic != b"\x28\xb5" or names["docker_mode"] != "ram":
                    raise ValueError("zstd dockerfs requires a zstd stream and docker_mode=ram")
            elif compression == "none":
                compressed = work / name
                with source.open("rb") as inp, compressed_output(compressed, "gzip", pigz=spec.get("pigz")) as out:
                    shutil.copyfileobj(inp, out, 1024 * 1024)
                prepared[field] = str(compressed)
            else:
                raise ValueError("unsupported " + setting + " " + compression)
        _build(prepared)
        if mode != "no_sign":
            signature = work / "signature.sig"
            if mode == "dev":
                command = [spec["openssl"], "cms", "-sign", "-nosmimecap", "-signer", spec["signing_cert"],
                           "-inkey", spec["signing_key"], "-binary", "-in", spec["out"], "-outform", "pem", "-out", str(signature)]
            else:
                command = [spec["signing_tool"]] + [arg.replace("{input}", spec["out"]).replace("{output}", str(signature)).replace("{mode}", mode)
                          for arg in spec.get("signing_tool_args", ["{input}", "{output}", "{mode}"])]
            subprocess.run(command, check=True)
            if not signature.is_file() or signature.stat().st_size == 0:
                raise ValueError("signing tool did not produce a CMS signature")
            with signature.open("rb") as inp, Path(spec["out"]).open("ab") as out:
                shutil.copyfileobj(inp, out, 1024 * 1024)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("spec")
    build(json.loads(Path(parser.parse_args().spec).read_text()))


if __name__ == "__main__":
    main()
