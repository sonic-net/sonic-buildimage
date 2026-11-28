"""Five image assembly rules consuming declared artifacts and execution tools."""

load(":initramfs.bzl", _sonic_initramfs = "sonic_initramfs")

sonic_initramfs = _sonic_initramfs


_IMAGE_DEFAULTS = {
    "payload_name": "fs.zip",
    "squashfs_name": "fs.squashfs",
    "dockerfs_name": "dockerfs.tar.gz",
    "docker_directory": "docker",
}

def _image_spec(ctx):
    return {name: getattr(ctx.attr, name) for name in _IMAGE_DEFAULTS} | {"docker_mode": ctx.attr.docker_mode}

_IMAGE_ATTRS = {name: attr.string(default = value) for name, value in _IMAGE_DEFAULTS.items()} | {
    "docker_mode": attr.string(default = "disk", values = ["disk", "ram"]),
}

def _single(target):
    files = target[DefaultInfo].files.to_list()
    if len(files) != 1:
        fail("%s must provide exactly one file or directory" % target.label)
    return files[0]

def _squashfs_impl(ctx):
    out = ctx.actions.declare_file(ctx.attr.out)
    args = ctx.actions.args()
    args.add("--mksquashfs", ctx.executable.mksquashfs)
    args.add("--out", out)
    args.add_all(ctx.attr.excludes, format_each = "--exclude=%s")
    args.add_all(["-comp", ctx.attr.compression, "-b", str(ctx.attr.block_size)], format_each = "--mksquashfs-arg=%s")
    args.add_all(ctx.files.layers)
    ctx.actions.run(
        executable = ctx.attr._merge[DefaultInfo].files_to_run,
        arguments = [args],
        inputs = ctx.files.layers,
        tools = [ctx.attr.mksquashfs[DefaultInfo].files_to_run],
        outputs = [out],
        mnemonic = "SonicSquashfs",
    )
    return [DefaultInfo(files = depset([out]))]

sonic_squashfs = rule(
    implementation = _squashfs_impl,
    attrs = {
        "layers": attr.label_list(allow_files = [".tar", ".tar.gz", ".tar.xz"], mandatory = True),
        "excludes": attr.string_list(),
        "compression": attr.string(mandatory = True),
        "block_size": attr.int(mandatory = True),
        "out": attr.string(mandatory = True),
        "mksquashfs": attr.label(mandatory = True, executable = True, cfg = "exec"),
        "_merge": attr.label(default = Label(":merge_layers"), executable = True, cfg = "exec"),
    },
)

def _spec_action(ctx, spec, inputs, tools, mnemonic):
    out = ctx.actions.declare_file(ctx.attr.out)
    spec["out"] = out.path
    request = ctx.actions.declare_file(ctx.label.name + ".inputs.json")
    ctx.actions.write(request, json.encode(spec))
    ctx.actions.run(
        executable = ctx.attr._tool[DefaultInfo].files_to_run,
        arguments = [request.path],
        inputs = depset([request] + inputs),
        tools = tools,
        outputs = [out],
        mnemonic = mnemonic,
    )
    return [DefaultInfo(files = depset([out]))]

def _docker_store_impl(ctx):
    if ctx.attr.compression == "zstd" and not ctx.attr.zstd:
        fail("zstd compression requires zstd")
    images = []
    inputs = []
    for target, tags in ctx.attr.images.items():
        layout = _single(target)
        images.append([layout.path, tags])
        inputs.append(layout)
        # Producers may expose referenced blobs via runfiles. A complete layout
        # tree or these declared files must cover every symlink in that layout.
        inputs.extend(target[DefaultInfo].default_runfiles.files.to_list())
    spec = {"images": images, "platform": ctx.attr.platform, "tar_split_json": ctx.executable.tar_split_json.path, "compression": ctx.attr.compression, "pigz": ctx.executable.pigz.path}
    tools = [ctx.attr.tar_split_json[DefaultInfo].files_to_run, ctx.attr.pigz[DefaultInfo].files_to_run]
    if ctx.attr.zstd:
        spec["zstd"] = ctx.executable.zstd.path
        tools.append(ctx.attr.zstd[DefaultInfo].files_to_run)
    if ctx.file.version_file:
        spec["version_file"] = ctx.file.version_file.path
        inputs.append(ctx.file.version_file)
    return _spec_action(ctx, spec, inputs, tools, "DockerOverlay2Store")

