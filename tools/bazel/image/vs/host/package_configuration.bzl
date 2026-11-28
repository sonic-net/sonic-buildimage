"""Generate configure-time package data from declared installation layers."""
def _impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    audit = ctx.actions.declare_file(ctx.label.name + ".audit.json")
    ca_bundle = ctx.actions.declare_file(ctx.label.name + ".ca-certificates.crt")
    spec = ctx.actions.declare_file(ctx.label.name + ".spec.json")
    layers = ctx.files.layers
    controls = {}
    inputs = [spec, ctx.file.answers, ctx.file._empty] + layers
    for target, package in ctx.attr.controls.items():
        files = target[DefaultInfo].files.to_list()
        if len(files) != 1:
            fail("control input must export one TAR: " + str(target.label))
        controls[package] = files[0].path
        inputs.extend(files)
    ctx.actions.write(spec, json.encode({"layers": [file.path for file in layers], "controls": controls,
                                        "architecture": ctx.attr.architecture, "pam_removed": ctx.attr.pam_removed, "openssl": ctx.executable.openssl.path,
                                        "openssl_config": ctx.file._empty.path, "ca_bundle": ca_bundle.path}))
    ctx.actions.run(executable = ctx.executable._tool,
                    arguments = [spec.path, ctx.file.answers.path, output.path, audit.path],
                    inputs = inputs, tools = [ctx.attr.openssl[DefaultInfo].files_to_run],
                    outputs = [output, audit, ca_bundle], mnemonic = "VsHostPackageConfiguration")
    return [DefaultInfo(files = depset([output])), OutputGroupInfo(audit = depset([audit]), ca_bundle = depset([ca_bundle]))]

host_package_configuration = rule(
    implementation = _impl,
    attrs = {
        "layers": attr.label_list(mandatory = True, allow_files = True),
        "controls": attr.label_keyed_string_dict(allow_files = True),
        "architecture": attr.string(mandatory = True, values = ["amd64", "arm64"]),
        "pam_removed": attr.string_list(),
        "answers": attr.label(default = Label(":debconf_answers.json"), allow_single_file = True),
        "openssl": attr.label(default = Label("@sonic_build_infra//tools/build_tools:openssl"), executable = True, cfg = "exec"),
        "_empty": attr.label(default = Label(":empty_file"), allow_single_file = True),
        "_tool": attr.label(default = Label(":package_configuration_tool"), executable = True, cfg = "exec"),
    },
)
