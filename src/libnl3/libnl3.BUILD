load("@rules_cc//cc:defs.bzl", "cc_library")
load("@tar.bzl", "tar", "mutate")
load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("@rules_deb//distroless:defs.bzl", "flatten")
load("@rules_flex//flex:flex.bzl", "flex")
load("@rules_bison//bison:bison.bzl", "bison_cc_library")

# =============================================================================
# Generated headers
# =============================================================================

# Generate lib/defs.h from autoconf template.
# On Linux/glibc x86_64 all standard headers exist.
genrule(
    name = "defs_h",
    srcs = ["lib/defs.h.in"],
    outs = ["lib/defs.h"],
    # TODO(bazel-ready): Define NL_DEBUG if necessary, based on configuration.
    # TODO(bazel-ready): Define VERSION from stamping, if necessary.
    cmd = """
        sed \
            -e 's|#undef DISABLE_PTHREADS|/* #undef DISABLE_PTHREADS */|' \
            -e 's|#undef HAVE_DLFCN_H|#define HAVE_DLFCN_H 1|' \
            -e 's|#undef HAVE_INTTYPES_H|#define HAVE_INTTYPES_H 1|' \
            -e 's|#undef HAVE_LIBPTHREAD|#define HAVE_LIBPTHREAD 1|' \
            -e 's|#undef HAVE_STDINT_H|#define HAVE_STDINT_H 1|' \
            -e 's|#undef HAVE_STDIO_H|#define HAVE_STDIO_H 1|' \
            -e 's|#undef HAVE_STDLIB_H|#define HAVE_STDLIB_H 1|' \
            -e 's|#undef HAVE_STRERROR_L|#define HAVE_STRERROR_L 1|' \
            -e 's|#undef HAVE_STRINGS_H|#define HAVE_STRINGS_H 1|' \
            -e 's|#undef HAVE_STRING_H|#define HAVE_STRING_H 1|' \
            -e 's|#undef HAVE_SYS_STAT_H|#define HAVE_SYS_STAT_H 1|' \
            -e 's|#undef HAVE_SYS_TYPES_H|#define HAVE_SYS_TYPES_H 1|' \
            -e 's|#undef HAVE_UNISTD_H|#define HAVE_UNISTD_H 1|' \
            -e 's|#undef LT_OBJDIR|#define LT_OBJDIR ".libs/"|' \
            -e 's|#undef NL_DEBUG|/* #undef NL_DEBUG */|' \
            -e 's|#undef PACKAGE$$|#define PACKAGE "libnl"|' \
            -e 's|#undef PACKAGE_BUGREPORT|#define PACKAGE_BUGREPORT "http://www.infradead.org/~tgr/libnl/"|' \
            -e 's|#undef PACKAGE_NAME|#define PACKAGE_NAME "libnl"|' \
            -e 's|#undef PACKAGE_STRING|#define PACKAGE_STRING "libnl 3.7.0"|' \
            -e 's|#undef PACKAGE_TARNAME|#define PACKAGE_TARNAME "libnl"|' \
            -e 's|#undef PACKAGE_URL|#define PACKAGE_URL "http://www.infradead.org/~tgr/libnl/"|' \
            -e 's|#undef PACKAGE_VERSION|#define PACKAGE_VERSION "3.7.0"|' \
            -e 's|#undef STDC_HEADERS|#define STDC_HEADERS 1|' \
            -e 's|#undef VERSION|#define VERSION "3.7.0"|' \
            $(SRCS) > $@
        echo '#define SYSCONFDIR "/etc/libnl"' >> $@
    """,
)

# =============================================================================
# Flex/Bison generated sources for libnl-route-3
# =============================================================================

# The BCR flex binary is built with M4="/bin/false", expecting M4 to be
# provided via environment variable at runtime.
FLEX_ENV = {"M4": "$(M4)"}

flex(
    name = "pktloc_grammar",
    src = "lib/route/pktloc_grammar.l",
)

