#!/usr/bin/env python3
"""Non-destructive host checks, executed as the runner service account."""
import argparse
import fcntl
import gzip
import json
import os
from pathlib import Path
import platform
import shutil
import stat
import subprocess
import tempfile

from apparmor_gs import check_ghostscript


def output(*args):
    result = subprocess.run(args, text=True, capture_output=True)
    if result.returncode:
        diagnostics = (result.stderr or result.stdout).strip()
        raise RuntimeError(f"{args[0]} failed ({result.returncode}): {diagnostics}")
    return result.stdout.strip()


def check_umask(mask, origin):
    # Docker COPY preserves checkout modes but changes ownership to root. Its
    # non-root build user must still read config files and traverse directories.
    if mask & 0o055:
        raise RuntimeError(f"{origin} umask {mask:04o} hides files/directories from Docker build users; use 0022")


def check_worker_umasks(host_agent=False):
    process_mask = next(line.split()[1] for line in Path('/proc/self/status').read_text().splitlines()
                        if line.startswith('Umask:'))
    check_umask(int(process_mask, 8), "Preflight process")
    if host_agent:
        print(f"Host-agent process file creation mask: {process_mask}")
        return
    service_mask = output("systemctl", "show", "sonic-vs-runner.service", "--property=UMask", "--value")
    if not service_mask:
        raise RuntimeError("Runner service umask is unavailable; run bootstrap.sh first")
    check_umask(int(service_mask, 8), "Runner service")
    print(f"File creation masks: process {process_mask}; runner service {service_mask}")


def check_legacy_nat(modules=Path("/proc/modules"), config=None,
                     compressed_config=Path("/proc/config.gz"), builtin_modules=None):
    """The builder uses iptables-legacy even when host Docker uses nftables."""
    kernel = platform.release()
    config = config or Path(f"/boot/config-{kernel}")
    builtin_modules = builtin_modules or Path(f"/lib/modules/{kernel}/modules.builtin")
    try:
        if any(line.split()[0] == "iptable_nat" for line in modules.read_text().splitlines() if line.split()):
            print("Nested Docker: iptable_nat is loaded")
            return
    except (FileNotFoundError, PermissionError):
        pass
    for path in (config, compressed_config):
        try:
            if path == compressed_config:
                with gzip.open(path, "rt") as stream:
                    text = stream.read()
            else:
                text = path.read_text()
            if "CONFIG_IP_NF_NAT=y" in text.splitlines():
                print("Nested Docker: legacy IPv4 NAT is built into the kernel")
                return
        except (FileNotFoundError, PermissionError):
            pass
    try:
        if any(line.endswith("/iptable_nat.ko") for line in builtin_modules.read_text().splitlines()):
            print("Nested Docker: iptable_nat is listed as built into the kernel")
            return
    except (FileNotFoundError, PermissionError):
        pass
    raise RuntimeError("Nested Docker needs host legacy IPv4 NAT support: run sudo modprobe iptable_nat "
                       "and rerun bootstrap.sh to persist it. If missing, install modules matching the running kernel; "
                       "the builder cannot load host modules from its own /lib/modules.")


def check_disk(workspace, docker_root, work_gib, docker_gib):
    """A shared filesystem must cover both budgets, not count free space twice."""
    work_free = shutil.disk_usage(workspace).free / 1024**3
    docker_free = shutil.disk_usage(docker_root).free / 1024**3
    shared = os.stat(workspace).st_dev == os.stat(docker_root).st_dev
    print(f"Free space: workspace {work_free:.1f} GiB; Docker {docker_free:.1f} GiB ({docker_root})")
    if shared:
        if work_free < work_gib + docker_gib:
            raise RuntimeError(f"Shared filesystem needs {work_gib + docker_gib} GiB free")
    elif work_free < work_gib or docker_free < docker_gib:
        raise RuntimeError(f"Need {work_gib} GiB workspace and {docker_gib} GiB Docker free")


