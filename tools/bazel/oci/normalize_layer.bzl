"""Adapt source/imported TARs to the checked base's directory layout."""

load("@bazel_skylib//lib:paths.bzl", "paths")

_DIRECTORY_ALIASES = {
    "bin": "usr/bin",
    "lib": "usr/lib",
    "lib64": "usr/lib64",
    "sbin": "usr/sbin",
    "var/run": "/run",
}

def _normalize_layer_impl(ctx):
    if not ctx.file.base.is_directory:
        fail("normalize_layer requires one OCI layout directory as base")
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    receipt = ctx.actions.declare_file(ctx.label.name + ".normalization.json")
    policy = ctx.actions.declare_file(ctx.label.name + ".policy.json")
    ctx.actions.write(policy, json.encode({
        "expected_platform": ctx.attr.expected_platform,
        "directory_aliases": {name: {"linkname": target, "target": paths.normalize(paths.join(paths.dirname(name), target)).lstrip("/")} for name, target in ctx.attr.directory_aliases.items()},
        "root_owned": ctx.attr.root_owned,
        "modes": ctx.attr.modes,
    }))
    args = ctx.actions.args()
    args.add("--src", ctx.file.src)
    args.add("--base", ctx.file.base.path)
    args.add("--output", output)
    args.add("--receipt", receipt)
    args.add("--policy", policy)
    ctx.actions.run(
        executable = ctx.attr._tool[DefaultInfo].files_to_run,
        arguments = [args],
        inputs = [ctx.file.src, ctx.file.base, policy],
        outputs = [output, receipt],
        mnemonic = "NormalizeImageLayer",
        progress_message = "Normalize payload paths in %{label}",
    )
    return [DefaultInfo(files = depset([output])), OutputGroupInfo(normalization = depset([receipt]))]

normalize_layer = rule(
    implementation = _normalize_layer_impl,
    doc = "Normalize one TAR while retaining imported ownership unless root_owned is selected.",
    attrs = {
        "src": attr.label(allow_single_file = True, mandatory = True),
        "base": attr.label(allow_single_file = True, mandatory = True),
        "expected_platform": attr.string(mandatory = True),
        "directory_aliases": attr.string_dict(default = _DIRECTORY_ALIASES),
        "root_owned": attr.bool(default = False),
        "modes": attr.string_dict(),
        "tars": attr.label_list(doc = "Original owner TAR edges retained for debug_symbols_layer's aspect."),
        "_tool": attr.label(default = Label("//tools/bazel/oci:normalize_layer_tool"), executable = True, cfg = "exec"),
    },
)
