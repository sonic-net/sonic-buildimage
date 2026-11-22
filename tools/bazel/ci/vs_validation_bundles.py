"""Publish installer exports, configured OCI inputs and fixtures for CI consumers."""

import argparse
import errno
from functools import lru_cache
import gzip
import hashlib
import io
import json
import re
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.build import execution_root_from_bep
from tools.bazel.ci.artifact_validation import require, sha
from tools.bazel.ci.vs_container_validation import PMON, static_i386_files
from tools.bazel.oci import oci_inventory
from tools.bazel.oci.boot_layout import direct_layout_for_archive
from tools.bazel.oci.package_state import file_bytes

BASE = "docker-base-trixie"
CONFIG = "docker-config-engine-trixie"
SWSS = "docker-swss-layer-trixie"
P4RT = "docker-sonic-p4rt"
# Each shared image needs its parent's inventory before its own probe can be staged.
PARENTS = (BASE, CONFIG, SWSS)
MAKE_FEATURES = {
    "docker-sonic-gnmi": ("dialout_enabled", "@sonic_gnmi//tools/bazel:enable_dialout"),
    PMON: ("pddf_enabled", "//dockers/docker-platform-monitor:pddf"),
}
DASH_LOCK = "@sonic_build_infra//apt:apt.lock.json"
APT_REPORT_FAMILIES = {"docker-orchagent", "docker-sonic-gnmi", "docker-sonic-otel", "docker-dash-engine", P4RT}

# The baseline is the owner's declared runtime OCI parent. A debug companion
# also has the selected runtime inventory in its own bundle.
PARENT_IMAGES = {
    CONFIG: BASE, "docker-dash-engine": BASE,
    SWSS: CONFIG, "docker-database": CONFIG, "docker-eventd": CONFIG, "docker-gbsyncd-vs": CONFIG,
    "docker-lldp": CONFIG, "docker-mux": CONFIG, "docker-orchagent": CONFIG, PMON: CONFIG,
    "docker-router-advertiser": CONFIG, "docker-snmp": CONFIG, "docker-sonic-gnmi": CONFIG,
    "docker-sonic-mgmt-framework": CONFIG, "docker-sonic-otel": CONFIG, P4RT: CONFIG,
    "docker-syncd-vs": CONFIG, "docker-sysmgr": CONFIG,
    "docker-gnmi-sidecar": CONFIG, "docker-restapi-sidecar": CONFIG,
    "docker-gnmi-watchdog": CONFIG, "docker-bmp-watchdog": CONFIG,
    "docker-dash-ha": SWSS, "docker-fpm-frr": SWSS, "docker-macsec": SWSS,
    "docker-sflow": SWSS, "docker-teamd": SWSS,
}
FULL_PARENT_INVENTORY = PARENT_IMAGES.keys() - {CONFIG, PMON, "docker-syncd-vs"}

# These owners validate the contents of allowed account merges. The remaining
# files, including gshadow, are compared using the complete filesystem hashes.
GROUP_CHECKS = {
    "docker-dash-engine", "docker-dash-ha", "docker-gbsyncd-vs", "docker-macsec", "docker-orchagent", "docker-sflow",
    "docker-sonic-gnmi", "docker-sonic-mgmt-framework", "docker-sonic-otel", P4RT, "docker-sysmgr",
}
ACCOUNT_PATHS = ("etc/group", "etc/passwd")
COMMON_FILES = (
    "run/redis/sonic-db/database_config.json",
    "usr/lib/x86_64-linux-gnu/libswsscommon.so.0.0.0",
    *ACCOUNT_PATHS,
)


def family(archive):
    return archive.removesuffix(".gz").removesuffix("-dbg")


