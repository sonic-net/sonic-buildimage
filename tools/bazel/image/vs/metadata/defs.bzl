"""Declared VS container metadata, with an explicit legacy Make input mode."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

def _metadata_impl(ctx):
    manifest = ctx.actions.declare_file(ctx.label.name + ".json")
    environment = ctx.actions.declare_file(ctx.label.name + ".env")
    version = ctx.actions.declare_file(ctx.label.name + ".version")
    args = ctx.actions.args()
    args.add("--spec", ctx.file.spec)
    args.add("--image", ctx.attr.image)
    args.add("--image-version", ctx.attr.image_version[BuildSettingInfo].value)
    args.add("--manifest", manifest)
    args.add("--environment", environment)
    args.add("--version", version)
    if ctx.attr.debug:
        args.add("--debug")
    ctx.actions.run(
        executable = ctx.attr._tool[DefaultInfo].files_to_run,
        inputs = [ctx.file.spec],
        outputs = [manifest, environment, version],
        arguments = [args],
        mnemonic = "VsContainerMetadata",
    )
    return [
        DefaultInfo(files = depset([manifest])),
        OutputGroupInfo(environment = depset([environment]), version = depset([version])),
    ]

container_metadata = rule(
    implementation = _metadata_impl,
    attrs = {
        "spec": attr.label(default = Label(":containers.json"), allow_single_file = True),
        "image": attr.string(mandatory = True),
        "debug": attr.bool(default = False),
        "image_version": attr.label(default = Label(":version"), providers = [BuildSettingInfo]),
        "_tool": attr.label(default = Label(":generate"), executable = True, cfg = "exec"),
    },
)

def _component_versions_impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".labels")
    args = ctx.actions.args()
    args.add("--component-receipt", ctx.file.receipt)
    args.add("--manifest", output)
    args.add_all(ctx.attr.packages, before_each = "--component-package")
    ctx.actions.run(
        executable = ctx.attr._tool[DefaultInfo].files_to_run,
        inputs = [ctx.file.receipt],
        outputs = [output],
        arguments = [args],
        mnemonic = "VsComponentVersions",
    )
    return [DefaultInfo(files = depset([output]))]

component_versions = rule(
    implementation = _component_versions_impl,
    attrs = {
        "receipt": attr.label(allow_single_file = True, mandatory = True),
        "packages": attr.string_list(mandatory = True),
        "_tool": attr.label(default = Label(":generate"), executable = True, cfg = "exec"),
    },
)
