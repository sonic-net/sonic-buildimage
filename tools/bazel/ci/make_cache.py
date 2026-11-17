#!/usr/bin/env python3
"""Prove Common cache reuse through Make's real image recipe in sonic-slave."""

import argparse
from collections import Counter
import hashlib
import importlib
import io
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tarfile


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel import build_helpers
from tools.bazel.ci import make_cache_sandbox

ARCHITECTURES = {"amd64": "x86_64", "arm64": "aarch64"}
SLAVE_REPOSITORY = "sonicdev-microsoft.azurecr.io/sonic-slave-trixie"
LOCAL_SLAVE = "sonic-cache-slave-trixie"
IMAGE = "target/docker-config-engine-trixie.gz"
PRIVATE = Path("artifacts/private")
PUBLIC = Path("artifacts/make-cache")
CONTAINER_PRIVATE = Path("/sonic") / PRIVATE
OUTPUT_BASE = CONTAINER_PRIVATE / "read-output"
INSTALL_BASE = CONTAINER_PRIVATE / "bazel-install"
ACTION_LOG = CONTAINER_PRIVATE / "make-actions.json"
GIB = 1024 ** 3


def require(condition, message):
    if not condition:
        raise ValueError(message)


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def native_architecture(architecture, machine=None):
    require((platform.machine() if machine is None else machine) == ARCHITECTURES[architecture],
            "The cache proof requires a native " + architecture + " runner")


def command(argv, *, log=None, helpers=None, env=None, preflight_diagnostics=False):
    """Keep raw command output private, including errors from Make and Bazel."""
    result = subprocess.run(argv, cwd=ROOT, env=env, text=True, capture_output=True)
    if log is not None:
        log.parent.mkdir(parents=True, exist_ok=True)
        log.write_text(result.stdout + result.stderr)
    if result.returncode:
        if helpers is not None:
            helpers.validate.print_safe_diagnostics(result.stdout + result.stderr)
        elif preflight_diagnostics:
            # Preflight removes cache authentication from the child environment.
            # Docker reports registry/network errors here, never action records.
            safe = re.sub(r"(?i)\b(?:basic|bearer)\s+\S+", "[REDACTED]", result.stderr)
            safe = re.sub(r"(?i)authorization\s*[:=]\s*[^\r\n]+", "Authorization: [REDACTED]", safe)
            print("\n".join(safe.splitlines()[-40:]), file=sys.stderr, flush=True)
        raise ValueError("Command failed: " + Path(argv[0]).name + "; see sanitized diagnostics")
    return result.stdout


def digest(value):
    require(bool(re.fullmatch(r"sha256:[0-9a-f]{64}", value)), "Invalid builder manifest digest")
    return value


def native_manifest(manifest, architecture):
    """Select a native image, rejecting absent or ambiguous platform entries."""
    entries = manifest.get("manifests")
    if entries is None:
        return None
    matches = [item for item in entries if item.get("platform", {}).get("os") == "linux"
               and item["platform"].get("architecture") == architecture]
    require(len(matches) == 1, "Builder index must contain exactly one native Linux image")
    return digest(matches[0]["digest"])


