"""Run the installed probes for VS images which do not provide a *_loaded.py entry point."""

import json
import shutil
from pathlib import Path
import subprocess
import tarfile
import tempfile
import uuid

from tools.bazel.ci import artifact_validation, base_images, bmp_loaded, frr_image, swss_layer, teamd_image, verify_dhcp_relay, vs_container_validation
from tools.bazel.ci.artifact_validation import require, sha
from tools.bazel.ci.vs_container_validation import PMON

ROOT = Path(__file__).resolve().parents[3]
OCI = ROOT / "tools/bazel/oci"
BASE = "docker-base-trixie"
CONFIG = "docker-config-engine-trixie"
SWSS = "docker-swss-layer-trixie"
SYNCD = "docker-syncd-vs"
TEAM = "docker-teamd"
FRR = "docker-fpm-frr"
NAT = "docker-nat"
BMP = "docker-sonic-bmp"
DHCP = "docker-dhcp-relay"
SUPERVISED = {"docker-gnmi-sidecar", "docker-restapi-sidecar", "docker-gnmi-watchdog", "docker-bmp-watchdog"}
WATCHDOGS = {"docker-gnmi-watchdog": "gnmi_watchdog", "docker-bmp-watchdog": "bmp_watchdog"}
STANDALONE_IMAGES = {BASE, CONFIG, SWSS, SYNCD, TEAM, FRR, NAT, BMP, DHCP, PMON} | SUPERVISED


def extra_standalone_targets(image, debug_selected):
    targets = {}
    if image in (BASE, CONFIG, SYNCD, "docker-orchagent"):
        package = "//platform/vs/" if image == SYNCD else "//dockers/"
        targets["installed-checks.json"] = package + image + ":installed_checks"
    if image in (BASE, CONFIG):
        targets["base.source.tar"] = "//dockers/docker-base-trixie:source_runtime"
        targets["base.packages.json"] = "//dockers/docker-base-trixie:runtime_package_metadata"
    if image in (CONFIG, SYNCD):
        targets["config-engine.source.tar"] = "//dockers/docker-config-engine-trixie:source_runtime"
    if image == CONFIG:
        targets.update({
            "config-engine.python-and-models.tar": "//dockers/docker-config-engine-trixie:python_and_models",
            "config-engine.startup.tar": "//dockers/docker-config-engine-trixie:config_files",
            "config-engine.packages.json": "//dockers/docker-config-engine-trixie:runtime_package_metadata",
        })
    if image == DHCP:
        package = "//dockers/docker-dhcp-relay:"
        targets.update({"dhcp-relay-source.tar": package + "source_runtime",
                        "runtime-packages.json": package + "runtime_package_metadata",
                        "runtime.status": package + "installed_packages"})
    if image == SWSS:
        targets.update({name: swss_layer.TARGETS[name] for name in
                        ("source-runtime.tar", "packages.json", "installed.status", "apt-selection.json")})
    if image == PMON:
        package = "//dockers/docker-platform-monitor:"
        targets.update({"source-runtime.tar": "@lm_sensors//:lm-sensors.runtime",
                        "libsensors.source.tar": "@lm_sensors//:libsensors5.runtime",
                        "sensord.source.tar": "@lm_sensors//:sensord.runtime",
                        "pmon.packages.json": package + "runtime_package_metadata",
                        "pmon.status": package + "installed_packages"})
        if debug_selected:
            targets.update({"pmon-debug.packages.json": package + "debug_package_metadata",
                            "pmon-debug.status": package + "debug_installed_packages"})
    if image == SYNCD:
        package = "//platform/vs/docker-syncd-vs:"
        targets.update({"syncd.source.tar": package + "source_runtime_payloads",
                        "syncd.packages.json": package + "runtime_package_metadata",
                        "syncd.status": package + "installed_packages"})
        if debug_selected:
            targets.update({"syncd-debug.packages.json": package + "debug_package_metadata",
                            "syncd-debug.status": package + "debug_installed_packages"})
    if image in SUPERVISED:
        package = "//dockers/" + image + ":"
        targets.update({"service.files.tar": package + "service_files",
                        "base.source.tar": "//dockers/docker-base-trixie:source_runtime",
                        "config-engine.source.tar": "//dockers/docker-config-engine-trixie:source_runtime"})
        if image in WATCHDOGS:
            targets["watchdog.source.tar"] = "//dockers/" + image + "/watchdog:runtime"
        if debug_selected:
            targets["all-debug.tar"] = package + image + ".debug_symbols"
            if image in WATCHDOGS:
                targets["watchdog-symbol-tools.tar.gz"] = "//tools/bazel/ci:watchdog_symbol_tools"
    return targets


