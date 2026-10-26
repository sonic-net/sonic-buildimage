"""Build the existing image validation contract from producer-owned metadata."""

def _impl(ctx):
    output = ctx.actions.declare_file(ctx.attr.out)
    args = ctx.actions.args()
    args.add("--image", ctx.attr.image)
    args.add("--architecture", ctx.attr.architecture)
    args.add("--distribution", ctx.attr.distribution)
    args.add("--output", output)
    inputs = []
    for target in ctx.attr.owner_metadata:
        files = target[DefaultInfo].files.to_list()
        if len(files) != 1:
            fail("owner metadata must provide one JSON file")
        inputs.extend(files)
        args.add("--metadata", str(target.label) + "=" + files[0].path)
    for name, target in ctx.attr.source_modules.items():
        files = target[DefaultInfo].files.to_list()
        if len(files) != 1:
            fail("owner module must provide one MODULE.bazel file")
        inputs.extend(files)
        args.add("--module", name + "=" + files[0].path)
    for name, target in ctx.attr.sources.items():
        files = target[DefaultInfo].files.to_list()
        if len(files) != 1:
            fail("source package must provide one runtime TAR")
        inputs.extend(files)
        args.add("--payload", name + "=" + files[0].path)
        args.add("--source", name + "=" + ctx.attr.declared_sources[name])
        args.add("--public-source", name + "=" + ctx.attr.public_sources[name])
    ctx.actions.run(
        executable = ctx.executable._tool,
        inputs = inputs,
        outputs = [output],
        arguments = [args],
        mnemonic = "SourcePackageContract",
    )
    return [DefaultInfo(files = depset([output]))]

_contract = rule(
    implementation = _impl,
    attrs = {
        "image": attr.string(mandatory = True),
        "architecture": attr.string(mandatory = True, values = ["amd64", "arm64"]),
        "distribution": attr.string(mandatory = True),
        "owner_metadata": attr.label_list(allow_files = [".json"]),
        "source_modules": attr.string_keyed_label_dict(allow_files = True),
        "sources": attr.string_keyed_label_dict(allow_files = True),
        "public_sources": attr.string_dict(),
        "declared_sources": attr.string_dict(),
        "out": attr.string(default = "source_packages.json"),
        "_tool": attr.label(default = Label("//tools/bazel/oci:source_contract"), executable = True, cfg = "exec"),
    },
)

def source_contract(name, sources, **kwargs):
    """Keep public target spelling while checking Bazel's resolved target identity."""
    _contract(
        name = name,
        sources = sources,
        public_sources = sources,
        declared_sources = {name: str(native.package_relative_label(target)) for name, target in sources.items()},
        **kwargs
    )
