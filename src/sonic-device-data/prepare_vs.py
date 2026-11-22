"""Produce the installed VS device-data layout for the Bazel TAR rule."""

import hashlib
import json
import os
from pathlib import Path
import runpy
import shutil
import subprocess
import sys
import tempfile

from generate_vs_hwsku import hwsku_directories, process_hwsku

KVM = "x86_64-kvm_x86_64-r0"
INSTALL = Path("usr/share/sonic/device")
DOC = Path("usr/share/doc/sonic-device-data")

# The original generator keeps the first copy of a shared SKU. Filesystem
# enumeration varies between workers. These choices preserve the installed
# VS configuration from sonic-buildimage PR #251; other ties use lexical order.
PREFERRED_PLATFORM = {
    "Arista-7060X6-64PE-O128": "x86_64-arista_7060x6_64pe_b",
    "Arista-7060X6-64PE-P32O64": "x86_64-arista_7060x6_64pe_b",
    "Arista-7060X6-64PE-P64": "x86_64-arista_7060x6_64pe_b",
    "Arista-7280CR3-32P4": "x86_64-arista_7280cr3mk_32p4",
    "Arista-7280CR3-C32P4": "x86_64-arista_7280cr3mk_32p4",
    "Arista-7280CR3-C40": "x86_64-arista_7280cr3mk_32p4",
    "FALCON32X25G": "x86_64-marvell_db98cx8580_32cd-r0",
    "FALCON_DB": "x86_64-marvell_db98cx8580_32cd-r0",
    "M2-W6940-128X1-FR4": "x86_64-micas_m2-w6940-128x1-fr4-r0",
    "M2-W6940-64X1-FR4": "x86_64-micas_m2-w6940-128x1-fr4-r0",
    "Mellanox-SN5640-C512S2": "x86_64-nvidia_sn5640_simx-r0",
    "NH-4010": "x86_64-nexthop_4010-r1",
    "NH-4010-128x400G-2x25G": "x86_64-nexthop_4010-r1",
    "NH-4010-256x200G-2x25G": "x86_64-nexthop_4010-r1",
    "Pensando-elba": "arm64-elba-asic-r0",
    "db98cx8580_32cd": "x86_64-marvell_db98cx8580_32cd-r0",
    "pddf": "x86_64-cel_ds1000-r0",
}


def files_below(root):
    for directory, _, files in os.walk(root, followlinks=True):
        for name in sorted(files):
            yield Path(directory, name)


def file_digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def validate(device, tests, platforms, vs_asic):
    # Each owner checker accepts the complete file list in one invocation.
    # Keep its working directory for config_checker's permitted_list.
    inputs = sorted(files_below(device))
    for checker, pattern in (
        ("config_checker", "*.config.bcm"),
        ("media_checker", "media_settings.json"),
        ("platform_json_checker", "platform.json"),
        ("hwsku_json_checker", "hwsku.json"),
    ):
        matches = [f.absolute() for f in inputs if f.match(pattern)]
        if matches:
            result = subprocess.run(
                [sys.executable, checker, *matches], cwd=tests, capture_output=True, text=True
            )
            if result.returncode:
                sys.stderr.write(result.stdout + result.stderr)
                result.check_returncode()
    check = runpy.run_path(str(tests / "platform_asic_checker"))["check_device_tree"]
    if not check(str(device), set(platforms), {f"virtual/{KVM}": str(vs_asic)}):
        raise ValueError("invalid platform ASIC selection")


def mtree_escape(path):
    # BSD mtree uses octal escapes, including for spaces in source paths.
    return "".join(chr(c) if 33 <= c < 127 and c not in b"#\\=" else f"\\{c:03o}"
                   for c in os.fsencode(path))


def prepare(device, owner, meta, changelog, generated, output):
    metadata = json.loads(meta.read_text())
    executable = set(metadata["executables"])
    files = {}
    sources = {}

    def mode(path):
        if path.is_relative_to(device) and str(path.relative_to(device)) in executable:
            # dh_fixperms clears executable bits on shared libraries.
            if not path.name.endswith(".so"):
                return 0o755
        return 0o644

    for source in sorted(files_below(device)):
        parts = source.relative_to(device).parts
        if len(parts) < 3 or any(p.startswith(".") for p in parts[:2]):
            continue
        destination = INSTALL.joinpath(*parts[1:])
        if destination in files:
            raise ValueError(f"{source} and {files[destination][0]} both install {destination}")
        files[destination] = (source, mode(source))
        sources.setdefault(file_digest(source), source)
    for source in sorted((owner / "src").glob("*.vs_profile")) + [owner / "src/fabriclanemap_vs.ini"]:
        sources.setdefault(file_digest(source), source)

    generated.mkdir(exist_ok=True, parents=True)

    def copy_file(src, dst):
        src = Path(src)
        shutil.copyfile(src, dst)
        Path(dst).chmod(mode(src))
        return dst

    def sku_order(sku):
        # The first platform wins when the same SKU appears in multiple trees.
        return (sku.parent.name != PREFERRED_PLATFORM.get(sku.name), sku)

    with tempfile.TemporaryDirectory() as temp:
        stage = Path(temp)
        virtual = stage / "device" / KVM
        shutil.copytree(device / "virtual" / KVM, virtual, copy_function=copy_file)
        vs_asic = virtual / "platform_asic"
        vs_asic.write_text("vs\n")
        validate(device, owner / "tests", metadata["platforms"], vs_asic)
        for sku in sorted(hwsku_directories(device), key=sku_order):
            process_hwsku(sku, stage, device, templates=owner / "src", copy_file=copy_file)

        # Most virtual profiles are identical to a declared physical input.
        # Reference that input directly rather than uploading thousands of
        # duplicate files as intermediate remote-cache outputs.
        for path in files_below(virtual):
            digest = file_digest(path)
            content = sources.get(digest)
            if content is None:
                content = generated / digest
                shutil.copyfile(path, content)
                sources[digest] = content
            bits = 0o755 if path.stat().st_mode & 0o111 else 0o644
            files[INSTALL / KVM / path.relative_to(virtual)] = (content, bits)

    files[DOC / "changelog.Debian.gz"] = (changelog, 0o644)
    files[DOC / "copyright"] = (owner / "src/debian/copyright", 0o644)
    parents = {p for file in files for p in file.parents}
    with output.open("w") as manifest:
        manifest.write("#mtree\n/set uid=0 gid=0 time=0\n")
        for path in sorted(parents | files.keys()):
            name = "." if path == Path(".") else "./" + mtree_escape(path)
            if path in parents:
                manifest.write(f"{name} type=dir mode=0755\n")
            else:
                source, bits = files[path]
                manifest.write(f"{name} type=file mode={bits:04o} content={mtree_escape(source)}\n")


if __name__ == "__main__":
    device_anchor, owner_anchor, *inputs = map(Path, sys.argv[1:])
    prepare(device_anchor.parent, owner_anchor.parent.parent, *inputs)