def preflight(architecture):
    native_architecture(architecture)
    require(shutil.disk_usage(ROOT).free >= 35 * GIB,
            "Need at least 35 GiB free before pulling sonic-slave")
    tag = SLAVE_REPOSITORY + ":master-" + architecture
    environment = dict(os.environ)
    environment.pop("BAZEL_REMOTE_AUTH", None)
    def docker(arguments, **kwargs):
        return command(["docker", *arguments], env=environment, preflight_diagnostics=True, **kwargs)
    tag_digest = digest(docker(["buildx", "imagetools", "inspect", tag,
                               "--format", "{{.Manifest.Digest}}"]).strip())
    reference = SLAVE_REPOSITORY + "@" + tag_digest
    manifest = json.loads(docker(["buildx", "imagetools", "inspect", "--raw", reference]))
    selected = native_manifest(manifest, architecture)
    if selected:
        reference = SLAVE_REPOSITORY + "@" + selected
        manifest = json.loads(docker(["buildx", "imagetools", "inspect", "--raw", reference]))
    require("layers" in manifest and "config" in manifest, "Builder has no image manifest")
    sizes = [item["size"] for item in [manifest["config"], *manifest["layers"]]]
    require(all(isinstance(size, int) and size >= 0 for size in sizes) and sum(sizes) > 0,
            "Builder manifest has invalid compressed sizes")
    builder_digest = selected or tag_digest
    docker(["pull", reference], log=ROOT / PRIVATE / "slave-pull.log")
    image = json.loads(docker(["image", "inspect", reference]))[0]
    require(image.get("Architecture") == architecture and image.get("Os") == "linux",
            "Pulled sonic-slave does not match the native Linux architecture")
    release = docker(["run", "--rm", "--entrypoint", "/bin/sh", reference, "-c",
                      '. /etc/os-release; printf "%s:%s" "$ID" "$VERSION_CODENAME"'])
    require(release == "debian:trixie", "The builder must be the real Debian Trixie sonic-slave")
    require(shutil.disk_usage(ROOT).free >= 18 * GIB,
            "Need at least 18 GiB free after pulling sonic-slave")
    local_tag = builder_digest[7:19] + "-" + architecture
    docker(["tag", reference, LOCAL_SLAVE + ":" + local_tag])
    receipt = {"architecture": architecture, "os": "linux", "release": "trixie",
               "registry_tag": tag, "tag_digest": tag_digest, "digest": builder_digest,
               "immutable_reference": reference, "compressed_bytes": sum(sizes),
               "uncompressed_bytes": image["Size"], "image_id": image["Id"], "local_image": LOCAL_SLAVE,
               "local_tag": local_tag}
    write_json(ROOT / PUBLIC / "builder.json", receipt)
    print("Pinned native sonic-slave: " + reference, flush=True)


def common_helpers(checkout):
    directory = checkout / "tools/bazel/cache"
    require((directory / "cross_repo.py").is_file(), "Common cache-proof helpers are missing")
    sys.path.insert(0, str(directory))
    return importlib.import_module("cross_repo")


def seed_receipt(architecture):
    path = ROOT / PUBLIC / "seed/cross-repo.json"
    seed = json.loads(path.read_text())
    require(seed.get("passed") is True and seed.get("mode") == "make_seed",
            "A successful Common CI make_seed receipt is required")
    require(seed.get("architecture") == architecture and seed.get("yang") is True,
            "Common seed must use the same architecture with YANG enabled")
    require(len(seed.get("phases", [])) == 1 and seed["phases"][0].get("phase") == "seed",
            "Expected one Common seed phase")
    for field in ("common_revision", "buildimage_revision"):
        require(bool(re.fullmatch(r"[0-9a-f]{40}", seed.get(field, ""))),
                "Seed must record the full " + field)
    return seed, sha256(path)


def verify_common_source(checkout, revision):
    """Verify the tracked export, without copying a producer's Bazel state."""
    actual = command(["git", "-C", str(checkout), "rev-parse", "HEAD"]).strip()
    require(actual == revision, "Common checkout does not match the successful seed")
    require(not command(["git", "-C", str(checkout), "status", "--porcelain", "--untracked-files=no"]),
            "Common checkout has uncommitted tracked changes")
    archive = subprocess.check_output(["git", "-C", str(checkout), "archive", "HEAD"])
    destination = ROOT / PRIVATE / "common-source"
    with tarfile.open(fileobj=io.BytesIO(archive)) as source:
        if not destination.exists():
            destination.mkdir(parents=True)
            source.extractall(destination, filter="data")
        expected = set()
        for member in source.getmembers():
            path = destination / member.name
            if member.isdir():
                continue
            expected.add(member.name)
            if member.issym():
                require(path.is_symlink() and os.readlink(path) == member.linkname,
                        "Common exported symlink differs from its seed revision")
            else:
                require(member.isfile() and path.is_file() and not path.is_symlink(),
                        "Common exported file is missing")
                with source.extractfile(member) as data:
                    require(hashlib.sha256(data.read()).hexdigest() == sha256(path),
                            "Common exported source differs from its seed revision")
                require(bool(path.stat().st_mode & 0o111) == bool(member.mode & 0o111),
                        "Common exported executable mode differs from its seed revision")
        actual_files = {str(path.relative_to(destination)) for path in destination.rglob("*")
                        if path.is_symlink() or path.is_file()}
        require(actual_files == expected, "Common export contains untracked files or build outputs")
    (ROOT / PRIVATE / "common-source.revision").write_text(revision + "\n")
    return hashlib.sha256(archive).hexdigest()