def validation_targets(archives, registered_debug, debug_enabled, validation_only_debug=()):
    from tools.bazel.ci import vs_probe_policy

    members = dict(archives)
    for name in validation_only_debug:
        runtime = family(name) + ".gz"
        if debug_enabled and runtime in archives:
            require(name.endswith("-dbg.gz"), "Expected a validation-only debug companion: " + name)
            members[name] = archives[runtime].removesuffix(".gz") + "-dbg.gz"
    for parent in PARENTS:
        name = parent + ".gz"
        members.setdefault(name, f"//dockers/{parent}:{name}")
    families = {}
    for name, archive_label in members.items():
        image = family(name)
        variant = "debug" if name.endswith("-dbg.gz") else "runtime"
        require(archive_label.endswith(".gz"), f"{name}: expected an explicit OCI archive target")
        entry = families.setdefault(image, {"members": {}})
        entry["members"][variant] = (name, archive_label, archive_label.removesuffix(".gz"))

    for image, entry in families.items():
        entry["supports_debug"] = image + "-dbg.gz" in registered_debug or image + "-dbg.gz" in validation_only_debug
        with_debug = debug_enabled and entry["supports_debug"]
        require(set(entry["members"]) == ({"runtime", "debug"} if with_debug else {"runtime"}),
                f"{image}: VS batch differs from debug selection")
        entry["sources"] = vs_probe_policy.preparation_targets(image, with_debug)
        package = entry["members"]["runtime"][1].rsplit(":", 1)[0]
        if image in APT_REPORT_FAMILIES:
            for stage in entry["members"]:
                entry["sources"][stage + "-apt.json"] = f"{package}:{stage}_apt_selection"
        if image == "docker-orchagent":
            entry["sources"]["libteam-source-receipt.json"] = package + ":libteam_source_receipt"
        if image == "docker-dash-engine":
            entry["apt_lock"] = DASH_LOCK
    return families


def extra_targets(families):
    # The archives, layouts and test fixtures share one Bazel configuration and
    # one build. Some fixtures are external targets with apparent repo names.
    labels = set()
    for entry in families.values():
        for _, archive, layout in entry["members"].values():
            labels.update((archive, layout))
        labels.update(entry["sources"].values())
        if "apt_lock" in entry:
            labels.add(entry["apt_lock"])
    return sorted(labels)


def configured_outputs(bazel, options, labels, events):
    # Build private owner fixtures as top-level targets. A filegroup wrapper
    # in this repository would create a cross-package visibility violation.
    # cquery reports canonical repository names and preserves TreeArtifact roots.
    external = {label for label in labels if label.startswith("@") and not label.startswith("@@")}
    canonical = {label: label for label in labels}
    if external:
        repositories = json.loads(subprocess.check_output([bazel, "mod", "dump_repo_mapping", ""], text=True))
        for label in external:
            repository, target = label[1:].split("//", 1)
            canonical[label] = "@@" + repositories[repository] + "//" + target
    # Source files use Bazel's null configuration. Keep them alongside the
    # selected top-level rule configuration, excluding test/exec transitions.
    query = ('let roots = set(' + ' '.join(sorted(set(labels))) +
             ') in config($roots, target) + kind("source file", $roots)')
    expression = 'str(target.label) + "\\t" + "\\t".join([f.path for f in target.files.to_list()])'
    result = subprocess.check_output(
        [bazel, "cquery", *options, query, "--output=starlark", "--starlark:expr=" + expression],
        text=True,
    )
    execution_root = execution_root_from_bep(events)
    def local_path(output):
        require(output and not Path(output).is_absolute() and ".." not in Path(output).parts,
                "unexpected Bazel output path: " + output)
        return (execution_root / output).resolve(strict=True)

    outputs = {}
    for line in result.splitlines():
        fields = line.split("\t")
        label = fields[0]
        require(len(fields) == 2, f"{label}: expected one configured output file or OCI directory")
        output = fields[1]
        if label.lstrip("@").startswith("//"):
            label = label.lstrip("@")
        path = local_path(output)
        require(label not in outputs or outputs[label] == path, "conflicting configured outputs: " + label)
        outputs[label] = path
    for requested, label in canonical.items():
        # cquery exposes the actual rule for an alias, which can even live in
        # a different external repository. Ask for that alias alone to bind
        # its unique output without guessing its target name or configuration.
        if label not in outputs:
            paths = subprocess.check_output(
                [bazel, "cquery", *options, f"config({requested}, target)", "--output=files"], text=True,
            ).splitlines()
            require(len(paths) == 1, requested + ": expected one configured alias output")
            outputs[label] = local_path(paths[0])
    return {key: outputs[label] for key, label in canonical.items()}


