"""Generate only TAR and OCI-directory inputs for the normalizer rule test."""

def _fixture_impl(ctx):
    base = ctx.actions.declare_directory(ctx.label.name + ".base")
    payload = ctx.actions.declare_file(ctx.label.name + ".tar")
    ctx.actions.run(
        executable = ctx.attr._tool[DefaultInfo].files_to_run,
        arguments = ["--fixture", base.path, payload.path],
        outputs = [base, payload],
    )
    return [DefaultInfo(files = depset([base])), OutputGroupInfo(payload = depset([payload]))]

normalization_fixture = rule(
    implementation = _fixture_impl,
    attrs = {"_tool": attr.label(default = Label("//tools/bazel/oci/tests:normalization_fixture_tool"), executable = True, cfg = "exec")},
)