bison_cc_library(
    name = "pktloc_syntax",
    src = "lib/route/pktloc_syntax.y",
    copts = [
        "-fPIC",
        "-I{REPO_DIR}/include/linux-private",
        "-Wno-unused-parameter",
        # Suppress legitimate bison warnings that are likely inconsequential
        "-Wno-empty-rule",
    ],
    deps = [":private_headers"],
)

flex(
    name = "ematch_grammar",
    src = "lib/route/cls/ematch_grammar.l",
)

bison_cc_library(
    name = "ematch_syntax",
    src = "lib/route/cls/ematch_syntax.y",
    copts = [
        "-fPIC",
        "-I{REPO_DIR}/include/linux-private",
        "-Wno-unused-parameter",
        # Suppress legitimate bison warnings that are likely inconsequential
        "-Wno-empty-rule",
    ],
    deps = [":private_headers"],
)

# =============================================================================
# Header libraries
# =============================================================================

# Public header files for packaging.
filegroup(
    name = "public_header_files",
    srcs = glob(["include/netlink/**/*.h"]),
)

# Public headers: consumers use #include <netlink/...>
cc_library(
    name = "public_headers",
    hdrs = glob(["include/netlink/**/*.h"]),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

# Compat headers: consumers use #include <libnl3/netlink/...>
# This matches the Debian libnl3-dev package layout.
cc_library(
    name = "public_headers_compat",
    hdrs = glob(["include/netlink/**/*.h"]),
    include_prefix = "libnl3",
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

# Private headers used only during compilation of libnl3 libraries.
# linux-private/ shadows system kernel headers (e.g. linux/snmp.h).
# Must use strip_include_prefix (generates -I _virtual_includes/) so it has
# higher priority than the toolchain's -isystem flags for linux-libc-dev.
cc_library(
    name = "linux_private_headers",
    hdrs = glob(["include/linux-private/**/*.h"]),
    strip_include_prefix = "include/linux-private",
)

# Includes netlink-private/ and the generated defs.h.
cc_library(
    name = "private_headers",
    hdrs = glob([
        "include/netlink-private/**/*.h",
    ]) + [":defs_h"],
    # include/netlink-private/netlink.h uses #include <defs.h>,
    # so we need lib/ in the include path (where defs.h is generated).
    #
    # Note: include/linux-private is intentionally NOT listed here; adding it
    # via includes= would generate -isystem, which GCC searches AFTER the
    # toolchain's -isystem paths. The linux-private headers need to shadow the
    # system linux-libc-dev headers, so they are added via -I in LIBNL_COPTS.
    includes = [
        "include",
        "lib",
    ],
    deps = [
        ":linux_private_headers",
        ":public_headers",
    ],
    # _GNU_SOURCE is required by the private headers (e.g. struct ucred in
    # netlink-private/types.h). Propagate it to all consumers via defines.
    defines = ["_GNU_SOURCE"],
)

# =============================================================================
# Common build settings
# =============================================================================

LIBNL_COPTS = [
    "-std=gnu11",
    "-fPIC",
    "-Wall",
    "-D_GNU_SOURCE",
    "-O2",
    "-DNDEBUG",
    # The libnl3 linux-private headers (e.g. linux/snmp.h) must shadow the
    # system linux-libc-dev headers, which are newer and have grown extra enum
    # values that break libnl3's static assertions (e.g. __ICMP6_MIB_MAX == 6
    # in lib/route/link/inet6.c). GCC ignores -I for a directory that is already
    # listed as -isystem, so include/linux-private must NOT appear in
    # private_headers.includes= (which generates -isystem). Instead, we use -I
    # here to get user-include priority over the toolchain system headers.
    "-I{REPO_DIR}/include/linux-private",
    # Suppress warnings from upstream third-party code.
    "-Wno-unused-parameter",
    "-Wno-sign-compare",
    "-Wno-format-truncation",
    "-Wno-maybe-uninitialized",
    "-Wno-return-type",
]

LIBNL_LINKOPTS = []

# =============================================================================
# Core library: libnl-3
# =============================================================================

cc_library(
    name = "libnl_3",
    srcs = glob(["lib/*.c"]),
    copts = LIBNL_COPTS,
    linkopts = LIBNL_LINKOPTS,
    deps = [
        ":public_headers",
        ":public_headers_compat",
        ":private_headers",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libnl_3_shared",
    deps = [":libnl_3"],
    user_link_flags = [
        "-Wl,--version-script=$(location libnl-3.sym)",
        "-Wl,-soname,libnl-3.so.200",
    ],
    additional_linker_inputs = ["libnl-3.sym"],
    visibility = ["//visibility:public"],
)

# =============================================================================
# Generic Netlink library: libnl-genl-3
# =============================================================================

cc_library(
    name = "libnl_genl_3",
    srcs = glob(["lib/genl/*.c"]),
    copts = LIBNL_COPTS,
    deps = [
        ":libnl_3",
        ":private_headers",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libnl_genl_3_shared",
    deps = [":libnl_genl_3"],
    dynamic_deps = [":libnl_3_shared"],
    user_link_flags = [
        "-Wl,--version-script=$(location libnl-genl-3.sym)",
        "-Wl,-soname,libnl-genl-3.so.200",
    ],
    additional_linker_inputs = ["libnl-genl-3.sym"],
    visibility = ["//visibility:public"],
)

# =============================================================================
# Route library: libnl-route-3
# =============================================================================

cc_library(
    name = "libnl_route_3",
    srcs = glob(
        [
            "lib/route/*.c",
            "lib/route/**/*.c",
            "lib/fib_lookup/*.c",
        ],
    ) + [
        ":pktloc_grammar",
        ":ematch_grammar",
    ],
    copts = LIBNL_COPTS,
    deps = [
        ":pktloc_syntax",
        ":ematch_syntax",
        ":libnl_3",
        ":private_headers",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libnl_route_3_shared",
    deps = [":libnl_route_3"],
    dynamic_deps = [":libnl_3_shared"],
    user_link_flags = [
        "-Wl,--version-script=$(location libnl-route-3.sym)",
        "-Wl,-soname,libnl-route-3.so.200",
    ],
    additional_linker_inputs = ["libnl-route-3.sym"],
    visibility = ["//visibility:public"],
)

# =============================================================================
# Netfilter library: libnl-nf-3
# =============================================================================

cc_library(
    name = "libnl_nf_3",
    srcs = glob(["lib/netfilter/*.c"]),
    copts = LIBNL_COPTS,
    deps = [
        ":libnl_3",
        ":libnl_route_3",
        ":private_headers",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libnl_nf_3_shared",
    deps = [":libnl_nf_3"],
    dynamic_deps = [
        ":libnl_3_shared",
        ":libnl_route_3_shared",
    ],
    user_link_flags = [
        "-Wl,--version-script=$(location libnl-nf-3.sym)",
        "-Wl,-soname,libnl-nf-3.so.200",
    ],
    additional_linker_inputs = ["libnl-nf-3.sym"],
    visibility = ["//visibility:public"],
)

# =============================================================================
# CLI library: libnl-cli-3
# =============================================================================

cc_library(
    name = "libnl_cli_3",
    srcs = glob(["src/lib/*.c"]),
    copts = LIBNL_COPTS,
    defines = [
        'PKGLIBDIR=\\"/usr/lib/x86_64-linux-gnu/libnl\\"',
    ],
    deps = [
        ":libnl_3",
        ":libnl_genl_3",
        ":libnl_nf_3",
        ":libnl_route_3",
        ":private_headers",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libnl_cli_3_shared",
    deps = [":libnl_cli_3"],
    dynamic_deps = [
        ":libnl_3_shared",
        ":libnl_genl_3_shared",
        ":libnl_nf_3_shared",
        ":libnl_route_3_shared",
    ],
    user_link_flags = [
        "-Wl,--version-script=$(location libnl-cli-3.sym)",
        "-Wl,-soname,libnl-cli-3.so.200",
    ],
    additional_linker_inputs = ["libnl-cli-3.sym"],
    visibility = ["//visibility:public"],
)

# CLI plugin shared objects installed to /usr/lib/x86_64-linux-gnu/libnl-3/cli/
# These are loadable modules (not linked libraries), built as individual .so files.

cc_library(
    name = "libnl_cli_cls_basic",
    srcs = ["lib/cli/cls/basic.c"],
    copts = LIBNL_COPTS,
    deps = [":libnl_3", ":libnl_cli_3", ":libnl_route_3", ":private_headers"],
)

cc_shared_library(
    name = "libnl_cli_cls_basic_shared",
    deps = [":libnl_cli_cls_basic"],
    dynamic_deps = [":libnl_3_shared", ":libnl_cli_3_shared", ":libnl_route_3_shared"],
)

cc_library(
    name = "libnl_cli_cls_cgroup",
    srcs = ["lib/cli/cls/cgroup.c"],
    copts = LIBNL_COPTS,
    deps = [":libnl_3", ":libnl_cli_3", ":libnl_route_3", ":private_headers"],
)

cc_shared_library(
    name = "libnl_cli_cls_cgroup_shared",
    deps = [":libnl_cli_cls_cgroup"],
    dynamic_deps = [":libnl_3_shared", ":libnl_cli_3_shared", ":libnl_route_3_shared"],
)

cc_library(
    name = "libnl_cli_qdisc_bfifo",
    srcs = ["lib/cli/qdisc/bfifo.c"],
    copts = LIBNL_COPTS,
    deps = [":libnl_3", ":libnl_cli_3", ":libnl_route_3", ":private_headers"],
)

cc_shared_library(
    name = "libnl_cli_qdisc_bfifo_shared",
    deps = [":libnl_cli_qdisc_bfifo"],
    dynamic_deps = [":libnl_3_shared", ":libnl_cli_3_shared", ":libnl_route_3_shared"],
)

cc_library(
    name = "libnl_cli_qdisc_blackhole",
    srcs = ["lib/cli/qdisc/blackhole.c"],
    copts = LIBNL_COPTS,
    deps = [":libnl_3", ":libnl_cli_3", ":libnl_route_3", ":private_headers"],
)

cc_shared_library(
    name = "libnl_cli_qdisc_blackhole_shared",
    deps = [":libnl_cli_qdisc_blackhole"],
    dynamic_deps = [":libnl_3_shared", ":libnl_cli_3_shared", ":libnl_route_3_shared"],
)

cc_library(
    name = "libnl_cli_qdisc_fq_codel",
    srcs = ["lib/cli/qdisc/fq_codel.c"],
    copts = LIBNL_COPTS,
    deps = [":libnl_3", ":libnl_cli_3", ":libnl_route_3", ":private_headers"],
)

cc_shared_library(
    name = "libnl_cli_qdisc_fq_codel_shared",
    deps = [":libnl_cli_qdisc_fq_codel"],
    dynamic_deps = [":libnl_3_shared", ":libnl_cli_3_shared", ":libnl_route_3_shared"],
)

cc_library(
    name = "libnl_cli_qdisc_hfsc",
    srcs = ["lib/cli/qdisc/hfsc.c"],
    copts = LIBNL_COPTS,
    deps = [":libnl_3", ":libnl_cli_3", ":libnl_route_3", ":private_headers"],
)

cc_shared_library(
    name = "libnl_cli_qdisc_hfsc_shared",
    deps = [":libnl_cli_qdisc_hfsc"],
    dynamic_deps = [":libnl_3_shared", ":libnl_cli_3_shared", ":libnl_route_3_shared"],
)

cc_library(
    name = "libnl_cli_qdisc_htb",
    srcs = ["lib/cli/qdisc/htb.c"],
    copts = LIBNL_COPTS,
    deps = [":libnl_3", ":libnl_cli_3", ":libnl_route_3", ":private_headers"],
)

cc_shared_library(
    name = "libnl_cli_qdisc_htb_shared",
    deps = [":libnl_cli_qdisc_htb"],
    dynamic_deps = [":libnl_3_shared", ":libnl_cli_3_shared", ":libnl_route_3_shared"],
)

cc_library(
    name = "libnl_cli_qdisc_ingress",
    srcs = ["lib/cli/qdisc/ingress.c"],
    copts = LIBNL_COPTS,
    deps = [":libnl_3", ":libnl_cli_3", ":libnl_route_3", ":private_headers"],
)

cc_shared_library(
    name = "libnl_cli_qdisc_ingress_shared",
    deps = [":libnl_cli_qdisc_ingress"],
    dynamic_deps = [":libnl_3_shared", ":libnl_cli_3_shared", ":libnl_route_3_shared"],
)

cc_library(
    name = "libnl_cli_qdisc_pfifo",
    srcs = ["lib/cli/qdisc/pfifo.c"],
    copts = LIBNL_COPTS,
    deps = [":libnl_3", ":libnl_cli_3", ":libnl_route_3", ":private_headers"],
)

cc_shared_library(
    name = "libnl_cli_qdisc_pfifo_shared",
    deps = [":libnl_cli_qdisc_pfifo"],
    dynamic_deps = [":libnl_3_shared", ":libnl_cli_3_shared", ":libnl_route_3_shared"],
)

cc_library(
    name = "libnl_cli_qdisc_plug",
    srcs = ["lib/cli/qdisc/plug.c"],
    copts = LIBNL_COPTS,
    deps = [":libnl_3", ":libnl_cli_3", ":libnl_route_3", ":private_headers"],
)

cc_shared_library(
    name = "libnl_cli_qdisc_plug_shared",
    deps = [":libnl_cli_qdisc_plug"],
    dynamic_deps = [":libnl_3_shared", ":libnl_cli_3_shared", ":libnl_route_3_shared"],
)

# TODO(bazel-ready): Parameterize tar files by target architecture.
# TODO(bazel-ready): Generate `.200` packages if needed.

tar(
    name = "libnl-3_pkg",
    srcs = [":libnl_3_shared"],
    mtree = [
        "./usr/lib/x86_64-linux-gnu/libnl-3.so.200 uid=0 gid=0 mode=0755 type=file content=$(location :libnl_3_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-3.so uid=0 gid=0 mode=0755 type=link link=libnl-3.so.200",
    ],
    visibility = ["//visibility:public"],
)

tar(
    name = "libnl-genl-3_pkg",
    srcs = [":libnl_genl_3_shared"],
    mtree = [
        "./usr/lib/x86_64-linux-gnu/libnl-genl-3.so.200 uid=0 gid=0 mode=0755 type=file content=$(location :libnl_genl_3_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-genl-3.so uid=0 gid=0 mode=0755 type=link link=libnl-genl-3.so.200",
    ],
    visibility = ["//visibility:public"],
)

tar(
    name = "libnl-route-3_pkg",
    srcs = [":libnl_route_3_shared"],
    mtree = [
        "./usr/lib/x86_64-linux-gnu/libnl-route-3.so.200 uid=0 gid=0 mode=0755 type=file content=$(location :libnl_route_3_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-route-3.so uid=0 gid=0 mode=0755 type=link link=libnl-route-3.so.200",
    ],
    visibility = ["//visibility:public"],
)

tar(
    name = "libnl-nf-3_pkg",
    srcs = [":libnl_nf_3_shared"],
    mtree = [
        "./usr/lib/x86_64-linux-gnu/libnl-nf-3.so.200 uid=0 gid=0 mode=0755 type=file content=$(location :libnl_nf_3_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-nf-3.so uid=0 gid=0 mode=0755 type=link link=libnl-nf-3.so.200",
    ],
    visibility = ["//visibility:public"],
)

tar(
    name = "libnl-cli-3_pkg",
    srcs = [
        ":libnl_cli_3_shared",
        ":libnl_cli_cls_basic_shared",
        ":libnl_cli_cls_cgroup_shared",
        ":libnl_cli_qdisc_bfifo_shared",
        ":libnl_cli_qdisc_blackhole_shared",
        ":libnl_cli_qdisc_fq_codel_shared",
        ":libnl_cli_qdisc_hfsc_shared",
        ":libnl_cli_qdisc_htb_shared",
        ":libnl_cli_qdisc_ingress_shared",
        ":libnl_cli_qdisc_pfifo_shared",
        ":libnl_cli_qdisc_plug_shared",
    ],
    mtree = [
        "./usr/lib/x86_64-linux-gnu/libnl-cli-3.so.200 uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_3_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-cli-3.so uid=0 gid=0 mode=0755 type=link link=libnl-cli-3.so.200",
        "./usr/lib/x86_64-linux-gnu/libnl-3/cli/cls/basic.so uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_cls_basic_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-3/cli/cls/cgroup.so uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_cls_cgroup_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-3/cli/qdisc/bfifo.so uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_qdisc_bfifo_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-3/cli/qdisc/blackhole.so uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_qdisc_blackhole_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-3/cli/qdisc/fq_codel.so uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_qdisc_fq_codel_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-3/cli/qdisc/hfsc.so uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_qdisc_hfsc_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-3/cli/qdisc/htb.so uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_qdisc_htb_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-3/cli/qdisc/ingress.so uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_qdisc_ingress_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-3/cli/qdisc/pfifo.so uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_qdisc_pfifo_shared)",
        "./usr/lib/x86_64-linux-gnu/libnl-3/cli/qdisc/plug.so uid=0 gid=0 mode=0755 type=file content=$(location :libnl_cli_qdisc_plug_shared)",
    ],
    visibility = ["//visibility:public"],
)

# =============================================================================
# Packaging: headers tarball
# =============================================================================

tar(
    name = "libnl3-dev_headers",
    srcs = [":public_header_files"],
    mutate = mutate(
        strip_prefix = "include",
        package_dir = "./usr/include/libnl3",
    ),
)

# =============================================================================
# Packaging: pkgconfig files
# =============================================================================

# TODO(bazel-ready): We should use rules_foreign_cc to run cmake and build this file.
# However, because we don't need to override the install directory,
write_file(
    name = "libnl3_pc_generated",
    out = "libnl-3.0.pc",
    content = [
        "prefix=/usr",
        "exec_prefix=${prefix}",
        "libdir=${prefix}/lib/x86_64-linux-gnu",
        "includedir=${prefix}/include/libnl3",
        "",
        "Name: libnl-3.0",
        "Description: Convenience library for netlink sockets",
        "Version: 3.7.0",
        "Libs: -L${libdir} -lnl-3",
        "Libs.private: -lpthread -lm",
        "Cflags: -I${includedir}",
        "",
    ],
)

write_file(
    name = "libnl3_genl_pc_generated",
    out = "libnl-genl-3.0.pc",
    content = [
        "prefix=/usr",
        "exec_prefix=${prefix}",
        "libdir=${prefix}/lib/x86_64-linux-gnu",
        "includedir=${prefix}/include/libnl3",
        "",
        "Name: libnl-genl-3.0",
        "Description: Generic Netlink Library",
        "Version: 3.7.0",
        "Requires: libnl-3.0 >= 3.7.0",
        "Libs: -L${libdir} -lnl-genl-3",
        "Cflags: -I${includedir}",
        "",
    ],
)

write_file(
    name = "libnl3_route_pc_generated",
    out = "libnl-route-3.0.pc",
    content = [
        "prefix=/usr",
        "exec_prefix=${prefix}",
        "libdir=${prefix}/lib/x86_64-linux-gnu",
        "includedir=${prefix}/include/libnl3",
        "",
        "Name: libnl-route-3.0",
        "Description: Routing/link Library",
        "Version: 3.7.0",
        "Requires: libnl-3.0 >= 3.7.0",
        "Libs: -L${libdir} -lnl-route-3",
        "Cflags: -I${includedir}",
        "",
    ],
)

write_file(
    name = "libnl3_nf_pc_generated",
    out = "libnl-nf-3.0.pc",
    content = [
        "prefix=/usr",
        "exec_prefix=${prefix}",
        "libdir=${prefix}/lib/x86_64-linux-gnu",
        "includedir=${prefix}/include/libnl3",
        "",
        "Name: libnl-nf-3.0",
        "Description: Netfilter Netlink Library",
        "Version: 3.7.0",
        "Requires: libnl-3.0 >= 3.7.0",
        "Libs: -L${libdir} -lnl-nf-3",
        "Cflags: -I${includedir}",
        "",
    ],
)

# Generated from the upstream libnl-cli-3.0.pc.in template with default
# Debian bookworm amd64 install paths (prefix=/usr, libdir=x86_64-linux-gnu).
write_file(
    name = "libnl3_cli_pc_generated",
    out = "libnl-cli-3.0.pc",
    content = [
        "prefix=/usr",
        "exec_prefix=${prefix}",
        "libdir=${prefix}/lib/x86_64-linux-gnu",
        "includedir=${prefix}/include/libnl3",
        "",
        "Name: libnl-cli-3.0",
        "Description: CLI Netlink Library",
        "Version: 3.7.0",
        "Requires: libnl-3.0 >= 3.7.0 libnl-route-3.0 >= 3.7.0 libnl-genl-3.0 >= 3.7.0 libnl-nf-3.0 >= 3.7.0",
        "Libs: -L${libdir} -lnl-cli-3",
        "Cflags: -I${includedir}",
        "",
    ],
)

tar(
    name = "libnl3_pkgconfig",
    srcs = [
        ":libnl3_pc_generated",
        ":libnl3_genl_pc_generated",
        ":libnl3_route_pc_generated",
        ":libnl3_nf_pc_generated",
        ":libnl3_cli_pc_generated",
    ],
    mutate = mutate(
        strip_prefix = package_name(),
        package_dir = "./usr/lib/x86_64-linux-gnu/pkgconfig",
    ),
)

# =============================================================================
# Packaging: -dev package tarballs (runtime + headers + pkgconfig)
# =============================================================================

flatten(
    name = "libnl-3-dev_pkg",
    tars = [
        ":libnl-3_pkg",
        ":libnl3-dev_headers",
        ":libnl3_pkgconfig",
    ],
    visibility = ["//visibility:public"],
    deduplicate = True,
)

flatten(
    name = "libnl-genl-3-dev_pkg",
    tars = [
        ":libnl-genl-3_pkg",
        ":libnl-3-dev_pkg",
    ],
    visibility = ["//visibility:public"],
    deduplicate = True,
)

flatten(
    name = "libnl-route-3-dev_pkg",
    tars = [
        ":libnl-route-3_pkg",
        ":libnl-3-dev_pkg",
    ],
    visibility = ["//visibility:public"],
    deduplicate = True,
)

flatten(
    name = "libnl-nf-3-dev_pkg",
    tars = [
        ":libnl-nf-3_pkg",
        ":libnl-3-dev_pkg",
        ":libnl-route-3_pkg",
    ],
    visibility = ["//visibility:public"],
)

# libnl-cli-3-dev only contains the unversioned symlink and pkgconfig;
# the runtime .so and plugins are in libnl-cli-3_pkg.
flatten(
    name = "libnl-cli-3-dev_pkg",
    tars = [
        ":libnl-cli-3_pkg",
        ":libnl-3-dev_pkg",
    ],
    visibility = ["//visibility:public"],
    deduplicate = True,
)
