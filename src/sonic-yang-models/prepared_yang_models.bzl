"""Prepare the production model tree with declared execution inputs."""

_OFFLINE_PYTHON_ENV = {
    "PIP_DISABLE_PIP_VERSION_CHECK": "1",
    "PIP_NO_INDEX": "1",
    "PYTHONDONTWRITEBYTECODE": "1",
    "PYTHONHASHSEED": "0",
    "PYTHONNOUSERSITE": "1",
    "SOURCE_DATE_EPOCH": "315532800",
}

def _prepared_yang_models_impl(ctx):
    output = ctx.actions.declare_directory("yang_models")
    wheel = ctx.actions.declare_file("sonic_yang_models-1.0-py3-none-any.whl")
    args = ctx.actions.args()
    args.add("--setup", ctx.file.setup.path)
    args.add("--readme", ctx.file.readme.path)
    args.add_all(ctx.files.sources, before_each = "--source")
    args.add("--output", output.path)
    args.add("--wheel", wheel.path)
    ctx.actions.run(
        executable = ctx.executable._preparer,
        arguments = [args],
        inputs = depset([ctx.file.setup, ctx.file.readme] + ctx.files.sources),
        outputs = [output, wheel],
        tools = [ctx.attr._preparer[DefaultInfo].files_to_run],
        env = _OFFLINE_PYTHON_ENV,
        execution_requirements = {"block-network": "1"},
        mnemonic = "PrepareYangModels",
        progress_message = "Preparing production YANG models for %{label}",
    )
    return [
        DefaultInfo(files = depset([output, wheel])),
        OutputGroupInfo(models = depset([output]), wheel = depset([wheel])),
    ]

prepared_yang_models = rule(
    implementation = _prepared_yang_models_impl,
    attrs = {
        "readme": attr.label(allow_single_file = True, mandatory = True),
        "setup": attr.label(allow_single_file = [".py"], mandatory = True),
        "sources": attr.label_list(allow_files = True, mandatory = True),
        "_preparer": attr.label(
            default = Label(":prepare_yang_models"),
            executable = True,
            cfg = "exec",
        ),
    },
)
