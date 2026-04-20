package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_library")
load("@tar.bzl//:tar.bzl", "tar")
load("//:build_constants.bzl", "CPP_SWIG_HEADERS", "EXTENSIONS_PLUGINS_DIR", "INSTALL_INCLUDEDIR", "INSTALL_LIBDIR", "INSTALL_PREFIX", "LIBYANG_DESCRIPTION", "LIBYANG_MAJOR_SOVERSION", "LIBYANG_MICRO_SOVERSION", "LIBYANG_MINOR_SOVERSION", "LIBYANG_SOVERSION_FULL", "LIBYANG_VERSION", "PUBLIC_HEADERS", "USER_TYPES_PLUGINS_DIR")
load("//:pkg_path.bzl", "pkg_path")
load("@sonic_build_infra//swig:defs.bzl", "swig_gen", "swig_lib_deb")
load("@sonic_build_infra//python:py_native_library.bzl", "py_native_library")
load("@sonic_build_infra//tar:assert_tar.bzl", "assert_tar")

LIBYANG_COPTS = [
    "-Wall",
    "-Wextra",
    # Suppress warnings that come from the particularities of our toolchain.
    "-Wno-unused-parameter",
]

# All public headers (non-generated + generated libyang.h).
# Used by both cc_library and the libyang-dev tar package.
filegroup(
    name = "public_header_files",
    srcs = PUBLIC_HEADERS + [":libyang_h"],
)

# C++ binding headers from swig/cpp/src/.
# Used by libyang-cpp, cpp_swig_public_headers, and swig_gen.
filegroup(
    name = "cpp_header_files",
    srcs = CPP_SWIG_HEADERS,
)

# -- Generated headers from CMake templates --

# debian/rules configures with: -DENABLE_LYD_PRIV=ON
# TODO(bazel-ready): Make options configurable if we need to.
genrule(
    name = "libyang_h",
    srcs = ["src/libyang.h.in"],
    outs = ["src/libyang.h"],
    cmd = """
        sed -e 's/#cmakedefine LY_ENABLED_CACHE/#define LY_ENABLED_CACHE/' \
            -e 's/#cmakedefine LY_ENABLED_LATEST_REVISIONS/#define LY_ENABLED_LATEST_REVISIONS/' \
            -e 's/#cmakedefine LY_ENABLED_LYD_PRIV/#define LY_ENABLED_LYD_PRIV/' \
            -e 's/@COMPILER_PACKED_ATTR@/__attribute__((__packed__))/' \
            -e 's/@LIBYANG_MAJOR_SOVERSION@/{major}/' \
            -e 's/@LIBYANG_MINOR_SOVERSION@/{minor}/' \
            -e 's/@LIBYANG_MICRO_SOVERSION@/{micro}/' \
            -e 's/@LIBYANG_SOVERSION_FULL@/{full}/' \
            $(SRCS) > $@
    """.format(
        major = LIBYANG_MAJOR_SOVERSION,
        minor = LIBYANG_MINOR_SOVERSION,
        micro = LIBYANG_MICRO_SOVERSION,
        full = LIBYANG_SOVERSION_FULL,
    ),
)

# common.h: internal header with compiler attributes
# TODO(bazel-ready): Support other compilers
#                    These attributes are compiler-dependent. Our toolchain is gcc for now, but we should make this configurable to support clang as well.
genrule(
    name = "common_h",
    srcs = ["src/common.h.in"],
    outs = ["src/common.h"],
    cmd = """
        sed -e 's/@COMPILER_UNUSED_ATTR@/UNUSED_ ## x __attribute__((__unused__))/' \
            $(SRCS) > $@
    """,
)

# plugin_config.h: plugin directory paths (non-STATIC build, so only the #defines matter)
# TODO(bazel-ready): Configure for static builds.
#                    We set a bunch of these configurations to empty, because they are in the `#ifdef STATIC` block,
#                    which is always false for our build.
#                    If we even need to build a static libyang for distribution, we'll have to fix these configurations.
# TODO(bazel-ready): Read plugins via runfiles, for testing.
genrule(
    name = "plugin_config_h",
    srcs = ["src/plugin_config.h.in"],
    outs = ["src/plugin_config.h"],
    cmd = """
        sed -e 's|@EXTENSIONS_PLUGINS_DIR_MACRO@|{ext_dir}|' \
            -e 's|@USER_TYPES_PLUGINS_DIR_MACRO@|{utype_dir}|' \
            -e 's/@EXTERN_EXTENSIONS_LIST@//' \
            -e 's/@EXTERN_USER_TYPE_LIST@//' \
            -e 's/@EXTENSIONS_LIST_SIZE@//' \
            -e 's/@USER_TYPE_LIST_SIZE@//' \
            -e 's/@MEMCPY_EXTENSIONS_LIST@//' \
            -e 's/@MEMCPY_USER_TYPE_LIST@//' \
            -e 's/@STATIC_LOADED_PLUGINS_COUNT@//' \
            -e 's/@STATIC_LOADED_PLUGINS@//' \
            $(SRCS) > $@
    """.format(
        ext_dir = EXTENSIONS_PLUGINS_DIR,
        utype_dir = USER_TYPES_PLUGINS_DIR,
    ),
)

