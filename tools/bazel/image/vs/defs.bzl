load("//tools/bazel/image:defs.bzl", "docker_overlay2_store", "onie_installer", "sonic_initramfs", "sonic_squashfs")
load(":kdump.bzl", "kdump_configuration", "kdump_installed")
"""Ordered VS host installation effects shared by initramfs and SquashFS."""
load("//tools/bazel/image/vs/host:accounts.bzl", "host_accounts")
load("//tools/bazel/image/vs/host:configuration.bzl", "vs_host_configuration")
load("//tools/bazel/image/vs/host:ldcache.bzl", "host_ldcache")
load("//tools/bazel/image/vs/host:maintainer_services.bzl", "host_maintainer_services")
load("//tools/bazel/image/vs/host:package_configuration.bzl", "host_package_configuration")
load("//tools/bazel/image/vs/host:python_cleanup.bzl", "host_python_cleanup")
load("@sonic_build_infra//oci:source_controls.bzl", "source_package_controls")
load(":dpkg_state.bzl", "host_dpkg_configured", "host_dpkg_unpacked")

def vs_host_layers(name, apt_packages, source_payloads, source_metadata, direct_controls, fips_payloads, fips_controls, python_runtime, kernel_runtime, kernel_metadata, kernel_control, containers, local_images, images, image_version, build_values, build_identity = None, debug = False):
    """Declare installation/configuration once; return layers, unpacked state and effects.

    Caller owns the complete source inventory and exact build identity. Missing
    producers remain ordinary Bazel dependency errors rather than empty layers.
    """
    by_package = {package: "@vs_host_debian//" + package + ":control" for package in apt_packages}
    by_package.update({package: label for label, package in direct_controls.items()})
    for metadata, packages in source_metadata.items():
        for package in packages:
            target = name + "_control_" + package
            source_package_controls(name = target, metadata = metadata, package = package)
            by_package[package] = ":" + target
    by_package.update({package: label for label, package in fips_controls.items()})
    controls = {label: package for package, label in by_package.items()}
    installed = {package: True for package in apt_packages + source_payloads.keys() + fips_payloads.keys()}
    final_payloads = {package: "@vs_host_debian//" + package + ":raw_data" for package in apt_packages}
    final_payloads.update(source_payloads)
    final_payloads.update(fips_payloads)
    package_pairs = {package: [payload, by_package[package]] for package, payload in final_payloads.items()}
    package_pairs[""] = [kernel_runtime, kernel_control]
    host_dpkg_unpacked(name = name + "_unpacked", packages = package_pairs, layout = "//tools/bazel/image/vs/host:merged_usr_skeleton")
    payloads = ["//tools/bazel/image/vs/host:merged_usr_skeleton", "@vs_host_debian//:raw_packages"]
    payloads += source_payloads.values() + [python_runtime] + fips_payloads.values()
    payloads += [kernel_runtime, ":" + name + "_unpacked", "//tools/bazel/image/vs/host:static_configuration"]
    host_accounts(
        name = name + "_accounts",
        data = {final_payloads[package]: package for package in ["base-passwd", "base-files", "cron", "dbus", "chrony", "openssh-client", "openssh-server", "ifupdown2"] if package in final_payloads},
        controls = controls,
        installed = installed.keys(),
        username = "admin",
        password = "YourPaSsWoRd",
        change_default_password = False,
        lastchg_day = int(build_values["lastchg_day"]),
        mkpasswd = "@sonic_build_infra//tools/build_tools:mkpasswd",
    )
    layers = payloads + [":" + name + "_accounts"]
    host_package_configuration(name = name + "_package_configuration", layers = layers, controls = controls, architecture = "amd64", pam_removed = ["tacplus", "ldap"])
    layers += [":" + name + "_package_configuration", "//tools/bazel/image/vs/host:host_locale"]
    host_maintainer_services(name = name + "_services", layers = layers, controls = controls.keys())
    layers += [":" + name + "_services"]
    vs_host_configuration(name = name + "_configuration", containers = containers, image_version = image_version, kernel_metadata = kernel_metadata,
                          base_layers = layers, local_images = local_images, image_tags = images, build_values = build_values, build_identity = build_identity, debug = debug)
    layers += [":" + name + "_configuration"]
    host_ldcache(name = name + "_ldcache", layers = layers, architecture = "amd64")
    layers += [":" + name + "_ldcache"]
    host_python_cleanup(name = name + "_python_cleanup", layers = layers, distributions = ["enum34"], cache_tag = "cpython-313")
    layers += [":" + name + "_python_cleanup"]
    effects = [":" + name + suffix for suffix in ["_accounts", "_package_configuration", "_services", "_configuration", "_ldcache", "_python_cleanup"]]
    effects += ["//tools/bazel/image/vs/host:host_locale"]
    return struct(layers = layers, unpacked = ":" + name + "_unpacked", effects = effects)