def copy_output(source, output):
    if output.exists() and source.samefile(output):
        return
    try:
        os.link(source.resolve(strict=True), output)
    except OSError as exc:
        if exc.errno not in (errno.EXDEV, errno.EACCES, errno.EPERM, errno.EMLINK):
            raise
        shutil.copyfile(source, output)


def stage_archive(selected, output, producer, handoff):
    """Keep the installer archive's identity intact when publishing validation artifacts."""
    if handoff and direct_layout_for_archive(selected, handoff):
        # Linking target/<archive> here changes ctime and invalidates the current
        # boot handoff. Its producer output was copied or compared byte for byte.
        if selected.samefile(producer):
            shutil.copyfile(selected, output)
        else:
            copy_output(producer, output)
        digest = sha(selected)
        if direct_layout_for_archive(selected, handoff):
            return digest
        # A concurrent publisher replaced the selected file while staging it.
        output.unlink()
    copy_output(selected, output)
    return sha(output)


def layout_metadata(directory):
    def blob(descriptor):
        digest = descriptor["digest"]
        require(re.fullmatch(r"sha256:[0-9a-f]{64}", digest), "invalid OCI blob path")
        return directory / "blobs" / "sha256" / digest[7:]

    def metadata(descriptor):
        data = blob(descriptor).read_bytes()
        require(len(data) == descriptor["size"] and "sha256:" + hashlib.sha256(data).hexdigest() == descriptor["digest"],
                "OCI metadata differs from its descriptor: " + descriptor["digest"])
        return json.loads(data)

    index = json.loads((directory / "index.json").read_bytes())
    manifest_descriptor, = index["manifests"]
    manifest = metadata(manifest_descriptor)
    return manifest_descriptor, manifest, metadata(manifest["config"]), [blob(item) for item in manifest["layers"]]


def write_inventory(directory, variant, bundle, prefixes, *, account_layers, retain=False, static_i386=None):
    descriptor, manifest, config, layers = layout_metadata(directory)
    digests = tuple(layer["digest"] for layer in manifest["layers"])
    # The installer emits many siblings with identical immutable parent layers.
    # Start with the longest already-indexed parent and apply only the suffix.
    prefix = max((key for key in prefixes if digests[:len(key)] == key), key=len, default=())
    files = prefixes.get(prefix, {}).copy()
    for layer in layers[len(prefix):]:
        entries = oci_inventory.apply_layer(layer, files, static_i386=static_i386)
        for name in ACCOUNT_PATHS:
            item = entries.get(name, {})
            if item.get("kind") == "file":
                account_layers[name, item["sha256"]] = layer
    if retain:
        prefixes[digests] = files
    report = {"config": config, "config_digest": manifest["config"]["digest"], "manifest_digest": descriptor["digest"],
              "layers": manifest["layers"], "files": files}
    path = bundle / (variant + ".oci.json.gz")
    with path.open("wb") as raw, gzip.GzipFile(
            fileobj=raw, filename="", mode="wb", compresslevel=3, mtime=0) as stream:
        stream.write(json.dumps(report, sort_keys=True, separators=(",", ":")).encode())
    return path, manifest, config, {name: files[name] for name in COMMON_FILES if name in files}


@lru_cache(maxsize=4)
def redis_config(source_tar):
    """The private Redis probe uses just this file from the normalized parent TAR."""
    name = "run/redis/sonic-db/database_config.json"
    with tarfile.open(source_tar) as source:
        member, = [item for item in source if Path(item.name).as_posix() == name]
        require(member.isfile(), f"{source_tar}: expected regular {name}")
        return source.extractfile(member).read()


