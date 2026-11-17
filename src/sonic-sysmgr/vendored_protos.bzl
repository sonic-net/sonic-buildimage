"""Repository rule that exposes the gnoi `.proto` files from the vendored submodule.

The //gnoi submodule is listed in .bazelignore,
because its own BUILD files describe a gRPC/Go build we don't want.
Therefore, we can't reference its files by glob, and need to import them at repository time.
"""

# Any file at the root of this workspace, used to find the submodule beside it.
_ANCHOR = Label("//:MODULE.bazel")

_SUBMODULE = "gnoi"
_IMPORT_PREFIX = "github.com/openconfig/gnoi"
_PROTOBUF_DEFS = Label("@sonic_protobuf//:defs.bzl")
_PROTOBUF_HEADERS = Label("@sonic_protobuf//:protobuf_headers")

# The protos rebootbackend needs. Same set, and same order, as //:Makefile.am compiles.
_PROTOS = [
    "types/types.proto",
    "common/common.proto",
    "system/system.proto",
]

def _vendored_protos_impl(ctx):
    workspace = ctx.path(_ANCHOR).dirname
    submodule = workspace.get_child(_SUBMODULE)

    for proto in _PROTOS:
        ctx.symlink(submodule.get_child(*proto.split("/")), _IMPORT_PREFIX + "/" + proto)

    ctx.file("BUILD.bazel", """\
load("{protobuf_defs}", "cpp_proto_sources")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cpp_proto_sources(
    name = "generated",
    srcs = {protos},
    output_prefix = "gen",
)

filegroup(name = "sources", srcs = [":generated"], output_group = "sources")
filegroup(name = "headers", srcs = [":generated"], output_group = "headers")
cc_library(
    name = "generated_headers",
    hdrs = [":headers"],
    includes = ["gen", "gen/github.com/openconfig/gnoi"],
    deps = ["{protobuf_headers}"],
)
""".format(
        protobuf_defs = _PROTOBUF_DEFS,
        protobuf_headers = _PROTOBUF_HEADERS,
        protos = repr([_IMPORT_PREFIX + "/" + proto for proto in _PROTOS]),
    ))

vendored_protos = repository_rule(
    implementation = _vendored_protos_impl,
    doc = "Mirrors the gnoi `.proto` files out of the .bazelignore'd //gnoi submodule.",
)