def sonic_vs_image(name, apt_packages, source_payloads, source_metadata, direct_controls, fips_payloads, fips_controls, python_runtime, images, local_images, debug, image_version, build_identity, build_values, platform_archive, platforms_asic):
    """Default mainline VS/AMD64/Trixie/FIPS/DASH ONIE installer assembly."""
    host = vs_host_layers(
        name = "host",
        apt_packages = apt_packages,
        source_payloads = source_payloads,
        source_metadata = source_metadata,
        direct_controls = direct_controls,
        fips_payloads = fips_payloads,
        fips_controls = fips_controls,
        python_runtime = python_runtime,
        kernel_runtime = "//tools/bazel/image/kernel:runtime",
        kernel_control = "//tools/bazel/image/kernel:control",
        kernel_metadata = "//tools/bazel/image/kernel:metadata",
        containers = "//tools/bazel/image/vs/metadata:containers.json",
        local_images = local_images,
        images = images,
        image_version = image_version,
        build_values = build_values,
        build_identity = build_identity,
        debug = debug,
    )
    layers = host.layers
    sonic_initramfs(
        name = "initramfs",
        package_layers = layers,
        layers = [],
        kernel_metadata = "//tools/bazel/image/kernel:metadata",
        busybox_applets = "//tools/bazel/image/vs/host:busybox_applets",
        modprobe = "@sonic_build_infra//tools/build_tools:modprobe",
        depmod = "@sonic_build_infra//tools/build_tools:depmod",
        zstd = "@sonic_build_infra//tools/build_tools:zstd",
        compression = "zstd",
        out = "initrd.img",
    )
    native.filegroup(name = "module_indexes", srcs = [":initramfs"], output_group = "module_indexes")
    native.filegroup(name = "kernel_links", srcs = [":initramfs"], output_group = "kernel_links")
    crash_layers = layers + [":module_indexes", ":kernel_links"]
    kdump_configuration(name = "kdump_configuration", layers = crash_layers, kernel_metadata = "//tools/bazel/image/kernel:metadata")
    sonic_initramfs(
        name = "kdump_initramfs",
        package_layers = crash_layers + [":kdump_configuration"],
        layers = [],
        kernel_metadata = "//tools/bazel/image/kernel:metadata",
        config_dir = "/var/lib/kdump/initramfs-tools",
        busybox_applets = "//tools/bazel/image/vs/host:busybox_applets",
        modprobe = "@sonic_build_infra//tools/build_tools:modprobe",
        depmod = "@sonic_build_infra//tools/build_tools:depmod",
        zstd = "@sonic_build_infra//tools/build_tools:zstd",
        compression = "zstd",
        out = "kdump-initrd.img",
    )
    kdump_installed(name = "kdump_installed", initramfs = ":kdump_initramfs", kernel_metadata = "//tools/bazel/image/kernel:metadata", sysctl = "@kdump_tools//:sysctl.conf")
    host_dpkg_configured(name = "host_installed", unpacked = host.unpacked,
                         effects = host.effects + [":module_indexes", ":kernel_links", ":kdump_configuration", ":kdump_installed"])
    sonic_squashfs(
        name = "squashfs",
        layers = layers + [":module_indexes", ":kernel_links", ":kdump_configuration", ":kdump_installed", ":host_installed", "//tools/bazel/image/vs/host:final_cleanup"],
        excludes = ["boot", "var/lib/docker", "platform"],
        compression = "zstd",
        block_size = 1048576,
        out = "sonic-vs.squashfs",
        mksquashfs = "@sonic_build_infra//tools/build_tools:mksquashfs",
    )
    docker_overlay2_store(
        name = "docker_store",
        images = images,
        platform = "linux/amd64",
        version_file = image_version,
        tar_split_json = "//tools/bazel/image/tar_split_json:tar_split_json",
        compression = "none",
        out = "dockerfs.tar",
    )
    onie_installer(
        name = name,
        squashfs = ":squashfs",
        boot_files = "//tools/bazel/image/kernel:boot",
        kernel_metadata = "//tools/bazel/image/kernel:metadata",
        initrd = ":initramfs",
        platform_archive = platform_archive,
        dockerfs = ":docker_store",
        version_file = image_version,
        dockerfs_compression = "none",
        platform_compression = "none",
        install_sh = "//:installer/install.sh",
        sharch_body = "//:installer/sharch_body.sh",
        machine_conf = ":machine_conf",
        default_platform_conf = "//:installer/default_platform.conf",
        onie_image_conf = "//:onie-image.conf",
        platforms_asic = platforms_asic,
        efi_sbatlevel = "//:installer/efi_sbatlevel.py",
        installer_variables = {"ARCH": "amd64", "DEMO_TYPE": "OS", "ONIE_IMAGE_PART_SIZE": "32768", "EXTRA_CMDLINE_LINUX": "", "OUTPUT_RAW_IMAGE": "target/sonic-vs.raw"},
        signing_mode = "no_sign",
        out = name,
    )
