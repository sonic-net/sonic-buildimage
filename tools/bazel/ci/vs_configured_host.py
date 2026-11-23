#!/usr/bin/python3
"""Restore or build the stock VS host before installer and account personalization."""

import fcntl
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

from vs_host_base import IMAGE as BASE, MARKER as BASE_MARKER, RECEIPT as BASE_RECEIPT
from vs_host_base import bazel_binary, check, data_bytes, fingerprint, private_dir, run_private_action, stage_options


IMAGE = Path("target/sonic-vs.configured-host.squashfs")
RECEIPT = Path("artifacts/vs/configured-host-cache.json")
PRODUCER = Path("scripts/build_vs_configured_host.sh")
DRIVER = Path("tools/bazel/ci/vs_configured_host.py")
COMMON = DRIVER.with_name("vs_host_base.py")
MARKER = "etc/sonic/.vs-configured-host-v1"
# configure-host exits here, before any runtime-image selection or package registration.
BOUNDARY = "\n## Bind docker path\n"
# Runtime environment used by the stock rendered shell and the Jinja templates
# it invokes. Do not pass the originating Make environment to a reusable action.
FIELDS = """
TARGET_MACHINE CONFIGURED_PLATFORM CONFIGURED_ARCH IMAGE_DISTRO IMAGE_TYPE kversion
debs_path files_path installer_device_data_tar OPENSSH_VERSION_FULL SONIC_VS_BATCH_PACKAGES MULTIARCH_QEMU_ENVIRON
CROSS_BUILD_ENVIRON INCLUDE_FIPS BUILD_REDUCE_IMAGE_SIZE build_reduce_image_size ZTP ZTP_DHCP_DISABLED
ENABLE_FRR_SNMP_AGENT default_buffer_model enable_auto_tech_support enable_pfcwd_on_start
include_dhcp_server include_iccpd include_kubernetes include_lldp include_macsec
include_mgmt_framework include_mux include_nat include_p4rt include_redfish include_restapi
include_router_advertiser include_sflow include_snmp include_system_eventd include_system_gnmi
include_system_otel include_system_telemetry include_teamd installer_services
shutdown_bgp_on_start sonic_asic_platform
""".split()

SOURCES = (
    PRODUCER, DRIVER, DRIVER.with_suffix(".bzl"), COMMON, "scripts/vs_host_base_state.sh",
    "tools/bazel/ci/vs_host_pypi.py", "tools/bazel/ci/vs_host_pypi.lock",
    "functions.sh", ".platform", ".arch", "files/image_config", "files/build_templates", "files/dhcp",
    "platform/vs/dash-engine.service", "src/sonic-platform-common/sonic_platform_base/bmc_fw_update.py",
    "target/versions/default/versions-mirror",
)
OPTIONAL = ("platform/vs/asic_table.j2", "platform/vs/peripheral_table.j2", "platform/vs/zero_profiles.j2")
DEVICE_TAR = "target/files/trixie/sonic-device-data-vs.tar"
DEVICE_REGISTRATION = ("scripts/install_device_data.py", "src/sonic-device-data/src/debian/control",
                       "src/sonic-device-data/src/debian/changelog")
SITE_FILES = ("files/build_templates/constants.yml.overlay.j2",
              "files/build_templates/init_cfg_organization.j2",
              "files/build_templates/sonic_debian_extension_organization_packages.j2",
              "files/build_templates/sonic_debian_extension_organization_docker_images.j2")


def options(repo, env):
    check(env.get("RFS_SPLIT_LAST_STAGE") == "y" and env.get("RFS_SPLIT_FIRST_STAGE") == "n"
          and env.get("DOCKER_LOAD_ON_BOOT") == "y", "expected the second-stage VS boot bundle installer")
    # Use exactly the first stage's site/mirror/account eligibility rules.
    stage_options(dict(env, RFS_SPLIT_FIRST_STAGE="y", RFS_SPLIT_LAST_STAGE="n"))
    for switch in ("include_kubernetes", "include_kubernetes_master", "enable_ztp"):
        check(env.get(switch) != "y", f"unsupported configured-host setting: {switch}")
    for source in SITE_FILES:
        check(not os.path.lexists(repo / source), f"site-specific host input: {source}")
    check(not env.get("sonic_packages", "").strip(), "remote package registration requires native host setup")
    # These stock packages register their local container through the image manager.
    # Keep custom packages on the original extension's transaction ordering.
    for spec in env.get("sonic_local_packages", "").split():
        name, path, _, _ = spec.split("|")
        check(name in ("docker-dhcp-relay.gz", "docker-dhcp-server.gz", "docker-macsec.gz")
              and path == f"target/{name}", "custom local image package requires native host setup")
    return {name: env[name] for name in FIELDS if name in env}


