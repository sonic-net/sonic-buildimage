#!/usr/bin/env python3
"""Run a reviewed source-image driver on a private AMD64/Trixie worker."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import stat
import subprocess
import sys
import threading
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import source_mirrors
from tools.bazel.ci.public_artifacts import safe_json, dependency_json, bep_summary
from tools.bazel.ci.source_worker_preflight import cpu_limit, measure, select_build_profile
from tools.bazel.gzip.source_archive import SOURCES

DRIVERS = {"dash-ha": "dash_ha", "sysmgr": "sysmgr", "orchagent": "orchagent", "sflow": "sflow", "otel": "otel", "syncd": "syncd", "gearbox": "gbsyncd", "dash-engine": "dash_engine", "gnmi": "gnmi", "gnmi-no-dialout": "gnmi", "teamd": "teamd"}
WRAPPERS = {name: "libteam_draft_bazel.sh" for name in DRIVERS}
WRAPPERS["sysmgr"] = "review_bazel.sh"
WRAPPERS["otel"] = "review_bazel.sh"
WRAPPERS["syncd"] = "review_bazel.sh"
WRAPPERS["gearbox"] = "review_bazel.sh"
WRAPPERS["dash-engine"] = "review_bazel.sh"
WRAPPERS["gnmi"] = "review_bazel.sh"
WRAPPERS["gnmi-no-dialout"] = "review_bazel.sh"
GNMI_PROFILES = {"gnmi": "enabled", "gnmi-no-dialout": "disabled"}
SNAPSHOT_FALLBACKS = {
    "pool/main/p/python3.13/libpython3.13-minimal_3.13.5-2+deb13u4_amd64.deb": "d63097e4bb174c04bfc29e38e6270b43c2db9c0b",
    "pool/main/p/python3.13/libpython3.13_3.13.5-2+deb13u4_amd64.deb": "a817f92905b030fcbca7d433db7dc4dcccf91c3d",
    "pool/main/c/cdebconf/libdebconfclient0_0.280_amd64.deb": "219dec5cd95ea5be91161ab1d30dc3f19efb6e01",
    "pool/main/libs/libselinux/libselinux1_3.8.1-1_amd64.deb": "44b247687c034c8a4142204f0827f7286614b354",
}
CACHE_MOUNT = Path("/bazel_cache")
WORK_MOUNT = Path("/actions")


def mirror_configuration(original):
    source_mirrors.verify(original)
    # BCR gawk@5.3.2 retains SHA256 f8c3486509de705192138b00ef2c00bbbdd0e84c30d5c07d23fc73a9dc4cc9cc.
    # The official Bazel mirror was independently verified to contain these exact bytes.
    destination = "https://mirror.bazel.build/ftp.gnu.org/gnu/gawk/gawk-5.3.2.tar.xz"
    expressions = [r"mirrors\.kernel\.org/gnu/gawk/gawk-5\.3\.2\.tar\.xz",
                   r"ftp\.gnu\.org/gnu/gawk/gawk-5\.3\.2\.tar\.xz",
                   r"ftpmirror\.gnu\.org/gnu/gawk/gawk-5\.3\.2\.tar\.xz"]
    lines = ["rewrite " + expression + " " + destination for expression in expressions]
    # These four observed snapshot503 failures have verified byte-identical official
    # /file endpoints. Their SHA1 URL identifiers do not replace the locked SHA256.
    # Retain the original URL as a second download alternative.
    for path, identifier in SNAPSHOT_FALLBACKS.items():
        original_url = "snapshot.debian.org/archive/debian/20260727T143429Z/" + path
        expression = re.escape(original_url)
        lines.extend(["rewrite " + expression + " https://snapshot.debian.org/file/" + identifier,
                      "rewrite " + expression + " https://" + original_url])
    return original + ("\n# Exact verified mirrors; existing integrity checks remain enforced by Bazel.\n" +
                       "\n".join(lines) + "\n").encode()


def require(value, message):
    if not value:
        raise ValueError(message)


def selection(image, revision):
    require(image in DRIVERS, "Unknown source image")
    require(re.fullmatch(r"[0-9a-f]{40}", revision), "Source revision must be a full lowercase Git SHA")


def run(argv, **kwargs):
    return subprocess.run(argv, check=True, **kwargs)


def output(argv, **kwargs):
    return subprocess.check_output(argv, text=True, **kwargs).strip()


def digest(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def input_run_id():
    value = os.environ.get("SOURCE_INPUTS_RUN", "")
    require(not value or re.fullmatch(r"[1-9][0-9]{0,19}", value), "Source-input run must be a numeric GitHub run ID")
    return value


def verify_input_run(args):
    run_id = input_run_id()
    require(run_id and os.environ.get("GITHUB_REPOSITORY") == "securely1g/sonic-buildimage", "Unexpected source-input repository")
    token = os.environ.get("GH_TOKEN", "")
    require(token, "Missing read-only Actions token")

    def api(suffix):
        request = urllib.request.Request("https://api.github.com/repos/securely1g/sonic-buildimage/" + suffix,
            headers={"Authorization": "Bearer " + token, "Accept": "application/vnd.github+json"})
        with urllib.request.urlopen(request, timeout=60) as response:
            return json.load(response)

    selected = api("actions/runs/" + run_id)
    require(selected["repository"]["full_name"] == "securely1g/sonic-buildimage" and
            selected["path"] == ".github/workflows/bazel-oci.yml" and selected["name"] == "Bazel OCI" and
            re.fullmatch(r"[0-9a-f]{40}", selected["head_sha"]), "Unexpected source-download workflow")
    jobs = api("actions/runs/" + run_id + "/jobs?per_page=100")
    require(jobs["total_count"] <= 100, "Source-download job inventory requires another page")
    producer = [job for job in jobs["jobs"] if job["name"] == "OCI regression checks (AMD64)" and job["conclusion"] == "success"]
    require(len(producer) == 1 and all(any(step["name"] == name and step["conclusion"] == "success"
        for step in producer[0]["steps"]) for name in ("Retain the verified GitLab source downloads", "Upload the source downloads for the VS job")),
        "The source-download producer has not succeeded")
    artifacts = api("actions/runs/" + run_id + "/artifacts?per_page=100")
    require(artifacts["total_count"] <= 100, "Source artifact inventory requires another page")
    matches = [item for item in artifacts["artifacts"] if item["name"] == "bazel-repository-inputs"]
    require(len(matches) == 1 and not matches[0]["expired"], "Expected one available source-download artifact")
    artifact = matches[0]
    require(artifact["workflow_run"]["id"] == int(run_id) and artifact["workflow_run"]["head_sha"] == selected["head_sha"] and
            artifact["workflow_run"]["repository_id"] == selected["repository"]["id"],
            "Source artifact provenance differs")
    receipt = {"repository": "securely1g/sonic-buildimage", "run": int(run_id), "source_revision": selected["head_sha"],
               "producer_job": producer[0]["id"], "producer_status": "success", "artifact": artifact["id"],
               "artifact_name": artifact["name"], "artifact_bytes": artifact["size_in_bytes"]}
    (args.artifacts / "worker-input-run.json").write_text(json.dumps(receipt, indent=2) + "\n")


def check_source_inventory(archives, reused):
    expected = {name + "-" + version + "/src.tar.gz" for name, (version, _) in SOURCES.items()} | {"receipt.json"}
    if reused:
        # The legacy artifact also carries an unused Redis test helper. Never execute it here.
        expected |= {"test-helpers/redis-server", "test-helpers/redis-copyright", "test-helpers/redis.json"}
    actual = set()
    directories = {str(parent) for name in expected for parent in Path(name).parents if str(parent) != "."}
    require(archives.is_dir() and not archives.is_symlink(), "Missing source-download directory")
    for path in archives.rglob("*"):
        relative = str(path.relative_to(archives))
        require(not path.is_symlink() and path.resolve().is_relative_to(archives.resolve()), "Unsafe source-download path")
        if path.is_dir():
            require(relative in directories, "Unexpected source-download directory")
        else:
            require(path.is_file(), "Source download is not a regular file")
            actual.add(relative)
    require(actual == expected, "Source-download artifact has missing or extra files")
    receipt = json.loads((archives / "receipt.json").read_text())
    require(isinstance(receipt, list) and len(receipt) == len(SOURCES), "Incomplete source-download receipt")
    require({item["module"] for item in receipt} == set(SOURCES), "Source-download receipt repeats or omits a module")
    prefix = "artifacts/repository-inputs" if reused else "artifacts/source-image-inputs"
    for item in receipt:
        version, integrity = SOURCES[item["module"]]
        relative = item["module"] + "-" + version + "/src.tar.gz"
        require(item["version"] == version and item["integrity"] == integrity and item["bytes"] == (archives / relative).stat().st_size
                and item["archive"] == prefix + "/" + relative, "Source-download receipt differs from the pinned inputs")


def prepare_source_downloads(archives, cache):
    """Reuse the VS workflow's BCR-integrity verifier and persistent distdir."""
    check_source_inventory(archives, bool(input_run_id()))
    destination = cache / "distdir"
    receipt = json.loads(output([sys.executable, "-B", str(ROOT / "tools/bazel/gzip/source_archive.py"),
        "--archives-dir", str(archives), "--output", str(destination)]))
    directories = [destination / (name + "-" + version) for name, (version, _) in SOURCES.items()]
    return directories, [{key: item[key] for key in ("module", "version", "integrity", "bytes")} for item in receipt]