docker_overlay2_store = rule(
    implementation = _docker_store_impl,
    attrs = {
        "images": attr.label_keyed_string_dict(mandatory = True),
        "tar_split_json": attr.label(mandatory = True, executable = True, cfg = "exec"),
        "compression": attr.string(default = "gzip", values = ["none", "gzip", "zstd"]),
        "version_file": attr.label(allow_single_file = True),
        "platform": attr.string(mandatory = True),
        "out": attr.string(mandatory = True),
        "pigz": attr.label(default = "@sonic_build_infra//tools/build_tools:pigz", executable = True, cfg = "exec"),
        "zstd": attr.label(executable = True, cfg = "exec"),
        "_tool": attr.label(default = Label(":overlay2_store"), executable = True, cfg = "exec"),
    },
)

def _onie_impl(ctx):
    spec = _image_spec(ctx)
    inputs = []
    for name in ["squashfs", "boot_files", "kernel_metadata", "initrd", "platform_archive", "dockerfs", "version_file", "install_sh", "sharch_body", "machine_conf", "default_platform_conf", "onie_image_conf", "arch_image_conf", "platforms_asic", "efi_sbatlevel", "platform_conf", "signing_cert", "signing_key"]:
        target = getattr(ctx.attr, name)
        if target:
            file = _single(target)
            if name == "boot_files" and not file.is_directory:
                fail("boot_files must provide the kernel producer's tree artifact")
            spec[name] = file.path
            inputs.append(file)
    spec.update({"installer_variables": ctx.attr.installer_variables, "dockerfs_compression": ctx.attr.dockerfs_compression, "platform_compression": ctx.attr.platform_compression, "boot_file_modes": ctx.attr.boot_file_modes, "signing_mode": ctx.attr.signing_mode})
    tools = []
    for name in ["openssl", "signing_tool", "pigz"]:
        target = getattr(ctx.attr, name)
        if target:
            spec[name] = getattr(ctx.executable, name).path
            tools.append(target[DefaultInfo].files_to_run)
    if ctx.attr.signing_mode == "dev" and not (ctx.attr.openssl and ctx.attr.signing_cert and ctx.attr.signing_key):
        fail("dev signing requires openssl, signing_cert and signing_key")
    if ctx.attr.signing_mode == "prod" and not ctx.attr.signing_tool:
        fail("prod signing requires signing_tool")
    spec["signing_tool_args"] = [ctx.expand_location(arg, targets = ctx.attr.signing_inputs) for arg in ctx.attr.signing_tool_args]
    inputs.extend(ctx.files.signing_inputs)
    spec["extra_files"] = []
    spec["extra_file_modes"] = ctx.attr.extra_file_modes
    for target, dest in ctx.attr.extra_files.items():
        file = _single(target)
        spec["extra_files"].append([file.path, dest])
        inputs.append(file)
    return _spec_action(ctx, spec, inputs, tools, "OnieInstaller")

onie_installer = rule(
    implementation = _onie_impl,
    attrs = _IMAGE_ATTRS | {
        name: attr.label(mandatory = name not in ["dockerfs", "version_file", "platform_conf", "arch_image_conf", "signing_cert", "signing_key"], allow_single_file = True)
        for name in ["squashfs", "boot_files", "kernel_metadata", "initrd", "platform_archive", "dockerfs", "version_file", "install_sh", "sharch_body", "machine_conf", "default_platform_conf", "onie_image_conf", "arch_image_conf", "platforms_asic", "efi_sbatlevel", "platform_conf", "signing_cert", "signing_key"]
    } | {
        "extra_files": attr.label_keyed_string_dict(allow_files = True),
        "extra_file_modes": attr.string_dict(),
        "installer_variables": attr.string_dict(mandatory = True),
        "dockerfs_compression": attr.string(default = "gzip", values = ["gzip", "none", "zstd"]),
        "platform_compression": attr.string(default = "gzip", values = ["gzip", "none"]),
        "boot_file_modes": attr.string_dict(),
        "signing_mode": attr.string(default = "no_sign", values = ["no_sign", "dev", "prod"]),
        "openssl": attr.label(executable = True, cfg = "exec"),
        "signing_tool": attr.label(executable = True, cfg = "exec"),
        "signing_inputs": attr.label_list(allow_files = True),
        "signing_tool_args": attr.string_list(default = ["{input}", "{output}", "{mode}"]),
        "out": attr.string(mandatory = True),
        "pigz": attr.label(default = "@sonic_build_infra//tools/build_tools:pigz", executable = True, cfg = "exec"),
        "_tool": attr.label(default = Label(":onie"), executable = True, cfg = "exec"),
    },
)

