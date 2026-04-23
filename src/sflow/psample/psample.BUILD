load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_shared_library")
load("@tar.bzl", "tar")

cc_library(
    name = "libpsample",
    srcs = [
        "src/mnlg.c",
        "src/mnlg.h",
        "src/psample.c",
    ],
    hdrs = [
        "include/linux/psample.h",
        "include/psample.h",
    ],
    copts = [
        "-g",
        "-fPIC",
    ],
    includes = ["include"],
    deps = [
        "@psample_deps//libmnl-dev:libmnl",
    ],
    visibility = ["//visibility:public"],
)

cc_shared_library(
    name = "libpsample_shared",
    deps = [":libpsample"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "psample",
    srcs = ["psample_tool/psample.c"],
    copts = ["-g"],
    includes = ["include"],
    deps = [":libpsample"],
    visibility = ["//visibility:public"],
)

tar(
    name = "psample_pkg",
    srcs = [
        ":libpsample_shared",
        ":psample",
    ],
    mtree = [
        "./usr/bin/psample uid=0 gid=0 mode=0755 type=file content=$(location :psample)",
        # TODO(bazel-ready): We only support x86_64 for now.
        "./usr/lib/x86_64-linux-gnu/libpsample.so uid=0 gid=0 mode=0755 type=file content=$(location :libpsample_shared)",
    ],
    visibility = ["//visibility:public"],
)