def source_check(workspace, revision):
    require(output(["git", "-C", str(workspace), "rev-parse", "HEAD"]) == revision,
            "Source checkout differs from the reviewed revision")
    run(["git", "-C", str(workspace), "diff", "--exit-code", "--ignore-submodules=none", "HEAD"])


def prepare_teamd_kernel(artifacts, modules=Path("/sys/module")):
    """Require the original Teamd fixture's preloaded modules without host changes."""
    required = ("team", "team_mode_loadbalance")
    missing = [name for name in required if not (modules / name).is_dir()]
    receipt = {"status": "failed" if missing else "passed", "read_only": True,
               "required_modules": list(required), "missing_modules": missing,
               "preloaded_modules": [name for name in required if name not in missing]}
    (artifacts / "worker-teamd-kernel.json").write_text(json.dumps(receipt, indent=2) + "\n")
    require(not missing, "Teamd worker requires preloaded kernel modules: " + ", ".join(missing))


def prepare_tap_fixture(artifacts):
    """Prove the original VS profile's TAP support before building its image."""
    device = Path("/dev/net/tun").lstat()
    require(stat.S_ISCHR(device.st_mode) and os.major(device.st_rdev) == 10 and os.minor(device.st_rdev) == 200,
            "Syncd requires the standard /dev/net/tun character device")
    parent_namespace = Path("/proc/self/ns/net").stat().st_ino
    script = '''import fcntl, json, os, struct, sys
namespace = os.stat("/proc/self/ns/net").st_ino
if namespace == int(sys.argv[1]):
    raise SystemExit("TAP probe is not in a private network namespace")
with open("/dev/net/tun", "r+b", buffering=0) as device:
    result = fcntl.ioctl(device.fileno(), 0x400454ca, struct.pack("16sH", b"sonic-probe%d", 0x0002 | 0x1000))
    interface = result[:16].split(b"\\0", 1)[0].decode()
    if not interface.startswith("sonic-probe"):
        raise SystemExit("Unexpected private TAP name")
# Closing this nonpersistent descriptor removes the TAP; unshare removes its namespace.
print(json.dumps({"private_network_namespace": True, "tap_created_and_closed": True}))
'''
    proof = json.loads(output(["sudo", "-n", "unshare", "--net", "--", sys.executable, "-I", "-c", script,
                               str(parent_namespace)], timeout=30))
    require(proof == {"private_network_namespace": True, "tap_created_and_closed": True},
            "Incomplete private TAP proof")
    (artifacts / "worker-tap.json").write_text(json.dumps({"status": "passed", **proof,
        "device_major": 10, "device_minor": 200, "scope": "disposable network namespace; no VM or KVM"}, indent=2) + "\n")