def _disk_impl(ctx):
    spec = {"disk_size_mib": ctx.attr.disk_size_mib, "sparse_threshold_bytes": ctx.attr.sparse_threshold_bytes, "grub_assets_subdir": ctx.attr.grub_assets_subdir, "uefi_mode": ctx.attr.uefi_mode, "efi_bootloader": ctx.attr.efi_bootloader, "boot_file_modes": ctx.attr.boot_file_modes}
    spec.update(_image_spec(ctx))
    spec["docker_store_compression"] = ctx.attr.docker_store_compression
    if ctx.attr.docker_store and ctx.attr.docker_mode == "disk" and ctx.attr.docker_store_compression == "zstd" and not ctx.attr.zstd:
        fail("zstd Docker extraction requires zstd")
    inputs, tools = [], []
    for name in ["squashfs", "boot_files", "kernel_metadata", "initrd", "docker_store", "onie_boot", "grub_assets", "grub_config", "machine_conf", "image_version_file", "partition_layout"]:
        target = getattr(ctx.attr, name)
        if target:
            file = _single(target)
            if name in ["boot_files", "grub_assets"] and not file.is_directory:
                fail(name + " must provide a tree artifact")
            spec[name] = file.path
            inputs.append(file)
    spec["efi_files"] = {}
    if ctx.attr.uefi_mode == "signed" and ctx.attr.efi_bootloader not in ctx.attr.efi_files:
        fail("signed UEFI requires efi_bootloader to name an efi_files destination")
    for destination, target in ctx.attr.efi_files.items():
        file = _single(target)
        spec["efi_files"][destination] = file.path
        inputs.append(file)
    spec["platform_layers"] = [f.path for f in ctx.files.platform_layers]
    inputs.extend(ctx.files.platform_layers)
    for name in ["mke2fs", "debugfs", "sgdisk", "qemu_img", "grub_mkimage", "grub_bios_setup", "grub_editenv", "mkfs_fat", "mcopy", "mmd", "zstd", "pigz"]:
        target = getattr(ctx.attr, name)
        if target:
            spec[name] = getattr(ctx.executable, name).path
            tools.append(target[DefaultInfo].files_to_run)
    return _spec_action(ctx, spec, inputs, tools, "SonicDiskImage")

sonic_disk_image = rule(
    implementation = _disk_impl,
    attrs = _IMAGE_ATTRS | {
        name: attr.label(mandatory = name != "docker_store", allow_single_file = True)
        for name in ["squashfs", "boot_files", "kernel_metadata", "initrd", "docker_store", "grub_assets", "grub_config", "machine_conf", "image_version_file", "partition_layout"]
    } | {
        name: attr.label(mandatory = name in ["mke2fs", "debugfs", "sgdisk", "qemu_img", "grub_editenv"], executable = True, cfg = "exec")
        for name, executable in {
            "mke2fs": "mke2fs", "debugfs": "debugfs", "sgdisk": "sgdisk", "qemu_img": "qemu-img",
            "grub_mkimage": "grub-mkimage", "grub_bios_setup": "grub-bios-setup", "grub_editenv": "grub-editenv", "mkfs_fat": "mkfs.fat", "mcopy": "mcopy", "mmd": "mmd",
        }.items()
    } | {
        "onie_boot": attr.label(mandatory = True, allow_single_file = [".tar", ".tar.gz", ".tar.xz"]),
        "platform_layers": attr.label_list(allow_files = [".tar", ".tar.gz", ".tar.xz"]),
        "grub_assets_subdir": attr.string(),
        "uefi_mode": attr.string(default = "grub", values = ["grub", "signed"]),
        "boot_file_modes": attr.string_dict(),
        "efi_files": attr.string_keyed_label_dict(allow_files = True),
        "efi_bootloader": attr.string(),
        "disk_size_mib": attr.int(mandatory = True),
        "sparse_threshold_bytes": attr.int(mandatory = True),
        "docker_store_compression": attr.string(default = "gzip", values = ["none", "gzip", "zstd"]),
        "zstd": attr.label(executable = True, cfg = "exec"),
        "out": attr.string(mandatory = True),
        "pigz": attr.label(default = "@sonic_build_infra//tools/build_tools:pigz", executable = True, cfg = "exec"),
        "_tool": attr.label(default = Label(":disk_image"), executable = True, cfg = "exec"),
    },
)
