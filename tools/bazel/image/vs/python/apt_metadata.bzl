"""Hand off existing Debian Python installations without copying their payloads."""

load("@sonic_build_infra//python:source_package.bzl", "PythonRuntimeInfo")

def _impl(ctx):
    metadata = ctx.actions.declare_file(ctx.label.name + ".runtime.json")
    args = ctx.actions.args()
    args.add("--archive", ctx.file.archive)
    args.add("--distribution", ctx.attr.distribution)
    args.add("--output", metadata)
    ctx.actions.run(executable = ctx.attr._tool[DefaultInfo].files_to_run, arguments = [args], inputs = [ctx.file.archive], outputs = [metadata], mnemonic = "DebianPythonMetadata")
    return [
        DefaultInfo(files = depset([ctx.file.archive])),
        PythonRuntimeInfo(
            packages = depset([struct(distribution = ctx.attr.distribution, version = "", tar = ctx.file.archive, metadata = metadata, producer = str(ctx.label))]),
            tars = depset([ctx.file.archive]),
            metadata = depset([metadata]),
        ),
    ]

apt_python_metadata = rule(implementation = _impl, attrs = {
    "archive": attr.label(mandatory = True, allow_single_file = True),
    "distribution": attr.string(mandatory = True),
    "_tool": attr.label(default = Label(":apt_metadata_tool"), executable = True, cfg = "exec"),
})