def require_fresh_workspace(root):
    """A new image and output base prevent local outputs from hiding cache misses."""
    paths = [PRIVATE / "read-output", PRIVATE / "bazel-install", PRIVATE / "repository-cache", PRIVATE / "make-actions.json",
             Path("MODULE.bazel.lock"), Path(".bazelrc.user"), Path("target"),
             Path(".platform"), Path(".arch")]
    paths.extend(path.relative_to(root) for path in root.glob("bazel-*"))
    require(not any((root / path).exists() or (root / path).is_symlink() for path in paths),
            "Make cache readback requires a fresh workspace and private output base")


def bazel_invocation(arguments):
    """cquery must not replace the execution evidence from the real image build."""
    require(bool(arguments), "A Bazel command is required")
    subcommand, *options = arguments
    if subcommand == "build":
        options = ["--execution_log_json_file=" + str(ACTION_LOG), *options]
    return ["bazel", "--batch", "--nosystem_rc", "--nohome_rc",
            "--install_base=" + str(INSTALL_BASE), "--output_base=" + str(OUTPUT_BASE), subcommand, *options]


def container_options():
    return ["--override_module=sonic-swss-common=" + str(CONTAINER_PRIVATE / "common-source"),
            "--jobs=4", "--local_resources=cpu=4", "--local_resources=memory=10000",
            "--spawn_strategy=linux-sandbox", "--strategy=Genrule=linux-sandbox",
            # Upstream OCI layout copies declare no-sandbox. Keep this exception
            # specific to copying; compilation and linking still use the sandbox.
            "--strategy=CopyToDirectory=local",
            "--noincompatible_legacy_local_fallback", "--disk_cache=",
            "--repository_cache=" + str(CONTAINER_PRIVATE / "repository-cache"),
            "--remote_download_outputs=all", "--remote_cache_async=false",
            # Avoid sorting the full execution log in Bazel's heap at shutdown.
            "--execution_log_sort=false",
            "--check_direct_dependencies=off", "--color=no", "--curses=no", "--noshow_progress"]


def make_command(architecture, builder, cache_root):
    require(bool(re.fullmatch(r"[/A-Za-z0-9_.-]+", str(cache_root))),
            "Make cache path must not contain shell metacharacters or whitespace")
    return ["make", "-f", "Makefile.work", "BLDENV=trixie", "PLATFORM=vs",
            "PLATFORM_ARCH=" + architecture, "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "INCLUDE_FIPS=y",
            "SONIC_BUILD_JOBS=4", "SONIC_DPKG_CACHE_METHOD=none",
            "SONIC_DPKG_CACHE_METHOD_OVERRIDE=none", "SONIC_VERSION_CACHE_METHOD=none",
            "SONIC_DPKG_CACHE_SOURCE=" + str(cache_root / "dpkg"),
            "SONIC_VERSION_CACHE_SOURCE=" + str(cache_root / "versions"),
            "SONIC_BAZEL_CACHE_SOURCE=" + str(cache_root / "bazel"),
            "SONIC_SLAVE_REUSE_TRIXIE_IMAGE=" + builder["local_image"],
            "SONIC_SLAVE_REUSE_TRIXIE_TAG=" + builder["local_tag"],
            "SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD=n",
            "SONIC_BUILDER_EXTRA_CMDLINE=-e BAZEL -e BAZEL_CONTAINER_ARGS", "UNATTENDED=y"]


