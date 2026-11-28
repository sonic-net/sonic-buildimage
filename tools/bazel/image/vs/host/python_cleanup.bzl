"""Uninstall the explicitly requested post-validation Python distributions."""
def _impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    audit = ctx.actions.declare_file(ctx.label.name + ".audit.json")
    spec = ctx.actions.declare_file(ctx.label.name + ".spec.json")
    ctx.actions.write(spec, json.encode({"layers": [file.path for file in ctx.files.layers],
                                        "distributions": ctx.attr.distributions, "cache_tag": ctx.attr.cache_tag}))
    ctx.actions.run(executable = ctx.executable._tool, arguments = [spec.path, output.path, audit.path],
                    inputs = [spec] + ctx.files.layers, outputs = [output, audit], mnemonic = "VsHostPythonCleanup")
    return [DefaultInfo(files = depset([output])), OutputGroupInfo(audit = depset([audit]))]

host_python_cleanup = rule(
    implementation = _impl,
    attrs = {
        "layers": attr.label_list(mandatory = True, allow_files = True),
        "distributions": attr.string_list(mandatory = True),
        "cache_tag": attr.string(mandatory = True),
        "_tool": attr.label(default = Label(":python_cleanup_tool"), executable = True, cfg = "exec"),
    },
)