cc_library(
    name = "public_headers",
    hdrs = [":public_header_files"],
    strip_include_prefix = "src",
    include_prefix = "libyang",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "libyang",
    srcs = [
        "src/common.c",
        "src/context.c",
        "src/hash_table.c",
        "src/log.c",
        "src/parser.c",
        "src/parser_json.c",
        "src/parser_lyb.c",
        "src/parser_xml.c",
        "src/parser_yang.c",
        "src/parser_yang_bis.c",
        "src/parser_yang_lex.c",
        "src/parser_yin.c",
        "src/plugins.c",
        "src/printer.c",
        "src/printer_info.c",
        "src/printer_json.c",
        "src/printer_json_schema.c",
        "src/printer_lyb.c",
        "src/printer_tree.c",
        "src/printer_xml.c",
        "src/printer_yang.c",
        "src/printer_yin.c",
        "src/resolve.c",
        "src/tree_data.c",
        "src/tree_schema.c",
        "src/validation.c",
        "src/xml.c",
        "src/xpath.c",
        "src/yang_types.c",

        # Generated headers (used as srcs since they are internal)
        ":common_h",
        ":plugin_config_h",
    ],
    hdrs = glob([
        "src/*.h",
        "models/*.h",
    ]),
    includes = [
        "src",
    ],
    copts = LIBYANG_COPTS + [
        "-std=c11",
        "-fvisibility=hidden",
        "-fPIC",
    ],
    local_defines = [
        "_GNU_SOURCE",
        "_FILE_OFFSET_BITS=64",
    ],
    deps = [
        ":public_headers",
        "@libyang_deps//libpcre3-dev:libpcre3",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libyang_shared",
    deps = [":libyang"],
    visibility = ["//visibility:public"],
)

# -- Extension plugins (shared objects loaded at runtime) --

# TODO(bazel-ready): Make plugin lists configurable (e.g. via build settings)
#   so consumers can choose which plugins to include.
EXTENSION_PLUGINS = [
    "nacm",
    "metadata",
    "yangdata",
]

[cc_binary(
    name = "ext_%s" % ext,
    srcs = ["src/extensions/%s.c" % ext],
    copts = LIBYANG_COPTS + [
        "-std=c11",
        "-fPIC",
    ],
    linkshared = 1,
    deps = [":libyang"],
    visibility = ["//visibility:public"],
) for ext in EXTENSION_PLUGINS]

# -- User type plugins (shared objects loaded at runtime) --

USER_TYPE_PLUGINS = [
    "user_yang_types",
    "user_inet_types",
]

[cc_binary(
    name = "utype_%s" % utype,
    srcs = ["src/user_types/%s.c" % utype],
    copts = LIBYANG_COPTS + [
        "-std=c11",
        "-fPIC",
    ],
    linkshared = 1,
    deps = [":libyang"],
    visibility = ["//visibility:public"],
) for utype in USER_TYPE_PLUGINS]

# -- CLI tools --

cc_binary(
    name = "yanglint",
    srcs = [
        "tools/lint/main.c",
        "tools/lint/main_ni.c",
        "tools/lint/commands.c",
        "tools/lint/completion.c",
        "tools/lint/configuration.c",
        "linenoise/linenoise.c",
    ] + glob(["tools/lint/*.h", "linenoise/*.h"]),
    copts = LIBYANG_COPTS + [
        "-std=c11",
    ],
    local_defines = ["_GNU_SOURCE"],
    deps = [":libyang"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "yangre",
    srcs = ["tools/re/main.c"],
    copts = LIBYANG_COPTS + [
        "-std=c11",
    ],
    local_defines = ["_GNU_SOURCE"],
    deps = [":libyang"],
    visibility = ["//visibility:public"],
)

# -- C++ bindings: libyang-cpp --
# Matches the libyang-cpp debian package

cc_library(
    name = "libyang-cpp",
    srcs = [
        "swig/cpp/src/Internal.cpp",
        "swig/cpp/src/Libyang.cpp",
        "swig/cpp/src/Tree_Data.cpp",
        "swig/cpp/src/Tree_Schema.cpp",
        "swig/cpp/src/Xml.cpp",
    ],
    hdrs = [":cpp_header_files"],
    includes = ["swig/cpp/src"],
    copts = LIBYANG_COPTS + [
        "-std=c++11",
        "-fPIC",
    ],
    deps = [":libyang"],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libyang-cpp_shared",
    deps = [":libyang-cpp"],
    visibility = ["//visibility:public"],
)

# -- C++ public headers (for libyang-cpp-dev package) --

cc_library(
    name = "cpp_swig_public_headers",
    hdrs = [":cpp_header_files"],
    strip_include_prefix = "swig/cpp/src",
    include_prefix = "libyang",
    visibility = ["//visibility:public"],
)

# -- Python bindings (SWIG) --

# Extract SWIG library files from apt package for hermetic builds
swig_lib_deb(
    name = "swig_lib",
    data = "@libyang_deps//swig",
    strip_prefix = "usr/share/swig4.0",
)

# Generate C++ wrapper and Python module from SWIG interface
# TODO(bazel-ready): Solve the warnings.
swig_gen(
    name = "yang_swig_gen",
    interface = "swig/python/yang.i",
    cpp_out = "swig/python/yang_wrap.cpp",
    python_out = "swig/python/yang.py",
    hdrs = [
        "swig/swig_base/python_base.i",
        "swig/swig_base/base.i",
        "swig/swig_base/cpp_classes.i",
        "swig/swig_base/libyangEnums.i",
    ] + CPP_SWIG_HEADERS + PUBLIC_HEADERS + [
        # Generated libyang.h (for libyangEnums.i %include "libyang.h")
        ":libyang_h",
    ],
    deps = [":libyang-cpp", ":libyang"],
    swig_lib = ":swig_lib",
)

# Compile SWIG-generated wrapper into a Python-loadable shared library
# TODO(bazel-ready): Revisit linkstatic. Currently dynamically linking
#   to match debian, but may want static linking for hermetic container images.
cc_binary(
    name = "_yang_swig_lib",
    srcs = ["swig/python/yang_wrap.cpp"],
    copts = LIBYANG_COPTS + [
        "-fvisibility=hidden",
        "-fPIC",
        "-std=c++11",
    ],
    linkshared = 1,
    deps = [
        ":libyang",
        ":libyang-cpp",
        "@libyang_deps//python3-dev:python3",
    ],
)

# Python package wrapping the native SWIG extension
py_native_library(
    name = "yang",
    native_so = ":_yang_swig_lib",
    native_py = "swig/python/yang.py",
    cc_deps = [":libyang-cpp", ":libyang"],
    visibility = ["//visibility:public"],
)

# -- Generated pkg-config file (needed by libyang-dev) --

genrule(
    name = "libyang_pc",
    srcs = ["libyang.pc.in"],
    outs = ["libyang.pc"],
    cmd = """
        sed -e 's|@CMAKE_INSTALL_PREFIX@|{prefix}|' \
            -e 's|@CMAKE_INSTALL_INCLUDEDIR@|{includedir}|' \
            -e 's|@CMAKE_INSTALL_LIBDIR@|{libdir}|' \
            -e 's|@PROJECT_NAME@|libyang|' \
            -e 's|@LIBYANG_DESCRIPTION@|{description}|' \
            -e 's|@LIBYANG_VERSION@|{version}|' \
            -e 's|@LIBYANG_SOVERSION_FULL@|{soversion}|' \
            $(SRCS) > $@
    """.format(
        prefix = INSTALL_PREFIX,
        includedir = INSTALL_INCLUDEDIR,
        libdir = INSTALL_LIBDIR,
        description = LIBYANG_DESCRIPTION,
        version = LIBYANG_VERSION,
        soversion = LIBYANG_SOVERSION_FULL,
    ),
)

# -- deb packages --

# TODO(bazel-ready): Add manual pages for yanglint and yangre
tar(
    name = "libyang_pkg",
    srcs = [
        ":yanglint",
        ":yangre",
        ":libyang_shared",
    ] + [":ext_%s" % ext for ext in EXTENSION_PLUGINS] + [
        ":utype_%s" % utype for utype in USER_TYPE_PLUGINS
    ],
    mtree = [
        "./usr/bin/yanglint uid=0 gid=0 time=0 mode=0755 type=file content=$(location :yanglint)",
        "./usr/bin/yangre uid=0 gid=0 time=0 mode=0755 type=file content=$(location :yangre)",
        pkg_path(".{prefix}/{libdir}/libyang.so.{soversion_full} uid=0 gid=0 time=0 mode=0755 type=file content=$(location :libyang_shared)"),
        pkg_path(".{prefix}/{libdir}/libyang.so.{soversion_major} uid=0 gid=0 time=0 mode=0755 type=link link=libyang.so.{soversion_full}"),
    ] + [
        pkg_path(
            ".{prefix}/{libdir}/libyang/extensions/{ext}.so uid=0 gid=0 time=0 mode=0755 type=file content=$(location :ext_{ext})",
            ext = ext,
        )
        for ext in EXTENSION_PLUGINS
    ] + [
        pkg_path(
            ".{prefix}/{libdir}/libyang/user_types/{utype}.so uid=0 gid=0 time=0 mode=0755 type=file content=$(location :utype_{utype})",
            utype = utype,
        )
        for utype in USER_TYPE_PLUGINS
    ],
    visibility = ["//visibility:public"],
)

tar(
    name = "libyang-dev_pkg",
    srcs = [
        ":libyang_pc",
    ] + PUBLIC_HEADERS + [
        ":libyang_h",
    ],
    mtree = [
        pkg_path(".{prefix}/{libdir}/libyang.so uid=0 gid=0 time=0 mode=0755 type=link link=libyang.so.{soversion_full}"),
        pkg_path(".{prefix}/{libdir}/pkgconfig/libyang.pc uid=0 gid=0 time=0 mode=0644 type=file content=$(location :libyang_pc)"),
    ] + [
        pkg_path(
            ".{prefix}/{includedir}/libyang/{hdr} uid=0 gid=0 time=0 mode=0644 type=file content=$(location {src})",
            hdr = hdr.split("/")[-1],
            src = hdr,
        )
        for hdr in PUBLIC_HEADERS
    ] + [
        pkg_path(".{prefix}/{includedir}/libyang/libyang.h uid=0 gid=0 time=0 mode=0644 type=file content=$(location :libyang_h)"),
    ],
    visibility = ["//visibility:public"],
)

tar(
    name = "python3-yang_pkg",
    srcs = [
        ":_yang_swig_lib",
        "swig/python/yang.py",
    ],
    mtree = [
        "./usr/lib/python3/dist-packages/_yang.so uid=0 gid=0 time=0 mode=0755 type=file content=$(location :_yang_swig_lib)",
        "./usr/lib/python3/dist-packages/yang.py uid=0 gid=0 time=0 mode=0644 type=file content=$(location swig/python/yang.py)",
    ],
    visibility = ["//visibility:public"],
)

tar(
    name = "libyang-cpp_pkg",
    srcs = [":libyang-cpp_shared"],
    mtree = [
        pkg_path(".{prefix}/{libdir}/libyang-cpp.so.{soversion_full} uid=0 gid=0 time=0 mode=0755 type=file content=$(location :libyang-cpp_shared)"),
        pkg_path(".{prefix}/{libdir}/libyang-cpp.so.{soversion_major} uid=0 gid=0 time=0 mode=0755 type=link link=libyang-cpp.so.{soversion_full}"),
    ],
    visibility = ["//visibility:public"],
)

assert_tar(
    name = "libyang_pkg_test",
    tar = ":libyang_pkg",
    expected = "bazel_tests/libyang_pkg.expected.txt",
    inconsistent_sizes = [
        "./usr/bin/yanglint",
        "./usr/bin/yangre",
        "./usr/lib/x86_64-linux-gnu/libyang.so.1.2.2",
        "./usr/lib/x86_64-linux-gnu/libyang/extensions/metadata.so",
        "./usr/lib/x86_64-linux-gnu/libyang/extensions/nacm.so",
        "./usr/lib/x86_64-linux-gnu/libyang/extensions/yangdata.so",
        "./usr/lib/x86_64-linux-gnu/libyang/user_types/user_inet_types.so",
        "./usr/lib/x86_64-linux-gnu/libyang/user_types/user_yang_types.so",
    ],
)

assert_tar(
    name = "libyang-dev_pkg_test",
    tar = ":libyang-dev_pkg",
    expected = "bazel_tests/libyang-dev_pkg.expected.txt",
)

assert_tar(
    name = "python3-yang_pkg_test",
    tar = ":python3-yang_pkg",
    expected = "bazel_tests/python3-yang_pkg.expected.txt",
    inconsistent_sizes = [
        "./usr/lib/python3/dist-packages/_yang.so",
    ],
)

assert_tar(
    name = "libyang-cpp_pkg_test",
    tar = ":libyang-cpp_pkg",
    expected = "bazel_tests/libyang-cpp_pkg.expected.txt",
    inconsistent_sizes = [
        "./usr/lib/x86_64-linux-gnu/libyang-cpp.so.1.2.2",
    ],
)
