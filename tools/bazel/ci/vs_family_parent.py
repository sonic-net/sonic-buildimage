"""Check the inspected service image against the parent selected by the VS build."""

from importlib import import_module
import json

from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import dash_engine_validate
from tools.bazel.ci.vs_container_validation import parent_with_make_settings, read_oci
from tools.bazel.ci.vs_validation_bundles import GROUP_CHECKS, PARENT_IMAGES
from tools.bazel.oci.image_validation import verify_directory_aliases


# Each entry names the exact configuration and developer-tool TARs used by the
# original image. Only a matching file from one of these may replace its parent.
CONFIG_TAR = {
    "dash_engine": "engine.files.tar", "dash_ha": "dash_ha.files.tar",
    "eventd": "config-files.tar", "gnmi": "config-files.tar", "lldp": "config-files.tar",
    "macsec": "macsec.files.tar", "mgmt_framework": "config-files.tar",
    "mux": "config-files.tar", "orchagent": "config.tar", "otel": "otel.files.tar",
    "p4rt": "p4rt.files.tar", "radv": "config-files.tar", "sflow": "sflow.files.tar",
    "sysmgr": "sysmgr.files.tar",
}
TOOL_TAR = {
    "dash_engine": "engine.debug-tools.tar", "dash_ha": "dash_ha.debug-tools.tar",
    "macsec": "macsec.debug-tools.tar", "orchagent": "debug.tools.tar",
    "otel": "otel.debug-tools.tar", "p4rt": "p4rt.debug-tools.tar",
    "sflow": "sflow.debug-tools.tar", "sysmgr": "sysmgr.debug-tools.tar",
}
DPKG = "var/lib/dpkg/status"
STATOVERRIDE = "var/lib/dpkg/statoverride"
ACCOUNTS = {"etc/passwd", "etc/shadow", "etc/group", "etc/gshadow"}


def ancestry(parent, child, image):
    """Bind both blob digests and uncompressed filesystem layers to the exact parent."""
    layers = parent["layers"]
    ids = parent["config"]["rootfs"]["diff_ids"]
    check.require(layers and len(layers) == len(ids)
                  and child["layers"][:len(layers)] == layers
                  and child["config"]["rootfs"]["diff_ids"][:len(ids)] == ids,
                  "The selected image does not inherit the configured parent: " + image)


def account_rows(directory, variant, name, files):
    source = directory / (variant + "." + name)
    check.require(check.sha(source) == files["etc/" + name]["sha256"],
                  "Account fixture differs from the image inventory: " + source.name)
    return source.read_text().splitlines()


def check_accounts(directory, family, variant, parent, files):
    before = account_rows(directory, "parent", "group", parent)
    after = account_rows(directory, variant, "group", files)
    if family in {"dash_ha", "sysmgr"}:
        original = (directory / "parent.group").read_bytes()
        expected = original
        if variant == "debug" and b"_ssh:x:104:" not in original.splitlines():
            if family == "sysmgr":
                check.require(not original or original.endswith(b"\n"), "Unterminated Sysmgr runtime group file")
            expected += (b"\n" if original and not original.endswith(b"\n") else b"") + b"_ssh:x:104:\n"
        check.require((directory / (variant + ".group")).read_bytes() == expected,
                      "Image changed inherited group file bytes: " + family)
    if family == "gbsyncd":
        # The Gearbox image includes the source OpenSSH client in runtime.
        for rows in (before, after):
            names = [row.split(":", 1)[0] for row in rows]
            check.require(len(names) == len(set(names)), "Duplicate Gearbox group name")
        check.require(set(after) == set(before) | {"_ssh:x:104:"},
                      "Gearbox account merge changed inherited groups")
        check.require(all(parent["etc/group"][k] == files["etc/group"][k]
                          for k in ("kind", "mode", "uid", "gid")), "Gearbox group metadata changed")
        return
    if family == "macsec" and not any(row.startswith("netdev:") for row in before):
        gids = {int(row.split(":")[2]) for row in before}
        gid = next(i for i in range(100, 1000) if i not in gids)
        before.append(f"netdev:x:{gid}:")
    if family == "otel":
        before.append("otelcol-contrib:!:999:")
        passwd = account_rows(directory, variant, "passwd", files)
        expected_user = "otelcol-contrib:!:999:999::/home/otelcol-contrib:/sbin/nologin"
        check.require(passwd.count(expected_user) == 1 and [row for row in passwd if row != expected_user]
                      == account_rows(directory, "parent", "passwd", parent),
                      "OTEL changed its inherited users or collector identity")
        for path in ("etc/passwd", "etc/group"):
            item = files[path]
            check.require((item["mode"], item["uid"], item["gid"]) == (0o644, 0, 0),
                          "OTEL account permissions changed: " + path)
    dash_engine_validate.verify_groups(before, after, parent["etc/group"], files["etc/group"], variant == "debug")


