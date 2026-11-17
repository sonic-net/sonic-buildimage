"""Bind the legacy IMAGE_VERSION argument to a declared Bazel build setting."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

def _environment_impl(ctx):
    version = ctx.attr.version[BuildSettingInfo].value
    if "\n" in version or "\r" in version:
        fail("IMAGE_VERSION must be one line")
    out = ctx.actions.declare_file(ctx.label.name + ".env")
    ctx.actions.write(out, "DEBIAN_FRONTEND=noninteractive\nPYTHONOPTIMIZE=1\nIMAGE_VERSION=" + version + "\n")
    return [DefaultInfo(files = depset([out]))]

image_environment = rule(
    implementation = _environment_impl,
    attrs = {"version": attr.label(mandatory = True)},
)
