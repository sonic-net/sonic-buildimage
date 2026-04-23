#!/usr/bin/env python3
"""Generate device data for SONiC Bazel builds.

Replicates the logic from src/sonic-device-data/Makefile:
1. Flatten vendor directories: device/VENDOR/PLATFORM/... -> PLATFORM/...
2. Write platform_asic for virtual switch
3. Handle Mellanox simx SAI profiles
4. Generate VS hwsku entries for all hwsku directories
"""

import os
import shutil
from pathlib import Path

KVM_PLATFORM = "x86_64-kvm_x86_64-r0"
EXCLUDED_DIRS = {"plugins", "led-code", "sonic_platform"}


def flatten_vendor_dirs(device_dir: Path, output_dir: Path):
    """Copy device/VENDOR/PLATFORM/* -> output/PLATFORM/*.

    Equivalent to: cp -r -L device/*/* output/
    """
    for vendor_path in sorted(device_dir.iterdir()):
        if not vendor_path.is_dir():
            continue
        for src in sorted(vendor_path.iterdir()):
            if not src.is_dir():
                continue
            dst = output_dir / src.name
            shutil.copytree(src, dst, symlinks=False, dirs_exist_ok=True)


def copy_vs_profiles(vs_dir: Path, dest_dir: Path, full: bool = True):
    """Copy VS profile files to destination with appropriate renaming.

    Args:
        vs_dir: Directory containing VS profile source files.
        dest_dir: Destination directory.
        full: If True, copy all profiles. If False, copy only sai and fabric
              (used for multi-ASIC subdirs).
    """
    copies = [
        ("sai.vs_profile", "sai.profile"),
        ("fabriclanemap_vs.ini", "fabriclanemap.ini"),
    ]
    if full:
        copies += [
            ("sai_mlnx.vs_profile", "sai_mlnx.profile"),
            ("sai_vpp.vs_profile", "sai_vpp.profile"),
            ("pai.vs_profile", "pai.profile"),
        ]
    for src_name, dst_name in copies:
        src = vs_dir / src_name
        if src.is_file():
            dst = dest_dir / dst_name
            shutil.copy2(src, dst)


def process_port_config(port_config: Path, dest_dir: Path, start_idx: int) -> int:
    """Generate lanemap.ini and coreportindexmap.ini from port_config.ini.

    The counter tracks interface numbering (ethN) and is shared across
    multi-ASIC subdirs for continuous numbering.

    Returns the counter value after processing.
    """
    if not port_config.is_file():
        return start_idx

    all_lines = port_config.read_text().splitlines(keepends=True)

    # Determine column count from last non-empty line (matches Makefile's tail -n 1)
    last_line = ""
    for line in reversed(all_lines):
        if line.strip():
            last_line = line.strip()
            break
    num_columns = len(last_line.split())

    i = start_idx

    lanemap_entries = []
    coremap_entries = []

    for line in all_lines:
        stripped = line.strip()
        if stripped.startswith("#") or not stripped:
            continue
        i += 1
        cols = stripped.split()
        if len(cols) >= 2:
            lanemap_entries.append(f"eth{i}:{cols[1]}")
        if num_columns >= 9 and len(cols) >= 9:
            coremap_entries.append(f"eth{i}:{cols[7]},{cols[8]}")

    if lanemap_entries:
        (dest_dir / "lanemap.ini").write_text(
            "".join(entry + "\n" for entry in lanemap_entries)
        )

    if coremap_entries:
        (dest_dir / "coreportindexmap.ini").write_text(
            "".join(entry + "\n" for entry in coremap_entries)
        )

    return i


def append_cpu0(dest_dir: Path):
    """Append Cpu0 entries to lanemap.ini and coreportindexmap.ini if they exist."""
    lanemap = dest_dir / "lanemap.ini"
    if lanemap.is_file():
        with lanemap.open(mode="a") as f:
            f.write("Cpu0:999\n")

    coremap = dest_dir / "coreportindexmap.ini"
    if coremap.is_file():
        with coremap.open(mode="a") as f:
            f.write("Cpu0:0,0\n")


def remove_if_exists(path: Path):
    if path.is_file():
        path.unlink()

def collect_hwsku_dirs(device_dir: Path):
    # Collect the hwsku_dirs as they do in the Makefile
    # for d in `find -L ../../../device -maxdepth 3 -mindepth 3 -type d | grep -vE "(plugins|led-code|sonic_platform)"`; do
    # Note that the iteration order is different (`find` is not deterministic).
    # When we combine that with deduplication (not copying a hwsku that has already appeared in another platform's vendor),
    # It may lead to differences between Bazel and Make.
    # However, these differences should be irrelevant in practice.
    hwsku_dirs = []
    for vendor_path in sorted(device_dir.iterdir()):
        if not vendor_path.is_dir():
            continue
        for platform_path in sorted(vendor_path.iterdir()):
            if not platform_path.is_dir():
                continue


            for hwsku_src in sorted(platform_path.iterdir()):
                if any(excl in hwsku_src.name for excl in EXCLUDED_DIRS):
                    continue
                if not hwsku_src.is_dir():
                    continue

                hwsku_dirs.append(hwsku_src)
    return hwsku_dirs

