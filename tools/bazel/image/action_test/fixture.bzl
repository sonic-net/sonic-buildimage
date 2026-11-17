"""Declared fixture inputs for the public image rule action test."""

def _fixture_impl(ctx):
    trees = {name: ctx.actions.declare_directory(ctx.label.name + "." + name) for name in ["boot_files", "oci"]}
    files = {name: ctx.actions.declare_file(ctx.label.name + "." + name) for name in [
        "arch_image_conf", "default_platform_conf", "efi_sbatlevel", "grub_config",
        "initrd", "install_sh", "kernel_metadata", "machine_conf", "onie_image_conf",
        "partition_layout", "platforms_asic", "sharch_body", "version",
    ]}
    files.update({
        "layer": ctx.actions.declare_file(ctx.label.name + ".layer.tar"),
        "onie_boot": ctx.actions.declare_file(ctx.label.name + ".onie-boot.tar"),
        "platform_archive": ctx.actions.declare_file(ctx.label.name + ".platform.tar"),
    })
    outputs = files | trees
    request = ctx.actions.declare_file(ctx.label.name + ".fixture.json")
    ctx.actions.write(request, json.encode({
        "architecture": ctx.attr.architecture,
        "outputs": {name: output.path for name, output in outputs.items()},
    }))
    ctx.actions.run(
        executable = ctx.attr._generator[DefaultInfo].files_to_run,
        arguments = [request.path],
        inputs = [request],
        outputs = outputs.values(),
        mnemonic = "ImageActionFixture",
    )
    return [
        DefaultInfo(files = depset(outputs.values())),
        OutputGroupInfo(**{name: depset([output]) for name, output in outputs.items()}),
    ]

image_action_fixture = rule(
    implementation = _fixture_impl,
    attrs = {
        "architecture": attr.string(mandatory = True, values = ["amd64", "arm64"]),
        "_generator": attr.label(default = Label(":fixture_generator"), executable = True, cfg = "exec"),
    },
)
