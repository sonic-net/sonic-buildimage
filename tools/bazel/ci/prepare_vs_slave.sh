#!/usr/bin/env bash
# Prepare one disposable SONiC-slave runner for the native VS installer build.
# The caller owns daemon shutdown and runs the build in this same container.
set -euo pipefail

die() { printf 'VS slave preparation: %s\n' "$*" >&2; exit 1; }
(( $# == 0 )) || die 'This script takes no arguments.'
[[ $(id -u):$(id -g) == 1000:1000 ]] || die 'Expected runner UID/GID 1000:1000.'
workspace=$(realpath "${GITHUB_WORKSPACE:?}")
runner_temp=$(realpath "${RUNNER_TEMP:?}")
[[ $workspace == /actions/_work/* && $runner_temp == /actions/_work/* ]] ||
    die 'Checkout and runner temporary files must be below /actions/_work.'
[[ -f $workspace/slave.mk && -f /.dockerenv ]] || die 'Expected a SONiC checkout inside a container.'
[[ $(lsb_release -cs):$(uname -m) == trixie:x86_64 ]] || die 'Expected native AMD64 Debian Trixie.'
[[ -z ${DOCKER_HOST:-}${DOCKER_CONTEXT:-}${DOCKER_TLS_VERIFY:-}${DOCKER_CERT_PATH:-} ]] ||
    die 'An existing Docker endpoint must not be inherited by this job.'
[[ ! -e /var/run/docker.sock ]] || die 'An existing Docker socket is not allowed.'
sudo -n true || die 'This build requires the privileged runner profile and passwordless sudo.'
for command in docker dockerd make git python3 dpkg-deb mount findmnt ip timeout unshare; do
    command -v "$command" >/dev/null || die "Missing required tool: $command"
done
mountpoint -q /actions || die '/actions must be a worker-private volume.'
mountpoint -q /dpkg_cache && [[ -w /dpkg_cache ]] || die "Required writable Make cache mount is missing: /dpkg_cache"
[[ ! -e /bazel_cache ]] || die "Unexpected persistent Bazel cache on the K3s worker."

artifacts="$workspace/artifacts/vs"
mkdir -p "$artifacts"
stage=preflight
finish() {
    local status=$?
    trap - EXIT
    python3 - "$artifacts/slave-preparation.json" "$stage" "$status" <<'PY'
import json
from pathlib import Path
import sys
path, stage, status = Path(sys.argv[1]), sys.argv[2], int(sys.argv[3])
report = json.loads(path.read_text()) if path.exists() else {"schema": 1}
report.update(status="prepared" if status == 0 else "failed", stage=stage, exit_code=status)
path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
PY
    if (( status )); then
        printf 'VS slave preparation failed during %s; inspect artifacts/vs/.\n' "$stage" >&2
    fi
    exit "$status"
}
trap finish EXIT

python3 - "$artifacts/slave-preparation.json" <<'PY'
import grp
import ipaddress
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import sys

def require(condition, message):
    if not condition:
        raise SystemExit(message)

marker_path = Path("/etc/sonic-slave-github-runner.json")
info = marker_path.lstat()
require(stat.S_ISREG(info.st_mode) and info.st_uid == 0 and info.st_mode & 0o222 == 0,
        "Runner image marker must be a root-owned, read-only regular file")
marker = json.loads(marker_path.read_text())
for key, expected in {"uid": 1000, "gid": 1000, "runner_root": "/opt/actions-runner",
                      "work_root": "/actions/_work"}.items():
    require(marker.get(key) == expected, f"Unexpected runner image marker field: {key}")
require(re.fullmatch(r"sha256:[0-9a-f]{64}", marker.get("base_image", "")),
        "Runner marker must identify the immutable SONiC slave base image")
expected = os.environ.get("EXPECTED_MANAGER_REVISION", "")
require(re.fullmatch(r"[0-9a-f]{40}", expected) and marker.get("manager_revision") == expected,
        "Runner manager revision differs from the workflow pin")
require(grp.getgrnam("docker").gr_gid in os.getgroups(), "Runner process lacks the Docker group")
require(Path("/proc/self/cgroup").read_text().strip() == "0::/",
        "Expected a fresh, private cgroup v2 namespace")
cgroups = Path("/sys/fs/cgroup")
memory = (cgroups / "memory.max").read_text().strip()
require(memory == str(20 * 1024**3), "Native runner must have a 20 GiB memory limit")
quota, period = (cgroups / "cpu.max").read_text().split()
require(quota != "max" and 0 < int(quota) <= 6 * int(period),
        "Native runner must have a CPU quota of at most six cores")
filesystem = subprocess.check_output(["findmnt", "-n", "-o", "FSTYPE", "-T", "/actions"], text=True).strip()
require(filesystem in {"ext4", "xfs"}, "Native Docker storage requires an ext4 or XFS worker volume")
bridge = ipaddress.ip_network("192.168.240.0/24")
pool = ipaddress.ip_network("192.168.248.0/21")
for route in json.loads(subprocess.check_output(["ip", "-j", "-4", "route", "show"], text=True)):
    destination = route.get("dst", "default")
    if destination != "default":
        network = ipaddress.ip_network(destination, strict=False)
        require(not network.overlaps(bridge) and not network.overlaps(pool),
                "Private Docker bridge/address pool overlaps an existing worker route")
report = {"schema": 1, "image": marker, "uid": os.getuid(), "gid": os.getgid(),
          "work_root": "/actions/_work", "build_root": "/sonic",
          "memory_limit_bytes": int(memory), "cpu_quota": int(quota), "cpu_period": int(period),
          "docker_storage_filesystem": filesystem, "docker_bridge": str(bridge),
          "make_jobs": 1, "package_jobs": 4}
Path(sys.argv[1]).write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
PY

stage=kernel-sandbox
# Exercise the producer's pinned helper as the build user, including its
# mounts: namespace creation alone does not establish sandbox readiness.
export BAZELISK_HOME=/actions/bazel/bazelisk
if timeout --kill-after=5s 300s python3 -B "$workspace/tools/bazel/ci/check_kernel_sandbox.py" \
    > "$artifacts/kernel-sandbox.log" 2>&1; then
    python3 - "$artifacts/slave-preparation.json" <<'PY'
import json
from pathlib import Path
import sys
path = Path(sys.argv[1])
report = json.loads(path.read_text())
report["kernel_sandbox_probe"] = {"uid": 1000, "status": "passed"}
path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
PY
else
    probe_status=$?
    tail -n 20 "$artifacts/kernel-sandbox.log" >&2
    die "The pinned kernel linux-sandbox failed for UID 1000 (probe exit $probe_status). Check the runner-scoped AppArmor profile sonic-slave-build and its user-namespace permission."
fi

stage=checkout
cd "$workspace"
# Checkout already initialized the exact recorded revisions. Do not fetch or
# update submodules here, especially after the build revision has been selected.
git submodule status --recursive > "$artifacts/submodule-revisions.txt"
if [[ ! -s $artifacts/submodule-revisions.txt ]] ||
   LC_ALL=C grep -q '^[+U-]' "$artifacts/submodule-revisions.txt"; then
    die 'Every submodule must already be initialized at its recorded revision.'
fi
if mountpoint -q /sonic; then
    [[ $(stat -c '%d:%i' /sonic) == "$(stat -c '%d:%i' "$workspace")" ]] ||
        die '/sonic is already mounted from another directory.'
else
    [[ -d /sonic && ! -L /sonic && -z $(find /sonic -mindepth 1 -maxdepth 1 -print -quit) ]] ||
        die '/sonic must be an empty image directory before binding the checkout.'
    sudo -n mount --bind "$workspace" /sonic
fi
cd /sonic
[[ -w /sonic && $(stat -c '%d:%i' /sonic) == "$(stat -c '%d:%i' "$workspace")" ]] ||
    die 'The writable /sonic checkout binding could not be verified.'
git rev-parse HEAD > "$artifacts/preparation-revision.txt"
ulimit -Sn 524288

stage=docker
sudo -n python3 tools/bazel/ci/prepare_nested_docker.py > "$artifacts/private-cgroup.json"
docker_root=/actions/sonic-native-docker
[[ ! -e $docker_root && ! -e $runner_temp/sonic-dockerd.pid ]] ||
    die 'Private Docker state already exists; this preparation requires a fresh worker.'
sudo -n install -d -m 0711 "$docker_root" /run/sonic-native-docker
export DOCKER_HOST=unix:///var/run/docker.sock
export DOCKER_CONFIG="$runner_temp/sonic-docker-client"
install -d -m 0700 "$DOCKER_CONFIG"
printf '{}\n' > "$runner_temp/sonic-dockerd.json"
chmod 0600 "$runner_temp/sonic-dockerd.json"
# No inherited credentials or host daemon configuration enter this daemon.
# Its root is on the worker volume, avoiding overlay-on-overlay storage.
sudo -n /usr/bin/env -i PATH=/usr/sbin:/usr/bin:/sbin:/bin HOME=/root \
    "$(command -v dockerd)" --config-file "$runner_temp/sonic-dockerd.json" \
    --host "$DOCKER_HOST" --group docker --data-root "$docker_root" \
    --exec-root /run/sonic-native-docker --pidfile "$runner_temp/sonic-dockerd.pid" \
    --storage-driver overlay2 --exec-opt native.cgroupdriver=cgroupfs \
    --bip 192.168.240.1/24 --default-address-pool base=192.168.248.0/21,size=24 \
    > "$artifacts/dockerd.log" 2>&1 &
ready=n
deadline=$((SECONDS + 120))
while (( SECONDS < deadline )); do
    if timeout 3s docker info >/dev/null 2>&1; then ready=y; break; fi
    sleep 1
done
if [[ $ready != y || ! -s $runner_temp/sonic-dockerd.pid ]]; then
    tail -n 40 "$artifacts/dockerd.log" >&2
    die 'Private Docker did not become ready within 120 seconds.'
fi
[[ $(docker info --format '{{.DockerRootDir}}') == "$docker_root" ]] || die 'Docker is using an unexpected data root.'
[[ $(docker info --format '{{.Driver}}') == overlay2 ]] || die 'Private Docker is not using overlay2.'
docker version --format '{{json .Server}}' > "$artifacts/private-docker-version.json"

stage=configuration
# Makefile.work normally supplies this identity. Native package recipes such
# as ifupdown2 create temporary Git repositories and need it in the build user
# configuration, which lives only in this ephemeral worker.
git config --global user.name "${SONIC_BUILD_GIT_NAME:?Set the build Git name}"
git config --global user.email "${SONIC_BUILD_GIT_EMAIL:?Set the build Git email}"
git var GIT_AUTHOR_IDENT >/dev/null
git var GIT_COMMITTER_IDENT >/dev/null
# Configure selects VS below and records it in .platform. Runtime Python code
# interprets a nonempty PLATFORM as a hardware identity, so do not leak the
# build selector into package tests (the normal configured wrapper leaves it
# empty as well).
unset PLATFORM
export PLATFORM_ARCH=amd64 TARGET_BOOTLOADER=grub
export BUILD_WITH_BAZEL_WHEN_AVAILABLE=y INCLUDE_VS_DASH_SAI=y INCLUDE_FIPS=y
export ENABLE_ASAN=n ENABLE_SYNCD_RPC=n CROSS_BUILD_ENVIRON=n MULTIARCH_QEMU_ENVIRON=n
export PREFER_DEBIAN_REPO="${PREFER_DEBIAN_REPO:-n}"
export DOCKER_USERNAME=sonic DOCKER_USERTAG=latest DOCKER_LOCKDIR=/tmp/docklock
docker_apt_options=$(python3 -B "$workspace/tools/bazel/ci/native_docker_apt.py" "$runner_temp" "$workspace")
export SONIC_DOCKER_APT_PROXY= DOCKER_BUILD_OPTS="${DOCKER_BUILD_OPTS:-}"
if [[ -n $docker_apt_options ]]; then
    SONIC_DOCKER_APT_PROXY=1
    DOCKER_BUILD_OPTS+="${DOCKER_BUILD_OPTS:+ }$docker_apt_options"
fi
export SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD=y SONIC_CONFIG_USE_DOCKER_CACHE=y
export SONIC_DPKG_CACHE_SOURCE=/dpkg_cache SONIC_DPKG_CACHE_METHOD=rwcache
export SONIC_BAZEL_CACHE_SOURCE= BAZEL_CONTAINER_CACHE_DIR= BAZEL_SWSS_CACHE_DIR= BAZELISK_HOME=/actions/bazel/bazelisk
export SONIC_DEFAULT_CONTAINER_REGISTRY='' DEFAULT_CONTAINER_REGISTRY=''
# The minimal installer rootfs installs CA certificates after its first apt
# update. Keep Make's HTTP bootstrap transport and signed APT metadata checks,
# using the public Debian mirrors instead of the default Traffic Manager mirror.
export MIRROR_URLS=http://deb.debian.org/debian/
export MIRROR_SECURITY_URLS=http://deb.debian.org/debian-security/
export SONIC_BUILD_JOBS=1 SONIC_BUILD_MEMORY=20g
# rules/config assigns these with '='; MAKEFLAGS gives the bounded values the
# same precedence as the outer wrapper's command-line assignments.
export MAKEFLAGS='SONIC_CONFIG_BUILD_JOBS=1 SONIC_CONFIG_MAKE_JOBS=4'
export BUILD_NUMBER="${GITHUB_RUN_NUMBER:?}" BUILD_TIMESTAMP="$(date -u +%Y%m%d.%H%M%S)"
export SOURCE_DATE_EPOCH="$(git show -s --format=%ct HEAD)" BUILD_HOSTNAME="$(hostname)"
export SLAVE_DIR=sonic-slave-trixie DISTRO=trixie IMAGENAME=sonic-slave-trixie
export SONIC_BUILD_TARGET=target/sonic-vs.bin
export SLAVE_TAG="$(python3 -c 'import json; print(json.load(open("/etc/sonic-slave-github-runner.json"))["base_image"].split(":")[1])')"
mkdir -p "$DOCKER_LOCKDIR" /dpkg_cache/vcache /actions/bazel/bazelisk
# The root build hook creates its package-cache path before Make's configure
# recipe runs. Keep the checkout's output directories owned by the build user.
mkdir -p "target/vcache/$IMAGENAME/deb" target/versions dpkg
profile=(BUILD_WITH_BAZEL_WHEN_AVAILABLE=y INCLUDE_VS_DASH_SAI=y INCLUDE_FIPS=y ENABLE_ASAN=n ENABLE_SYNCD_RPC=n)

# Resolve the repository's defaults without entering either outer Makefile.
# The wrapper normally exports these feature settings through command-line
# assignments; native package submakes need the same resolved values.
settings=$(make --no-print-directory -s -f rules/config -f - .vs-preparation-defaults <<'MAKE'
-include rules/config.organization
-include rules/config.user
$(info VS_DEFAULT:TRUSTED_GPG_URLS=$(TRUSTED_GPG_URLS))
$(info VS_DEFAULT:SONIC_VERSION_CONTROL_COMPONENTS=$(SONIC_VERSION_CONTROL_COMPONENTS))
$(info VS_DEFAULT:SONIC_VERSION_CACHE=$(filter-out none,$(SONIC_VERSION_CACHE_METHOD)))
$(info VS_DEFAULT:SONIC_VERSION_CACHE_SOURCE=$(SONIC_VERSION_CACHE_SOURCE))
$(info VS_DEFAULT:BUILD_PACKAGES_URL=$(BUILD_PACKAGES_URL))
$(info VS_DEFAULT:BUILD_PUBLIC_URL=$(BUILD_PUBLIC_URL))
$(info VS_DEFAULT:BUILD_SNAPSHOT_URL=$(BUILD_SNAPSHOT_URL))
$(info VS_DEFAULT:MIRROR_SNAPSHOT=$(MIRROR_SNAPSHOT))
$(info VS_DEFAULT:PIP_HTTP_TIMEOUT=$(PIP_HTTP_TIMEOUT))
$(info VS_DEFAULT:ENABLE_TRANSLIB_WRITE=$(ENABLE_TRANSLIB_WRITE))
$(info VS_DEFAULT:ENABLE_NATIVE_WRITE=$(ENABLE_NATIVE_WRITE))
$(info VS_DEFAULT:ENABLE_HOST_SERVICE_ON_START=$(ENABLE_HOST_SERVICE_ON_START))
.PHONY: .vs-preparation-defaults
.vs-preparation-defaults: ; @:
MAKE
)
while IFS= read -r line; do
    [[ $line == VS_DEFAULT:* ]] || continue
    setting=${line#VS_DEFAULT:}
    export "$setting"
done <<< "$settings"
bash scripts/generate_buildinfo_config.sh
make --no-print-directory -C src/sonic-build-hooks all 2>&1 | tee "$artifacts/build-hooks.log"
# slave.mk refreshes its runtime buildinfo from this directory on every parse.
mkdir -p "$SLAVE_DIR/buildinfo"
cp -a src/sonic-build-hooks/buildinfo/. "$SLAVE_DIR/buildinfo/"
sudo -n install -d /usr/local/share/buildinfo/config
sudo -n cp src/sonic-build-hooks/buildinfo/sonic-build-hooks_1.0_all.deb /usr/local/share/buildinfo/
sudo -n cp src/sonic-build-hooks/buildinfo/config/buildinfo.config /usr/local/share/buildinfo/config/
sudo -n dpkg -i /usr/local/share/buildinfo/sonic-build-hooks_1.0_all.deb >> "$artifacts/build-hooks.log" 2>&1
scripts/versions_manager.py generate -t "$runner_temp/native-buildinfo-versions" \
    -n build-sonic-slave-trixie -d trixie -a amd64
sudo -n rm -rf /usr/local/share/buildinfo/versions
sudo -n cp -a "$runner_temp/native-buildinfo-versions" /usr/local/share/buildinfo/versions
sudo -n touch /usr/local/share/buildinfo/versions/versions-deb /usr/local/share/buildinfo/versions/versions-web
sudo -n env IMAGENAME=sonic-slave-trixie DISTRO=trixie pre_run_buildinfo sonic-slave-trixie \
    >> "$artifacts/build-hooks.log" 2>&1
make -f slave.mk configure PLATFORM=vs PLATFORM_ARCH=amd64 "${profile[@]}" \
    2>&1 | tee "$artifacts/configure.log"

stage=profile
make --no-print-directory -s -f slave.mk -f - .vs-preparation-profile "${profile[@]}" \
    > "$runner_temp/native-make-profile.txt" <<'MAKE'
$(info VS_PROFILE:BLDENV=$(BLDENV))
$(info VS_PROFILE:PLATFORM=$(CONFIGURED_PLATFORM))
$(info VS_PROFILE:ARCH=$(CONFIGURED_ARCH))
$(info VS_PROFILE:MAKE_JOBS=$(SONIC_BUILD_JOBS))
$(info VS_PROFILE:PACKAGE_JOBS=$(SONIC_CONFIG_MAKE_JOBS))
$(info VS_PROFILE:PREFER_DEBIAN_REPO=$(PREFER_DEBIAN_REPO))
$(info VS_PROFILE:BOOKWORM=$(filter $(SONIC_BOOKWORM_DOCKERS) $(SONIC_BOOKWORM_DBG_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS) $(SONIC_PACKAGES_LOCAL)))
$(info VS_PROFILE:INSTALLER_DOCKERS=$(DOCKER_IMAGES_FOR_INSTALLERS))
$(info VS_PROFILE:BAZEL_DOCKERS=$(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES))
.PHONY: .vs-preparation-profile
.vs-preparation-profile:
	@printf 'VS_PROFILE:RUNTIME_PLATFORM=%s\n' "$${PLATFORM:-}"
MAKE
python3 - "$runner_temp/native-make-profile.txt" "$artifacts/slave-preparation.json" <<'PY'
import json
import os
from pathlib import Path
import sys
settings = dict(line.removeprefix("VS_PROFILE:").split("=", 1)
                for line in Path(sys.argv[1]).read_text().splitlines() if line.startswith("VS_PROFILE:"))
for key, expected in {"BLDENV": "trixie", "PLATFORM": "vs", "ARCH": "amd64",
                      "MAKE_JOBS": "1", "PACKAGE_JOBS": "4", "BOOKWORM": "",
                      "PREFER_DEBIAN_REPO": os.environ["PREFER_DEBIAN_REPO"],
                      "RUNTIME_PLATFORM": ""}.items():
    if settings.get(key, "<missing>").strip() != expected:
        raise SystemExit(f"Unexpected native Make profile {key}: {settings.get(key)!r}")
selected = set(settings["BAZEL_DOCKERS"].split())
required = {f"docker-{image}{debug}.gz" for image in ("orchagent", "syncd-vs") for debug in ("", "-dbg")}
if not required <= selected:
    raise SystemExit("The requested VS profile must select both Bazel runtime/debug image pairs")
path = Path(sys.argv[2])
report = json.loads(path.read_text())
report["make_profile"] = settings
report["docker_apt_proxy_enabled"] = os.environ["SONIC_DOCKER_APT_PROXY"] == "1"
report["source_revision"] = Path("artifacts/vs/preparation-revision.txt").read_text().strip()
path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
PY

# This file contains only the named build settings, never a dump of the runner
# environment. Workflow steps source it before entering /sonic.
stage=environment
environment_file="$runner_temp/sonic-vs-slave.env"
(umask 077; printf 'unset PLATFORM\n' > "$environment_file")
for variable in PLATFORM_ARCH TARGET_BOOTLOADER BUILD_WITH_BAZEL_WHEN_AVAILABLE \
    INCLUDE_VS_DASH_SAI INCLUDE_FIPS ENABLE_ASAN ENABLE_SYNCD_RPC CROSS_BUILD_ENVIRON \
    PREFER_DEBIAN_REPO \
    MULTIARCH_QEMU_ENVIRON DOCKER_HOST DOCKER_CONFIG DOCKER_USERNAME DOCKER_USERTAG DOCKER_LOCKDIR \
    SONIC_DOCKER_APT_PROXY DOCKER_BUILD_OPTS \
    SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD SONIC_CONFIG_USE_DOCKER_CACHE \
    SONIC_DPKG_CACHE_SOURCE SONIC_DPKG_CACHE_METHOD SONIC_BAZEL_CACHE_SOURCE BAZEL_CONTAINER_CACHE_DIR BAZEL_SWSS_CACHE_DIR BAZELISK_HOME \
    SONIC_DEFAULT_CONTAINER_REGISTRY DEFAULT_CONTAINER_REGISTRY MIRROR_URLS MIRROR_SECURITY_URLS \
    SONIC_BUILD_JOBS SONIC_BUILD_MEMORY MAKEFLAGS BUILD_NUMBER BUILD_TIMESTAMP SOURCE_DATE_EPOCH BUILD_HOSTNAME \
    SLAVE_DIR SLAVE_TAG DISTRO IMAGENAME SONIC_BUILD_TARGET TRUSTED_GPG_URLS \
    SONIC_VERSION_CONTROL_COMPONENTS SONIC_VERSION_CACHE SONIC_VERSION_CACHE_SOURCE \
    BUILD_PACKAGES_URL BUILD_PUBLIC_URL BUILD_SNAPSHOT_URL MIRROR_SNAPSHOT PIP_HTTP_TIMEOUT \
    ENABLE_TRANSLIB_WRITE ENABLE_NATIVE_WRITE ENABLE_HOST_SERVICE_ON_START; do
    printf 'export %s=%q\n' "$variable" "${!variable}" >> "$environment_file"
done
printf 'ulimit -Sn 524288\n' >> "$environment_file"
stage=complete
printf 'Prepared native AMD64 Trixie VS build at /sonic; source %s before Make.\n' "$environment_file"
