"""Common flags used througout the project."""

_COMPILE_OPTIONS = [
    "-Wall",
    "-fPIC",
    "-Wno-write-strings",
    "-Werror",
    "-Wno-reorder",
    "-Wcast-align",
    "-Wcast-qual",
    # TODO (b/314850353): Re-enable conversion errors with updated protoc compiler.
    # "-Wconversion",
    "-Wdisabled-optimization",
    "-Wextra",
    "-Wfloat-equal",
    "-Wformat=2",
    "-Wformat-nonliteral",
    "-Wformat-security",
    "-Wformat-y2k",
    "-Wimport",
    "-Winit-self",
    "-Winvalid-pch",
    "-Wlong-long",
    "-Wmissing-field-initializers",
    "-Wmissing-format-attribute",
    "-Wno-aggregate-return",
    "-Wno-padded",
    "-Wno-switch-enum",
    "-Wno-unused-parameter",
    "-Wpacked",
    "-Wpointer-arith",
    "-Wredundant-decls",
    "-Wstack-protector",
    "-Wstrict-aliasing=3",
    "-Wswitch",
    "-Wswitch-default",
    "-Wunreachable-code",
    "-Wunused",
    "-Wvariadic-macros",
    "-Wno-switch-default",
    "-Wno-long-long",
    "-Wno-redundant-decls",
]

CFLAGS_COMMON = ["-std=c++14"] + _COMPILE_OPTIONS
# The test binary includes GoogleTest headers, which require C++17 from 1.17.
CFLAGS_GTEST = ["-std=c++17"] + _COMPILE_OPTIONS

# TODO(bazel-ready): Enable debug flags
DBGFLAGS = []

CFLAGS_ASAN = select({
    "@sonic_build_infra//:asan_enabled": ["-fsanitize=address"],
    "//conditions:default": [],
})

LDFLAGS_ASAN = select({
    "@sonic_build_infra//:asan_enabled": ["-lasan"],
    "//conditions:default": [],
})