def configuration_files(family, owners):
    if family not in CONFIG_TAR:
        return {}
    config = owners[CONFIG_TAR[family]]
    # These owners name the permitted COPY destinations in their existing
    # Dockerfile checks. Other inherited files must still match the parent.
    if family in {"eventd", "lldp", "mux", "radv"}:
        declared = import_module("tools.bazel.ci." + family + "_image").CONFIGS.values()
        paths = [item if isinstance(item, str) else item[0] for item in declared]
        return {path: config[path] for path in paths}
    return config


def verify_files(family, variant, parent, child, owners):
    """Compare all inherited non-directory records and allow the owner's exact replacements."""
    before, after = parent["files"], child["files"]
    debug = variant == "debug"
    mutable = {DPKG}
    if family == "eventd" and not debug:
        mutable.clear()
    if family in {"lldp", "mux", "radv"}:
        mutable |= ACCOUNTS
    if family == "lldp":
        mutable.add(STATOVERRIDE)
    if family == "database":
        check.require(STATOVERRIDE not in before, "Database must retain pre-existing dpkg statoverride records")
    if family in {"database", "eventd"} and debug:
        mutable.update(("etc/group", "etc/gshadow"))
    if family == "otel":
        mutable.add("etc/passwd")
    if family in {"macsec", "otel"} or debug:
        mutable.add("etc/group")
    if family == "gnmi" and "usr/bin/dialout.sh" in after:
        # verify_dialout separately binds this feature-selected launcher to the Make profile and Git source.
        mutable.add("usr/bin/dialout.sh")
    replacements = configuration_files(family, owners)
    if debug:
        replacements = {**replacements, **owners[TOOL_TAR.get(family, "debug-tools.tar")]}
        if family == "orchagent":
            replacements.update(owners["debug.accounts.tar"])
    check.verify_inherited_files(before, after, replacements, mutable, family)


def verify_service_config(directory, family, variant, parent, current, fixtures):
    if family not in ("dash_engine", "gnmi", "mgmt_framework"):
        return
    if family == "gnmi" and variant == "debug":
        parent = read_oci(directory, "runtime")
    expected = parent_with_make_settings(directory, variant, parent["config"]["config"], fixtures)
    actual = current["config"]["config"]
    if family == "mgmt_framework":
        from tools.bazel.ci.mgmt_framework_image import verify_environment
        verify_environment(expected, actual)
        return
    manifest = json.loads((directory / (variant + ".make.manifest.json")).read_bytes())
    # The common archive check independently verifies this label against the installer release.
    version = actual["Labels"]["Tag"]
    if family == "dash_engine":
        dash_engine_validate.verify_config(expected, actual, manifest, version)
    else:
        from tools.bazel.ci.gnmi_image import verify_config
        verify_config(actual, expected, manifest, version, runtime=variant == "runtime")


def verify_parent(directory, bundle, family, variant, current, evidence):
    """Reapply original parent contracts to this installer artifact and its selected variant."""
    if family in ("teamd", "frr"):
        return  # These services also compare their live filesystem in verify_full_parent.
    image = bundle["image"]
    parent = read_oci(directory, "parent")
    record = json.loads((directory / "parent.json").read_bytes())
    check.require(record["image"] == PARENT_IMAGES[image] and record["diff_ids"] == parent["config"]["rootfs"]["diff_ids"],
                  "Wrong configured parent for " + image)
    ancestry(parent, current, image)
    verify_service_config(directory, family, variant, parent, current, bundle["fixtures"])
    verify_directory_aliases(current["files"])
    if image in GROUP_CHECKS:
        check_accounts(directory, family, variant, parent["files"], current["files"])
    if family in ("snmp", "gbsyncd"):
        return  # These services verify parent ancestry and owner ELFs.
    owners = evidence["owners"]
    if family == "dash_engine":
        base = parent if variant == "runtime" else read_oci(directory, "runtime")
        debug = variant == "debug"
        selection = json.loads((directory / (variant + "-apt.json")).read_bytes())
        payload = owners["engine.debug-tools.tar" if debug else "engine.runtime-apt.tar"]
        config = dash_engine_validate.debug_alternatives(payload) if debug else owners["engine.files.tar"]
        dash_engine_validate.verify_parent_payload(base["files"], current["files"], payload, selection, config)
    elif family == "p4rt" and variant == "debug":
        from tools.bazel.ci.p4rt_validate import inherited_files, verify_payload
        runtime = read_oci(directory, "runtime")
        # The native image adds its .build-id symbols as a layer on runtime.
        ancestry(runtime, current, image)
        tools = owners["p4rt.debug-tools.tar"]
        inherited_files(runtime["files"], current["files"], tools, group=True)
        verify_payload(current["files"], tools, "P4RT debug tools changed")
    else:
        verify_files(family, variant, parent, current, owners)
