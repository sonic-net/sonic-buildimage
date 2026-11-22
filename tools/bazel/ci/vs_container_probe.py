#!/usr/bin/env python3
"""Validate one VS build's immutable container export and run its installed service probes."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import vs_container_validation as common
from tools.bazel.ci import vs_package_stages
from tools.bazel.ci import vs_probe_policy as policy
from tools.bazel.ci.vs_archive_reuse import verify_execution_settings
from tools.bazel.oci.oci_inventory import resolve_path


# These services' install contracts require their detached debug payload to be
# absent from the runtime image. Read only owner inputs selected by this VS build.
DEBUG_ONLY_PAYLOADS = {
    **dict.fromkeys(("docker-database", "docker-eventd", "docker-lldp", "docker-mux", "docker-router-advertiser",
                     "docker-sonic-gnmi", "docker-sonic-mgmt-framework"), ("all-debug.tar",)),
    **dict.fromkeys(("docker-gnmi-sidecar", "docker-restapi-sidecar",
                     "docker-gnmi-watchdog", "docker-bmp-watchdog"), ("all-debug.tar",)),
    "docker-orchagent": ("openssh.source.tar",),
    "docker-dhcp-relay": ("all-debug.tar",),
    "docker-sonic-p4rt": ("p4rt.debug.tar", "sairedis.source.tar", "saimetadata.source.tar"),
}


def verify_debug_separation(image, variant, native, owners, runtime_paths):
    payloads = DEBUG_ONLY_PAYLOADS.get(image, ())
    if not payloads:
        return
    if variant == "debug":
        paths = {path for name in payloads for path in owners[name]}
    elif image == "docker-orchagent":
        policy = json.loads((ROOT / "dockers/docker-orchagent/bazel/debug_source_paths.json").read_bytes())
        paths = policy["packages"]["openssh-client"]["required_paths"]
    else:
        # The debug-disabled build still knows the build IDs of its runtime owners.
        paths = {f"usr/lib/debug/.build-id/{build_id[:2]}/{build_id[2:]}.debug"
                 for item in native.values() if (build_id := item["build_id"])}
    if variant == "runtime" and image == "docker-sonic-p4rt":
        paths |= {"usr/lib/x86_64-linux-gnu/" + name for name in
                  ("libsairedis.so.0.0.0", "libsaimeta.so.0.0.0", "libsaimetadata.so.0.0.0")}
    leaked = set(paths).intersection(runtime_paths)
    check.require(not leaked, image + " runtime contains debug-only files: " + ", ".join(sorted(leaked)))


def artifact_records(directory, bundle):
    hashes = dict(bundle["fixtures"])
    hashes.update({item["file"]: item["sha256"] for item in bundle["archives"].values()})
    return {name: {"sha256": digest, "bytes": (directory / name).stat().st_size}
            for name, digest in hashes.items()}


def native_expectations(directory, bundle, variant, files, evidence):
    """Verify native source bytes against the final filesystem, including installed overrides."""
    native, pairs = {}, []
    owner_native = evidence["native"]
    for name, item in owner_native.items():
        installed = files.get(name, {})
        check.require(installed.get("elf_machine") == 62 and installed.get("sha256") == item["sha256"],
                      "image changed source owner ELF: " + name)
        check.require(all(installed.get(key) == item[key] for key in ("mode", "uid", "gid")),
                      "image changed source owner permissions: " + name)
        native[name] = item
        identifier = item["build_id"]
        if variant == "debug" and identifier:
            symbols = f"usr/lib/debug/.build-id/{identifier[:2]}/{identifier[2:]}.debug"
            if item["has_debuglink"]:
                check.require(symbols in files, "installed native ELF has no matching detached symbols: " + name)
            if symbols in files:
                pairs.append({"path": name, "debug_path": symbols, "needed": item["needed"]})
    if variant == "debug" and bundle["image"] in ("docker-dash-engine", "docker-gbsyncd-vs"):
        if bundle["image"] == "docker-gbsyncd-vs":
            forwarding = json.loads((directory / "policy.json").read_bytes())["native"]["debug_forwarding"]
        else:
            from tools.bazel.ci.dash_engine_validate import DEBUG_FORWARDING
            forwarding = DEBUG_FORWARDING
        indexed = {p["path"]: p for p in pairs}
        for launcher, spec in forwarding.items():
            provider = resolve_path(spec["provider"], files)
            indexed[launcher]["debug_forwarding"] = {**spec, "debug_path": indexed[provider]["debug_path"]}
    for product, members in evidence["owners"].items():
        if not isinstance(members, dict):
            continue
        for path, item in members.items():
            installed = files.get(path, {})
            keys = ("kind", "uid", "gid")
            if item["kind"] == "file":
                keys += ("sha256", "mode")
            elif item["kind"] == "directory":
                keys += ("mode",)
            else:
                keys += ("linkname",)
            check.require(all(installed.get(key) == item.get(key) for key in keys),
                          product + " owner payload differs in the VS image: " + path)
    source_metadata = {row["sha256"] for path, row in files.items()
                       if path.startswith("usr/share/sonic/bazel/") and path.endswith("-packages.json") and "sha256" in row}
    for name, digest in bundle["fixtures"].items():
        if name.endswith(".status") and "symbols" not in name and ("debug" in name) == (variant == "debug"):
            check.require(files.get("var/lib/dpkg/status", {}).get("sha256") == digest,
                          "installed dpkg state differs from " + name)
        if not name.endswith("packages.json") or ("debug" in name and variant != "debug"):
            continue
        if bundle["image"] == "docker-sonic-p4rt" and ("symbols" in name or "library-packages" in name):
            continue
        check.require(digest in source_metadata, "installed package metadata differs from " + name)
        for package in json.loads((directory / name).read_text()).get("source_packages", []):
            for row in package.get("native_files", []):
                path = row["path"]
                check.require(all(files.get(path, {}).get(key) == row[key] for key in ("sha256", "mode", "uid", "gid")),
                              name + " declares a missing or changed source ELF: " + path)
    return native, pairs


def verify_service_installation(image, variant, directory, config, files):
    """Check metadata the runtime probe cannot see underneath its writable mounts."""
    package_record = {
        "docker-dhcp-relay": "runtime-packages.json",
        "docker-database": variant + "-packages.json",
        "docker-lldp": "runtime-packages.json",
        "docker-mux": "runtime-packages.json",
        "docker-router-advertiser": "config-engine-packages.json",
        "docker-snmp": "config-engine.packages.json",
    }.get(image)
    if package_record:
        packages = json.loads((directory / package_record).read_bytes())["dependency_check"]["packages"]
        check.verify_swss_component_labels(config["Labels"], packages)
    if image == "docker-database":
        from tools.bazel.ci.database_image import verify_redis_permissions
        verify_redis_permissions(files)
    elif image == "docker-snmp":
        from tools.bazel.ci import snmp_image
        snmp_image.verify_state_directory(files)


def runtime_report(directory, bundle):
    image = bundle["image"]
    if image == "docker-teamd":
        from tools.bazel.ci.teamd_image import verify_parent_runtime_packages
        verify_parent_runtime_packages(directory)
    owner = ROOT / "src/sonic-p4rt/sonic-pins"
    headers = subprocess.check_output(["git", "cat-file", "-p", bundle["revision"]], cwd=ROOT, text=True).split("\n\n", 1)[0]
    report = {"architecture": bundle["architecture"], "revision": bundle["revision"],
              "debug_enabled": "debug" in bundle["archives"],
              "tested_revision_parents": [line.split()[1] for line in headers.splitlines() if line.startswith("parent ")],
              "image_version": bundle["build_version"], "artifacts": artifact_records(directory, bundle),
              "images": {}, "source_debug_pairs": [], "own_debug_pairs": []}
    for variant in ("runtime", "debug"):
        if variant not in bundle["archives"]:
            continue
        spec = bundle["archives"][variant]
        name = spec["file"].removesuffix(".gz")
        inventory = common.read_oci(directory, variant)
        files = inventory["files"]
        verify_service_installation(image, variant, directory, inventory["config"]["config"], files)
        common.verify_amd64_files(image, files)
        owners = json.loads((directory / (variant + ".owners.json")).read_text())
        vs_package_stages.verify(directory, bundle, variant, inventory, owners)
        if image in policy.FAMILIES:
            from tools.bazel.ci.vs_family_parent import verify_parent
            verify_parent(directory, bundle, policy.FAMILIES[image], variant, inventory, owners)
        native, pairs = native_expectations(directory, bundle, variant, files, owners)
        if variant == "runtime":
            runtime_paths = set(files)
        verify_debug_separation(image, variant, native, owners["owners"], runtime_paths)
        if image == "docker-dhcp-relay":
            from tools.bazel.ci import dhcp_relay_image
            native = dhcp_relay_image.verify_vs_owners(directory, variant, inventory, owners, native, pairs)
        if image == "docker-snmp":
            from tools.bazel.ci import snmp_image
            for filename in snmp_image.TARGETS:
                if filename.endswith(".whl"):
                    snmp_image.wheel_payload(files, directory / filename)
        if image in policy.FAMILIES:
            check.require(native, image + " is missing source-owner expectations")
        if variant == "runtime":
            report["runtime_native"] = native
        report["native"] = native
        report["elfs"] = [{"path": path, "needed": row["needed"]} for path, row in native.items()]
        if variant == "debug":
            report["source_debug_pairs"] = pairs
        env = dict(row.split("=", 1) for row in inventory["config"]["config"].get("Env", []))
        manifest = variant + ".make.manifest.json"
        if image == "docker-nat":
            check.require(manifest in bundle["fixtures"], "NAT archive is missing its Make manifest: " + name)
            verify_execution_settings(inventory["config"]["config"], ["/usr/local/bin/supervisord"])
        if manifest in bundle["fixtures"]:
            declared = json.loads((directory / manifest).read_bytes())
            actual = inventory["config"]["config"]["Labels"]["com.azure.sonic.manifest"]
            check.require(json.loads(actual) == declared, "OCI manifest label differs from the Make input: " + name)
        for filename, settings in (("image.env", env), ("components.labels", inventory["config"]["config"]["Labels"])):
            fixture = variant + ".make." + filename
            if fixture in bundle["fixtures"]:
                for line in (directory / fixture).read_text().splitlines():
                    key, value = line.split("=", 1)
                    check.require(settings.get(key) == value, "OCI setting differs from Make for " + key)
        if image == "docker-gbsyncd-vs":
            from tools.bazel.ci.gbsyncd_image import verify_installer_source_payload
            verify_installer_source_payload(directory, variant, files, owners["source_sha256"]["source-runtime.tar"])
        if image == "docker-database":
            report["include_system_eventd"] = env["INCLUDE_SYSTEM_EVENTD"]
            report["own_debug_pairs"] = [p for p in pairs if p["path"] in owners["owners"]["source-runtime.tar"]]
        if image == "docker-sonic-gnmi":
            from tools.bazel.ci.gnmi_image import verify_dialout
            verify_dialout(files, bundle["dialout_enabled"])
            report["dialout_enabled"] = bundle["dialout_enabled"]
        if image == "docker-orchagent":
            from tools.bazel.ci.orchagent_image import verify_scapy
            report["scapy_files"] = verify_scapy(files, directory / "scapy-2.6.1.dev0-py3-none-any.whl")
            report["config_files"] = owners["owners"]["config.tar"]
        if image == "docker-sonic-p4rt":
            from tools.bazel.ci import p4rt_image
            revision = subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD:src/sonic-p4rt/sonic-pins"],
                                               text=True).strip()
            p4rt_image.verify_owner_source(owner, revision)
            report["owner_revision"] = revision
            report["p4rt_owner_test_sha256"] = check.sha(owner / "p4rt_app/runtime_package_test.py")
        summary = {"status": "passed", "native_paths": sorted(native), "config_digest": spec["config_digest"],
                   "rootfs": spec["rootfs"], "elfs": report["elfs"], "native": native,
                   "source_debug_pairs": pairs}
        report["images"][name] = summary
        (directory / (name + ".validation.json")).write_text(json.dumps(summary) + "\n")
    report["status"] = "passed"
    (directory / "validation.json").write_text(json.dumps(report) + "\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--image", required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--debug-layer", choices=("y", "n"), required=True)
    parser.add_argument("--build-version", required=True)
    args = parser.parse_args()
    directory = args.artifacts.resolve()
    bundle = common.validate_artifacts(directory, args.image, args.revision, args.debug_layer == "y", args.build_version)
    report = runtime_report(directory, bundle)
    from tools.bazel.ci.vs_standalone_probes import STANDALONE_IMAGES, standalone
    if args.image in STANDALONE_IMAGES:
        standalone(args.image, directory, bundle, report)
    else:
        family = policy.FAMILIES[args.image]
        script = ROOT / "tools/bazel/ci" / (family + "_loaded.py")
        arguments = ["--artifacts", str(directory)]
        if family == "p4rt":
            arguments = ["--directory", str(directory), "--owner-source", str(ROOT / "src/sonic-p4rt/sonic-pins")]
        subprocess.run([sys.executable, "-B", str(script), *arguments], check=True, cwd=ROOT)
    result = {"status": "passed", "image": args.image, "revision": args.revision, "build_version": args.build_version,
              "archive_sha256": {variant: record["sha256"] for variant, record in bundle["archives"].items()}}
    (directory / "vs-validation.json").write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()
