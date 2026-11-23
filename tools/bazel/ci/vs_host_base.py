#!/usr/bin/python3
"""Build or restore the stock amd64/Trixie VS host filesystem using a private Bazel disk cache."""

import contextlib
import hashlib
import json
import os
from pathlib import Path
import pwd
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import time


IMAGE = Path("target/sonic-vs.bin__vs__rfs.squashfs")
INVENTORY = Path("target/versions/host-base-image/versions-deb-trixie-amd64")
RECEIPT = Path("artifacts/vs/host-base-cache.json")
MARKER = "etc/sonic/.vs-host-base-v1"
TOOL = Path(__file__).resolve()
BAZEL_DIGEST = "61d89402f0368e64b6c827be5de79d8e65382e8124c3cbb97325611a1851392e"

# Environment consumed by the first stage. The installer applies passwords, keys,
# release/version metadata, and other build-specific settings after extracting the base.
FIELDS = """
TARGET_PATH TARGET_MACHINE TARGET_BOOTLOADER IMAGE_TYPE IMAGE_DISTRO CONFIGURED_PLATFORM
CONFIGURED_ARCH BLDENV RFS_SQUASHFS_NAME RFS_SPLIT_FIRST_STAGE RFS_SPLIT_LAST_STAGE
SONIC_VS_UNPERSONALIZED_BASE USERNAME debs_path kversion linux_kernel initramfs_tools
sonicadmin_user sonic_asic_platform image_type INCLUDE_KUBERNETES INCLUDE_KUBERNETES_MASTER
MULTIARCH_QEMU_ENVIRON CROSS_BUILD_ENVIRON ENABLE_VERSION_CONTROL_DEB MIRROR_SNAPSHOT
BUILD_PUBLIC_URL BUILD_SNAPSHOT_URL TRUSTED_GPG_URLS BUILD_PACKAGES_URL MIRROR_URLS
MIRROR_SECURITY_URLS SONIC_VERSION_CONTROL_COMPONENTS SONIC_VERSION_CACHE ENABLE_SBOM APT_RETRIES_COUNT
""".split()
PERSONAL_SETTINGS = ("PASSWORD", "DEFAULT_PASSWORD", "CHANGE_DEFAULT_PASSWORD", "BMC_NOS_ACCOUNT_USERNAME",
                     "BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD", "MAKEFLAGS", "MFLAGS", "MAKEOVERRIDES")

SOURCE_PATHS = (
    "build_debian.sh", "functions.sh", "onie-image.conf", ".platform", ".arch",
    "scripts/vs_host_base_state.sh", "scripts/build_debian_base_system.sh",
    "scripts/prepare_debian_image_buildinfo.sh", "scripts/build_mirror_config.sh", "scripts/versions_manager.py",
    "files/initramfs-tools", "files/apt/apt.conf.d", "files/apt/sources.list.j2", "files/apt/apt-retries-count.j2",
    "files/apparmor/usr.bin.tcpdump", "files/docker/docker.service.conf", "files/sshd",
    "files/image_config/kdump/vmcore-sysctl.conf", "files/image_config/logrotate/logrotateOverride.conf",
    "files/image_config/sysctl/90-sonic.conf", "files/image_config/pip/pip.conf", "files/dhcp",
    "files/scripts/core_cleanup.py", "files/scripts/network_setup", "files/scripts/network_setup.sh",
    "files/build_scripts/generate_asic_config_checksum.py", "files/build/versions/default",
    "files/build/versions/host-image", "files/build/versions/host-base-image", "src/sonic-build-hooks/buildinfo",
    "src/sonic-swss/swssconfig/sample/netbouncer.json", "target/versions/default/versions-mirror",
)
OUTPUTS = {"inventory": INVENTORY, "image": IMAGE}


def check(condition, message):
    if not condition:
        raise ValueError(message)


def data_bytes(value):
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode()


def fingerprint(path):
    sha = hashlib.sha256()
    size = 0
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            sha.update(block)
            size += len(block)
    return {"sha256": sha.hexdigest(), "bytes": size}


def private_dir(path):
    check(path.is_absolute() and path.resolve() == path, "cache directory must be canonical without symlinks")
    path.mkdir(mode=0o700, exist_ok=True)
    info = path.lstat()
    check(stat.S_ISDIR(info.st_mode) and info.st_uid == os.geteuid() and stat.S_IMODE(info.st_mode) == 0o700,
          "cache directory must be private (0700) and owned by the build user")