def check_data_storage(path=Path("/data"), build_target=None):
    """Allow a real local ext4 directory only for the bin-only build."""
    if os.path.ismount(path):
        print(f"Runner storage: {path} is a mountpoint")
        return
    if build_target != "sonic-vs.bin":
        raise RuntimeError(f"{path} must be mounted")
    if not stat.S_ISDIR(path.lstat().st_mode):
        raise RuntimeError(f"{path} must be a real directory")
    try:
        data = json.loads(output("findmnt", "--json", "--target", str(path), "--output", "SOURCE,FSTYPE"))
    except (TypeError, json.JSONDecodeError) as error:
        raise RuntimeError(f"Could not identify the filesystem backing {path}") from error
    filesystems = data.get("filesystems") if isinstance(data, dict) else None
    if not isinstance(filesystems, list) or len(filesystems) != 1:
        raise RuntimeError(f"Could not identify the filesystem backing {path}")
    filesystem = filesystems[0]
    if not isinstance(filesystem, dict) or not isinstance(filesystem.get("source"), str):
        raise RuntimeError(f"Could not identify the filesystem backing {path}")
    source = filesystem["source"].split("[", 1)[0]
    if filesystem.get("fstype") != "ext4" or not source.startswith("/dev/"):
        raise RuntimeError(f"{path} must be mounted or backed by a local ext4 block filesystem")
    if not stat.S_ISBLK(os.stat(source).st_mode):
        raise RuntimeError(f"The filesystem backing {path} is not a block device: {source}")
    print(f"Runner storage: {path} uses {source} (ext4)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", type=Path, default=Path("/data/sonic-runner"))
    parser.add_argument("--workspace-gib", type=int, default=100)
    parser.add_argument("--docker-gib", type=int, default=100)
    parser.add_argument("--build-target", help="Output target; sonic-vs.bin does not require KVM")
    parser.add_argument("--check-data-only", action="store_true", help="Validate /data storage and exit")
    parser.add_argument("--host-agent", action="store_true",
                        help="Check an existing host agent without legacy service or /data layout requirements")
    args = parser.parse_args()
    if args.check_data_only:
        check_data_storage(build_target=args.build_target)
        return
    if args.workspace_gib <= 0 or args.docker_gib <= 0:
        parser.error("Disk budgets must be positive")
    if os.geteuid() == 0:
        parser.error("Run as sonic-runner so permission failures are visible")
    if platform.machine() != "x86_64":
        parser.error("The VS job needs native x86_64")
    check_worker_umasks(args.host_agent)
    check_ghostscript()
    check_legacy_nat()
    if not args.host_agent:
        check_data_storage(build_target=args.build_target)
    flags = set(output("findmnt", "--noheadings", "--output", "OPTIONS", "--target", str(args.workspace)).split(","))
    blocked = flags & {"nodev", "nosuid", "noexec", "ro"}
    if blocked:
        parser.error(f"Workspace mount blocks image construction: {', '.join(sorted(blocked))}")
    for command in ("git", "make", "docker", "j2", "python3", "wget"):
        if not shutil.which(command):
            parser.error(f"Missing executable: {command}")
    os_release = platform.freedesktop_os_release()
    print(f"Host: {os_release.get('PRETTY_NAME')}; CPUs: {os.cpu_count()}")
    available_kib = int(next(line.split()[1] for line in Path('/proc/meminfo').read_text().splitlines()
                             if line.startswith('MemAvailable:')))
    print(f"Available memory: {available_kib / 1024**2:.1f} GiB")
    if available_kib < 10 * 1024**2:
        requirement = ("The native VS build requires at least 10 GiB available"
                       if args.build_target == "sonic-vs.bin"
                       else "The VS installer VM alone requests 10 GiB")
        parser.error(requirement + "; free memory before building")
    if available_kib < 12 * 1024**2:
        print("NOTE: less than 2 GiB headroom beyond the 10 GiB minimum; reduce competing work.")
    if "overlay" not in Path('/proc/filesystems').read_text().split():
        parser.error("Load the overlay filesystem module on the host: sudo modprobe overlay")
    if os_release.get("ID") != "ubuntu" or os_release.get("VERSION_ID") not in ("22.04", "24.04"):
        print("NOTE: use Ubuntu 22.04/24.04 for the documented SONiC host baseline; this host needs full build validation.")
    print(output("docker", "version", "--format", "Docker server {{.Server.Version}}"))
    print(output("docker", "buildx", "version"))
    docker_root = Path(output("docker", "info", "--format", "{{.DockerRootDir}}"))
    check_disk(args.workspace, docker_root, args.workspace_gib, args.docker_gib)
    with tempfile.TemporaryDirectory(prefix="runner-preflight-", dir=args.workspace) as temporary:
        template = Path(temporary) / "check.j2"
        template.write_text("{{ 6 * 7 }}")
        if output("j2", str(template)) != "42":
            raise RuntimeError("j2 failed to render a template")
    if args.build_target != "sonic-vs.bin":
        with open("/dev/kvm", "r+b", buffering=0) as device:
            version = fcntl.ioctl(device.fileno(), 0xAE00, 0)
        if version != 12:
            raise RuntimeError(f"Unexpected KVM API: {version}")
        print("PASS: non-root Docker/buildx, writable workspace, disk budgets, j2 and KVM API 12")
    else:
        print("PASS: non-root Docker/buildx, writable workspace, disk budgets and j2; sonic-vs.bin does not require KVM")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"Preflight failed: {error}")
