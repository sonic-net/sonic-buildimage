load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_shared_library")
load("@tar.bzl", "tar")

# Header-only targets for modules that resolve symbols at runtime from
# the hsflowd binary (loaded via dlopen, symbols exported via -rdynamic).
cc_library(
    name = "libsflow_headers",
    hdrs = [
        "src/sflow/sflow.h",
        "src/sflow/sflow_api.h",
        "src/sflow/sflow_drop.h",
    ],
    includes = ["src/sflow"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "libcjson_headers",
    hdrs = ["src/json/cJSON.h"],
    includes = ["src/json"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "libsflow",
    srcs = [
        "src/sflow/sflow_agent.c",
        "src/sflow/sflow_notifier.c",
        "src/sflow/sflow_poller.c",
        "src/sflow/sflow_receiver.c",
        "src/sflow/sflow_sampler.c",
    ],
    hdrs = [
        "src/sflow/sflow.h",
        "src/sflow/sflow_api.h",
        "src/sflow/sflow_drop.h",
    ],
    copts = [
        "-Wall",
        "-Wcast-align",
    ],
    defines = [
        "_GNU_SOURCE",
        "STDC_HEADERS",
    ],
    includes = ["src/sflow"],
    visibility = ["//visibility:public"],
)

CJSON_COPTS = [
    "-std=c89",
    "-fstack-protector-strong",
    "-fPIC",
    "-pedantic",
    "-Wall",
    "-Werror",
    "-Wstrict-prototypes",
    "-Wwrite-strings",
    "-Wshadow",
    "-Winit-self",
    "-Wcast-align",
    "-Wformat=2",
    "-Wmissing-prototypes",
    "-Wstrict-overflow=2",
    "-Wcast-qual",
    "-Wc++-compat",
    "-Wundef",
    "-Wswitch-default",
    "-Wconversion",
]

cc_library(
    name = "libcjson",
    srcs = ["src/json/cJSON.c"],
    hdrs = ["src/json/cJSON.h"],
    copts = CJSON_COPTS,
    includes = ["src/json"],
    linkopts = ["-lm"],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libcjson_shared",
    deps = [":libcjson"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "libcjson_utils",
    srcs = ["src/json/cJSON_Utils.c"],
    hdrs = ["src/json/cJSON_Utils.h"],
    copts = CJSON_COPTS,
    includes = ["src/json"],
    deps = [":libcjson"],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libcjson_utils_shared",
    deps = [":libcjson_utils"],
    visibility = ["//visibility:public"],
)

# ===== src/Linux targets (FEATURES=SONIC) =====

HSFLOWD_HEADERS = glob([
    "src/Linux/*.h",
    "src/Linux/linux/*.h",
])

HSFLOWD_COPTS = [
    "-std=gnu99",
    "-g",
    "-O2",
    "-fPIC",
    "-Wall",
    "-Wstrict-prototypes",
    "-Wunused-value",
    "-Wunused-function",
]

HSFLOWD_DEFINES = [
    "_GNU_SOURCE",
    "UTHEAP",
    "HSP_OPTICAL_STATS",
    "HSP_VERSION=2_0_51_26",
    "HSP_MOD_DIR=/etc/hsflowd/modules",
    "HSP_LOAD_SONIC",
    "PROCFS=/proc",
    "SYSFS=/sys",
    "ETCFS=/etc",
    "VARFS=/var",
]

HSFLOWD_INCLUDES = [
    "src/Linux",
    "src/sflow",
    "src/json",
]

cc_library(
    name = "libhsflowd",
    srcs = [
        "src/Linux/evbus.c",
        "src/Linux/hsflowconfig.c",
        "src/Linux/hsflowd.c",
        "src/Linux/readCpuCounters.c",
        "src/Linux/readDiskCounters.c",
        "src/Linux/readHidCounters.c",
        "src/Linux/readInterfaces.c",
        "src/Linux/readMemoryCounters.c",
        "src/Linux/readNioCounters.c",
        "src/Linux/readPackets.c",
        "src/Linux/readTcpipCounters.c",
        "src/Linux/util.c",
    ],
    hdrs = HSFLOWD_HEADERS,
    copts = HSFLOWD_COPTS,
    defines = HSFLOWD_DEFINES,
    includes = HSFLOWD_INCLUDES,
    linkopts = [
        "-lm",
        "-pthread",
        "-ldl",
        "-lrt",
    ],
    deps = [
        ":libcjson",
        ":libsflow",
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "hsflowd",
    linkopts = ["-rdynamic"],
    deps = [":libhsflowd"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "util_netlink",
    srcs = ["src/Linux/util_netlink.c"] + HSFLOWD_HEADERS,
    copts = HSFLOWD_COPTS,
    defines = HSFLOWD_DEFINES,
    includes = HSFLOWD_INCLUDES,
    deps = [":libsflow_headers"],
    visibility = ["//visibility:public"],
)

# --- Module: mod_sonic (SONIC) ---
# TODO(bazel-ready): We don't support REDISONLY (HSP_SONIC_TEST_REDISONLY) mode.

cc_library(
    name = "mod_sonic",
    srcs = ["src/Linux/mod_sonic.c"] + HSFLOWD_HEADERS,
    copts = HSFLOWD_COPTS,
    defines = HSFLOWD_DEFINES,
    includes = HSFLOWD_INCLUDES,
    deps = [
        ":libcjson_headers",
        ":libsflow_headers",
        "@hsflowd_deps//libhiredis-dev:libhiredis",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "mod_sonic_shared",
    deps = [":mod_sonic"],
    visibility = ["//visibility:public"],
)

# --- Module: mod_psample (PSAMPLE) ---

cc_library(
    name = "mod_psample",
    srcs = ["src/Linux/mod_psample.c"] + HSFLOWD_HEADERS,
    copts = HSFLOWD_COPTS,
    defines = HSFLOWD_DEFINES,
    includes = HSFLOWD_INCLUDES,
    deps = [
        ":libsflow_headers",
        ":util_netlink",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "mod_psample_shared",
    deps = [":mod_psample"],
    visibility = ["//visibility:public"],
)

# --- Module: mod_docker (DOCKER) ---

cc_library(
    name = "mod_docker",
    srcs = ["src/Linux/mod_docker.c"] + HSFLOWD_HEADERS,
    copts = HSFLOWD_COPTS,
    defines = HSFLOWD_DEFINES,
    includes = HSFLOWD_INCLUDES,
    linkopts = ["-lm"],
    deps = [
        ":libcjson_headers",
        ":libsflow_headers",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "mod_docker_shared",
    deps = [":mod_docker"],
    visibility = ["//visibility:public"],
)

# --- Module: mod_dropmon (DROPMON) ---

cc_library(
    name = "mod_dropmon",
    srcs = ["src/Linux/mod_dropmon.c"] + HSFLOWD_HEADERS,
    copts = HSFLOWD_COPTS,
    defines = HSFLOWD_DEFINES,
    includes = HSFLOWD_INCLUDES,
    deps = [
        ":libsflow_headers",
        ":util_netlink",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "mod_dropmon_shared",
    deps = [":mod_dropmon"],
    visibility = ["//visibility:public"],
)

# --- Module: mod_json (always built) ---

cc_library(
    name = "mod_json",
    srcs = ["src/Linux/mod_json.c"] + HSFLOWD_HEADERS,
    copts = HSFLOWD_COPTS,
    defines = HSFLOWD_DEFINES,
    includes = HSFLOWD_INCLUDES,
    deps = [
        ":libcjson_headers",
        ":libsflow_headers",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "mod_json_shared",
    deps = [":mod_json"],
    visibility = ["//visibility:public"],
)

# --- Module: mod_dnssd (always built) ---

cc_library(
    name = "mod_dnssd",
    srcs = ["src/Linux/mod_dnssd.c"] + HSFLOWD_HEADERS,
    copts = HSFLOWD_COPTS,
    defines = HSFLOWD_DEFINES,
    includes = HSFLOWD_INCLUDES,
    # libresolv is part of libc6-dev (no separate -dev package). The libc6
    # cc_library exposes libresolv.so as a linker input but we still need
    # -lresolv to actually link against it.
    linkopts = ["-lresolv"],
    deps = [
        ":libsflow_headers",
        "@hsflowd_deps//libc6-dev:libc6",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "mod_dnssd_shared",
    deps = [":mod_dnssd"],
    visibility = ["//visibility:public"],
)

# ===== Packaging =====

tar(
    name = "hsflowd_pkg",
    srcs = [
        ":hsflowd",
        ":mod_dnssd_shared",
        ":mod_docker_shared",
        ":mod_dropmon_shared",
        ":mod_json_shared",
        ":mod_psample_shared",
        ":mod_sonic_shared",
        "src/Linux/scripts/hsflowd.conf.sonic",
        "src/Linux/scripts/hsflowd.deb",
        "src/Linux/scripts/hsflowd.service",
        "src/Linux/scripts/net.sflow.hsflowd.conf",
    ],
    mtree = [
        "./usr/sbin/hsflowd uid=0 gid=0 mode=0700 type=file content=$(location :hsflowd)",
        # TODO(bazel-ready): We only support FEATURES=SONIC; other features use different config files.
        "./etc/hsflowd.conf uid=0 gid=0 mode=0644 type=file content=$(location src/Linux/scripts/hsflowd.conf.sonic)",
        "./etc/hsflowd/modules/mod_sonic.so uid=0 gid=0 mode=0755 type=file content=$(location :mod_sonic_shared)",
        "./etc/hsflowd/modules/mod_psample.so uid=0 gid=0 mode=0755 type=file content=$(location :mod_psample_shared)",
        "./etc/hsflowd/modules/mod_docker.so uid=0 gid=0 mode=0755 type=file content=$(location :mod_docker_shared)",
        "./etc/hsflowd/modules/mod_dropmon.so uid=0 gid=0 mode=0755 type=file content=$(location :mod_dropmon_shared)",
        "./etc/hsflowd/modules/mod_json.so uid=0 gid=0 mode=0755 type=file content=$(location :mod_json_shared)",
        "./etc/hsflowd/modules/mod_dnssd.so uid=0 gid=0 mode=0755 type=file content=$(location :mod_dnssd_shared)",
        # TODO(bazel-ready): We only support Debian; other distros use different init scripts.
        "./etc/init.d/hsflowd uid=0 gid=0 mode=0755 type=file content=$(location src/Linux/scripts/hsflowd.deb)",
        "./lib/systemd/system/hsflowd.service uid=0 gid=0 mode=0644 type=file content=$(location src/Linux/scripts/hsflowd.service)",
        "./etc/dbus-1/system.d/net.sflow.hsflowd.conf uid=0 gid=0 mode=0644 type=file content=$(location src/Linux/scripts/net.sflow.hsflowd.conf)",
    ],
    visibility = ["//visibility:public"],
)
