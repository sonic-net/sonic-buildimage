"""Declared output files for setup.py's py/cvl model rendering."""

def _impl(ctx):
    args = ctx.actions.args()
    args.add("--template", ctx.file.template)
    args.add("--py", ctx.outputs.py)
    args.add("--cvl", ctx.outputs.cvl)
    ctx.actions.run(
        executable = ctx.executable._tool,
        arguments = [args],
        inputs = [ctx.file.template, ctx.file.validation],
        tools = [ctx.attr._tool[DefaultInfo].files_to_run],
        outputs = [ctx.outputs.py, ctx.outputs.cvl],
        env = {"PYTHONHASHSEED": "0"},
        use_default_shell_env = False,
        mnemonic = "RenderYangModel",
    )

render_model = rule(
    implementation = _impl,
    attrs = {
        "validation": attr.label(allow_single_file = True, mandatory = True),
        "template": attr.label(allow_single_file = True),
        "py": attr.output(mandatory = True),
        "cvl": attr.output(mandatory = True),
        "_tool": attr.label(default = Label(":render_model_tool"), executable = True, cfg = "exec"),
    },
)

def _validate_impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".txt")
    args = ctx.actions.args()
    args.add("--validate")
    args.add("--setup", ctx.file.setup)
    args.add_all(ctx.files.sources, before_each = "--source")
    args.add("--receipt", output)
    ctx.actions.run(
        executable = ctx.executable._tool,
        arguments = [args],
        inputs = [ctx.file.setup] + ctx.files.sources,
        tools = [ctx.attr._tool[DefaultInfo].files_to_run],
        outputs = [output],
        use_default_shell_env = False,
        mnemonic = "ValidateYangManifest",
    )
    return [DefaultInfo(files = depset([output]))]

validate_manifest = rule(
    implementation = _validate_impl,
    attrs = {
        "setup": attr.label(allow_single_file = True, mandatory = True),
        "sources": attr.label_list(allow_files = True),
        "_tool": attr.label(default = Label(":render_model_tool"), executable = True, cfg = "exec"),
    },
)
