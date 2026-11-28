"""Generate the VS host's selected configuration from declared product inputs."""

load(":profile.bzl", "HOST_GENERATED_SERVICES", "VS_CONTEXT")

def _single(target):
    files = target[DefaultInfo].files.to_list()
    if len(files) != 1:
        fail("%s must provide one declared artifact" % target.label)
    return files[0]

def _configuration_impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    audit = ctx.actions.declare_file(ctx.label.name + ".audit.json")
    request = ctx.actions.declare_file(ctx.label.name + ".inputs.json")
    inputs = [ctx.file.containers, ctx.file.image_version, ctx.file.kernel_metadata]
    layers = list(ctx.files.base_layers)
    inputs.extend(layers)
    layouts = {}
    for target, repository in ctx.attr.local_images.items():
        layout = _single(target)
        layouts[repository] = layout.path
        inputs.append(layout)
        inputs.extend(target[DefaultInfo].default_runfiles.files.to_list())
    image_tags = {target.label: tags for target, tags in ctx.attr.image_tags.items()}
    image_repositories = {
        repository: image_tags[target.label].split(",")[0].rsplit(":", 1)[0]
        for target, repository in ctx.attr.local_images.items()
    }
    templates = {}
    for file in ctx.files._templates:
        name = file.short_path
        if "sonic-utilities-data/templates/" in name:
            name = "sonic-utilities-data/templates/" + file.basename
        templates[name] = file.path
    units = {file.basename: file.path for file in ctx.files._units}
    inputs.extend(ctx.files._templates + ctx.files._units + ctx.files._modules + ctx.files._chrony_override)
    if ctx.file.build_identity:
        inputs.append(ctx.file.build_identity)
    context = dict(VS_CONTEXT)
    context.update(ctx.attr.build_values)
    ctx.actions.write(request, json.encode({
        "containers": ctx.file.containers.path,
        "image_version": ctx.file.image_version.path,
        "kernel_metadata": ctx.file.kernel_metadata.path,
        "base_layers": [file.path for file in layers],
        "local_images": layouts,
        "image_repositories": image_repositories,
        "templates": templates,
        "units": units,
        "modules": ctx.file._modules.path,
        "chrony_override": ctx.file._chrony_override.path,
        "context": context,
        "build_identity": ctx.file.build_identity.path if ctx.file.build_identity else None,
        "debug": ctx.attr.debug,
        "generated_services": HOST_GENERATED_SERVICES,
        "out": output.path,
        "audit": audit.path,
    }) + "\n")
    tool = ctx.attr._tool[DefaultInfo].files_to_run
    ctx.actions.run(
        executable = tool,
        arguments = [request.path],
        inputs = depset([request] + inputs),
        tools = [tool],
        outputs = [output, audit],
        env = {"LC_ALL": "C", "TZ": "UTC", "PYTHONHASHSEED": "0"},
        mnemonic = "VsHostConfiguration",
        progress_message = "Generating declared VS host configuration",
    )
    return [DefaultInfo(files = depset([output])), OutputGroupInfo(audit = depset([audit]))]

vs_host_configuration = rule(
    implementation = _configuration_impl,
    attrs = {
        "containers": attr.label(allow_single_file = [".json"], mandatory = True),
        "image_version": attr.label(allow_single_file = True, mandatory = True),
        "kernel_metadata": attr.label(allow_single_file = [".json"], mandatory = True),
        "base_layers": attr.label_list(mandatory = True, allow_files = True),
        "local_images": attr.label_keyed_string_dict(),
        "image_tags": attr.label_keyed_string_dict(mandatory = True),
        "build_values": attr.string_dict(mandatory = True),
        "build_identity": attr.label(allow_single_file = [".json"]),
        "debug": attr.bool(default = False),
        "_templates": attr.label(default = Label(":configuration_templates")),
        "_units": attr.label(default = Label(":host_units")),
        "_chrony_override": attr.label(default = Label("//files/image_config:chrony/override.conf"), allow_single_file = True),
        "_modules": attr.label(default = Label("//files/initramfs-tools:modules"), allow_single_file = True),
        "_tool": attr.label(default = Label(":configuration_tool"), executable = True, cfg = "exec"),
    },
)
