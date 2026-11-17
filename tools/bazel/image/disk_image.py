"""Populate sparse GPT filesystems from declared inputs, then export qcow2."""
import argparse
import copy
import json
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import subprocess
import tarfile
import tempfile

from overlay2_store import StoreWriter, entry, relative
from kernel_inputs import boot_inputs, boot_mode
from image_config import image_options, boot_config, archive_input, compressed_output
import shutil

MIB = 1024 * 1024


def run(tool, *arguments):
    subprocess.run([tool, *map(str, arguments)], check=True)


def file_member(archive, name, source, mode=0o644):
    source = Path(source)
    info = tarfile.TarInfo(name)
    info.mode, info.size = mode, source.stat().st_size
    with source.open("rb") as stream:
        archive.addfile(info, stream)


def tree_members(archive, tree, prefix=""):
    """Read GRUB resources, following Bazel's materialized input links.

    Resource files have explicit root ownership and non-executable modes.
    Filesystem payloads use TAR instead, preserving their own link metadata.
    """
    tree = Path(tree)
    for source in sorted(tree.rglob("*")):
        name = "/".join(filter(None, [prefix, source.relative_to(tree).as_posix()]))
        info = tarfile.TarInfo(name)
        if source.is_dir():
            info.type, info.mode = tarfile.DIRTYPE, 0o755
        elif source.is_file():
            file_member(archive, name, source, 0o644)
            continue
        else:
            raise ValueError("unexpected source tree member " + str(source))
        archive.addfile(info)


def tar_members(archive, path, prefix="", zstd=None):
    with archive_input(path, zstd) as source:
        for original in source:
            name = relative(original.name)
            if not name:
                continue
            info = copy.copy(original)
            info.pax_headers = dict(original.pax_headers)
            for key in ("path", "linkpath"):
                info.pax_headers.pop(key, None)
            info.name = "/".join(filter(None, [prefix, name]))
            if info.islnk():
                info.linkname = "/".join(filter(None, [prefix, relative(info.linkname)]))
            archive.addfile(info, source.extractfile(original) if info.isfile() else None)


def add_input(archive, source, prefix=""):
    if Path(source).is_dir():
        raise ValueError("filesystem payload needs an installation TAR: " + str(source))
    tar_members(archive, source, prefix)


def populate_ext4(spec, disk, partition, size_mib, emit):
    process = subprocess.Popen([spec["mke2fs"], "-F", "-t", "ext4", "-L", partition["label"],
                                # The raw file is newly truncated and already sparse/zeroed.
                                # Avoid issuing discard I/O against the underlying build disk.
                                "-E", "offset=" + str(partition["start_mib"] * MIB) + ",root_owner=0:0,nodiscard",
                                "-d", "-", str(disk), str(size_mib) + "M"], stdin=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdin, mode="w|", format=tarfile.PAX_FORMAT) as archive:
            emit(StoreWriter(archive))
    except BrokenPipeError:
        pass
    finally:
        try:
            process.stdin.close()
        except BrokenPipeError:
            pass
        returncode = process.wait()
    if returncode:
        raise subprocess.CalledProcessError(returncode, process.args)


def layout_partitions(layout, size_mib):
    result, end = [], 1
    numbers, roles = set(), set()
    for original in sorted(layout["partitions"], key=lambda p: p["start_mib"]):
        p = dict(original)
        if p["number"] in numbers or p["role"] in roles:
            raise ValueError("duplicate disk partition number/role")
        numbers.add(p["number"])
        roles.add(p["role"])
        p["size_mib"] = p.get("size_mib") or size_mib - p["start_mib"] - 1
        if p["start_mib"] < end or p["size_mib"] <= 0 or p["start_mib"] + p["size_mib"] > size_mib - 1:
            raise ValueError("overlapping or out-of-bounds partition " + p["role"])
        end = p["start_mib"] + p["size_mib"]
        result.append(p)
    if not {"onie", "sonic"}.issubset(roles):
        raise ValueError("disk layout requires ONIE and SONiC partitions")
    return result


def prepare_grub_environment(spec, work):
    path = Path(work) / "grubenv"
    run(spec["grub_editenv"], path, "create")
    return path


