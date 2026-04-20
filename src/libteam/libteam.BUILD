load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_shared_library")
load("@rules_deb//distroless:defs.bzl", "flatten")
load("@tar.bzl", "mutate", "tar")

# From configure.ac: AC_SUBST(LIBTEAM_{CURRENT,REVISION,AGE}, ...)
LIBTEAM_CURRENT = 11
LIBTEAM_REVISION = 1
LIBTEAM_AGE = 6

# From libteamdctl/Makefile.am: libteamdctl_la_LDFLAGS = -version-info 1:5:1
LIBTEAMDCTL_CURRENT = 1
LIBTEAMDCTL_REVISION = 5
LIBTEAMDCTL_AGE = 1

# Libtool soname = CURRENT - AGE; full = soname.AGE.REVISION
LIBTEAM_SOVERSION = LIBTEAM_CURRENT - LIBTEAM_AGE
LIBTEAM_FULL_VERSION = "{}.{}.{}".format(LIBTEAM_SOVERSION, LIBTEAM_AGE, LIBTEAM_REVISION)

LIBTEAMDCTL_SOVERSION = LIBTEAMDCTL_CURRENT - LIBTEAMDCTL_AGE
LIBTEAMDCTL_FULL_VERSION = "{}.{}.{}".format(LIBTEAMDCTL_SOVERSION, LIBTEAMDCTL_AGE, LIBTEAMDCTL_REVISION)

# TODO(bazel-ready): Split PIC/non-PIC targets if static libraries are needed.
# -fPIC is not in the upstream Makefile.am — libtool adds it automatically when
# building shared libraries. We add it explicitly because Bazel doesn't use
# libtool. debian/rules disables static building when building debian packages,
# so this should be fine for now.
IMPLICIT_LIBTOOL_OPTS = ["-fPIC"]

# =============================================================================
# Generated headers
# =============================================================================

# Generate config.h with the defines libteamdctl needs.
write_file(
    name = "config_h",
    out = "config.h",
    content = [
        "#define ENABLE_DBUS 1",
        "#define ENABLE_ZMQ 1",
        "// TODO(bazel-ready): Make PACKAGE_VERSION configurable via build settings.",
        "#define PACKAGE_VERSION \"1.31\"",
    ],
)

# =============================================================================
# Header libraries
# =============================================================================

# All headers under include/, equivalent to -I${top_srcdir}/include.
# Provides: team.h, teamdctl.h, private/list.h, private/misc.h, linux/if_team.h, etc.
cc_library(
    name = "include_headers",
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)

# Public header: consumers use #include <teamdctl.h>
cc_library(
    name = "libteamdctl_headers",
    hdrs = ["include/teamdctl.h"],
    deps = [":include_headers"],
    visibility = ["//visibility:public"],
)

# For internal source to reference, not meant to be published.
# Also not meant to have any `includes`, dependents are expected to use absolute paths.
cc_library(
    name = "all_headers",
    hdrs = glob(["**/*.h"]),
)

# =============================================================================
# libteamdctl
# =============================================================================

cc_library(
    name = "libteamdctl",
    srcs = [
        "libteamdctl/libteamdctl.c",
        "libteamdctl/cli_usock.c",
        "libteamdctl/cli_dbus.c",
        "libteamdctl/cli_zmq.c",
        # Private headers
        "libteamdctl/teamdctl_private.h",
        ":config_h",
    ],
    copts = [
        "-fvisibility=hidden",
        "-ffunction-sections",
        "-fdata-sections",
    ] + IMPLICIT_LIBTOOL_OPTS,
    defines = ["_GNU_SOURCE"],
    deps = [
        ":all_headers",
        ":include_headers",
        "@libteam_deps//libdaemon-dev:libdaemon",
        "@libteam_deps//libjansson-dev:libjansson",
        "@libteam_deps//libdbus-1-dev:libdbus-1",
        "@libteam_deps//libzmq3-dev:libzmq3",
        "@libteam_deps//libbsd-dev:libbsd",
    ],
    visibility = ["//visibility:public"],
)

# TODO(bazel-ready): Fix the libbsd issue in `rules_distroless`, then switch to cc_shared_library.
cc_binary(
    name = "teamdctl", # Note: Bazel will produce libteamdctl.so from this name.
    deps = [":libteamdctl"],
    linkshared = True,
    linkopts = [
        "-Wl,-soname,libteamdctl.so.{}".format(LIBTEAMDCTL_SOVERSION),
        "-Wl,--gc-sections",
        "-Wl,--as-needed",
    ],
    visibility = ["//visibility:public"],
)

# TODO(bazel-ready): Generate libteamdctl.pc from sources.
#  For now, because we only need to support one version, we can copy the result of running autoconf and configure on the repository.
write_file(
    name = "libteamdctl_pc",
    out = "libteamdctl.pc",
    content = """
prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

Name: libteamdctl
Description: Teamd daemon control library.
Version: 1.31
Libs: -L${libdir} -lteamdctl
Cflags: -I${includedir}
""".split("\n"),
)

# TODO(bazel-ready): Generate libteam.pc from sources.
#  For now, because we only need to support one version, we can copy the result of running autoconf and configure on the repository.
write_file(
    name = "libteam_pc",
    out = "libteam.pc",
    content = """
prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

Name: libteam
Description: Libteam library.
Requires: libnl-3.0
Version: 1.31
Libs: -L${libdir} -lteam
Cflags: -I${includedir}
""".split("\n"),
)

# =============================================================================
# libteam
# =============================================================================