def make_environment():
    return dict(os.environ, BAZEL=str(CONTAINER_PRIVATE / "bazel-read"),
                BAZEL_CONTAINER_ARGS=shlex.join(container_options()),
                BAZEL_CONTAINER_CACHE_DIR="", BAZEL_SWSS_CACHE_DIR="")


def prepare(architecture, checkout):
    native_architecture(architecture)
    require(os.geteuid() != 0, "Run outer Make as the hosted runner user, not root")
    helpers = common_helpers(checkout)
    seed, seed_hash = seed_receipt(architecture)
    helpers.check_actions(seed["phases"][0], remote=False)
    revision = command(["git", "rev-parse", "HEAD"]).strip()
    require(bool(re.fullmatch(r"[0-9a-f]{40}", revision)), "Buildimage must have a full commit revision")
    require_fresh_workspace(ROOT)
    source_hash = verify_common_source(checkout, seed["common_revision"])
    builder = json.loads((ROOT / PUBLIC / "builder.json").read_text())
    require(builder["architecture"] == architecture, "Builder preflight used a different architecture")
    local_image = json.loads(command(["docker", "image", "inspect",
                                     builder["local_image"] + ":" + builder["local_tag"]]))[0]
    require(local_image["Id"] == builder["image_id"], "Pinned local builder changed after preflight")
    cache_root = Path(os.environ["RUNNER_TEMP"]) / "make-common-cache"
    require(not cache_root.exists(), "The Make proof requires fresh job cache directories")
    for name in ("dpkg", "versions", "bazel"):
        (cache_root / name).mkdir(parents=True)
    private = ROOT / PRIVATE
    private.mkdir(parents=True, exist_ok=True)
    wrapper = private / "bazel-read"
    wrapper.write_text("#!/usr/bin/env python3\n"
                       "# AUTO-GENERATED by tools/bazel/ci/make_cache.py. DO NOT EDIT.\n"
                       "import os, sys\n"
                       "sys.path.insert(0, '/sonic')\n"
                       "from tools.bazel.ci.make_cache import bazel_invocation\n"
                       "os.execvp('bazel', bazel_invocation(sys.argv[1:]))\n")
    wrapper.chmod(0o755)
    environment = make_environment()
    prefix = make_command(architecture, builder, cache_root)
    receipt = {"passed": False, "architecture": architecture, "buildimage_revision": revision,
               "seed_buildimage_revision": seed["buildimage_revision"],
               "common_revision": seed["common_revision"], "common_source_archive_sha256": source_hash,
               "seed_receipt_sha256": seed_hash, "builder": builder, "make_command": [*prefix, IMAGE],
               "cache_writes": False, "disk_cache": False, "fresh_workspace": True,
               "fresh_output_base": True, "yang": True}
    write_json(ROOT / PUBLIC / "result.json", receipt)
    write_json(private / "make-plan.json", {"architecture": architecture, "make_prefix": prefix,
                                           "buildimage_revision": revision})
    probe_command = ("python3 tools/bazel/ci/make_cache.py sandbox-probe --phase before --architecture "
                     + architecture)
    command([*prefix, "sonic-slave-run", "SONIC_RUN_CMDS=" + probe_command],
            log=private / "sandbox-probe-before.log", helpers=helpers, env=environment)


