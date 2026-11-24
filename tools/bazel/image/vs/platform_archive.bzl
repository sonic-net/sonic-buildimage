"""Platform resources preserve original Debian package bytes for first boot."""
def _impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    request = ctx.actions.declare_file(ctx.label.name + ".inputs.json")
    spec = []
    inputs = []
    for target, destination in ctx.attr.packages.items():
        files = target[DefaultInfo].files.to_list()
        if len(files) != 1:
            fail("platform package input must provide one original DEB or DEB directory")
        platform = ctx.attr.lazy_platform if destination == "common" else ""
        spec.append([files[0].path, destination, platform])
        inputs.extend(files)
    ctx.actions.write(request, json.encode(spec))
    tool = ctx.attr._tool[DefaultInfo].files_to_run
    ctx.actions.run(executable = tool, arguments = [request.path, output.path], inputs = [request] + inputs, outputs = [output], mnemonic = "VsPlatformArchive")
    return [DefaultInfo(files = depset([output]))]

vs_platform_archive = rule(
    implementation = _impl,
    attrs = {
        "packages": attr.label_keyed_string_dict(mandatory = True, allow_files = True),
        "lazy_platform": attr.string(mandatory = True),
        "_tool": attr.label(default = Label(":platform_archive_tool"), executable = True, cfg = "exec"),
    },
)


def _platforms_impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name)
    request = ctx.actions.declare_file(ctx.label.name + ".inputs.json")
    markers = [[file.short_path.split("/")[-2], file.path] for file in ctx.files.markers]
    inputs = list(ctx.files.markers)
    for target, platform in ctx.attr.generated.items():
        files = target[DefaultInfo].files.to_list()
        if len(files) != 1:
            fail("generated ASIC marker must provide one declared file")
        markers.append([platform, files[0].path])
        inputs.extend(files)
    ctx.actions.write(request, json.encode({"machine": ctx.attr.machine, "markers": markers}))
    tool = ctx.attr._tool[DefaultInfo].files_to_run
    ctx.actions.run(executable = tool, arguments = [request.path, output.path], inputs = [request] + inputs, outputs = [output], mnemonic = "VsPlatformsAsic")
    return [DefaultInfo(files = depset([output]))]

vs_platforms_asic = rule(implementation = _platforms_impl, attrs = {
    "markers": attr.label_list(mandatory = True, allow_files = True),
    "generated": attr.label_keyed_string_dict(allow_files = True),
    "machine": attr.string(mandatory = True),
    "_tool": attr.label(default = Label(":platforms_tool"), executable = True, cfg = "exec"),
})
