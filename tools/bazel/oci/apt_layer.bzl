"""Checked OCI APT layers with container policy declared in BUILD.bazel.

Use retained_source="none" for source-built native layers, or "make" with a
retained_manifest for imported Make packages. The common selector validates
package controls and binds debug selection to the exact runtime receipt and
Make manifest. Conflicting inherited package records are rejected.
"""

load("@sonic_build_infra//apt:apt_layer.bzl", _apt_layer = "apt_layer")

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
        **kwargs):
    """Generate a declared policy input and select/flatten checked APT payloads.

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
    _apt_layer(
        name = name,
        packages = packages,
        lock = lock,
        dependency_set = dependency_set,
        base = base,
        policy = ":" + name + "_policy",
        selector = "//tools/bazel/oci:select_apt_payloads",
        architecture = policy["architecture"],
        variant = variant,
        retained_manifest = source_receipt if source_receipt != None else retained_manifest,
        base_package_metadata = base_package_metadata,
        **kwargs
    )