def build(architecture):
    native_architecture(architecture)
    require(os.geteuid() != 0, "Run outer Make as the hosted runner user, not root")
    build_helpers.credentials.authorization()
    helpers = common_helpers(ROOT / PRIVATE / "common-source")
    private = ROOT / PRIVATE
    plan = json.loads((private / "make-plan.json").read_text())
    require(plan["architecture"] == architecture and command(["git", "rev-parse", "HEAD"]).strip()
            == plan["buildimage_revision"], "Prepared Make source or architecture changed")
    require(not (ROOT / PRIVATE / "make-actions.json").exists()
            and not (ROOT / "MODULE.bazel.lock").exists() and not (ROOT / "bazel-out").exists(),
            "Unexpected Bazel build or analysis occurred before the cache readback")
    prefix = plan["make_prefix"]
    environment = make_environment()
    print("Running native Make image recipe in pinned sonic-slave: " + architecture, flush=True)
    probe_command = "python3 tools/bazel/ci/make_cache.py sandbox-probe --phase after --architecture " + architecture
    command([*prefix, "sonic-slave-run", "SONIC_RUN_CMDS=" + probe_command],
            log=private / "sandbox-probe.log", helpers=helpers, env=environment)
    command([*prefix, "configure"], log=private / "configure.log", helpers=helpers, env=environment)
    try:
        command([*prefix, IMAGE], log=private / "make-build.log", helpers=helpers, env=environment)
    finally:
        lock = ROOT / "MODULE.bazel.lock"
        if lock.is_file():
            shutil.copyfile(lock, ROOT / PUBLIC / "read-MODULE.bazel.lock")
    inspect_command = "python3 tools/bazel/ci/make_cache.py inspect --architecture " + architecture
    command([*prefix, "sonic-slave-run", "SONIC_RUN_CMDS=" + inspect_command],
            log=private / "inspect.log", helpers=helpers, env=environment)
    require(json.loads((ROOT / PUBLIC / "result.json").read_text()).get("passed") is True,
            "The sonic-slave inspection did not produce a passing receipt")
    print("Verified Make image recipe reused Common CI cache: " + architecture, flush=True)


def sandbox_probe(architecture, phase):
    """Exercise the installed sandbox as Make's non-root builder before compilation."""
    native_architecture(architecture)
    require(ROOT == Path("/sonic") and Path("/.dockerenv").is_file() and os.geteuid() != 0,
            "Probe must run as Make's non-root user inside sonic-slave")
    helpers = common_helpers(ROOT / PRIVATE / "common-source")
    restriction_before = make_cache_sandbox.restriction_value()
    host = {"installed": False, "restriction_before": restriction_before}
    if phase == "after":
        host = json.loads((ROOT / PUBLIC / "sandbox-host.json").read_text())
        require(host["executable"] == str(INSTALL_BASE / "linux-sandbox"),
                "Host sandbox policy has a different executable attachment")
    command(bazel_invocation(["version"]), log=ROOT / PRIVATE / "bazel-version.log", helpers=helpers)
    sandbox = INSTALL_BASE / "linux-sandbox"
    require(sandbox.is_file() and not sandbox.is_symlink(), "Bazel did not install the expected sandbox")
    before = {kind: os.readlink("/proc/self/ns/" + kind) for kind in ("user", "mnt")}
    script = ("import json,os;from pathlib import Path;"
              "print(json.dumps({'uid':os.getuid(),'profile':Path('/proc/self/attr/current').read_text().strip(),"
              "'sandbox_profile':Path('/proc/'+str(os.getppid())+'/attr/current').read_text().strip(),"
              "'namespaces':{k:os.readlink('/proc/self/ns/'+k) for k in ('user','mnt')}}))")
    result = subprocess.run([str(sandbox), "-T", "15", "--", "/usr/bin/python3", "-c", script],
                            cwd=ROOT, env={}, text=True, capture_output=True, timeout=30)
    (ROOT / PRIVATE / ("linux-sandbox-" + phase + ".log")).write_text(result.stdout + result.stderr)
    restriction = make_cache_sandbox.restriction_value()
    require(restriction == host["restriction_before"], "Global namespace restriction changed before sandbox probe")
    receipt = {"passed": False, "phase": phase, "architecture": architecture, "builder_uid": os.geteuid(),
               "sandbox_path": str(sandbox), "sandbox_sha256": sha256(sandbox),
               "host_profile_installed": host["installed"], "returncode": result.returncode,
               "restriction_before": host["restriction_before"], "restriction_after": restriction,
               "global_policy_changed": False, "parent_namespaces": before}
    path = ROOT / PUBLIC / ("sandbox-probe-" + phase + ".json")
    if result.returncode:
        # The direct probe has an empty environment and a fixed command. Retain
        # its bounded startup error, never Bazel action or build diagnostics.
        receipt["startup_error"] = result.stderr[-2000:]
        write_json(path, receipt)
        if phase == "before":
            return
        helpers.validate.print_safe_diagnostics(result.stderr)
        raise ValueError("Non-root linux-sandbox probe failed after scoped AppArmor setup")
    child = json.loads(result.stdout)
    receipt.update(profile=child["profile"], sandbox_profile=child["sandbox_profile"],
                   sandbox_namespaces=child["namespaces"], sandbox_uid=child["uid"])
    write_json(path, receipt)
    require(all(child["namespaces"][kind] != before[kind] for kind in before),
            "linux-sandbox did not create user and mount namespaces")
    if host["installed"]:
        require(child["sandbox_profile"] == host["profile"] + " (unconfined)",
                "linux-sandbox did not enter the job's exact AppArmor profile")
    if phase == "after":
        baseline = json.loads((ROOT / PUBLIC / "sandbox-probe-before.json").read_text())
        require(baseline["restriction_before"] == host["restriction_before"],
                "Global namespace restriction changed between sandbox probes")
        require(baseline["sandbox_sha256"] == receipt["sandbox_sha256"],
                "The sandbox executable changed between probes")
    receipt["passed"] = True
    write_json(path, receipt)


