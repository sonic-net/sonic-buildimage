"""`sonic_layer`: assemble an image layer the way the Make build would.

TODO(bazel-ready): move this to sonic-build-infra/tar once dpkg_01_drop can be inlined there.
It lives here for now because the file is in //dockers/docker-base-trixie.
"""

load("@rules_distroless//distroless:defs.bzl", "flatten")
load("@tar.bzl//tar:tar.bzl", "tar_lib")

_EXCLUDES = "//tools/bazel/dpkg:dpkg_excludes.txt"

# bsdtar can read --excludes from a file, but not includes.
# So, we need to list the arguments by hand.
# This is kept in sync with dpkg_01_drop via //tools/bazel/dpkg:test_dpkg_patterns_up_to_date.
PATH_INCLUDES = [
    "./usr/share/doc/*/copyright",
    "usr/share/doc/*/copyright",
]

_TAR_ARGS = ["--format", "gnutar"]

def _copy(ctx, mnemonic, bsdtar, output, srcs, add_extra_args = lambda args: args):
    """Use bsdtar to copy `srcs` into `output`, using `add_extra_args` to keep or discard entries."""
    args = ctx.actions.args()
    args.add_all(_TAR_ARGS)
    args.add("--create")
    args.add("--file", output)
    add_extra_args(args)
    args.add_all(srcs, format_each = "@%s")

    ctx.actions.run(
        executable = bsdtar.tarinfo.binary,
        arguments = [args],
        inputs = srcs + ctx.files.excludes,
        outputs = [output],
        tools = bsdtar.default.files,
        mnemonic = mnemonic,
        progress_message = mnemonic + " %%{label}",
    )

def _dpkg_filter_impl(ctx):
    bsdtar = ctx.toolchains[tar_lib.toolchain_type]

    def add_excludes(args):
        for pattern_file in ctx.files.excludes:
            args.add("--exclude-from", pattern_file)

    excluded = ctx.actions.declare_file(ctx.attr.name + "_excluded.tar")
    _copy(ctx, "DpkgExclude", bsdtar, excluded, [ctx.file.src], add_excludes)

    included = ctx.actions.declare_file(ctx.attr.name + "_included.tar")
    _copy(
        ctx,
        "DpkgInclude",
        bsdtar,
        included,
        [ctx.file.src],
        lambda args: args.add_all(ctx.attr.includes, format_each = "--include=%s"),
    )

    output = ctx.actions.declare_file(ctx.attr.name + ".tar")
    _copy(ctx, "DpkgMerge", bsdtar, output, [excluded, included])

    return [DefaultInfo(files = depset([output]))]

dpkg_filter = rule(
    doc = """Drop from a layer the paths the base image's dpkg config excludes.

dpkg applies path-include after path-exclude and lets the last match win.
In bsdtar, however, --exclude always wins over --include, regardless of the order.
So we need to build the excluded first, then the included, and then merge them.
""",
    implementation = _dpkg_filter_impl,
    attrs = {
        "src": attr.label(
            doc = "The layer to filter.",
            allow_single_file = tar_lib.common.accepted_tar_extensions,
            mandatory = True,
        ),
        "excludes": attr.label_list(
            doc = "Files of bsdtar exclude patterns, one per line.",
            allow_files = True,
            default = [_EXCLUDES],
        ),
        "includes": attr.string_list(
            doc = "bsdtar include patterns, carving back out of the excludes.",
            default = PATH_INCLUDES,
        ),
    },
    toolchains = [tar_lib.toolchain_type],
)

def sonic_layer(name, tars, deduplicate = True, **kwargs):
    """Flatten `tars` into one layer, minus whatever dpkg would have filtered out (e.g. man pages).

    Use this anywhere an image layer is built out of .deb archives,
    so that the layer holds exactly what the Make image would hold.

    Args:
        name: the filtered layer.
        tars: the archives to flatten, as interpreted by rules_distroless' flatten().
        deduplicate: drop duplicate directory entries after flattening.
        **kwargs: passed to the final, filtered layer (e.g. visibility).
    """
    flatten(
        name = name + "_unfiltered",
        tars = tars,
        deduplicate = deduplicate,
        visibility = ["//visibility:private"],
    )

    dpkg_filter(
        name = name,
        src = name + "_unfiltered",
        **kwargs
    )