def bind(path, target):
    return ["--mount", f"type=bind,src={path.resolve()},dst={target},readonly"]


def run(tag, entrypoint, arguments, *, options=(), stdin=None):
    name = "sonic-vs-check-" + uuid.uuid4().hex
    command = [
        "docker", "run", "--rm", "--name", name, "--network=none", "--read-only", "--cap-drop=ALL",
        "--security-opt=no-new-privileges", "--pids-limit=128", "--memory=4g", "--cpus=2",
        "--tmpfs", "/tmp:rw,nosuid,nodev,noexec,size=256m", *options,
        *(["--interactive"] if stdin is not None else []), "--entrypoint", entrypoint, tag, *arguments,
    ]
    try:
        result = subprocess.run(command, input=stdin, stdout=subprocess.PIPE, text=True, check=True, timeout=1200)
    except subprocess.TimeoutExpired:
        subprocess.run(["docker", "rm", "--force", name], capture_output=True, check=False, timeout=60)
        raise
    print(result.stdout, flush=True)
    return result.stdout


def verify_installed_declaration(directory, bundle, variant, tag):
    """Inspect the actual files and directory symlinks selected by the image owner."""
    declaration = json.loads((directory / "installed-checks.json").read_bytes())
    require(declaration["architecture"] == bundle["architecture"], "Installed policy has wrong architecture")
    name = bundle["archives"][variant]["file"].removesuffix(".gz")
    spec, = (spec for spec in declaration["images"] if spec["name"] == name)
    aliases = spec.get("directory_aliases", {})
    paths = spec.get("files", {})
    query = json.dumps({"files": list(paths), "directory_aliases": list(aliases)})
    actual = json.loads(run(tag, "/usr/bin/python3", ["-B", "/probe/check.py", "--inspect-files", query],
                            options=bind(OCI / "runtime_smoke.py", "/probe/check.py")))
    require(actual["directory_aliases"] == aliases, "Installed directory aliases differ for " + name)
    inventory = vs_container_validation.read_oci(directory, variant)
    for path, constraints in paths.items():
        installed = actual["files"][path]
        expected = inventory["files"][path]
        require(all(installed.get(key) == value for key, value in constraints.items()) and
                all(installed.get(key) == expected[key] for key in ("kind", "sha256", "mode", "uid", "gid")),
                "Installed file differs from the declared OCI payload: " + path)
    if variant == "debug":
        for pair in declaration.get("symbol_pairs", ()):
            if pair["debug"] == name:
                owners = json.loads((directory / "runtime.owners.json").read_bytes())["native"]
                for path in pair["paths"]:
                    build_id = owners[path]["build_id"]
                    symbols = f"usr/lib/debug/.build-id/{build_id[:2]}/{build_id[2:]}.debug"
                    require(inventory["files"].get(symbols, {}).get("kind") == "file",
                            "Missing installed symbols for " + path)
                    probe = {"architecture": bundle["architecture"], "elfs": [], "policy": {},
                             "debug_pairs": [{"path": path, "debug_path": symbols}]}
                    run(tag, "/usr/bin/python3", ["-B", "/probe/check.py", "--variant", "debug"],
                        options=bind(OCI / "runtime_smoke.py", "/probe/check.py"), stdin=json.dumps(probe))
    if bundle["image"] != BASE:
        verify_parent(directory, bundle, variant, inventory, spec.get("inherit_files", []))
    return actual


def verify_parent(directory, bundle, variant, inventory, inherited):
    if variant == "debug":
        runtime = vs_container_validation.read_oci(directory, "runtime")
        expected_layers, expected_files = runtime["config"]["rootfs"]["diff_ids"], runtime["files"]
    else:
        parent = json.loads((directory / "parent.json").read_bytes())
        expected_image = BASE if bundle["image"] == CONFIG else CONFIG
        require(parent["image"] == expected_image, "Wrong VS parent for " + bundle["image"])
        expected_layers, expected_files = parent["diff_ids"], parent["files"]
    child_layers = inventory["config"]["rootfs"]["diff_ids"]
    require(expected_layers and child_layers[:len(expected_layers)] == expected_layers,
            "OCI image did not inherit the VS parent's layers")
    for path in inherited:
        require(inventory["files"][path] == expected_files[path], "Child changed a parent file: " + path)