def compare_image_actions(seed, image):
    """A seed may build extra inputs, but every image action must match exactly."""
    identity = lambda row: (row["mnemonic"], row["target"], tuple(row["outputs"]), row["digest"])
    require(bool(image["common_actions"]), "The image has no Common compile/link evidence")
    for row in image["common_actions"]:
        require(row["runner"] == "remote cache hit" and row["cache_hit"] is True,
                "The Make image rebuilt a Common compile/link action")
    require(not (Counter(map(identity, image["common_actions"])) -
                 Counter(map(identity, seed["common_actions"]))),
            "The image used Common action identities or digests absent from the CI seed")


def image_receipt(path, architecture):
    require(path.is_file() and path.stat().st_size > 0, "Make did not produce its Docker archive")
    with tarfile.open(path) as archive:
        manifest = json.load(archive.extractfile("manifest.json"))
        require(len(manifest) == 1, "Expected one exported Docker image")
        config = json.load(archive.extractfile(manifest[0]["Config"]))
    require(config.get("architecture") == architecture and config.get("os") == "linux",
            "Make archive does not contain the expected native Linux image")
    return {"path": IMAGE, "bytes": path.stat().st_size, "sha256": sha256(path),
            "os": "linux", "architecture": architecture}


def inspect(architecture):
    native_architecture(architecture)
    require(ROOT == Path("/sonic") and Path("/.dockerenv").is_file(),
            "Inspect must run inside the real sonic-slave through outer Make")
    build_helpers.credentials.authorization()
    helpers = common_helpers(ROOT / PRIVATE / "common-source")
    seed, seed_hash = seed_receipt(architecture)
    receipt_path = ROOT / PUBLIC / "result.json"
    receipt = json.loads(receipt_path.read_text())
    require(receipt["seed_receipt_sha256"] == seed_hash, "Seed receipt changed during the image build")
    summary = helpers.summarize_actions(ACTION_LOG)
    receipt.update(common_actions=summary["common_actions"],
                   common_action_counts=dict(Counter(row["mnemonic"] for row in summary["common_actions"])),
                   common_remote_hits=sum(row["runner"] == "remote cache hit" and row["cache_hit"]
                                          for row in summary["common_actions"]))
    receipt["common_local_actions"] = len(summary["common_actions"]) - receipt["common_remote_hits"]
    write_json(receipt_path, receipt)
    helpers.check_actions(seed["phases"][0], remote=False)
    helpers.check_actions(summary, remote=True)
    compare_image_actions(seed["phases"][0], summary)
    image_log_hash = sha256(ACTION_LOG)
    platform_label = "@sonic_build_infra//platforms:" + helpers.MACHINES[architecture][1]
    options = [*container_options(), *build_helpers.remote_cache_options(),
               "--platforms=" + platform_label, "--host_platform=" + platform_label]
    artifacts = {}
    outputs = {}
    for index, target in enumerate(helpers.MAKE_PACKAGES):
        files = command(bazel_invocation(["cquery", *options, "--output=files", target]),
                        log=ROOT / PRIVATE / ("package-query-" + str(index) + ".log"),
                        helpers=helpers).splitlines()
        require(len(files) == 1, "Expected one Common package output")
        path = (ROOT / files[0]).resolve()
        require(path.is_relative_to(OUTPUT_BASE), "Common package escaped the private output base")
        outputs[target] = path
    # Debug package TARs are not always inputs to the runtime image. Materialize
    # those packages separately; never replace the real image's action log.
    materialized = any(not path.is_file() for path in outputs.values())
    package_actions = {"common_actions": [], "action_counts": []}
    if materialized:
        package_log = ROOT / PRIVATE / "package-actions.json"
        argv = bazel_invocation(["build", *options, *helpers.MAKE_PACKAGES])
        argv = ["--execution_log_json_file=" + str(package_log) if value.startswith(
            "--execution_log_json_file=") else value for value in argv]
        command(argv, log=ROOT / PRIVATE / "package-build.log", helpers=helpers)
        package_actions = helpers.summarize_actions(package_log)
        if package_actions["common_actions"]:
            compare_image_actions(seed["phases"][0], package_actions)
    for target, path in outputs.items():
        require(path.is_file(), "Common package did not materialize")
        artifacts[target] = helpers.package_receipt(path, architecture, debug=target != helpers.TARGETS[0])
    require(artifacts == seed["phases"][0]["artifacts"],
            "Common runtime/debug TARs differ from the producer CI seed")
    require(sha256(ACTION_LOG) == image_log_hash, "Package inspection replaced the Make execution log")
    receipt.update(passed=True, image=image_receipt(ROOT / IMAGE, architecture), artifacts=artifacts,
                   common_actions=summary["common_actions"],
                   common_action_counts=dict(Counter(row["mnemonic"] for row in summary["common_actions"])),
                   common_remote_hits=len(summary["common_actions"]), common_local_actions=0,
                   separate_package_materialization=materialized,
                   package_materialization_actions=package_actions)
    write_json(receipt_path, receipt)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("preflight", "prepare", "build", "inspect", "sandbox-probe"))
    parser.add_argument("--architecture", choices=ARCHITECTURES, required=True)
    parser.add_argument("--common-checkout", type=Path, default=ROOT / PRIVATE / "common-checkout")
    parser.add_argument("--phase", choices=("before", "after"), default="after")
    args = parser.parse_args()
    try:
        if args.command == "preflight":
            preflight(args.architecture)
        elif args.command == "prepare":
            prepare(args.architecture, args.common_checkout.resolve())
        elif args.command == "build":
            build(args.architecture)
        elif args.command == "sandbox-probe":
            sandbox_probe(args.architecture, args.phase)
        else:
            inspect(args.architecture)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        path = ROOT / PUBLIC / "result.json"
        receipt = json.loads(path.read_text()) if path.exists() else {"architecture": args.architecture}
        receipt.update(passed=False, failed_phase=args.command, error_type=type(error).__name__)
        if type(error) is ValueError:
            receipt["error"] = str(error)
        write_json(path, receipt)
        helpers = sys.modules.get("cross_repo")
        if helpers is not None:
            helpers.validate.print_safe_diagnostics("Make cache proof failed: " + str(error))
        else:
            # Our explicit validation messages contain no command output or
            # environment values. Unknown exception details stay private.
            detail = str(error) if type(error) is ValueError else type(error).__name__
            print("Make cache proof failed: " + detail, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