def stage_probe_fixtures(image, artifacts, bundle):
    from tools.bazel.ci.vs_standalone_probes import STANDALONE_IMAGES, SUPERVISED

    # Standalone checks use the owner records projected in *.owners.json.
    # P4RT reads the Redis configuration from its config-engine parent TAR.
    if image in STANDALONE_IMAGES:
        for name in ("base.source.tar", "config-engine.source.tar", "syncd.source.tar", "source-runtime.tar"):
            artifacts.pop(name, None)
    if image in SUPERVISED:
        for name in ("service.files.tar", "watchdog.source.tar", "all-debug.tar"):
            artifacts.pop(name, None)
    elif image not in STANDALONE_IMAGES and image != "docker-sonic-p4rt":
        for name in ("config-engine-source.tar", "config-engine.source.tar"):
            source = artifacts.pop(name, None)
            if source is None:
                continue
            fixture = bundle / "redis-config-fixture.tar"
            if not fixture.exists():
                data = redis_config(source)
                item = tarfile.TarInfo("run/redis/sonic-db/database_config.json")
                item.size = len(data)
                with tarfile.open(fixture, "w") as target:
                    target.addfile(item, io.BytesIO(data))
            artifacts[fixture.name] = fixture
    return artifacts


@lru_cache(maxsize=128)
def account_bytes(layer, path):
    return file_bytes(layer, {path})[path]


def stage_accounts(bundle, image, variant, files, layers):
    if image not in GROUP_CHECKS and not (image == "docker-teamd" and variant == "parent"):
        return {}
    fixtures = {}
    paths = ACCOUNT_PATHS if image == "docker-sonic-otel" else ("etc/group",)
    for path in paths:
        # Read the effective member from the exact layer that supplied its
        # inventory entry. Shared parent account tables are tiny and cacheable.
        digest = files[path]["sha256"]
        output = bundle / (variant + "." + Path(path).name)
        output.write_bytes(account_bytes(layers[path, digest], path))
        fixtures[output.name] = digest
    return fixtures


def stage_parent(bundle, image, references, account_layers):
    parent = PARENT_IMAGES.get(image)
    if parent is None:
        return {}
    require(parent in references, f"{image}: missing configured VS parent {parent}")
    summary = bundle / "parent.json"
    summary.write_text(json.dumps(references[parent], sort_keys=True) + "\n")
    fixtures = {summary.name: sha(summary)}
    if image in FULL_PARENT_INVENTORY:
        # The parent bundle has already indexed the exact configured OCI
        # layers. Reuse its compact inventory for the child's overlay checks.
        source = bundle.parent / parent
        parent_manifest = json.loads((source / "bundle.json").read_bytes())
        filename = "runtime.oci.json.gz"
        copy_output(source / filename, bundle / "parent.oci.json.gz")
        fixtures["parent.oci.json.gz"] = parent_manifest["fixtures"][filename]
    fixtures.update(stage_accounts(bundle, image, "parent", references[parent]["files"], account_layers))
    return fixtures


def make_features(image, build_settings):
    """Record what Make requested so probes can compare it with the installed image."""
    if image not in MAKE_FEATURES:
        return {}
    name, flag = MAKE_FEATURES[image]
    value = build_settings.get(flag)
    require(value in ("true", "false"), f"missing Make-selected {flag} setting for {image}")
    return {name: value == "true"}