def efi_entries(spec, layout, work, partitions, assets):
    """EFI paths follow the installer; signed files remain byte-identical."""
    fallback_name = layout["efi_boot_filename"]
    if not isinstance(fallback_name, str) or "\\" in fallback_name:
        raise ValueError("EFI fallback filename must be a basename without backslashes")
    fallback_path = PurePosixPath(fallback_name)
    if fallback_path.is_absolute() or len(fallback_path.parts) != 1 or ".." in fallback_path.parts:
        raise ValueError("EFI fallback filename must be a basename")
    fallback_name = fallback_path.name
    mode = spec.get("uefi_mode", "grub")
    if mode == "signed":
        files = {relative(name): Path(path) for name, path in spec["efi_files"].items()}
        loader = spec["efi_bootloader"]
        if loader not in files:
            raise ValueError("signed EFI bootloader missing from efi_files")
    elif mode == "grub":
        firmware = work / "boot.efi"
        run(spec["grub_mkimage"], "--format=" + layout["grub_platform"], "--directory=" + str(assets),
            "--prefix=" + layout["grub_prefix"], "--output=" + str(firmware), *layout["grub_modules"])
        files = {"EFI/" + next(p["label"] for p in partitions if p["role"] == "sonic") + "/" + relative(layout["efi_grub_filename"]): firmware}
        loader = next(iter(files))
    else:
        raise ValueError("unsupported UEFI mode " + mode)
    for name in files:
        # Mtools treats both slash and backslash as directory separators.
        if "\\" in name or not name.startswith("EFI/") or name.endswith("/"):
            raise ValueError("EFI file destination must be a POSIX path beneath EFI/: " + name)
    fallback = "EFI/BOOT/" + fallback_name
    if fallback in files and files[fallback] != files[loader]:
        raise ValueError("EFI fallback conflicts with selected bootloader")
    files[fallback] = files[loader]
    if "EFI/debian/grub.cfg" not in files:
        label = next(p["label"] for p in partitions if p["role"] == layout["grub_partition_role"])
        # GRUB uses shell-style quoting. Literal quotes are escaped outside quotes.
        quote = shlex.quote
        config = work / "efi-grub.cfg"
        config.write_text("search --no-floppy --label --set=root " + quote(label) + "\nset prefix=($root)" +
                          quote("/" + relative(layout["grub_directory"])) + "\nconfigfile $prefix/grub.cfg\n")
        files["EFI/debian/grub.cfg"] = config
    return files


