load("@rules_python//python:defs.bzl", "py_library")

# Export the source files for building
filegroup(
    name = "srcs",
    srcs = glob(["**/*"]),
    visibility = ["//visibility:public"],
)

# The actual Python package will be built by the parent module
# using sdist_build_with_deps or similar rule that links against libyang3
exports_files(glob(["**/*"]))