def generate_vs_hwskus(device_dir: Path, output_dir: Path, vs_dir: Path):
    """Generate VS hwsku entries for all known hwsku directories.

    For each hwsku dir at depth 3 in the device tree (device/VENDOR/PLATFORM/HWSKU):
    - If the hwsku already exists in the kvm platform dir: just copy VS profiles.
    - If new: copy the hwsku, generate lanemap/coreportindexmap, handle multi-ASIC.
    """
    kvm_out = output_dir / KVM_PLATFORM

    hwsku_dirs = collect_hwsku_dirs(device_dir)

    for hwsku_dir in hwsku_dirs:
        platform_path = hwsku_dir.parent
        has_chassisdb = (platform_path / "chassisdb.conf").is_file()

        hwsku_dst = kvm_out / hwsku_dir.name

        if hwsku_dst.is_dir():
            # Existing hwsku in kvm dir: just copy VS profiles and exit
            copy_vs_profiles(vs_dir, hwsku_dst, full=True)
            continue

        # New hwsku: copy entire directory to kvm platform
        shutil.copytree(
            hwsku_dir, hwsku_dst, symlinks=False, dirs_exist_ok=True
        )

        # Copy asic.conf from parent platform if it exists
        asic_conf = platform_path / "asic.conf"
        if asic_conf.is_file():
            shutil.copy2(asic_conf, hwsku_dst / "asic.conf")

        # Copy VS profiles
        copy_vs_profiles(vs_dir, hwsku_dst, full=True)

        # Generate lanemap.ini and coreportindexmap.ini from port_config.ini
        process_port_config(
            hwsku_dst / "port_config.ini", hwsku_dst, 1 if has_chassisdb else 0
        )

        # Remove context_config.json
        remove_if_exists(hwsku_dst / "context_config.json")

        # Append Cpu0 entries if chassisdb exists
        if has_chassisdb:
            append_cpu0(hwsku_dst)

        # Process multi-ASIC subdirs (0, 1, 2)
        # Counter is shared across subdirs for continuous eth numbering
        i = 1 if has_chassisdb else 0
        for subdir_idx in ["0", "1", "2"]:
            subdir = hwsku_dst / subdir_idx
            if not subdir.is_dir():
                continue

            # Subdirs only get sai.vs_profile and fabriclanemap_vs.ini
            copy_vs_profiles(vs_dir, subdir, full=False)

            # Remove context_config.json
            remove_if_exists(subdir / "context_config.json")

            # Generate lanemap and coreportindexmap
            i = process_port_config(
                subdir / "port_config.ini", subdir, i
            )

            # Append Cpu0 if chassisdb
            if has_chassisdb:
                lanemap = subdir / "lanemap.ini"
                if lanemap.is_file():
                    with lanemap.open("a") as f:
                        f.write("Cpu0:999\n")
                    i += 1
                coremap = subdir / "coreportindexmap.ini"
                if coremap.is_file():
                    with coremap.open("a") as f:
                        f.write("Cpu0:0,0\n")


def main():
    ruledir = Path(os.environ.get("RULEDIR", "."))
    output_dir = ruledir / "device_output"
    device_dir = Path("device")
    vs_dir = Path("src", "sonic-device-data", "src")
    platform = os.environ.get("PLATFORM", "vs")

    output_dir.mkdir(parents=True, exist_ok=True)

    # Step 1: Flatten vendor dirs (device/VENDOR/PLATFORM -> output/PLATFORM)
    flatten_vendor_dirs(device_dir, output_dir)

    # Step 2: Write platform_asic
    kvm_dir = output_dir / KVM_PLATFORM
    kvm_dir.mkdir(parents=True, exist_ok=True)
    (kvm_dir / "platform_asic").write_text(platform + "\n")

    # Step 3: Mellanox simx handling
    if platform == "mellanox":
        for root, _dirs, files in os.walk(output_dir):
            if "simx" in root and "sai.profile" in files:
                with (Path(root) / "sai.profile").open(mode='a') as f:
                    f.write("SAI_KEY_IS_SIMX=1\n")

    # Step 4: Generate VS hwsku entries
    generate_vs_hwskus(device_dir, output_dir, vs_dir)


if __name__ == "__main__":
    main()
