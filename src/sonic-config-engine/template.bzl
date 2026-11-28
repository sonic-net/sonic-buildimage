"""Render declared build-time templates with the source-owned sonic-cfggen."""

load("@bazel_skylib//rules:run_binary.bzl", "run_binary")

def sonic_cfggen_template(name, template, out, additional_data = {}, srcs = [], **kwargs):
    """Run sonic-cfggen with explicit data and declared template/include inputs.

    The standard run_binary rule selects the command and its native libraries in
    the execution configuration. Additional data is JSON, not a shell command.
    Device-dependent templates remain inputs to the installed runtime command.
    """
    run_binary(
        name = name,
        srcs = [template] + srcs,
        outs = [out],
        args = [
            "-a",
            # Skylib preserves each argument without shell tokenization, keeping
            # JSON escapes and literal shell punctuation intact.
            json.encode(additional_data),
            "-t",
            "$(location %s),$(location :%s)" % (template, out),
        ],
        env = {
            "PLATFORM": "sonic-bazel-build",
            "NAMESPACE_ID": "",
        },
        tool = Label("//src/sonic-config-engine:sonic_cfggen"),
        **kwargs
    )
