"""Cache the authoritative Docker exports, individual runtime layers and final boot bundle."""

BootLayerInfo = provider(
    doc = "A verified original layer and its Docker-compatible gzip representation.",
    fields = {"blob": "Original or normalized gzip file.", "receipt": "Validated source and result digests."},
)

def _exec_tool(label):
    return attr.label(default = Label(label), executable = True, cfg = "exec")

def _archive_impl(ctx):
    tree = ctx.actions.declare_directory(ctx.label.name + "/archive")
    info = ctx.actions.declare_file(ctx.label.name + "/image.json")
    args = ctx.actions.args()
    args.add("archive")
    args.add("--src", ctx.file.src)
    args.add("--dest", tree.path)
    args.add("--metadata", info)
    args.add("--pigz", ctx.executable._pigz)
    inputs = [ctx.file.src]
    if ctx.file.receipt and ctx.file.index:
        args.add("--receipt", ctx.file.receipt)
        args.add("--index", ctx.file.index)
        inputs.extend([ctx.file.receipt, ctx.file.index])
    elif ctx.file.receipt or ctx.file.index:
        fail("boot_archive requires both the sidecar receipt and index")
    ctx.actions.run(
        executable = ctx.attr._worker[DefaultInfo].files_to_run,
        arguments = [args],
        inputs = inputs,
        outputs = [tree, info],
        tools = [ctx.attr._pigz[DefaultInfo].files_to_run],
        mnemonic = "SonicBootArchive",
        progress_message = "Checking boot image export %{input}",
    )
    return [DefaultInfo(files = depset([tree])), OutputGroupInfo(metadata = depset([info]))]

boot_archive = rule(
    implementation = _archive_impl,
    attrs = {
        "src": attr.label(allow_single_file = True, mandatory = True),
        "receipt": attr.label(allow_single_file = True),
        "index": attr.label(allow_single_file = True),
        "_worker": _exec_tool(":boot_bundle_action"),
        "_pigz": _exec_tool("@sonic_build_infra//tools/build_tools:pigz"),
    },
)

def _members_impl(ctx):
    args = ctx.actions.args()
    args.use_param_file("@%s")
    args.set_param_file_format("multiline")
    args.add("project")
    args.add("--archive", ctx.file.src.path)
    args.add_all(ctx.outputs.outs, before_each = "--out")
    ctx.actions.run(
        executable = ctx.attr._worker[DefaultInfo].files_to_run,
        arguments = [args],
        inputs = [ctx.file.src],
        outputs = ctx.outputs.outs,
        mnemonic = "SonicBootArchiveMembers",
        progress_message = "Selecting Docker export members for %{label}",
    )
    return [DefaultInfo(files = depset(ctx.outputs.outs))]

boot_archive_members = rule(
    implementation = _members_impl,
    attrs = {
        "src": attr.label(allow_single_file = True, mandatory = True),
        "outs": attr.output_list(mandatory = True),
        "_worker": _exec_tool(":boot_bundle_action"),
    },
)

def _layer_impl(ctx):
    args = ctx.actions.args()
    args.add("layer")
    args.add("--src", ctx.file.src)
    args.add("--source-digest", ctx.attr.source_digest)
    args.add("--source-size", ctx.attr.source_size)
    args.add("--diff-id", ctx.attr.diff_id)
    args.add("--media-type", ctx.attr.media_type)
    receipt = ctx.actions.declare_file(ctx.label.name + "/layer.json")
    args.add("--receipt", receipt)
    outputs = [receipt]
    blob = ctx.file.src
    tools = []
    if not (ctx.attr.media_type.endswith("+gzip") or ctx.attr.media_type.endswith(".gzip")):
        blob = ctx.actions.declare_file(ctx.label.name + "/layer.tar.gz")
        args.add("--out", blob)
        args.add("--pigz", ctx.executable._pigz)
        args.add("--gzip-level", ctx.attr.gzip_level)
        outputs.append(blob)
        tools.append(ctx.attr._pigz[DefaultInfo].files_to_run)
    if ctx.attr.media_type.endswith("+zstd"):
        args.add("--zstd", ctx.executable.zstd)
        tools.append(ctx.attr.zstd[DefaultInfo].files_to_run)
    ctx.actions.run(
        executable = ctx.attr._worker[DefaultInfo].files_to_run,
        arguments = [args],
        inputs = [ctx.file.src],
        outputs = outputs,
        tools = tools,
        mnemonic = "SonicBootLayer",
        progress_message = "Preparing boot image layer %{input}",
    )
    return [DefaultInfo(files = depset([receipt, blob])), BootLayerInfo(blob = blob, receipt = receipt)]

