"""Public adapters for source-owned kernel Debian artifacts."""

def _kernel_impl(ctx):
    runtime = ctx.actions.declare_file(ctx.label.name + ".runtime.tar")
    control = ctx.actions.declare_file(ctx.label.name + ".control.tar")
    boot = ctx.actions.declare_directory(ctx.label.name + ".boot")
    metadata = ctx.actions.declare_file(ctx.label.name + ".metadata.json")
    ctx.actions.run(
        executable = ctx.attr._tool[DefaultInfo].files_to_run,
        arguments = [ctx.file.deb.path, ctx.file.receipt.path, runtime.path, boot.path, metadata.path, ctx.executable.zstd.path, control.path],
        inputs = [ctx.file.deb, ctx.file.receipt],
        outputs = [runtime, boot, metadata, control],
        tools = [ctx.attr.zstd[DefaultInfo].files_to_run],
        mnemonic = "SonicKernelRuntime",
    )
    return [DefaultInfo(files = depset([runtime])), OutputGroupInfo(control = depset([control]), runtime = depset([runtime]), boot = depset([boot]), metadata = depset([metadata]))]

sonic_kernel_runtime = rule(
    implementation = _kernel_impl,
    attrs = {
        "deb": attr.label(allow_single_file = [".deb"], mandatory = True),
        "receipt": attr.label(allow_single_file = [".json"], mandatory = True),
        "zstd": attr.label(default = "@sonic_build_infra//tools/build_tools:zstd", executable = True, cfg = "exec"),
        "_tool": attr.label(default = Label(":kernel_deb"), executable = True, cfg = "exec"),
    },
)
