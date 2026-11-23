"""Checked OCI APT layers with container policy declared in BUILD.bazel.

Use retained_source="none" for source-built native layers, or "make" with a
retained_manifest for imported Make packages. The common selector validates
package controls and binds debug selection to the exact runtime receipt and
Make manifest. Conflicting inherited package records are rejected.
"""

load("@aspect_bazel_lib//lib:jq.bzl", "jq")
load("@sonic_build_infra//apt:apt_layer.bzl", _apt_layer = "apt_layer")
load("//tools/bazel/dpkg:defs.bzl", "OCI_INCLUDES")

_SHARED_BASE_INVENTORIES = {
    "//dockers/docker-base-trixie:docker-base-trixie": "//dockers/docker-base-trixie:base_inventory",
    "//dockers/docker-config-engine-trixie:docker-config-engine-trixie": "//dockers/docker-config-engine-trixie:base_inventory",
}

def apt_subset_lock(name, selection, dependency_set, out):
    """Expose one selected closure to the OCI selector with its original lock metadata."""
    jq(
        name = name,
        srcs = ["@sonic_build_infra//apt:apt.lock.json"],
        args = [
            "--sort-keys",
            "--slurpfile",
            "selection",
            "$(execpath %s)" % selection,
            "--arg",
            "subset",
            dependency_set,
        ],
        data = [selection],
        expand_args = True,
        filter_file = "//tools/bazel/oci:apt_subset.jq",
        out = out,
    )

def _policy_impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".json")
    ctx.actions.write(output, ctx.attr.document + "\n")
    return [DefaultInfo(files = depset([output]))]

_policy = rule(
    implementation = _policy_impl,
    attrs = {"document": attr.string(mandatory = True)},
)

def apt_layer(
        name,
        packages,
        lock,
        dependency_set,
        base,
        policy,
        variant,
        retained_manifest = None,
        base_package_metadata = None,
        source_receipt = None,
        base_inventory = None,
        filter_layer = False,
        **kwargs):
    """Generate a declared policy input and assemble selected APT payloads.

    The generated <name>_policy target can also be used by container policy
    tests. Runtime/debug layers share the same policy; dynamic package controls
    and package identities come from a declared source receipt or Make manifest.
    """
    if retained_manifest != None and source_receipt != None:
        fail("APT selection accepts an imported manifest or a source receipt, not both")
    _policy(
        name = name + "_policy",
        document = json.encode(policy),
        visibility = kwargs.get("visibility", None),
    )
    # These published bases are common to many image owners. A policy carrying
    # extra architecture exceptions must inventory its exact inputs separately.
    if base_inventory == None and not policy.get("static_i386") and type(base) == "string":
        base_inventory = _SHARED_BASE_INVENTORIES.get(base)
    _apt_layer(
        name = name,
        packages = packages,
        lock = lock,
        dependency_set = dependency_set,
        base = base,
        base_inventory = base_inventory,
        direct_layer = True,
        layer_excludes = ["//tools/bazel/dpkg:dpkg_excludes.txt"] if filter_layer else [],
        layer_includes = OCI_INCLUDES if filter_layer else [],
        policy = ":" + name + "_policy",
        selector = "//tools/bazel/oci:select_apt_payloads",
        architecture = policy["architecture"],
        variant = variant,
        retained_manifest = source_receipt if source_receipt != None else retained_manifest,
        base_package_metadata = base_package_metadata,
        **kwargs
    )
