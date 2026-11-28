"""Crash initramfs configuration and kernel-hook installation artifacts."""
def _impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    audit = ctx.actions.declare_file(ctx.label.name + ".audit.json")
    request = ctx.actions.declare_file(ctx.label.name + ".inputs.json")
    inputs = [ctx.file.kernel_metadata]
    spec = {"kernel_metadata": ctx.file.kernel_metadata.path}
    if ctx.attr.initramfs:
        mode = "installed"
        spec.update(initramfs = ctx.file.initramfs.path, initramfs_audit = ctx.file.initramfs_audit.path, sysctl = ctx.file.sysctl.path)
        inputs.extend([ctx.file.initramfs, ctx.file.initramfs_audit, ctx.file.sysctl])
    else:
        mode = "configuration"
        inputs.extend(ctx.files.layers)
        spec.update(layers = [file.path for file in ctx.files.layers], modules = ctx.attr.modules)
    ctx.actions.write(request, json.encode(spec))
    tool = ctx.attr._tool[DefaultInfo].files_to_run
    ctx.actions.run(executable = tool, arguments = [mode, request.path, output.path, audit.path], inputs = [request] + inputs,
                    outputs = [output, audit], mnemonic = "VsKdump" + mode.capitalize())
    return [DefaultInfo(files = depset([output])), OutputGroupInfo(audit = depset([audit]))]

_kdump = rule(implementation = _impl, attrs = {
    "kernel_metadata": attr.label(mandatory = True, allow_single_file = True),
    "layers": attr.label_list(allow_files = True),
    "modules": attr.string_list(),
    "initramfs": attr.label(allow_single_file = True),
    "initramfs_audit": attr.label(allow_single_file = True),
    "sysctl": attr.label(allow_single_file = True),
    "_tool": attr.label(default = Label(":kdump_tool"), executable = True, cfg = "exec"),
})

def kdump_configuration(name, layers, kernel_metadata, modules = [], **kwargs):
    _kdump(name = name, layers = layers, kernel_metadata = kernel_metadata, modules = modules, **kwargs)

def kdump_installed(name, initramfs, kernel_metadata, sysctl, **kwargs):
    native.filegroup(name = name + "_initramfs_audit", srcs = [initramfs], output_group = "audit")
    _kdump(name = name, initramfs = initramfs, initramfs_audit = ":" + name + "_initramfs_audit", kernel_metadata = kernel_metadata, sysctl = sysctl, **kwargs)
