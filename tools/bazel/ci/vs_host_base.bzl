def _host_base(ctx):
    image = ctx.actions.declare_file("host.squashfs")
    inventory = ctx.actions.declare_file("host.inventory")
    manifest = ctx.actions.declare_file("host.manifest")
    ctx.actions.run(
        executable = ctx.file.driver,
        inputs = [ctx.file.identity],
        outputs = [image, inventory, manifest],
        arguments = ["--action", ctx.file.identity.path, image.path, inventory.path, manifest.path],
        mnemonic = "SonicVsHostBase",
        progress_message = "Building the VS host filesystem in the native Debian build slave",
        execution_requirements = {"no-remote-exec": "1"},
    )
    return [DefaultInfo(files = depset([image, inventory, manifest]))]

vs_host_base = rule(
    implementation = _host_base,
    attrs = {
        "driver": attr.label(allow_single_file = True, mandatory = True),
        "identity": attr.label(allow_single_file = True, mandatory = True),
    },
)
