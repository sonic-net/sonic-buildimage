"""Host dynamic linker cache using a TAR-backed filesystem reader."""
def _impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    audit = ctx.actions.declare_file(ctx.label.name + ".audit.json")
    spec = ctx.actions.declare_file(ctx.label.name + ".spec.json")
    layers = ctx.files.layers
    ctx.actions.write(spec, json.encode({"architecture": ctx.attr.architecture, "layers": [file.path for file in layers]}))
    ctx.actions.run(executable = ctx.executable._tool, arguments = [spec.path, output.path, audit.path],
                    inputs = [spec] + layers, outputs = [output, audit], mnemonic = "VsHostLdCache")
    return [DefaultInfo(files = depset([output])), OutputGroupInfo(audit = depset([audit]))]

host_ldcache = rule(
    implementation = _impl,
    attrs = {
        "layers": attr.label_list(mandatory = True, allow_files = True),
        "architecture": attr.string(mandatory = True, values = ["amd64", "arm64"]),
        "_tool": attr.label(default = Label(":ldcache_tool"), executable = True, cfg = "exec"),
    },
)