def build(spec):
    names = image_options(spec)
    layout = json.loads(Path(spec["partition_layout"]).read_text())
    firmware = layout["firmware"]
    if firmware not in ("bios", "uefi"):
        raise ValueError("unrecognized firmware " + firmware)
    required = ["mke2fs", "debugfs", "sgdisk", "qemu_img", "grub_editenv"]
    required += ["grub_mkimage", "grub_bios_setup"] if firmware == "bios" else ["mkfs_fat", "mcopy", "mmd"]
    if firmware == "uefi" and spec.get("uefi_mode", "grub") == "grub":
        required.append("grub_mkimage")
    missing = [name for name in required if not spec.get(name)]
    if missing:
        raise ValueError("missing tools for " + firmware + ": " + ", ".join(missing))
    kernel, boot_files = boot_inputs(spec["kernel_metadata"], spec["boot_files"])
    version = Path(spec["image_version_file"]).read_text().strip()
    if not version or "/" in version or any(c in version for c in ('"', '\n', '\r', '\0')) or version in (".", ".."):
        raise ValueError("unsafe or empty image version")
    partitions = layout_partitions(layout, spec["disk_size_mib"])
    if layout["grub_partition_role"] not in ("onie", "sonic"):
        raise ValueError("GRUB resources need an ext4 ONIE or SONiC partition")
    grub_directory = relative(layout["grub_directory"])
    if not grub_directory:
        raise ValueError("empty GRUB resource directory")
    asset_root = Path(spec["grub_assets"]).absolute() / spec.get("grub_assets_subdir", "")
    image = "image-" + version
    output = Path(spec["out"]).absolute()
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".disk-", dir=output.parent) as temporary:
        work = Path(temporary)
        disk = work / "disk.raw"
        with disk.open("wb") as stream:
            stream.truncate(spec["disk_size_mib"] * MIB)
        args = ["-o"]
        for p in partitions:
            args += ["-n", f"{p['number']}:{p['start_mib'] * 2048}:+{p['size_mib']}M",
                     "-t", f"{p['number']}:{p['type']}", "-c", f"{p['number']}:{p['label']}"]
        run(spec["sgdisk"], *args, disk)
        grubenv = prepare_grub_environment(spec, work)
        grub_config = work / "grub.cfg"
        sonic_label = next(p["label"] for p in partitions if p["role"] == "sonic")
        grub_config.write_text(boot_config(Path(spec["grub_config"]).read_text(), kernel, names,
                                         version, "LABEL=" + sonic_label))
        docker_archive = spec.get("docker_store")
        if docker_archive:
            with open(docker_archive, "rb") as source:
                magic = source.read(4)
            actual = "gzip" if magic[:2] == b"\x1f\x8b" else ("zstd" if magic == b"\x28\xb5\x2f\xfd" else "none")
            if actual != spec.get("docker_store_compression", "gzip"):
                raise ValueError("docker_store_compression does not match input stream")
        if docker_archive and names["docker_mode"] == "ram" and spec.get("docker_store_compression", "gzip") == "none":
            compressed = work / names["dockerfs_name"]
            with open(docker_archive, "rb") as source, compressed_output(compressed, "gzip", pigz=spec.get("pigz")) as destination:
                shutil.copyfileobj(source, destination, 1024 * 1024)
            docker_archive = str(compressed)
        def emit_grub(archive, partition):
            if partition["role"] == layout["grub_partition_role"]:
                tree_members(archive, asset_root / layout["grub_platform"], grub_directory + "/" + layout["grub_platform"])
                file_member(archive, grub_directory + "/grub.cfg", grub_config)
                file_member(archive, grub_directory + "/grubenv", grubenv)
        for p in partitions:
            if p["role"] == "onie":
                def emit_onie(archive):
                    add_input(archive, spec["onie_boot"])
                    emit_grub(archive, p)
                populate_ext4(spec, disk, p, p["size_mib"], emit_onie)
            elif p["role"] == "sonic":
                def emit_sonic(archive):
                    entry(archive, image, mode=0o755, kind=tarfile.DIRTYPE)
                    entry(archive, image + "/boot", mode=0o744, kind=tarfile.DIRTYPE)
                    for boot_file in boot_files:
                        name = boot_file.relative_to(Path(spec["boot_files"]) / "boot").as_posix()
                        if name == "initrd.img-" + kernel["kernel_release"]:
                            continue
                        if boot_file.is_dir():
                            entry(archive, image + "/boot/" + name, mode=boot_mode(spec, name, True), kind=tarfile.DIRTYPE)
                        else:
                            file_member(archive, image + "/boot/" + name, boot_file,
                                        boot_mode(spec, name))
                    file_member(archive, image + "/boot/initrd.img-" + kernel["kernel_release"], spec["initrd"])
                    file_member(archive, "machine.conf", spec["machine_conf"])
                    emit_grub(archive, p)
                    for layer in spec["platform_layers"]:
                        add_input(archive, layer, image + "/platform")
                    if spec.get("docker_store"):
                        if names["docker_mode"] == "ram":
                            file_member(archive, image + "/" + names["dockerfs_name"], docker_archive)
                        else:
                            tar_members(archive, docker_archive, image + "/" + names["docker_directory"], spec.get("zstd"))
                populate_ext4(spec, disk, p, p["size_mib"], emit_sonic)
                # Directly write the immutable SquashFS, avoiding a staging copy.
                command = work / "debugfs.commands"
                squashfs = Path(spec["squashfs"]).absolute()
                if any(c in str(squashfs) for c in ('"', '\n')):
                    raise ValueError("unsupported debugfs source pathname")
                command.write_text(f'write "{squashfs}" /{image}/{names["squashfs_name"]}\n')
                device = str(disk) + "?offset=" + str(p["start_mib"] * MIB)
                run(spec["debugfs"], "-w", "-f", command, device)
                result = subprocess.run([spec["debugfs"], "-R", "stat /" + image + "/" + names["squashfs_name"], device],
                                        capture_output=True, text=True, check=True)
                size = re.search(r"\bSize:\s+(\d+)", result.stdout)
                if size is None or int(size.group(1)) != squashfs.stat().st_size:
                    raise ValueError("debugfs did not install the complete SquashFS: " + result.stdout + result.stderr)
        grub = work / "grub"
        grub.mkdir()
        modules = layout["grub_modules"]
        assets = asset_root / layout["grub_platform"]
        if layout["firmware"] == "bios":
            os.symlink(assets / "boot.img", grub / "boot.img")
            run(spec["grub_mkimage"], "--format=i386-pc", "--directory=" + str(assets),
                "--prefix=" + layout["grub_prefix"], "--output=" + str(grub / "core.img"), *modules)
            run(spec["grub_bios_setup"], "--directory=" + str(grub), "--skip-fs-probe", "--device-map=/dev/null", disk)
        elif layout["firmware"] == "uefi":
            efi = next(p for p in partitions if p["role"] == "efi")
            run(spec["mkfs_fat"], "-F", "32", "-n", efi["label"], "--offset", efi["start_mib"] * 2048,
                disk, efi["size_mib"] * 1024)
            files = efi_entries(spec, layout, grub, partitions, assets)
            fat = str(disk) + "@@" + str(efi["start_mib"] * MIB)
            directories = {str(parent) for name in files for parent in PurePosixPath(name).parents if str(parent) != "."}
            for directory in sorted(directories, key=lambda name: (name.count("/"), name)):
                run(spec["mmd"], "-i", fat, "::/" + directory)
            for name, source in files.items():
                run(spec["mcopy"], "-i", fat, source, "::/" + name)
        else:
            raise ValueError("unrecognized firmware " + layout["firmware"])
        run(spec["qemu_img"], "convert", "-f", "raw", "-O", "qcow2", "-S", spec["sparse_threshold_bytes"], disk, output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("spec")
    build(json.loads(Path(parser.parse_args().spec).read_text()))


if __name__ == "__main__":
    main()