def write_bundles(args, families, paths, build_settings):
    from tools.bazel.ci import vs_probe_policy

    version = build_settings["//tools/bazel/oci:image_version"]
    destination = args.validation_directory
    destination.parent.mkdir(parents=True, exist_ok=True)
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    with TemporaryDirectory(prefix=".vs-bundles-", dir=destination.parent) as work:
        staged = Path(work) / "containers"
        staged.mkdir()
        prefixes, references, account_layers = {}, {}, {}
        for image in [name for name in PARENTS if name in families] + sorted(families.keys() - set(PARENTS)):
            entry = families[image]
            bundle = staged / image
            bundle.mkdir()
            source_paths = {name: paths[label] for name, label in entry["sources"].items()}
            archive_specs = {}
            settings = make_features(image, build_settings)
            if "apt_lock" in entry:
                settings["apt_lock_sha256"] = sha(paths[entry["apt_lock"]])
                settings["apt_layers"] = {}
            fixtures = stage_parent(bundle, image, references, account_layers)
            static_i386 = static_i386_files(image)
            for variant in sorted(entry["members"], key=lambda name: name == "debug"):
                archive, label, layout_label = entry["members"][variant]
                layout_path = paths[layout_label]
                parent = image in PARENTS and variant == "runtime"
                inventory, manifest, config, common_files = write_inventory(
                    layout_path, variant, bundle, prefixes, account_layers=account_layers,
                    retain=parent, static_i386=static_i386)
                fixtures.update(stage_accounts(bundle, image, variant, common_files, account_layers))
                if "apt_lock" in entry:
                    payload = source_paths["engine.runtime-apt.tar" if variant == "runtime" else "engine.debug-tools.tar"]
                    settings["apt_layers"][variant] = {"digest": "sha256:" + sha(payload), "size": payload.stat().st_size}
                if parent:
                    references[image] = {"image": image, "diff_ids": config["rootfs"]["diff_ids"],
                                         "files": common_files}
                # Use the actual target/ file when Make selected this member.
                # Auxiliary parents come from the same completed batch build.
                source = args.target_path / archive if archive in args.archive else paths[label]
                digest = stage_archive(source, bundle / archive, paths[label],
                                       os.environ.get("SONIC_VS_BOOT_HANDOFF") if archive in args.archive else None)
                archive_specs[variant] = {
                    "file": archive, "sha256": digest,
                    "config_digest": manifest["config"]["digest"],
                    "layers": manifest["layers"], "rootfs": config["rootfs"],
                }
                artifacts = vs_probe_policy.prepare(image, version, variant, layout_path, source_paths, bundle,
                                                   make_manifests=args.target_path / "bazel-manifests")
                fixtures_for_variant = stage_probe_fixtures(image, artifacts, bundle)
                for name, source in {**fixtures_for_variant, inventory.name: inventory}.items():
                    require(Path(name).name == name and name != "bundle.json" and name not in args.archive,
                            "invalid validation fixture name: " + name)
                    digest = sha(source)
                    if name in fixtures:
                        require(fixtures[name] == digest, f"{image}: inconsistent fixture {name}")
                    else:
                        copy_output(source, bundle / name)
                        fixtures[name] = digest
            (bundle / "bundle.json").write_text(json.dumps({
                "image": image, "revision": revision, "build_version": version, "architecture": args.architecture,
                "debug_enabled": args.validation_debug == "y", "supports_debug": entry["supports_debug"],
                "archives": archive_specs, "fixtures": fixtures, **settings,
            }, sort_keys=True) + "\n")
        (staged / "selection.json").write_text(json.dumps({
            "revision": revision, "build_version": version, "architecture": args.architecture,
            "debug_enabled": args.validation_debug == "y",
            "installer_archives": sorted(args.archive),
            "auxiliary_parents": list(PARENTS),
            "registered_debug": sorted(set(args.registered_debug)),
            "validation_only_debug": sorted(set(args.validation_only_debug)),
        }, sort_keys=True) + "\n")
        # A Make retry gets a complete replacement, including removal of any
        # debug bundles from an earlier attempt in this checkout.
        if destination.exists():
            destination.rename(Path(work) / "previous")
        staged.rename(destination)


def matrix(directory, debug_layer):
    # Use Make's selected archive list, including expected companion variants.
    # Losing a bundle or manifest member on disk must not shrink the job matrix.
    selection = json.loads((directory / "selection.json").read_bytes())
    expected = {family(name) for name in selection["installer_archives"]} | set(PARENTS)
    require(selection["debug_enabled"] == (debug_layer == "y"), "Make and workflow debug settings differ")
    require({p.name for p in directory.iterdir() if p.is_dir()} == expected,
            "validation bundle list differs from the VS build")
    output = []
    for image in sorted(expected):
        bundle = json.loads((directory / image / "bundle.json").read_bytes())
        variants = {"runtime"}
        if debug_layer == "y" and image + "-dbg.gz" in selection["registered_debug"] + selection["validation_only_debug"]:
            variants.add("debug")
        require(set(bundle["archives"]) == variants, f"{image}: missing selected archive during publication")
        output.append({"image": image})
    return {"include": output}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("matrix",))
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--debug-layer", required=True, choices=("y", "n"))
    args = parser.parse_args()
    print(json.dumps(matrix(args.directory, args.debug_layer), separators=(",", ":")))