def verify_full_parent(directory, bundle, variant, tag):
    """Compare the assembled service with its published parent and declared owner replacements."""
    image = bundle["image"]
    parent_image = CONFIG if image == SWSS or image in SUPERVISED else SWSS
    parent_record = json.loads((directory / "parent.json").read_bytes())
    require(parent_record["image"] == parent_image, "Wrong VS parent for " + image)
    parent = vs_container_validation.read_oci(directory, "parent")
    current = vs_container_validation.read_oci(directory, variant)
    layers = parent["config"]["rootfs"]["diff_ids"]
    require(layers and current["config"]["rootfs"]["diff_ids"][:len(layers)] == layers,
            image + " did not inherit the selected parent's layers")
    files, inherited = current["files"], parent["files"]
    base_images.verify_directory_aliases(files)
    debug = variant == "debug"
    owners = json.loads((directory / (variant + ".owners.json")).read_bytes())["owners"]
    if image in (TEAM, FRR):
        packages = json.loads((directory / "config-engine.packages.json").read_bytes())
        artifact_validation.verify_swss_component_labels(current["config"]["config"]["Labels"],
                                                         packages["dependency_check"]["packages"])
    if image in SUPERVISED:
        artifact_validation.verify_inherited_files(inherited, files, owners["service.files.tar"], set(), image)
    elif image == SWSS:
        artifact_validation.verify_inherited_files(inherited, files, {}, {"var/lib/dpkg/status"}, "SWSS layer")
    elif image == TEAM:
        teamd_image.verify_parent_files(inherited, files, owners["teamd.files.tar"],
                                        owners["teamd.debug-tools.tar"] if debug else {}, debug=debug)
        groups = run(tag, "/bin/cat", ["/etc/group"]).splitlines()
        teamd_image.verify_inherited_accounts(inherited, files, (directory / "parent.group").read_text().splitlines(),
                                              groups, add_ssh=debug)
    else:
        frr_image.verify_parent_files(inherited, files, owners["frr.files.tar"], owners["frr.python.tar"],
                                      owners["frr.debug-tools.tar"] if debug else {})


def verify_watchdog_symbols(directory, image, tag, work):
    """Inspect exact installed bytes with the validation runner's GDB, never add image packages."""
    from tools.bazel.oci.runtime_smoke import gdb_lookup

    tools = work / "inspection-tools"
    tools.mkdir()
    with tarfile.open(directory / "watchdog-symbol-tools.tar.gz") as archive:
        archive.extractall(tools, filter="data")
    executables = {name: tools / "tools/bazel/ci" / ("watchdog_" + name)
                   for name in ("gdb", "readelf", "objcopy")}
    require(all(path.is_file() and not path.is_symlink() for path in executables.values()),
            "Missing declared watchdog inspection executable")
    gdb, readelf, objcopy = (str(executables[name]) for name in ("gdb", "readelf", "objcopy"))
    binary_name = WATCHDOGS[image]
    path = "usr/bin/" + binary_name
    runtime = vs_container_validation.read_oci(directory, "runtime")["files"]
    debug = vs_container_validation.read_oci(directory, "debug")["files"]
    owner = json.loads((directory / "runtime.owners.json").read_bytes())["native"][path]
    build_id = owner["build_id"]
    require(build_id and owner["has_debuglink"], "Watchdog owner lacks split symbols: " + image)
    debug_path = f"usr/lib/debug/.build-id/{build_id[:2]}/{build_id[2:]}.debug"
    root = work / "symbols"
    binary, symbols = root / path, root / debug_path
    binary.parent.mkdir(parents=True)
    symbols.parent.mkdir(parents=True)
    container = subprocess.check_output([
        "docker", "create", "--name", "sonic-symbol-check-" + uuid.uuid4().hex,
        "--network=none", "--read-only", "--cap-drop=ALL", "--security-opt=no-new-privileges",
        "--entrypoint=/bin/true", tag], text=True, timeout=60).strip()
    try:
        for source, target in ((path, binary), (debug_path, symbols)):
            subprocess.run(["docker", "cp", container + ":/" + source, str(target)], check=True, timeout=60)
    finally:
        subprocess.run(["docker", "rm", "--force", container], check=True, capture_output=True, timeout=60)
    require(sha(binary) == runtime[path]["sha256"] == debug[path]["sha256"],
            "Extracted watchdog differs from its selected runtime: " + path)
    require(sha(symbols) == debug[debug_path]["sha256"], "Extracted watchdog symbols differ: " + debug_path)
    original, detached = artifact_validation.elf_info(binary, readelf=readelf), artifact_validation.elf_info(symbols, readelf=readelf)
    require(original["build_id"] == detached["build_id"] == build_id and detached["has_dwarf"],
            "Watchdog symbols lack matching build ID or DWARF")
    artifact_validation.verify_debuglink(binary, symbols, path, expected_name=build_id[2:] + ".debug", objcopy=objcopy)
    isolated, empty = work / binary_name, work / "empty-symbols"
    shutil.copyfile(binary, isolated)
    empty.mkdir()
    symbol = binary_name + "::main"
    without = gdb_lookup(isolated, work, str(empty), symbol, gdb=gdb)
    require(not without.get("found"), "Watchdog source lookup succeeded without packaged symbols")
    lookup = gdb_lookup(binary, work, str(root / "usr/lib/debug"), symbol, gdb=gdb)
    require(lookup.get("found") and lookup.get("source") == "dockers/" + image + "/watchdog/src/main.rs",
            "Watchdog symbols do not provide source, line and type information for the service")
    return {"path": path, "debug_path": debug_path, "build_id": build_id,
            "without_symbols": without, "with_symbols": lookup,
            "tool_archive_sha256": sha(directory / "watchdog-symbol-tools.tar.gz"),
            "gdb_version": artifact_validation.command([gdb, "--version"]).splitlines()[0]}


