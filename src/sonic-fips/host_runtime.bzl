"""Runtime and original controls for the Make-selected prebuilt FIPS packages."""
def _impl(ctx):
    data = ctx.actions.declare_file(ctx.label.name + ".tar")
    control = ctx.actions.declare_file(ctx.label.name + ".control.tar")
    receipt = ctx.actions.declare_file(ctx.label.name + ".receipt.json")
    ctx.actions.run(
        executable = ctx.attr._tool[DefaultInfo].files_to_run,
        arguments = [ctx.file.deb.path, data.path, control.path, receipt.path, ctx.attr.package, ctx.attr.version, ctx.attr.architecture],
        inputs = [ctx.file.deb], outputs = [data, control, receipt],
        mnemonic = "FipsHostRuntime",
    )
    return [DefaultInfo(files = depset([data])), OutputGroupInfo(control = depset([control]), receipt = depset([receipt]))]

fips_host_runtime = rule(implementation = _impl, attrs = {
    "deb": attr.label(allow_single_file = [".deb"], mandatory = True),
    "package": attr.string(mandatory = True), "version": attr.string(mandatory = True),
    "architecture": attr.string(mandatory = True),
    "_tool": attr.label(default = Label(":host_extract"), executable = True, cfg = "exec"),
})
