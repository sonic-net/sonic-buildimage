"""Assemble OCI layers with Make's file exclusions while retaining legal notices.

TODO(bazel-ready): move this to sonic-build-infra/tar once dpkg_01_drop can be inlined there.
It lives here for now because the file is in //dockers/docker-base-trixie.
"""

load("@rules_distroless//distroless:defs.bzl", "flatten")
load("@sonic_build_infra//binary:debug_symbols.bzl", "DebugSymbolsInfo")
load("@tar.bzl//tar:tar.bzl", "tar_lib")

_EXCLUDES = "//tools/bazel/dpkg:dpkg_excludes.txt"

# bsdtar can read --excludes from a file, but not includes.
# So, we need to list the arguments by hand.
# This is kept in sync with dpkg_01_drop via //tools/bazel/tests:test_dpkg_patterns_up_to_date.
PATH_INCLUDES = [
    "./usr/share/doc/*/copyright",
    "usr/share/doc/*/copyright",
]

# Source TARs retain upstream license names rather than Debian's "copyright".
# Keep these notices in Bazel OCI layers without changing Make's dpkg policy.
# Both spellings are needed because bsdtar matches the original member name.
_OCI_NOTICE_INCLUDES = [
    prefix + name
    for prefix in ["./usr/share/doc/*/", "usr/share/doc/*/"]
    for name in [
        "LICENSE*",
        "LICENCE*",
        "*-LICENSE*",
        "*-LICENCE*",
        "COPYING*",
        "NOTICE*",
        "license*",
        "licence*",
        "*-license*",
        "*-licence*",
        "copying*",
        "notice*",
    ]
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
    merged = ctx.actions.declare_file(ctx.attr.name + "_merged.tar") if ctx.attr.root_owned else output
    _copy(ctx, "DpkgMerge", bsdtar, merged, [excluded, included])
    if ctx.attr.root_owned:
        ctx.actions.run(
            executable = ctx.executable._root_owned_tar,
            arguments = [merged.path, output.path],
            inputs = [merged],
            outputs = [output],
            mnemonic = "RootOwnedLayer",
        )

    # flatten and this filter must not hide the original archives' debug trees.
    # The image collector traverses base/tars edges, so it stops at our src edge
    # and relies on this provider to retain the corresponding deployed symbols.
    debug_deps = [ctx.attr.src] + ctx.attr.debug_symbol_deps + ctx.attr.tars
    return [
        DefaultInfo(files = depset([output])),
        DebugSymbolsInfo(symbols = depset(transitive = [
            dep[DebugSymbolsInfo].symbols
            for dep in debug_deps
            if DebugSymbolsInfo in dep
        ])),
    ]

dpkg_filter = rule(
    doc = """Apply the base image's dpkg exclusions, preserving OCI license notices.

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
        # Retain the original tars edge for debug_symbols_layer's transitive
        # traversal, including dependency packages assembled with flatten().
        "tars": attr.label_list(allow_files = True),
        "root_owned": attr.bool(
            default = False,
            doc = "Normalize component runtime ownership; preserve Debian package ownership by default.",
        ),
        "_root_owned_tar": attr.label(
            default = Label("@sonic_build_infra//tar:root_owned_tar"),
            executable = True,
            cfg = "exec",
        ),
        "debug_symbol_deps": attr.label_list(
            doc = "Original archives whose debug metadata must survive flattening and filtering.",
            allow_files = True,
        ),
        "excludes": attr.label_list(
            doc = "Files of bsdtar exclude patterns, one per line.",
            allow_files = True,
            default = [_EXCLUDES],
        ),
        "includes": attr.string_list(
            doc = "bsdtar include patterns, carving back out of the excludes.",
            default = PATH_INCLUDES + _OCI_NOTICE_INCLUDES,
        ),
    },
    toolchains = [tar_lib.toolchain_type],
)

def sonic_layer(name, tars, deduplicate = True, **kwargs):
    """Flatten `tars` into one layer, minus whatever dpkg would have filtered out (e.g. man pages).

    Use this for source or Debian payload TARs. Apply Make's documentation and
    locale exclusions while also keeping upstream source license/notice names.

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
        tars = tars,
        **kwargs
    )