def local_packages(repo, cfg, prerequisites, host_script):
    # slave.mk declares its host DEBs/wheels. Also include the siblings and
    # retained versions matched by the extension's shell globs (e.g. ssh-client).
    packages = set(prerequisites)
    debs = repo / cfg["debs_path"]
    for pattern in re.findall(r"\$debs_path/([^\s\x22\x27]+?\.deb)", host_script):
        pattern = pattern.replace("${OPENSSH_VERSION_FULL}", cfg.get("OPENSSH_VERSION_FULL", ""))
        check(re.fullmatch(r"[A-Za-z0-9._:+~*?-]+", pattern), f"unsupported host package glob: {pattern}")
        packages.update(str(p.relative_to(repo)) for p in debs.glob(pattern))
    packages.update(re.findall(r"(?<![\w/.-])(target/[\w/.+:~-]+\.(?:deb|whl))\b", host_script))
    check(packages, "missing configured-host packages")
    for name in packages:
        path = Path(name)
        check(path.parts[0] == "target" and ".." not in path.parts and path.suffix in (".deb", ".whl"),
              f"unexpected host package path: {path}")
    return sorted(packages)


def identity(repo, cfg, builder, prerequisites):
    check(re.fullmatch(r"sha256:[0-9a-f]{64}", builder), "expected immutable build-slave image ID")
    check(cfg["debs_path"] == "target/debs/trixie", "unsupported host package directory")
    script = (repo / "sonic_debian_extension.sh").read_text()
    check(script.count(BOUNDARY) == 1, "unexpected rendered host extension layout")
    setup, _ = script.split(BOUNDARY)
    files = {}

    def record(path):
        name = path.relative_to(repo).as_posix()
        mode = path.lstat().st_mode
        item = {"mode": stat.S_IMODE(mode)}
        if path.is_dir():
            check(not path.is_symlink(), f"unexpected host asset directory symlink: {name}")
            for child in sorted(path.iterdir()):
                record(child)
        else:
            check(path.is_file(), f"invalid configured host input: {name}")
            if path.is_symlink():
                item["link"] = os.readlink(path)
            item["sha256"] = fingerprint(path)["sha256"]
        files[name] = item

    for name in SOURCES:
        record(repo / name)
    for name in OPTIONAL:
        if os.path.lexists(repo / name):
            record(repo / name)
    packages = {name: fingerprint(repo / name) for name in local_packages(repo, cfg, prerequisites, setup)}
    # The VS TAR replaces the device DEB. Its installer also reads package
    # metadata from the checkout to register ownership in the rootfs dpkg database.
    if device_tar := cfg.get("installer_device_data_tar"):
        check(device_tar == DEVICE_TAR, f"unexpected device-data archive: {device_tar}")
        packages[device_tar] = fingerprint(repo / device_tar)
        for name in DEVICE_REGISTRATION:
            record(repo / name)
    base = fingerprint(repo / BASE)
    receipt = json.loads((repo / BASE_RECEIPT).read_bytes())
    check(receipt["builder"] == builder and receipt["outputs"]["image"] == base,
          "host-base receipt does not identify the selected sealed image")
    return {"format": 1, "builder": builder, "base": base, "files": files, "packages": packages, "options": cfg,
            "host_program_sha256": hashlib.sha256(setup.encode()).hexdigest()}


