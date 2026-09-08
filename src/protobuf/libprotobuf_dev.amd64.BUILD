load("@rules_cc//cc:cc_library.bzl", "cc_library")

# BUILD file for the unpacked libprotobuf-dev `.deb`.
#
# `includes` is resolved against this repository's root, which is why the
# cc_library lives here rather than in //protobuf:BUILD.bazel.
#
# This is similar to what `rules_distroless` would generate, but hand-crafted to match the Make build.
cc_library(
    name = "libprotobuf",
    srcs = ["@libprotobuf32_amd64//:usr/lib/x86_64-linux-gnu/libprotobuf.so.32.0.12"],
    hdrs = glob([
        "usr/include/google/**/*.h",
        "usr/include/google/**/*.inc",
    ]),
    includes = ["usr/include"],
    visibility = ["//visibility:public"],
)

# The headers on their own, for a library that compiles against protobuf but doesn't link it.
# Usually used to maintain Make equivalence, but otherwise should probably link against libprotobuf.
cc_library(
    name = "libprotobuf_headers",
    hdrs = glob([
        "usr/include/google/**/*.h",
        "usr/include/google/**/*.inc",
    ]),
    includes = ["usr/include"],
    visibility = ["//visibility:public"],
)

# The well-known types.
# We export a single file here so that in defs.bzl we can use it to locate the proto install.
exports_files(["usr/include/google/protobuf/descriptor.proto"])

filegroup(
    name = "well_known_protos",
    srcs = glob(["usr/include/google/protobuf/**/*.proto"]),
    visibility = ["//visibility:public"],
)