EXECUTION_NETWORK_PROBE = '''import json, os, urllib.error, urllib.request
names = ("http_proxy", "https_proxy", "all_proxy", "no_proxy",
         "HTTP_PROXY", "HTTPS_PROXY", "ALL_PROXY", "NO_PROXY")
result = {"uid": os.getuid(), "proxy_names_present": {name: name in os.environ for name in names},
          "urllib_proxy_schemes": sorted(set(urllib.request.getproxies()) & {"http", "https", "ftp", "all", "no"})}
try:
    with urllib.request.urlopen("https://deb.debian.org/debian/dists/trixie/InRelease", timeout=10) as response:
        result["http_status"] = response.status
except urllib.error.HTTPError as error:
    result.update(http_status=error.code, error_class="HTTPError")
except Exception as error:
    result["error_class"] = type(error).__name__
print(json.dumps(result))
'''


def prepare_execution_network(artifacts):
    """Compare the two existing APT execution identities without recording values."""
    result = {}
    names = {"http_proxy", "https_proxy", "all_proxy", "no_proxy",
             "HTTP_PROXY", "HTTPS_PROXY", "ALL_PROXY", "NO_PROXY"}
    for context, prefix, expected_uid in (("worker", [], 1000), ("sudo", ["sudo", "-n"], 0)):
        try:
            value = json.loads(output([*prefix, sys.executable, "-I", "-c", EXECUTION_NETWORK_PROBE],
                                      timeout=20, stderr=subprocess.DEVNULL))
            require(set(value) <= {"uid", "proxy_names_present", "urllib_proxy_schemes", "http_status", "error_class"},
                    "Unexpected network diagnostic field")
            require(type(value["uid"]) is int and value["uid"] == expected_uid, "Unexpected diagnostic UID")
            require(set(value["proxy_names_present"]) == names and
                    all(type(present) is bool for present in value["proxy_names_present"].values()),
                    "Unexpected proxy presence diagnostic")
            require(isinstance(value["urllib_proxy_schemes"], list) and
                    all(scheme in {"http", "https", "ftp", "all", "no"} for scheme in value["urllib_proxy_schemes"]),
                    "Unexpected proxy scheme diagnostic")
            require("http_status" in value or "error_class" in value, "Missing network diagnostic outcome")
            if "http_status" in value:
                require(type(value["http_status"]) is int and 100 <= value["http_status"] <= 599,
                        "Unexpected HTTP status diagnostic")
            if "error_class" in value:
                require(isinstance(value["error_class"], str) and
                        re.fullmatch(r"[A-Za-z][A-Za-z0-9_]{0,63}", value["error_class"]),
                        "Unexpected network error class")
            safe_json(value)
            result[context] = value
        except Exception as error:
            result[context] = {"error_class": type(error).__name__}
    try:
        (artifacts / "worker-execution-network.json").write_text(json.dumps(result, indent=2) + "\n")
    except OSError:
        # The existing APT command remains authoritative even if diagnostics cannot be retained.
        print("Worker execution network diagnostic could not be retained", file=sys.stderr)


