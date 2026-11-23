"""Produce one common payload and two deltas from already-checked image layers."""

def _impl(ctx):
    base_paths = ctx.actions.declare_file(ctx.label.name + "/base-paths.json")
    base_args = ctx.actions.args()
    base_args.add("--base", ctx.file.base.path)
    base_args.add("--out", base_paths)
    ctx.actions.run(
        executable = ctx.attr._inspect_base[DefaultInfo].files_to_run,
        arguments = [base_args],
        # rules_oci may symlink layer blobs out of the layout TreeArtifact.
        inputs = depset([ctx.file.base], transitive = [ctx.attr.base[DefaultInfo].default_runfiles.files]),
        outputs = [base_paths],
        mnemonic = "InspectVsAptBase",
    )
    args = ctx.actions.args()
    for name, file in [
        ("first", ctx.file.first),
        ("second", ctx.file.second),
        ("base-paths", base_paths),
        ("common-out", ctx.outputs.common),
        ("first-out", ctx.outputs.first_delta),
        ("second-out", ctx.outputs.second_delta),
        ("receipt", ctx.outputs.receipt),
    ]:
        args.add("--" + name, file)
    args.add("--pigz", ctx.executable._pigz)
    ctx.actions.run(
        executable = ctx.executable._worker,
        arguments = [args],
        inputs = [ctx.file.first, ctx.file.second, base_paths],
        tools = [ctx.attr._worker[DefaultInfo].files_to_run, ctx.attr._pigz[DefaultInfo].files_to_run],
        outputs = [ctx.outputs.common, ctx.outputs.first_delta, ctx.outputs.second_delta, ctx.outputs.receipt],
        mnemonic = "ShareVsAptRuntime",
    )
    return [DefaultInfo(files = depset([
        ctx.outputs.common,
        ctx.outputs.first_delta,
        ctx.outputs.second_delta,
        ctx.outputs.receipt,
    ]))]

shared_runtime_layers = rule(
    implementation = _impl,
    attrs = {
        "base": attr.label(allow_single_file = True, mandatory = True),
        "first": attr.label(allow_single_file = True, mandatory = True),
        "second": attr.label(allow_single_file = True, mandatory = True),
        "_inspect_base": attr.label(
            default = Label(":shared_runtime_base"),
            executable = True,
            cfg = "exec",
        ),
        "_worker": attr.label(
            default = Label(":shared_runtime_layers"),
            executable = True,
            cfg = "exec",
        ),
        "_pigz": attr.label(
            default = Label("@sonic_build_infra//tools/build_tools:pigz"),
            executable = True,
            cfg = "exec",
        ),
    },
    outputs = {
        "common": "%{name}/common.tar.gz",
        "first_delta": "%{name}/first.tar.gz",
        "second_delta": "%{name}/second.tar.gz",
        "receipt": "%{name}/receipt.json",
    },
)