_boot_layer = rule(
    implementation = _layer_impl,
    attrs = {
        "src": attr.label(allow_single_file = True, mandatory = True),
        "source_digest": attr.string(mandatory = True),
        "source_size": attr.string(mandatory = True),
        "diff_id": attr.string(mandatory = True),
        "media_type": attr.string(mandatory = True),
        "gzip_level": attr.int(default = 6, values = range(1, 10)),
        "_worker": _exec_tool(":boot_bundle_action"),
        "_pigz": _exec_tool("@sonic_build_infra//tools/build_tools:pigz"),
        "zstd": attr.label(executable = True, cfg = "exec"),
    },
)

def boot_layer(name, src, source_digest, source_size, diff_id, media_type, gzip_level = 6):
    # A layer can be larger than the largest signed 32-bit Bazel integer attr.
    _boot_layer(
        name = name,
        src = src,
        source_digest = source_digest,
        source_size = str(source_size),
        diff_id = diff_id,
        media_type = media_type,
        gzip_level = gzip_level,
        zstd = Label("@sonic_build_infra//tools/build_tools:zstd") if media_type.endswith("+zstd") else None,
    )

def _bundle_impl(ctx):
    args = ctx.actions.args()
    args.use_param_file("@%s")
    args.set_param_file_format("multiline")
    args.add("bundle")
    args.add("--spec", ctx.file.spec)
    args.add("--platform", ctx.attr.platform)
    args.add("--out", ctx.outputs.bundle)
    args.add("--inventory", ctx.outputs.inventory)
    args.add("--labels", ctx.outputs.labels)
    args.add("--pigz", ctx.executable._pigz)
    args.add_all(ctx.files.configs, before_each = "--config")
    inputs = [ctx.file.spec] + ctx.files.metadata + ctx.files.configs
    for target in ctx.attr.layers:
        layer = target[BootLayerInfo]
        args.add("--layer")
        args.add(layer.receipt)
        args.add(layer.blob)
        inputs.extend([layer.receipt, layer.blob])
    outputs = [ctx.outputs.bundle, ctx.outputs.inventory, ctx.outputs.labels]
    ctx.actions.run(
        executable = ctx.attr._worker[DefaultInfo].files_to_run,
        arguments = [args],
        inputs = depset(inputs),
        outputs = outputs,
        tools = [ctx.attr._pigz[DefaultInfo].files_to_run],
        mnemonic = "SonicBootBundle",
        progress_message = "Assembling offline SONiC boot image bundle",
    )
    return [DefaultInfo(files = depset(outputs))]

boot_bundle = rule(
    implementation = _bundle_impl,
    attrs = {
        "spec": attr.label(allow_single_file = True, mandatory = True),
        "configs": attr.label_list(allow_files = True, mandatory = True),
        "layers": attr.label_list(providers = [BootLayerInfo], mandatory = True),
        "metadata": attr.label_list(allow_files = True),
        "platform": attr.string(mandatory = True),
        "_worker": _exec_tool(":boot_bundle_action"),
        "_pigz": _exec_tool("@sonic_build_infra//tools/build_tools:pigz"),
    },
    outputs = {
        "bundle": "%{name}/bundle.tar.gz",
        "inventory": "%{name}/inventory.json",
        "labels": "%{name}/labels.json",
    },
)
