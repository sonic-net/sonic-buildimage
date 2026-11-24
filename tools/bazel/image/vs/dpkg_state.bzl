"""Two-phase dpkg state from selected package controls and original payloads."""
def _single(target):
    files = target[DefaultInfo].files.to_list()
    if len(files) != 1:
        fail("dpkg package input must provide one raw TAR: " + str(target.label))
    return files[0]

def _impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    receipt = ctx.actions.declare_file(ctx.label.name + ".receipt.json")
    request = ctx.actions.declare_file(ctx.label.name + ".inputs.json")
    inputs = []
    if ctx.attr.unpacked:
        unpacked = _single(ctx.attr.unpacked)
        effects = ctx.files.effects
        spec = {"unpacked": unpacked.path, "effects": [file.path for file in effects]}
        inputs = [unpacked] + effects
    else:
        packages = []
        for expected, payload, control in zip(ctx.attr.expected, ctx.attr.payloads, ctx.attr.controls):
            payload_file, control_file = _single(payload), _single(control)
            packages.append([expected, payload_file.path, control_file.path])
            inputs.extend([payload_file, control_file])
        spec = {"packages": packages}
        if ctx.file.layout:
            spec["layout"] = ctx.file.layout.path
            inputs.append(ctx.file.layout)
    ctx.actions.write(request, json.encode(spec))
    tool = ctx.attr._tool[DefaultInfo].files_to_run
    ctx.actions.run(executable = tool, arguments = [request.path, output.path, receipt.path], inputs = [request] + inputs,
                    outputs = [output, receipt], mnemonic = "VsDpkgState")
    return [DefaultInfo(files = depset([output])), OutputGroupInfo(receipt = depset([receipt]))]

_state = rule(implementation = _impl, attrs = {
    "expected": attr.string_list(),
    "payloads": attr.label_list(allow_files = True),
    "controls": attr.label_list(allow_files = True),
    "unpacked": attr.label(allow_single_file = True),
    "effects": attr.label_list(allow_files = True),
    "layout": attr.label(allow_single_file = True),
    "_tool": attr.label(default = Label(":dpkg_state_tool"), executable = True, cfg = "exec"),
})

def host_dpkg_unpacked(name, packages, **kwargs):
    """packages maps expected name (or empty for producer identity) to [TAR, control]."""
    _state(name = name, expected = packages.keys(), payloads = [pair[0] for pair in packages.values()], controls = [pair[1] for pair in packages.values()], **kwargs)

def host_dpkg_configured(name, unpacked, effects, **kwargs):
    _state(name = name, unpacked = unpacked, effects = effects, **kwargs)