def bazel_binary():
    path = Path("downloads/sha256") / BAZEL_DIGEST / "bin/bazel"
    roots = [Path(os.getenv("BAZELISK_HOME", Path.home() / ".cache/bazelisk")), Path("/bazel_cache/bazelisk")]
    candidates = [p / path for p in roots]
    candidates += [Path(p) for p in (shutil.which("bazel-real"), shutil.which("bazel")) if p]
    for binary in candidates:
        if binary.is_file() and fingerprint(binary)["sha256"] == BAZEL_DIGEST:
            return binary.resolve()
    raise ValueError("pinned Bazel 8.5.1 is unavailable in the build slave")


def stage_options(env):
    # Check the actual invocation for e.g. site hooks/proxies before narrowing
    # the producer's environment. Only the settings deferred to stage two are removed.
    candidate = dict(env, SONIC_VS_UNPERSONALIZED_BASE="y")
    for name in PERSONAL_SETTINGS:
        candidate.pop(name, None)
    subprocess.run(["bash", "scripts/vs_host_base_state.sh", "check-inputs"], check=True, env=candidate)
    expected = {"TARGET_PATH": "target", "RFS_SQUASHFS_NAME": IMAGE.name}
    for key, value in expected.items():
        check(candidate.get(key) == value, f"unsupported first-stage setting {key}")
    check(candidate.get("SONIC_VERSION_CACHE", "") in ("", "none"), "unsupported build-hook version cache")
    check(not candidate.get("sysctl_net_cmd_string"), "unsupported site sysctl settings")
    return {key: candidate[key] for key in FIELDS if key in candidate}


def inputs(repo, cfg, builder, driver=TOOL):
    check(re.fullmatch(r"sha256:[0-9a-f]{64}", builder), "builder must be an immutable Docker image ID")
    options = dict(cfg)
    for field in ("debs_path", "linux_kernel", "initramfs_tools"):
        options[field] = (repo / cfg[field]).resolve(strict=True).relative_to(repo).as_posix()
    files = {}

    def add(path):
        actual = path.resolve(strict=True)
        actual.relative_to(repo)
        key = path.relative_to(repo).as_posix()
        info = actual.stat()
        entry = {"mode": stat.S_IMODE(info.st_mode)}
        if path.is_symlink():
            entry["target"] = os.readlink(path)
        files[key] = entry
        if stat.S_ISDIR(info.st_mode):
            check(not path.is_symlink(), f"unsupported directory symlink: {key}")
            for child in sorted(path.iterdir()):
                add(child)
        elif stat.S_ISREG(info.st_mode):
            entry["sha256"] = fingerprint(path)["sha256"]
        else:
            raise ValueError(f"unsupported special file: {key}")

    for source in SOURCE_PATHS:
        add(repo / source)
    for optional in ("platform/vs/modules", "files/apt/sources.list.amd64.j2"):
        path = repo / optional
        if path.exists() or path.is_symlink():
            add(path)
    # build_debian.sh copies all matching .deb files. Include every matching
    # initramfs package and every non-debug kernel candidate, including retained
    # packages, even when Make names just one of them as a prerequisite.
    debs = repo / options["debs_path"]
    for pattern in ("initramfs-tools-core_*.deb", "initramfs-tools_*.deb", "linux-image-*_amd64.deb"):
        matches = [p for p in debs.glob(pattern) if "-dbg_" not in p.name and "-dbgsym_" not in p.name]
        check(matches, f"no host package matches {pattern}")
        for path in matches:
            add(path)
    for name in (options["linux_kernel"], options["initramfs_tools"], "files/initramfs-tools/arista-convertfs",
                 "files/initramfs-tools/union-mount", "src/sonic-build-hooks/buildinfo/sonic-build-hooks_1.0_all.deb"):
        check(name in files, f"missing selected host input {name}")
    return {"format": 1, "builder": builder, "options": options, "inputs": files,
            "driver": fingerprint(driver), "rule": fingerprint(driver.with_suffix(".bzl"))}


