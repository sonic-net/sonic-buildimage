"""Cache the stock, unpersonalized Trixie VS host after package configuration."""

def _configured_host(ctx):
    image = ctx.actions.declare_file("configured-host.squashfs")
    manifest = ctx.actions.declare_file("configured-host.json")
    ctx.actions.run(
        executable = ctx.file.driver,
        inputs = [ctx.file.identity, ctx.file.common],
        outputs = [image, manifest],
        arguments = ["--action", ctx.file.identity.path, image.path, manifest.path],
        mnemonic = "SonicVsConfiguredHost",
        progress_message = "Configuring the VS host packages and cleaning build dependencies",
        execution_requirements = {"no-remote-exec": "1"},
    )
    return [DefaultInfo(files = depset([image, manifest]))]

vs_configured_host = rule(
    implementation = _configured_host,
    attrs = {
        "common": attr.label(allow_single_file = True, mandatory = True),
        "driver": attr.label(allow_single_file = True, mandatory = True),
        "identity": attr.label(allow_single_file = True, mandatory = True),
    },
)