def prepare_validation_tools(artifacts):
    """Install only missing execution tools in the disposable slave container."""
    packages = {"bsdtar": "libarchive-tools", "gdb": "gdb", "readelf": "binutils", "objcopy": "binutils"}
    missing = sorted({package for tool, package in packages.items() if shutil.which(tool) is None})
    if missing:
        prepare_execution_network(artifacts)
        run(["sudo", "-n", "apt-get", "update"])
        run(["sudo", "-n", "env", "DEBIAN_FRONTEND=noninteractive", "apt-get", "install", "-y",
             "--no-install-recommends", *missing])
    for tool in packages:
        require(shutil.which(tool), "Missing image validation tool: " + tool)
    versions = output(["dpkg-query", "-W", "-f=${binary:Package}\t${Version}\n", *sorted(set(packages.values()))])
    (artifacts / "worker-execution-tools.json").write_text(json.dumps(
        {"packages_installed": missing, "package_versions": versions.splitlines(),
         "scope": "disposable_worker_execution_tools"}, indent=2) + "\n")


def catalog_for(image, workspace):
    workflow = (workspace / ".github/workflows/bazel-oci.yml").read_text()
    catalogs = sorted(set(re.findall(r"^ +SONIC_BAZEL_REGISTRY_REVISION: ([0-9a-f]{40})$", workflow, re.M)))
    wrapper = WRAPPERS[image]
    require(re.search(r"^ +BAZEL: (?:\$\{\{ github.workspace \}\}/)?tools/bazel/ci/" + re.escape(wrapper) + r"$", workflow, re.M),
            "Selected workflow uses another Bazel entrypoint")
    if wrapper != "review_bazel.sh":
        text = (workspace / "tools/bazel/ci" / wrapper).read_text()
        selected = re.findall(r"^export SONIC_BAZEL_REGISTRY_REVISION=([0-9a-f]{40})$", text, re.M)
        require(len(selected) == 1 and (not catalogs or selected == catalogs),
                "Workflow and compatibility entrypoint catalogs differ")
        return selected[0]
    require(len(catalogs) == 1, "Expected one immutable global workflow catalog")
    return catalogs[0]


def memory_snapshot():
    root = Path("/sys/fs/cgroup")
    result = {name: int((root / ("memory." + name)).read_text()) for name in ("current", "peak")}
    result["limit"] = (root / "memory.max").read_text().strip()
    result["swap_max"] = (root / "memory.swap.max").read_text().strip()
    result["swap_current"] = int((root / "memory.swap.current").read_text())
    result["cpu_max"] = (root / "cpu.max").read_text().strip()
    result["events"] = {name: int(value) for name, value in
                        (line.split() for line in (root / "memory.events").read_text().splitlines())}
    statistics = dict(line.split() for line in (root / "memory.stat").read_text().splitlines())
    result["usage"] = {name: int(statistics[name]) for name in ("anon", "file", "kernel")}
    return result


def cache_layout(image):
    """Share immutable downloads/action results; keep each Bazel server private."""
    require(image in DRIVERS, "Unknown source image")

    def checked(raw, mount):
        require(isinstance(raw, str) and re.fullmatch(r"/[A-Za-z0-9_./-]+", raw),
                "Invalid Bazel cache/state path")
        path = Path(raw)
        require(path.is_absolute() and not any(part in (".", "..") for part in raw.split("/"))
                and path.is_relative_to(mount), "Bazel cache/state path is outside its mount")
        require(path.resolve() == path, "Bazel cache/state path must not traverse symlinks")
        require(not path.exists() or path.is_dir(), "Bazel cache/state path is not a directory")
        return path

    cache = checked(os.environ.get("SONIC_BAZEL_CACHE_ROOT", str(CACHE_MOUNT)), CACHE_MOUNT)
    state = checked(os.environ.get("SONIC_BAZEL_STATE_ROOT", str(WORK_MOUNT / "bazel")), WORK_MOUNT)
    paths = {"repository": cache / "repository", "disk": cache / "disk", "distdir": cache / "distdir",
             "bazelisk": os.environ.get("BAZELISK_HOME", str(cache / "bazelisk")),
             "output_base": state / image / "output-base"}
    for name, path in paths.items():
        paths[name] = checked(str(path), WORK_MOUNT if name == "output_base" else CACHE_MOUNT)
    values = list(paths.values())
    require(all(not a.is_relative_to(b) and not b.is_relative_to(a)
                for index, a in enumerate(values) for b in values[index + 1:]),
            "Bazel cache/state directories must not overlap")
    # The verified distdir publisher uses atomic replacement, but its parent
    # directories must not redirect a shared write outside the cache mount.
    for name, (version, _) in SOURCES.items():
        directory = checked(str(paths["distdir"] / (name + "-" + version)), CACHE_MOUNT)
        require(not (directory / "src.tar.gz").is_symlink(), "Source archive must not be a symlink")
    return paths