def native_action(identity_path, image, inventory, manifest):
    # Source symlinks in the standalone execroot point into this invocation's
    # own workspace. The checkout location is local execution context and must
    # never enter the reusable action key.
    work = identity_path.resolve(strict=True).parent.parent
    context = json.loads((work / "native.json").read_bytes())
    key = fingerprint(identity_path)["sha256"]
    check(context["key"] == key, "native workspace identity mismatch")
    repo = Path(context["checkout"])
    account = pwd.getpwuid(os.geteuid())
    identity = json.loads(identity_path.read_bytes())
    env = dict(identity["options"], USER=account.pw_name, LOGNAME=account.pw_name,
               HOME=account.pw_dir, LANG="C", PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/bin")

    started = time.monotonic()
    (work / "started").touch()
    for rel in OUTPUTS.values():
        (repo / rel).unlink(missing_ok=True)
    command = ["./build_debian.sh"]
    fifo = work / "timing"
    if fifo.exists():
        command = ["bash", "-c", 'exec {SONIC_VS_RFS_TIMING_FD}>"$1"; export SONIC_VS_RFS_TIMING_FD; '
                   'exec ./build_debian.sh', "bash", str(fifo)]
    log = repo / "target/vs-host-base-native.log"
    with log.open("wb") as stream:
        proc = subprocess.run(command, cwd=repo, env=env, stdout=stream, stderr=subprocess.STDOUT)
    check(proc.returncode == 0, f"native host build failed ({proc.returncode}), see {log}")
    (work / "native-seconds").write_text(str(time.monotonic() - started))
    # The privileged native recipe reads the checkout directly. Reject a
    # concurrent edit or package replacement before Bazel publishes its outputs.
    actual = inputs(repo, identity["options"], identity["builder"], repo / "tools/bazel/ci/vs_host_base.py")
    check(actual == identity, "host inputs changed during the native build")
    marker = subprocess.check_output(["/usr/bin/unsquashfs", "-processors", "1", "-cat", repo / IMAGE, MARKER])
    check(marker.splitlines()[:1] == [b"vs-host-base-v1"], "native image is missing the stock host base marker")
    check((repo / INVENTORY).stat().st_size > 0, "empty native package inventory")
    shutil.copyfile(repo / IMAGE, image)
    shutil.copyfile(repo / INVENTORY, inventory)
    manifest.write_bytes(data_bytes({"identity_sha256": key,
                                    "outputs": {"image": fingerprint(image), "inventory": fingerprint(inventory)}}))


def bazel_build(work, binary, cache, identity):
    ws = work / "workspace"
    ws.mkdir()
    (ws / "WORKSPACE").write_text('workspace(name = "sonic_vs_host_base")\n')
    (ws / "resolved.bzl").write_text("resolved = []\n")
    (ws / "identity.json").write_bytes(data_bytes(identity))
    shutil.copyfile(TOOL, ws / "driver.py")
    (ws / "driver.py").chmod(0o755)
    shutil.copyfile(TOOL.with_suffix(".bzl"), ws / "host.bzl")
    (ws / "BUILD").write_text(
        'load(":host.bzl", "vs_host_base")\nplatform(name = "native")\n'
        'vs_host_base(name = "base", driver = ":driver.py", identity = ":identity.json")\n')
    return run_private_action(work, binary, cache, "//:base", "SonicVsHostBase")


def run_private_action(work, binary, cache, target, mnemonic):
    ws = work / "workspace"
    log = work / "actions.json"
    subprocess.run([
        binary, "--batch", "--ignore_all_rc_files", f"--output_user_root={work}/user", f"--output_base={work}/base",
        "--host_jvm_args=-XX:ActiveProcessorCount=2", "build", "--jobs=1", "--noenable_bzlmod", "--enable_workspace",
        "--incompatible_autoload_externally=", f"--experimental_resolved_file_instead_of_workspace={ws}/resolved.bzl",
        "--repository_disable_download", "--platforms=//:native", "--host_platform=//:native", "--spawn_strategy=local",
        "--bes_backend=", "--remote_cache=", "--remote_executor=", f"--disk_cache={cache}",
        f"--execution_log_json_file={log}", target,
    ], cwd=ws, check=True, env={"HOME": str(work), "PATH": "/usr/bin:/bin", "LANG": "C"})
    event = json.loads(log.read_bytes())
    check(event["mnemonic"] == mnemonic, f"missing Bazel {mnemonic} action")
    hit = bool(event.get("cacheHit", False))
    check(event.get("runner") == ("disk cache hit" if hit else "local") and hit != (work / "started").exists(),
          f"unexpected Bazel executor for {mnemonic}")
    return ws / "bazel-bin", hit


@contextlib.contextmanager
def timing_relay(work):
    # Make's public timing FD cannot pass through the Bazel server. Relay the
    # producer's original phase markers live from a private FIFO on a miss.
    name = os.environ.get("SONIC_VS_RFS_TIMING_FD", "")
    if not re.fullmatch(r"[1-9][0-9]+", name):
        yield None
        return
    fd = int(name)
    try:
        os.fstat(fd)
    except OSError:
        yield None
        return
    fifo = work / "timing"
    os.mkfifo(fifo, mode=0o600)
    with os.fdopen(os.open(fifo, os.O_RDONLY | os.O_NONBLOCK), "rb") as reader:
        with fifo.open("wb", buffering=0) as writer:
            os.set_blocking(reader.fileno(), True)
            child = subprocess.Popen(["/usr/bin/cat"], stdin=reader, stdout=fd)
            try:
                yield writer
            finally:
                writer.close()
                child.wait()


def materialize(repo, output, key):
    manifest = json.loads((output / "host.manifest").read_bytes())
    check(manifest["identity_sha256"] == key, "cached host-base identity mismatch")
    expected = manifest["outputs"]
    staged = {}
    with contextlib.ExitStack() as cleanup:
        for name, rel in OUTPUTS.items():
            src = output / ("host.squashfs" if name == "image" else "host.inventory")
            dest = repo / rel
            digest = expected[name]
            if (not dest.is_symlink() and dest.is_file() and dest.stat().st_size == digest["bytes"]
                    and fingerprint(dest) == digest):
                # Keep its inode and mtime: Make uses the SquashFS timestamp to
                # decide whether an already assembled installer is still current.
                check(fingerprint(src) == digest, f"cached host-base {name} digest mismatch")
                continue
            dest.parent.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(dir=dest.parent, delete=False) as stream:
                tmp = Path(stream.name)
                cleanup.callback(tmp.unlink, missing_ok=True)
                with src.open("rb") as inp:
                    shutil.copyfileobj(inp, stream, 1024 * 1024)
            check(fingerprint(tmp) == digest, f"cached host-base {name} digest mismatch")
            tmp.chmod(0o644)
            staged[dest] = tmp
        # The Make target is the commit point: both outputs have passed verification.
        for dest, tmp in staged.items():
            os.replace(tmp, dest)
    return expected


def cached_build(repo, root, binary, identity):
    key = hashlib.sha256(data_bytes(identity)).hexdigest()
    private_dir(root)
    cache = root / key
    private_dir(cache)
    with tempfile.TemporaryDirectory(prefix="sonic-vs-host-base-") as temporary:
        work = Path(temporary)
        (work / "native.json").write_bytes(data_bytes({"checkout": str(repo), "key": key}))
        before = time.monotonic()
        try:
            with timing_relay(work) as timing:
                output, hit = bazel_build(work, binary, cache, identity)
                bazel_seconds = time.monotonic() - before
                hashes = materialize(repo, output, key)
                if hit and timing is not None:
                    timing.write(b"[ vs timing ] [ rootfs-prepare ] [ end ]\n")
        except BaseException as exc:
            # Native assembly writes to the Make target before the Bazel action
            # finishes. Never mask a native failure or retain a partial target.
            if (work / "started").exists():
                (repo / IMAGE).unlink(missing_ok=True)
                raise RuntimeError("VS host-base producer failed; see target/vs-host-base-native.log") from exc
            raise
        native_seconds = 0 if hit else float((work / "native-seconds").read_text())
    return {"identity_sha256": key, "cache_hit": hit, "builder": identity["builder"], "outputs": hashes,
            "bazel_invocation_seconds": bazel_seconds, "native_build_seconds": native_seconds}


def main():
    started = time.monotonic()
    repo = Path.cwd().resolve()
    receipt = repo / RECEIPT
    receipt.unlink(missing_ok=True)
    try:
        cfg = stage_options(os.environ)
        identity = inputs(repo, cfg, os.environ["SONIC_HOSTBASE_BUILDER_ID"])
        revision = subprocess.check_output(["git", "-c", f"safe.directory={repo}", "rev-parse", "HEAD"], text=True).strip()
        result = cached_build(repo, Path(os.environ["SONIC_HOSTBASE_CACHE_DIR"]), bazel_binary(), identity)
    except (OSError, KeyError, ValueError, subprocess.CalledProcessError) as error:
        # A cache is optional. Restore failures and unsupported settings use the
        # original Make environment, including the credentials for a stock build.
        print(f"VS host-base cache unavailable ({error}); using the native Make recipe", flush=True)
        os.execv("./build_debian.sh", ["./build_debian.sh"])
    result.update(revision=revision, elapsed_seconds=time.monotonic() - started)
    receipt.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=receipt.parent, delete=False) as stream:
        stream.write(data_bytes(result))
    os.replace(stream.name, receipt)
    print(f"VS host-base cache {'hit' if result['cache_hit'] else 'miss'} {result['identity_sha256']} "
          f"({result['elapsed_seconds']:.3f}s)", flush=True)


if __name__ == "__main__":
    if len(sys.argv) == 6 and sys.argv[1] == "--action":
        native_action(*(Path(p) for p in sys.argv[2:]))
    elif len(sys.argv) == 1:
        main()
    else:
        sys.exit("run vs_host_base.py from the first-stage Make recipe")
