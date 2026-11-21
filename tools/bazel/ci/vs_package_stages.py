"""Check the package decisions against the OCI stages exported by the VS build."""

import json

from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import base_images, dash_engine_validate, p4rt_validate
from tools.bazel.ci.vs_container_validation import read_oci


def read(path):
    return json.loads(path.read_bytes())


def verify_p4rt_stage(directory, stage, base, current, evidence):
    selection = directory / (stage + "-apt.json")
    metadata_path = directory / ("p4rt-" + stage + ".packages.json")
    metadata = read(metadata_path)
    contract = {"runtime": "p4rt_runtime_contract.json", "symbols": "p4rt_runtime_debug_contract.json"}.get(stage)
    p4rt_validate.stage_binding(
        stage, read(selection), metadata, base_digest=base["manifest_digest"], apt_sha=check.sha(selection),
        contract_sha=check.sha(directory / contract) if contract else None,
    )
    # Only the runtime changes the parent's command/environment. Both exported
    # variants replace their manifest; the symbols stage inherits the full config.
    manifest = read(directory / (stage + ".make.manifest.json")) if stage != "symbols" else None
    p4rt_validate.verify_config(current["config"]["config"], base["config"]["config"],
                                runtime=stage == "runtime", manifest=manifest)
    installed = current["files"]
    handoff = read(directory / "handoff.json")
    owners = evidence["owners"]
    hashes = evidence["source_sha256"]
    if contract:
        tar_name = "p4rt.source.tar" if stage == "runtime" else "p4rt.owner-debug.tar"
        upstream = "p4rt_runtime.tar" if stage == "runtime" else "p4rt_runtime_debug.tar"
        definition = read(directory / contract)
        p4rt_validate.owner_package_state(metadata, definition, owners[tar_name])
        package, = definition["packages"].values()
        check.require(hashes[tar_name] == handoff["files"][upstream]["sha256"] == package["payload_sha256"] and
                      check.sha(directory / contract) == handoff["files"][contract]["sha256"],
                      "P4RT payload or contract differs from the source owner's handoff: " + upstream)
        p4rt_validate.verify_payload(installed, owners[tar_name], "Installed P4RT package changed in " + stage)
    if stage == "runtime":
        common = handoff["parent_common"]
        check.require(installed.get(common["path"], {}).get("sha256") == common["sha256"] and
                      check.sha(directory / "config-engine.source.tar") == common["parent_source_tar_sha256"],
                      "Installed libswsscommon or its source TAR differs from the P4RT SDK handoff")
    if stage == "debug":
        packages = {"libsairedis": "sairedis.source.tar", "libsaimetadata": "saimetadata.source.tar"}
        p4rt_validate.verify_debug_sai(
            read_oci(directory, "runtime")["files"], base["files"], installed, metadata,
            {name: owners[tar] for name, tar in packages.items()}, {name: hashes[tar] for name, tar in packages.items()},
            read(directory / "sairedis.library-packages.json"),
        )
    for path, source in (
        ("var/lib/dpkg/status", directory / ("p4rt-" + stage + ".status")),
        ("usr/share/sonic/bazel/p4rt-" + stage + "-packages.json", metadata_path),
        ("usr/share/sonic/bazel/p4rt-handoff.json", directory / "handoff.json"),
    ):
        check.require(installed.get(path, {}).get("sha256") == check.sha(source),
                      "Installed P4RT package input changed in stage " + stage + ": " + path)


def verify(directory, bundle, variant, current, owners):
    image = bundle["image"]
    if image not in ("docker-orchagent", "docker-sonic-gnmi", "docker-sonic-otel", "docker-dash-engine",
                     "docker-sonic-p4rt"):
        return
    if variant == "debug" and image in ("docker-orchagent", "docker-sonic-otel"):
        # These two owner checks concern the runtime selection inherited by debug.
        return
    parent = read_oci(directory, "parent" if variant == "runtime" else "runtime")
    if image == "docker-sonic-p4rt":
        if variant == "debug":
            intermediate = read_oci(directory, "symbols")
            verify_p4rt_stage(directory, "symbols", parent, intermediate, owners)
            parent = intermediate
        verify_p4rt_stage(directory, variant, parent, current, owners)
        return

    selection_path = directory / (variant + "-apt.json")
    selection = read(selection_path)
    base_digest = parent["manifest_digest"]
    if image == "docker-orchagent":
        base_images.verify_orchagent_selection(selection, base_digest, directory / "libteam-source-receipt.json")
    elif image == "docker-sonic-otel":
        check.verify_apt_binding(selection, base_digest)
    elif image == "docker-sonic-gnmi":
        metadata = "packages.json" if variant == "runtime" else "debug-packages.json"
        check.verify_apt_binding(selection, base_digest, metadata=read(directory / metadata),
                                 selection_sha=check.sha(selection_path))
    else:
        metadata = "engine.packages.json" if variant == "runtime" else "engine-debug.packages.json"
        dash_engine_validate.verify_package_binding(read(directory / metadata), selection, base_digest, selection_path,
                                                    variant, bundle["apt_lock_sha256"])
        dash_engine_validate.verify_added_layer(current["layers"], len(parent["layers"]), bundle["apt_layers"][variant])
