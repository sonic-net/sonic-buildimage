"""Apply this component's Cargo release profile to its deployable binary."""

_LTO = str(Label("@rules_rust//rust/settings:lto"))
_CODEGEN_UNITS = str(Label("@rules_rust//rust/settings:codegen_units"))

def _release_impl(_settings, _attr):
    return {
        "//command_line_option:compilation_mode": "opt",
        _LTO: "fat",
        _CODEGEN_UNITS: 1,
    }

_release = transition(
    implementation = _release_impl,
    inputs = [],
    outputs = ["//command_line_option:compilation_mode", _LTO, _CODEGEN_UNITS],
)

def _release_binary_impl(ctx):
    info = ctx.attr.binary[0][DefaultInfo]
    return [DefaultInfo(
        files = info.files,
        default_runfiles = info.default_runfiles,
        data_runfiles = info.data_runfiles,
    )]

release_binary = rule(
    implementation = _release_binary_impl,
    attrs = {
        "binary": attr.label(mandatory = True, allow_single_file = True, cfg = _release),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)
