load("@bazel_lib//lib:copy_to_directory.bzl", "copy_to_directory_bin_action")
load("@bazel_skylib//lib:paths.bzl", "paths")

def _root_path(header, package):
    basedir = header.short_path[:-len(header.basename) - 1]
    partition = basedir.partition(package)
    return partition[1] + partition[2]

def _extract_proto_headers(ctx):
    copy_bin = ctx.toolchains["@bazel_lib//lib:copy_to_directory_toolchain_type"].copy_to_directory_info.bin

    all_headers = ctx.attr.cc_proto_target[CcInfo].compilation_context.direct_public_headers
    external_includes = ctx.attr.cc_proto_target[CcInfo].compilation_context.external_includes
    headers_to_move = [
        f
        for f in all_headers
        if "_virtual_includes" in f.dirname and f.path.endswith(".pb.h")
    ]

    package = ctx.attr.cc_proto_target.label.package
    root_paths = [_root_path(f, package) for f in headers_to_move]

    dst = ctx.actions.declare_directory(ctx.attr.outdir)

    copy_to_directory_bin_action(
        ctx,
        name = ctx.attr.name,
        dst = dst,
        copy_to_directory_bin = copy_bin,
        files = headers_to_move,
        root_paths = root_paths,
        include_external_repositories = ["*"],
    )

    return [
        DefaultInfo(
            files = depset([dst]),
            runfiles = ctx.runfiles([dst]),
        ),
    ]

extract_proto_headers = rule(
    implementation = _extract_proto_headers,
    doc = "A rule that extracts all the pb.h headers from a CcInfo and puts them in an appropriately-named directory. Please note that the files will all be placed at the root of the directory.",
    attrs = {
        "cc_proto_target": attr.label(
            mandatory = True,
            providers = [CcInfo],
        ),
        "outdir": attr.string(
            mandatory = True,
        ),
    },
    toolchains = ["@bazel_lib//lib:copy_to_directory_toolchain_type"],
)