def launcher_text(binary, base, profile):
    flags = ["--remote_cache=", "--remote_download_regex=(^|.*/)tools/build_tools/kernel_runtime_(amd64|arm64)(/.*)?$",
             "--jobs=" + str(profile["jobs"]), "--local_resources=cpu=" + str(profile["cpu"]),
             "--local_resources=memory=" + str(profile["scheduler_memory_mb"])]
    return ("#!/usr/bin/env python3\nimport os, sys\n" +
            "binary = " + repr(binary) + "\nbase = " + repr(str(base)) + "\nargs = sys.argv[1:]\n" +
            "if any(value in ('build', 'test', 'aquery', 'cquery') for value in args):\n" +
            "    args.extend(" + repr(flags) + ")\n" +
            "os.execv(binary, [binary, '--output_base=' + base, '--host_jvm_args=-Xmx" +
            str(profile["jvm_heap_gib"]) + "g', " +
            "'--host_jvm_args=-Djavax.net.ssl.trustStore=/etc/ssl/certs/java/cacerts', *args])\n")


def phase_command(image, phase, workspace, artifacts):
    """Only the selected existing, fixed driver interfaces are executable."""
    require(image in DRIVERS and phase in ("build", "validate", "incremental", "loaded"), "Unknown driver phase")
    prefix = [sys.executable, "-B"]
    wrapper = str(workspace / "tools/bazel/ci" / WRAPPERS[image])
    if phase == "loaded":
        return prefix + [str(workspace / f"tools/bazel/ci/{DRIVERS[image]}_loaded.py"), "--artifacts", str(artifacts)]
    if phase == "incremental" and image == "orchagent":
        return prefix + [str(workspace / "tools/bazel/ci/swss_image_rebuild.py"), "--bazel", wrapper,
                         "--runtime-target", "//dockers/docker-orchagent:docker-orchagent.gz",
                         "--additional-runtime-target", "//dockers/docker-orchagent:docker-orchagent-dbg.gz",
                         "--additional-runtime-target", "//dockers/docker-orchagent:rdeps",
                         "--symbols-target", "//dockers/docker-orchagent:rdeps_debug_symbols",
                         "--unchanged-target", "//dockers/docker-config-engine-trixie:docker-config-engine-trixie.gz",
                         "--unchanged-target", "@scapy//:wheel", "--output", str(artifacts / "rebuild")]
    command = prefix + [str(workspace / f"tools/bazel/ci/{DRIVERS[image]}_image.py"), phase,
                        "--artifacts", str(artifacts)]
    if image in GNMI_PROFILES:
        command += ["--dialout", GNMI_PROFILES[image]]
    return command + (["--bazel", wrapper] if phase != "validate" else [])