# From libteam/Makefile.am
cc_library(
    name = "libteam",
    srcs = [
        "libteam/libteam.c",
        "libteam/ports.c",
        "libteam/options.c",
        "libteam/ifinfo.c",
        "libteam/stringify.c",
        # Private headers
        "libteam/team_private.h",
        "libteam/nl_updates.h",
        ":config_h",
    ],
    copts = [
        "-fvisibility=hidden",
        "-ffunction-sections",
        "-fdata-sections",
    ] + IMPLICIT_LIBTOOL_OPTS,
    defines = ["_GNU_SOURCE"],
    deps = [
        ":all_headers",
        ":include_headers",
        "@libteam_deps//libnl-3-dev:libnl-3",
        "@libteam_deps//libnl-genl-3-dev:libnl-genl-3",
        "@libteam_deps//libnl-route-3-dev:libnl-route-3",
        "@libteam_deps//libnl-cli-3-dev:libnl-cli-3",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libteam_shared",
    deps = [":libteam"],
    user_link_flags = [
        "-Wl,-soname,libteam.so.{}".format(LIBTEAM_SOVERSION),
        "-Wl,--gc-sections",
        "-Wl,--as-needed",
    ],
    visibility = ["//visibility:public"],
)

# =============================================================================
# teamd
# =============================================================================

cc_binary(
    name = "teamd",
    srcs = [
        "teamd/teamd.c",
        "teamd/teamd_common.c",
        "teamd/teamd_json.c",
        "teamd/teamd_config.c",
        "teamd/teamd_state.c",
        "teamd/teamd_workq.c",
        "teamd/teamd_events.c",
        "teamd/teamd_per_port.c",
        "teamd/teamd_option_watch.c",
        "teamd/teamd_ifinfo_watch.c",
        "teamd/teamd_lw_ethtool.c",
        "teamd/teamd_lw_psr.c",
        "teamd/teamd_lw_arp_ping.c",
        "teamd/teamd_lw_nsna_ping.c",
        "teamd/teamd_lw_tipc.c",
        "teamd/teamd_link_watch.c",
        "teamd/teamd_ctl.c",
        "teamd/teamd_dbus.c",
        "teamd/teamd_zmq.c",
        "teamd/teamd_usock.c",
        "teamd/teamd_phys_port_check.c",
        "teamd/teamd_bpf_chef.c",
        "teamd/teamd_hash_func.c",
        "teamd/teamd_balancer.c",
        "teamd/teamd_runner_basic_ones.c",
        "teamd/teamd_runner_activebackup.c",
        "teamd/teamd_runner_loadbalance.c",
        "teamd/teamd_runner_lacp.c",
        ":config_h",
    ],
    defines = [
        "_GNU_SOURCE",
        "LOCALSTATEDIR=\"/var\"",
    ],
    deps = [
        ":all_headers",
        ":libteam",
        "@libteam_deps//libdaemon-dev:libdaemon",
        "@libteam_deps//libjansson-dev:libjansson",
        "@libteam_deps//libdbus-1-dev:libdbus-1",
        "@libteam_deps//libzmq3-dev:libzmq3",
    ],
    visibility = ["//visibility:public"],
)

# =============================================================================
# libteam-utils
# =============================================================================

# From utils/Makefile.am: teamdctl depends on libteamdctl + jansson
cc_binary(
    # Note: We cannot name it `teamdctl` because the `cc_binary` we use to produce libteamdctl.so needs to be named `teamdctl`.
    name = "teamdctl_bin",
    srcs = ["utils/teamdctl.c"],
    defines = ["_GNU_SOURCE"],
    deps = [
        ":libteamdctl",
        ":include_headers",
        "@libteam_deps//libjansson-dev:libjansson",
    ],
    visibility = ["//visibility:public"],
)

# From utils/Makefile.am: teamnl depends on libteam
cc_binary(
    name = "teamnl",
    srcs = ["utils/teamnl.c"],
    defines = ["_GNU_SOURCE"],
    deps = [
        ":libteam",
        ":include_headers",
    ],
    visibility = ["//visibility:public"],
)

# =============================================================================
# Packaging: runtime library tarballs
# =============================================================================

tar(
    name = "libteam5_pkg",
    srcs = [":libteam_shared"],
    mtree = [
        "./usr/lib/x86_64-linux-gnu/libteam.so.{full} uid=0 gid=0 type=file content=$(location :libteam_shared)".format(full = LIBTEAM_FULL_VERSION),
        "./usr/lib/x86_64-linux-gnu/libteam.so.{so} uid=0 gid=0 type=link link=libteam.so.{full}".format(so = LIBTEAM_SOVERSION, full = LIBTEAM_FULL_VERSION),
    ],
    visibility = ["//visibility:public"],
)

tar(
    name = "libteamdctl0_pkg",
    srcs = [":teamdctl"],
    mtree = [
        "./usr/lib/x86_64-linux-gnu/libteamdctl.so.{full} uid=0 gid=0 type=file content=$(location :teamdctl)".format(full = LIBTEAMDCTL_FULL_VERSION),
        "./usr/lib/x86_64-linux-gnu/libteamdctl.so.{so} uid=0 gid=0 type=link link=libteamdctl.so.{full}".format(so = LIBTEAMDCTL_SOVERSION, full = LIBTEAMDCTL_FULL_VERSION),
    ],
    visibility = ["//visibility:public"],
)

tar(
    name = "libteam-utils_pkg",
    srcs = [
        ":teamd",
        ":teamdctl_bin",
        ":teamnl",
    ],
    mtree = [
        # TODO(bazel-ready): Remove teamd binary if not needed.
        "./usr/bin/teamd uid=0 gid=0 type=file content=$(location :teamd)",
        "./usr/bin/teamdctl uid=0 gid=0 type=file content=$(location :teamdctl_bin)",
        "./usr/bin/teamnl uid=0 gid=0 type=file content=$(location :teamnl)",
    ],
    visibility = ["//visibility:public"],
)

# TODO(bazel-ready): Create -dev dist packages.
