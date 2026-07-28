#!/usr/bin/env python3
"""Generate VS HwSKU data from platform device directories.

Replaces the bash loop in sonic-device-data/src/Makefile that takes ~9 minutes
by processing all HwSKUs in parallel with native Python file I/O.
"""
import os
import shutil
import sys
from pathlib import Path

def process_hwsku(hwsku_dir, src_dir, device_base, vs_platform="x86_64-kvm_x86_64-r0"):
    """Process a single HwSKU directory for VS mapping."""
    hwsku_name = hwsku_dir.name
    vs_hwsku = Path(src_dir, "device", vs_platform, hwsku_name)
    parent = hwsku_dir.parent

    # Check if VS hwsku already exists (from device copy)
    if vs_hwsku.exists():
        # Just copy profile files
        for profile in ["sai.vs_profile:sai.profile",
                        "sai_mlnx.vs_profile:sai_mlnx.profile",
                        "sai_vpp.vs_profile:sai_vpp.profile",
                        "pai.vs_profile:pai.profile",
                        "fabriclanemap_vs.ini:fabriclanemap.ini"]:
            src_name, dst_name = profile.split(":")
            src_path = Path(src_dir, src_name)
            if src_path.exists():
                shutil.copy2(str(src_path), str(vs_hwsku / dst_name))
        return

    # Copy HwSKU dir to VS platform
    shutil.copytree(str(hwsku_dir), str(vs_hwsku), symlinks=False)

    # Copy asic.conf if exists
    asic_conf = parent / "asic.conf"
    if asic_conf.exists():
        shutil.copy2(str(asic_conf), str(vs_hwsku / "asic.conf"))

    # Copy profile files
    for profile in ["sai.vs_profile:sai.profile",
                    "sai_mlnx.vs_profile:sai_mlnx.profile",
                    "sai_vpp.vs_profile:sai_vpp.profile",
                    "pai.vs_profile:pai.profile",
                    "fabriclanemap_vs.ini:fabriclanemap.ini"]:
        src_name, dst_name = profile.split(":")
        src_path = Path(src_dir, src_name)
        if src_path.exists():
            shutil.copy2(str(src_path), str(vs_hwsku / dst_name))

    # Remove context_config.json if exists
    ctx_cfg = vs_hwsku / "context_config.json"
    if ctx_cfg.exists():
        ctx_cfg.unlink()

    # Check for chassis and midplane
    chassisdb = parent / "chassisdb.conf"
    has_chassis = chassisdb.exists()
    reserve_midplane = False
    if has_chassis:
        platform_env = parent / "platform_env.conf"
        if platform_env.exists():
            with open(platform_env) as f:
                reserve_midplane = not any(
                    line.strip().startswith("disaggregated_chassis=1")
                    for line in f
                )
        else:
            reserve_midplane = True

    # Generate lanemap.ini and coreportindexmap.ini from port_config.ini
    port_config = vs_hwsku / "port_config.ini"
    if port_config.exists():
        generate_lanemap(vs_hwsku, port_config, has_chassis, reserve_midplane)

    # Process numbered subdirectories (0, 1, 2) for multi-ASIC
    i = 1 if reserve_midplane else 0
    for subdir_idx in ["0", "1", "2"]:
        subdir = vs_hwsku / subdir_idx
        if subdir.is_dir():
            # Copy profiles to subdir
            for profile in ["sai.vs_profile:sai.profile",
                            "fabriclanemap_vs.ini:fabriclanemap.ini"]:
                src_name, dst_name = profile.split(":")
                src_path = Path(src_dir, src_name)
                if src_path.exists():
                    shutil.copy2(str(src_path), str(subdir / dst_name))

            # Remove context_config.json
            sub_ctx = subdir / "context_config.json"
            if sub_ctx.exists():
                sub_ctx.unlink()

            # Generate lanemap for subdir
            sub_port_config = subdir / "port_config.ini"
            if sub_port_config.exists():
                i = generate_lanemap_for_subdir(subdir, sub_port_config, has_chassis, i)

def generate_lanemap_for_subdir(target_dir, port_config_path, has_chassis, start_i):
    """Generate lanemap.ini and coreportindexmap.ini, returning the updated i."""
    lanemap_lines = []
    coremap_lines = []
    i = start_i
    num_columns = 0

    with open(port_config_path) as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or not line.strip() or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 2:
                continue
            num_columns = len(parts)
            i += 1
            lanes = parts[1]
            lanemap_lines.append(f"eth{i}:{lanes}")
            if num_columns >= 9:
                core = parts[7]
                core_port = parts[8]
                coremap_lines.append(f"eth{i}:{core},{core_port}")

    if lanemap_lines:
        lanemap_path = target_dir / "lanemap.ini"
        with open(lanemap_path, "w") as f:
            f.write("\n".join(lanemap_lines) + "\n")
            if has_chassis:
                f.write("Cpu0:999\n")
                i += 1  # bash loop increments i for Cpu0 if lanemap.ini is written

    if coremap_lines:
        coremap_path = target_dir / "coreportindexmap.ini"
        with open(coremap_path, "w") as f:
            f.write("\n".join(coremap_lines) + "\n")
            if has_chassis:
                f.write("Cpu0:0,0\n")

    return i

def generate_lanemap(target_dir, port_config_path, has_chassis, reserve_midplane):
    """Generate lanemap.ini and coreportindexmap.ini from port_config.ini."""
    lanemap_lines = []
    coremap_lines = []
    i = 1 if reserve_midplane else 0
    num_columns = 0

    with open(port_config_path) as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or not line.strip() or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 2:
                continue
            num_columns = len(parts)
            i += 1
            lanes = parts[1]
            lanemap_lines.append(f"eth{i}:{lanes}")
            if num_columns >= 9:
                core = parts[7]
                core_port = parts[8]
                coremap_lines.append(f"eth{i}:{core},{core_port}")

    if lanemap_lines:
        lanemap_path = target_dir / "lanemap.ini"
        with open(lanemap_path, "w") as f:
            f.write("\n".join(lanemap_lines) + "\n")
            if has_chassis:
                f.write("Cpu0:999\n")

    if coremap_lines:
        coremap_path = target_dir / "coreportindexmap.ini"
        with open(coremap_path, "w") as f:
            f.write("\n".join(coremap_lines) + "\n")
            if has_chassis:
                f.write("Cpu0:0,0\n")
def main():
    script_dir = Path(__file__).resolve().parent
    device_base = sys.argv[1] if len(sys.argv) > 1 else str(script_dir / ".." / ".." / ".." / "device")
    src_dir = str(script_dir)

    # Find all HwSKU directories
    hwsku_dirs = []
    for platform_dir in Path(device_base).iterdir():
        if not platform_dir.is_dir():
            continue
        for vendor_dir in platform_dir.iterdir():
            if not vendor_dir.is_dir():
                continue
            for hwsku_dir in vendor_dir.iterdir():
                if not hwsku_dir.is_dir():
                    continue
                # Match bash's grep -vE "(plugins|led-code|sonic_platform)"
                name = hwsku_dir.name
                if "plugins" in name or "led-code" in name or "sonic_platform" in name:
                    continue
                hwsku_dirs.append(hwsku_dir)

    print(f"Processing {len(hwsku_dirs)} HwSKUs...", file=sys.stderr)
    for hwsku_dir in hwsku_dirs:
        process_hwsku(hwsku_dir, src_dir, device_base)
    print(f"Done.", file=sys.stderr)
if __name__ == "__main__":
    main()