def prepare(args):
    workspace, artifacts = args.workspace, args.artifacts
    if args.image == "teamd":
        prepare_teamd_kernel(artifacts)
    preflight = measure(os.environ.get("RUNNER_NAME", ""))
    (artifacts / "worker-preflight.json").write_text(json.dumps(preflight, indent=2) + "\n")
    require(preflight["status"] == "passed", "Worker resource/capability preflight failed")
    profile = preflight["build_profile"]
    require(os.getuid() == 1000 and os.getgid() == 1000, "Expected worker UID/GID1000")
    require(platform.machine() == "x86_64" and
            platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie", "Expected native AMD64 Debian Trixie")
    require(Path("/.dockerenv").is_file() and workspace.is_relative_to(WORK_MOUNT / "_work"), "Expected a private SONiC slave workspace")
    require(not any(os.environ.get(k) for k in ("DOCKER_HOST", "DOCKER_CONTEXT", "DOCKER_TLS_VERIFY", "DOCKER_CERT_PATH"))
            and not Path("/var/run/docker.sock").exists(), "Refuse an inherited Docker daemon")
    runner = os.environ.get("RUNNER_NAME", "")
    require(re.fullmatch(r"[A-Za-z0-9_.-]+", runner) and runner not in (".", ".."), "Invalid worker name")
    marker = Path("/etc/sonic-slave-github-runner.json")
    info = marker.lstat()
    require(stat.S_ISREG(info.st_mode) and info.st_uid == 0 and not info.st_mode & 0o222, "Runner marker must be root-owned and read-only")
    marker_data = json.loads(marker.read_text())
    require(all(marker_data.get(k) == v for k, v in {"uid": 1000, "gid": 1000, "work_root": "/actions/_work"}.items()), "Unexpected worker profile")
    require(re.fullmatch(r"sha256:[0-9a-f]{64}", marker_data.get("base_image", "")), "Missing immutable slave image identity")
    for name in ("bazel", "docker", "dockerd", "sudo", "git", "make", "readelf", "mountpoint", "findmnt"):
        require(shutil.which(name), "Missing worker tool: " + name)
    require(Path("/etc/ssl/certs/java/cacerts").is_file() and os.access("/etc/ssl/certs/java/cacerts", os.R_OK),
            "Missing readable system Java trust store")
    run(["sudo", "-n", "true"])
    for mount in ("/actions", "/bazel_cache"):
        run(["mountpoint", "-q", mount])
    require(output(["findmnt", "-n", "-o", "FSTYPE", "-T", "/actions"]) in ("ext4", "xfs"), "Docker storage requires ext4 or XFS")
    if args.image == "syncd":
        prepare_tap_fixture(artifacts)
    prepare_validation_tools(artifacts)
    source_check(workspace, args.revision)
    for name in (f"{DRIVERS[args.image]}_image.py", f"{DRIVERS[args.image]}_loaded.py", WRAPPERS[args.image], "review_bazel.sh"):
        path = workspace / "tools/bazel/ci" / name
        require(path.is_file() and path.resolve().is_relative_to(workspace), "Missing reviewed driver: " + name)
    catalog = catalog_for(args.image, workspace)
    if args.image == "sysmgr":
        run(["git", "-C", str(workspace), "submodule", "update", "--init", "--", "src/sonic-sysmgr/gnoi"])
    caches = cache_layout(args.image)
    for path in caches.values():
        path.mkdir(parents=True, exist_ok=True)
    temporary = Path(os.environ["RUNNER_TEMP"]).resolve()
    require(temporary.is_relative_to(WORK_MOUNT / "_work"), "Expected a worker-private temporary directory")
    distdirs, download_receipt = prepare_source_downloads(temporary / "source-image-inputs", caches["distdir"].parent)
    (artifacts / "worker-source-downloads.json").write_text(json.dumps(download_receipt, indent=2) + "\n")
    launch = temporary / "source-image-bazel"
    launch.write_text(launcher_text(shutil.which("bazel"), caches["output_base"], profile))
    launch.chmod(0o755)
    mirror = temporary / "source-image-mirrors.cfg"
    mirror.write_bytes(mirror_configuration(source_mirrors.download()))
    rc = workspace / ".bazelrc.user"
    require(not rc.exists(), "Expected a fresh source checkout without a local Bazel override")
    rc.write_text("\n".join([
        "# Shared download/action caches and a worker-private Bazel output base; no Make package cache.",
        "common --repository_cache=" + str(caches["repository"]),
        "common --disk_cache=" + str(caches["disk"]),
        "common --downloader_config=" + str(mirror),
        *["common --distdir=" + str(directory) for directory in distdirs],
    ]) + "\n")
    run_key = os.environ.get("GITHUB_RUN_ID", "") + "-" + os.environ.get("GITHUB_RUN_ATTEMPT", "")
    require(re.fullmatch(r"[0-9]+-[0-9]+", run_key), "Missing workflow run identity")
    docker_root = temporary / ("source-image-docker-" + run_key)
    pidfile = temporary / ("source-image-dockerd-" + run_key + ".pid")
    require(not docker_root.exists() and not pidfile.exists(), "Private daemon state must be fresh")
    client = temporary / "source-image-docker-client"
    client.mkdir(mode=0o700)
    daemon_config = temporary / "source-image-dockerd.json"
    daemon_config.write_text("{}\n")
    env = {"BAZEL_REAL": str(launch), "BAZELISK_HOME": str(caches["bazelisk"]),
           "SONIC_BAZEL_REGISTRY_REVISION": catalog, "CI_HEAD_SHA": args.revision, "CI_BASE_SHA": "",
           "DOCKER_HOST": "unix:///var/run/docker.sock", "DOCKER_CONFIG": str(client), "BAZELISK_GITHUB_TOKEN": ""}
    state = {"image": args.image, "revision": args.revision, "workspace": str(workspace), "environment": env,
             "pidfile": str(pidfile), "docker_root": str(docker_root), "build_profile": profile}
    (artifacts / "worker-private.json").write_text(json.dumps(state, indent=2) + "\n")
    run(["sudo", "-n", sys.executable, str(ROOT / "tools/bazel/ci/prepare_nested_docker.py")])
    with (artifacts / "dockerd.log").open("w") as log:
        subprocess.Popen(["sudo", "-n", "/usr/bin/env", "-i", "PATH=/usr/sbin:/usr/bin:/sbin:/bin", "HOME=/root",
                          shutil.which("dockerd"), "--config-file=" + str(daemon_config),
                          "--host=" + env["DOCKER_HOST"], "--group=docker", "--data-root=" + str(docker_root),
                          "--exec-root=/run/sonic-source-image-docker", "--pidfile=" + str(pidfile),
                          "--storage-driver=overlay2", "--exec-opt=native.cgroupdriver=cgroupfs",
                          "--bridge=none", "--iptables=false", "--ip6tables=false", "--ip-forward=false", "--ip-masq=false"],
                         stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        try:
            probe = subprocess.run(["docker", "info", "--format", "{{.DockerRootDir}}"],
                                   env={**os.environ, **env}, text=True, capture_output=True, timeout=5)
        except subprocess.TimeoutExpired:
            continue
        if probe.returncode == 0:
            require(probe.stdout.strip() == str(docker_root), "Docker data root differs")
            break
        time.sleep(1)
    else:
        raise ValueError("Private Docker did not become ready")
    receipt = {"schema": 1, "status": "prepared", "worker": runner, "architecture": "amd64", "distribution": "trixie",
               "source_revision": args.revision, "dispatcher_revision": output(["git", "-C", str(ROOT), "rev-parse", "HEAD"]),
               "review_catalog": catalog, "slave_image": marker_data["base_image"],
               "cache_mode": "shared_downloads_and_actions_private_output",
               "build_profile": profile, "memory": memory_snapshot(),
               "docker_network": "none", "image": args.image}
    safe_json(receipt)
    (artifacts / "worker.json").write_text(json.dumps(receipt, indent=2) + "\n")


def state_for(args):
    state = json.loads((args.artifacts / "worker-private.json").read_text())
    require(state["image"] == args.image and state["revision"] == args.revision and state["workspace"] == str(args.workspace),
            "Worker preparation does not match this selection")
    return state


def prepared_profile(state, memory):
    profile = select_build_profile(int(memory["limit"]), cpu_limit(memory["cpu_max"]))
    require(state.get("build_profile") == profile, "Worker resources/profile differ from preparation")
    return profile


def execute_phase(args):
    state = state_for(args)
    source_check(args.workspace, args.revision)
    before = memory_snapshot()
    profile = prepared_profile(state, before)
    if args.phase == "loaded":
        # The worker's own server is idle here; disk/output caches remain intact.
        run([state["environment"]["BAZEL_REAL"], "shutdown"], cwd=args.workspace,
            env={**os.environ, **state["environment"]}, timeout=90)
    started = time.monotonic()
    finished = threading.Event()

    def sample():
        while not finished.wait(30):
            print(json.dumps({"phase": args.phase, "elapsed_seconds": round(time.monotonic() - started),
                              "configured_graph_audited": (args.artifacts / "action-audit.json").is_file(),
                              "memory": memory_snapshot()}), flush=True)

    monitor = threading.Thread(target=sample, daemon=True)
    monitor.start()
    try:
        completed = subprocess.run(phase_command(args.image, args.phase, args.workspace, args.artifacts),
                                   cwd=args.workspace, env={**os.environ, **state["environment"]})
    finally:
        finished.set()
        monitor.join()
    source_check(args.workspace, args.revision)
    after = memory_snapshot()
    memory_ok = all(after["events"].get(key, 0) == before["events"].get(key, 0) for key in ("oom", "oom_kill"))
    receipt = {"phase": args.phase, "status": "passed" if completed.returncode == 0 and memory_ok else "failed",
               "source_revision": args.revision, "image": args.image, "elapsed_seconds": round(time.monotonic() - started, 3),
               "exit_code": completed.returncode, "memory_before": before, "memory_after": after,
               "build_profile": profile, "no_oom_events": memory_ok}
    (args.artifacts / ("worker-" + args.phase + ".json")).write_text(json.dumps(receipt, indent=2) + "\n")
    require(completed.returncode == 0, "Image phase failed: " + args.phase)
    require(memory_ok, "Worker exhausted its memory during " + args.phase)


def publish(args):
    """Stage only declared payloads and checked public evidence, never private logs/env."""
    source, destination = args.artifacts, args.artifacts / "public"
    destination.mkdir(exist_ok=True)
    receipt_path = source / "build-receipt.json"
    if receipt_path.exists():
        receipt = json.loads(receipt_path.read_text())
        if getattr(args, "image", None) in GNMI_PROFILES:
            require(receipt.get("dialout_enabled") is (GNMI_PROFILES[args.image] == "enabled"),
                    "gNMI receipt differs from the selected fixed dialout profile")
        public = {k: receipt[k] for k in ("status", "architecture", "revision", "tested_revision_parents", "dialout_enabled", "image_version", "artifacts") if k in receipt}
        safe_json(public)
        (destination / "public-receipt.json").write_text(json.dumps(public, indent=2) + "\n")
        for name, item in receipt.get("artifacts", {}).items():
            require(Path(name).name == name and name not in (".", ".."), "Invalid artifact filename")
            path = source / name
            require(path.is_file() and not path.is_symlink() and path.stat().st_size == item["bytes"] and digest(path) == item["sha256"],
                    "Declared artifact differs: " + name)
            if path.suffix == ".json":
                value = json.loads(path.read_text())
                if name == "policy.json" and getattr(args, "image", None) in ("syncd", "gearbox"):
                    # Reviewed image policy, not a captured process environment.
                    # Validate only this known public constant; retain original bytes.
                    require(value["image_validation"].pop("environment") == {"DEBIAN_FRONTEND": "noninteractive"},
                            "Unexpected image policy environment")
                safe_json(value)
            target = destination / name
            if target.exists():
                require(target.is_file() and not target.is_symlink() and digest(target) == item["sha256"],
                        "Existing public artifact differs: " + name)
            else:
                os.link(path, target)
    names = ["worker.json", "action-audit.json", "validation.json", "loaded-validation.json", "incremental.json", "rebuild/rebuild.json"]
    names += [p.name for pattern in ("worker-*.json", "*.validation.json", "*.runtime.json") for p in source.glob(pattern)
              if p.name != "worker-private.json"]
    for name in sorted(set(names)):
        path = source / name
        if path.is_file():
            safe_json(json.loads(path.read_text()))
            target = destination / name
            target.parent.mkdir(exist_ok=True)
            shutil.copyfile(path, target)
    for name, kind in (("MODULE.bazel.lock", "lock"), ("module-graph.json", "graph")):
        path = source / name
        if path.is_file():
            dependency_json(json.loads(path.read_text()), kind)
            shutil.copyfile(path, destination / name)
    summary = {str(path.relative_to(source)): bep_summary(path.read_text().splitlines(), partial=True)
               for path in source.rglob("*events.json*") if not path.is_relative_to(destination)}
    safe_json(summary)
    (destination / "build-summary.json").write_text(json.dumps(summary, indent=2) + "\n")


def stop(args):
    path = args.artifacts / "worker-private.json"
    if not path.exists():
        return
    state = state_for(args)
    pidfile = Path(state["pidfile"])
    if not pidfile.exists():
        require(not Path(state["docker_root"]).exists(), "Daemon left data without a PID; retain it for diagnosis")
        return
    raw = pidfile.read_text().strip()
    require(raw.isdecimal() and int(raw) > 1, "Invalid private daemon PID")
    process = Path("/proc") / raw
    if process.exists():
        require((process / "comm").read_text().strip() == "dockerd" and
                ("--data-root=" + state["docker_root"]).encode() in (process / "cmdline").read_bytes().split(b"\0"),
                "PID no longer identifies this worker's Docker daemon")
        run(["sudo", "-n", "kill", "-TERM", raw])
        deadline = time.monotonic() + 60
        while process.exists() and time.monotonic() < deadline:
            time.sleep(0.5)
        require(not process.exists(), "Private Docker has not stopped; retain its data")
    root = Path(state["docker_root"])
    temporary = Path(os.environ["RUNNER_TEMP"]).resolve()
    require(root.parent == temporary and temporary.is_relative_to("/actions/_work")
            and re.fullmatch(r"source-image-docker-[0-9]+-[0-9]+", root.name), "Unexpected private Docker data path")
    # Delete only this stopped job's Docker data, never its persistent Bazel caches.
    cleanup = """import os, pathlib, re, shutil, sys
root = pathlib.Path(sys.argv[1])
assert not root.is_symlink() and root.parent.resolve() == root.parent
for line in pathlib.Path('/proc/self/mountinfo').read_text().splitlines():
    mount = re.sub(r'\\\\([0-7]{3})', lambda m: chr(int(m[1], 8)), line.split()[4])
    assert not pathlib.Path(mount).is_relative_to(root), 'Private Docker still has a mounted child'
if root.exists():
    shutil.rmtree(root)
"""
    run(["sudo", "-n", sys.executable, "-c", cleanup, str(root)], timeout=120)
    pidfile.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("selection", "kernel-preflight", "verify-input-run", "prepare", "build", "validate", "incremental", "loaded", "publish", "stop"))
    parser.add_argument("--image", required=True, choices=DRIVERS)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--workspace", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    args = parser.parse_args()
    selection(args.image, args.revision)
    input_run_id()
    args.workspace = args.workspace.resolve()
    args.artifacts = args.artifacts.resolve()
    require(not args.artifacts.is_relative_to(args.workspace) and not args.workspace.is_relative_to(args.artifacts),
            "Keep worker evidence outside the source checkout")
    if args.phase == "selection":
        return
    args.artifacts.mkdir(parents=True, exist_ok=True)
    if args.phase == "kernel-preflight":
        require(args.image == "teamd", "Kernel module preflight is only defined for Teamd")
        prepare_teamd_kernel(args.artifacts)
    elif args.phase == "verify-input-run":
        verify_input_run(args)
    elif args.phase == "prepare":
        prepare(args)
    elif args.phase == "publish":
        publish(args)
    elif args.phase == "stop":
        stop(args)
    else:
        execute_phase(args)


if __name__ == "__main__":
    main()