def native_action(identity_file, image, manifest):
    work = identity_file.resolve().parent.parent
    key = fingerprint(identity_file)["sha256"]
    context = json.loads((work / "native.json").read_bytes())
    check(context["key"] == key, "native workspace identity mismatch")
    repo = Path(context["repo"])
    inputs = json.loads(identity_file.read_bytes())
    account = pwd.getpwuid(os.geteuid())
    env = dict(inputs["options"], USER=account.pw_name, LOGNAME=account.pw_name, HOME=account.pw_dir,
               PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", LANG="C")
    start = time.monotonic()
    (work / "started").touch()
    log = repo / "target/vs-configured-host-native.log"
    with log.open("wb") as stream:
        subprocess.run([repo / PRODUCER, repo / BASE, image.resolve()], cwd=repo, env=env, stdin=subprocess.DEVNULL,
                       stdout=stream, stderr=subprocess.STDOUT, check=True)
    marker = subprocess.check_output(["/usr/bin/unsquashfs", "-processors", "1", "-cat", image, MARKER])
    check(marker == b"vs-configured-host-v1\n", "missing configured-host marker")
    base_marker = subprocess.check_output(["/usr/bin/unsquashfs", "-processors", "1", "-cat", image, BASE_MARKER])
    check(base_marker.splitlines()[:1] == [b"vs-host-base-v1"], "missing deferred personalization marker")
    # The privileged producer reads the native checkout. Never publish a snapshot
    # under a stale identity if another recipe replaced a package during setup.
    check(identity(repo, inputs["options"], inputs["builder"], context["prerequisites"]) == inputs,
          "configured-host inputs changed during package setup")
    manifest.write_bytes(data_bytes({"identity_sha256": key, "image": fingerprint(image)}))
    (work / "native-seconds").write_text(str(time.monotonic() - start))


def cached_build(repo, cache, binary, inputs, prerequisites):
    key = hashlib.sha256(data_bytes(inputs)).hexdigest()
    private_dir(cache)
    private_dir(cache / key)
    # Large temporary Bazel outputs belong on the host's build filesystem.
    with tempfile.TemporaryDirectory(prefix=".vs-configured-host-bazel-", dir=repo / "target") as tmp:
        work = Path(tmp)
        ws = work / "workspace"
        ws.mkdir()
        (ws / "WORKSPACE").write_text('workspace(name = "sonic_vs_configured_host")\n')
        (ws / "resolved.bzl").write_text("resolved = []\n")
        (ws / "identity.json").write_bytes(data_bytes(inputs))
        (work / "native.json").write_bytes(data_bytes({"repo": str(repo), "key": key, "prerequisites": prerequisites}))
        for path in (DRIVER, COMMON, DRIVER.with_suffix(".bzl")):
            shutil.copy2(repo / path, ws / path.name)
        (ws / DRIVER.name).chmod(0o755)
        (ws / "BUILD").write_text(
            'load(":vs_configured_host.bzl", "vs_configured_host")\nplatform(name = "native")\n'
            'vs_configured_host(name="host", driver=":vs_configured_host.py", '
            'common=":vs_host_base.py", identity=":identity.json")\n')
        bazel_start = time.monotonic()
        try:
            out, hit = run_private_action(work, binary, cache / key, "//:host", "SonicVsConfiguredHost")
        except (ValueError, OSError, subprocess.CalledProcessError) as error:
            if (work / "started").is_file():
                raise RuntimeError("VS configured host producer failed; see target/vs-configured-host-native.log") from error
            raise
        materialize_start = time.monotonic()
        manifest = json.loads((out / "configured-host.json").read_bytes())
        check(manifest["identity_sha256"] == key, "configured-host cache manifest identity mismatch")
        # Verify the actual exported bytes before replacing any retained image.
        with tempfile.NamedTemporaryFile(dir=(repo / IMAGE).parent, delete=False) as stream:
            staged = Path(stream.name)
        try:
            subprocess.run(["/bin/cp", "--reflink=auto", "--", out / "configured-host.squashfs", staged], check=True)
            check(fingerprint(staged) == manifest["image"], "configured-host cache image digest mismatch")
            staged.chmod(0o644)
            os.replace(staged, repo / IMAGE)
        finally:
            staged.unlink(missing_ok=True)
        return {"identity_sha256": key, "cache_hit": hit, "outputs": {"image": manifest["image"]},
                "input_sha256": {section: hashlib.sha256(data_bytes(value)).hexdigest()
                                 for section, value in inputs.items()},
                "bazel_invocation_seconds": materialize_start - bazel_start,
                "materialize_seconds": time.monotonic() - materialize_start,
                "native_build_seconds": 0 if hit else float((work / "native-seconds").read_text())}


def main():
    repo = Path.cwd()
    receipt = repo / RECEIPT
    before = time.monotonic()
    try:
        with (repo / "target/.vs-configured-host.lock").open("a") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            receipt.unlink(missing_ok=True)
            cfg = options(repo, os.environ)
            builder = os.environ["SONIC_HOSTBASE_BUILDER_ID"]
            packages = os.environ["SONIC_CONFIGURED_HOST_PACKAGES"].split()
            inputs = identity(repo, cfg, builder, packages)
            result = cached_build(repo, Path(os.environ["SONIC_HOSTBASE_CACHE_DIR"]), bazel_binary(), inputs, packages)
            result.update(builder=builder, elapsed_seconds=time.monotonic() - before)
            receipt.parent.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(dir=receipt.parent, delete=False) as stream:
                stream.write(data_bytes(result))
            os.replace(stream.name, receipt)
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as error:
        print(f"VS configured host cache unavailable ({error}); using native host setup", flush=True)
        return 75
    print(f"VS configured host cache {'hit' if result['cache_hit'] else 'miss'} {result['identity_sha256']} "
          f"({result['elapsed_seconds']:.3f}s)", flush=True)
    return 0


if __name__ == "__main__":
    if len(sys.argv) == 5 and sys.argv[1] == "--action":
        native_action(*(Path(p) for p in sys.argv[2:]))
    elif len(sys.argv) == 1:
        sys.exit(main())
    else:
        sys.exit("run vs_configured_host.py from the VS installer recipe")
