"""Repository rule that exposes the gnoi `.proto` files from the vendored submodule.

The //gnoi submodule is listed in .bazelignore,
because its own BUILD files describe a gRPC/Go build we don't want.
Therefore, we can't reference its files by glob, and need to import them at repository time.
"""

# Any file at the root of this workspace, used to find the submodule beside it.
_ANCHOR = Label("//:MODULE.bazel")

_SUBMODULE = "gnoi"

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
        ctx.symlink(submodule.get_child(*proto.split("/")), proto)

    ctx.file("BUILD.bazel", """\
package(default_visibility = ["//visibility:public"])

exports_files({protos})

filegroup(
    name = "protos",
    srcs = {protos},
)
""".format(protos = repr(_PROTOS)))

vendored_protos = repository_rule(
    implementation = _vendored_protos_impl,
    doc = "Mirrors the gnoi `.proto` files out of the .bazelignore'd //gnoi submodule.",
)
