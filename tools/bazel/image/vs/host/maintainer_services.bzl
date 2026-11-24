"""Replay configure-time service state from the selected package controls."""
def _impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    audit = ctx.actions.declare_file(ctx.label.name + ".audit.log")
    arguments = ctx.actions.args()
    arguments.add("--out", output)
    arguments.add("--log", audit)
    arguments.add_all(ctx.files.layers, before_each = "--payload")
    arguments.add_all(ctx.files.controls, before_each = "--control")
    inputs = ctx.files.layers + ctx.files.controls
    if ctx.file.extra:
        arguments.add("--extra", ctx.file.extra)
        inputs.append(ctx.file.extra)
    ctx.actions.run(executable = ctx.executable._tool, arguments = [arguments], inputs = inputs,
                    outputs = [output, audit], mnemonic = "VsHostMaintainerServices")
    return [DefaultInfo(files = depset([output])), OutputGroupInfo(audit = depset([audit]))]

host_maintainer_services = rule(
    implementation = _impl,
    attrs = {
        "layers": attr.label_list(mandatory = True, allow_files = True),
        "controls": attr.label_list(mandatory = True, allow_files = True),
        "extra": attr.label(allow_single_file = True),
        "_tool": attr.label(default = Label(":maintainer_services_tool"), executable = True, cfg = "exec"),
    },
)