def validate_base(directory, variant, name, summary):
    inventory = vs_container_validation.read_oci(directory, variant)["files"]
    base_images.verify_directory_aliases(inventory)
    base_images.verify_editor_commands(inventory)
    reports = [json.loads((directory / "base.packages.json").read_bytes())]
    if name == CONFIG:
        reports.append(json.loads((directory / "config-engine.packages.json").read_bytes()))
        summary["installed_owner_layers"] = {
            fixture: {"files": base_images.verify_installed_layer(inventory, directory / fixture)}
            for fixture in ("config-engine.python-and-models.tar", "config-engine.startup.tar")
        }
    summary["native"] = base_images.verify_source_files(inventory, reports, 62)


def standalone(image, directory, bundle, report):
    with tempfile.TemporaryDirectory(prefix="vs-installed-probe-") as temporary:
        work = Path(temporary)
        if image in (FRR, TEAM):
            redis = work / "redis"
            redis.mkdir()
            with tarfile.open(directory / "test-redis.tar") as archive:
                archive.extractall(redis, filter="data")
        if image == TEAM:
            for module in ("team", "team_mode_loadbalance"):
                if not (Path("/sys/module") / module).is_dir():
                    subprocess.run(["sudo", "modprobe", module], check=True)
        for variant, spec in bundle["archives"].items():
            name = spec["file"].removesuffix(".gz")
            summary = {**report["images"][name], "architecture": bundle["architecture"],
                       "runtime_native": report["runtime_native"]}
            with vs_container_validation.loaded_image(directory, spec, bundle) as tag:
                if image in (SWSS, FRR, TEAM) or image in SUPERVISED:
                    verify_full_parent(directory, bundle, variant, tag)
                if image in (BASE, CONFIG, SYNCD):
                    verify_installed_declaration(directory, bundle, variant, tag)
                if image == SYNCD:
                    require("usr/bin/syncd" in summary["native"], "Syncd executable missing from the owner artifact")
                    probe = {"architecture": bundle["architecture"], "elfs": summary["elfs"], "debug_pairs": [],
                             "policy": {"startup_profile": {"script": "/usr/bin/start.sh",
                                        "input": "/usr/share/sonic/hwsku/sai.profile",
                                        "output": "/etc/sai.d/sai.profile", "contents": "SAI_VS_SWITCH_TYPE=SAI_VS_SWITCH_TYPE_BCM56850\n"},
                                        "supervisor_config": "/etc/supervisor/conf.d/supervisord.conf"}}
                    run(tag, "/usr/bin/python3", ["-B", "/probe/check.py", "--variant", variant],
                        options=[*bind(OCI / "runtime_smoke.py", "/probe/check.py"),
                                 "--tmpfs", "/etc/sai.d", "--tmpfs", "/usr/share/sonic/hwsku"],
                        stdin=json.dumps(probe))
                elif image in SUPERVISED:
                    output = run(tag, "/usr/bin/python3", ["-B", "/probe/check.py", image],
                                 options=bind(ROOT / "tools/bazel/ci/supervised_runtime_probe.py", "/probe/check.py"))
                    result = json.loads(output)
                    require(result["status"] == "passed", name + " installed service probe did not pass")
                    if variant == "debug" and image in WATCHDOGS:
                        result["debug"] = verify_watchdog_symbols(directory, image, tag, work)
                    (directory / (name + ".runtime.json")).write_text(json.dumps(result) + "\n")
                elif image == NAT:
                    script = (ROOT / "dockers/docker-nat/verify-installed.sh").read_text()
                    run(tag, "/bin/sh", ["-s", "--", name], stdin=script)
                elif image == DHCP:
                    run(tag, "/bin/sh", ["-ec", verify_dhcp_relay.PROBE],
                        options=["--env", "PYTHONDONTWRITEBYTECODE=1"])
                elif image == BMP:
                    run(tag, "/bin/sh", ["-ec", bmp_loaded.PROBE_SCRIPT],
                        options=["--env", "PYTHONDONTWRITEBYTECODE=1"])
                elif image == PMON:
                    inventory = vs_container_validation.read_oci(directory, variant)
                    verify_parent(directory, bundle, variant, inventory, [])
                    require("usr/bin/sensors" in summary["native"], "Missing lm-sensors owner executable")
                    require(type(bundle.get("pddf_enabled")) is bool, "Missing Make-selected PMON PDDF mode")
                    pddf = "y" if bundle["pddf_enabled"] else "n"
                    run(tag, "/bin/sh", ["-s", "--", variant, pddf], stdin=Path(__file__).with_name("pmon_installed.sh").read_text(),
                        options=["--env", "PYTHONDONTWRITEBYTECODE=1"])
                elif image in (BASE, CONFIG):
                    validate_base(directory, variant, image, summary)
                elif image == SWSS:
                    require(summary["native"], "Missing SWSS source ELF inventory")
                    swss_layer.verify_libteam_metadata(json.loads((directory / "packages.json").read_bytes()),
                                                       summary["native"])
                    inventory = vs_container_validation.read_oci(directory, variant)
                    verify_parent(directory, bundle, variant, inventory, [
                        "run/redis/sonic-db/database_config.json", "usr/lib/x86_64-linux-gnu/libswsscommon.so.0.0.0",
                    ])
                    files = inventory["files"]
                    for path, fixture in (("var/lib/dpkg/status", "installed.status"),
                                          ("usr/share/sonic/bazel/swss-layer-packages.json", "packages.json")):
                        require(files[path]["sha256"] == sha(directory / fixture), "Wrong installed " + path)
                    selected = json.loads((directory / "apt-selection.json").read_bytes())["selected"]
                    require(not {"libteam5", "libteamdctl0"}.intersection(row["package"] for row in selected),
                            "APT replaced source-built libteam")
                if image in (CONFIG, SWSS, FRR, TEAM):
                    probe_input = work / (variant + ".json")
                    probe_input.write_text(json.dumps(summary))
                    probe_input.chmod(0o644)
                    options = bind(probe_input, "/probe/validation.json")
                    arguments = ["/probe/validation.json"]
                    if image in (FRR, TEAM):
                        script = ROOT / "dockers" / image / "bazel/runtime_probe.py"
                        options += bind(redis, "/probe/redis")
                        arguments = ["--validation", "/probe/validation.json"]
                        if variant == "debug":
                            arguments.append("--debug")
                        if image == FRR:
                            options += ["--user=300:300", "--group-add=102"]
                        else:
                            options += ["--cap-add=NET_ADMIN", "--cap-add=NET_RAW", "--mount", "type=volume,dst=/run/redis",
                                        "--tmpfs", "/run/teamd:rw,nosuid,nodev,noexec,size=16m",
                                        "--tmpfs", "/var/warmboot:rw,nosuid,nodev,noexec,size=16m"]
                    else:
                        script = OCI / ("base_runtime_smoke.py" if image == CONFIG else "swss_layer_runtime.py")
                    options += bind(script, "/probe/check.py") + bind(OCI / "runtime_smoke.py", "/probe/runtime_smoke.py")
                    output = run(tag, "/usr/bin/python3", ["-B", "/probe/check.py", *arguments], options=options)
                    require(json.loads(output)["status"] == "passed", name + " installed probe did not pass")
