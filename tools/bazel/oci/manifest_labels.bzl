"""Convert a Make manifest and SONiC release version into OCI labels."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

def _manifest_labels_impl(ctx):
    args = ctx.actions.args()
    args.add("--manifest", ctx.file.manifest)
    args.add("--output", ctx.outputs.out)
    inputs = [ctx.file.manifest]
    if ctx.file.component_labels:
        args.add("--component-labels", ctx.file.component_labels)
        inputs.append(ctx.file.component_labels)
    if ctx.file.package_receipt:
        args.add("--package-metadata", ctx.file.package_receipt)
        args.add_all(ctx.attr.version_packages, before_each = "--version-package")
        inputs.append(ctx.file.package_receipt)
    image_version = ctx.attr.image_version[BuildSettingInfo].value
    if not image_version and not ctx.attr._use_make_metadata[BuildSettingInfo].value:
        image_version = ctx.attr._product_version[BuildSettingInfo].value
    args.add("--image-version=" + image_version)
    ctx.actions.run(
        arguments = [args],
        executable = ctx.attr._tool[DefaultInfo].files_to_run,
        inputs = inputs,
        outputs = [ctx.outputs.out],
        mnemonic = "ContainerManifestLabel",
    )

_manifest_labels = rule(
    implementation = _manifest_labels_impl,
    attrs = {
        "component_labels": attr.label(allow_single_file = True),
        "manifest": attr.label(allow_single_file = True, mandatory = True),
        "image_version": attr.label(default = Label("//tools/bazel/oci:image_version"), providers = [BuildSettingInfo]),
        "_product_version": attr.label(default = Label("//tools/bazel/image/vs/metadata:version"), providers = [BuildSettingInfo]),
        "_use_make_metadata": attr.label(default = Label("//tools/bazel/image/vs/metadata:use_make_metadata"), providers = [BuildSettingInfo]),
        "package_receipt": attr.label(allow_single_file = True),
        "version_packages": attr.string_list(),
        "out": attr.output(mandatory = True),
        "_tool": attr.label(
            default = Label("//tools/bazel/oci:manifest_labels"),
            executable = True,
            cfg = "exec",
        ),
    },
)

def manifest_labels(name, manifest, out = None, package_receipt = None, version_packages = [], component_labels = None, **kwargs):
    """Encode Make's manifest, release Tag and optional component version labels."""
    if bool(package_receipt) != bool(version_packages):
        fail("package_receipt and version_packages must be supplied together")
    _manifest_labels(
        name = name,
        manifest = manifest,
        component_labels = component_labels,
        out = out or name + ".labels",
        package_receipt = package_receipt,
        version_packages = version_packages,
        **kwargs
    )
