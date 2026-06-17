"""Rule to extract SWIG library files from apt package data."""

def _swig_lib_extract_impl(ctx):
    out_dir = ctx.actions.declare_directory(ctx.label.name)

    # strip_components needs to account for leading ./ in tar paths
    # e.g., ./usr/share/swig4.0/swig.swg -> strip 4 components to get swig.swg
    strip_count = ctx.attr.strip_prefix.count("/") + 2  # +1 for leading ./, +1 for the path itself

    ctx.actions.run_shell(
        inputs = ctx.files.data,
        outputs = [out_dir],
        command = """
            set -e
            mkdir -p {out}
            for tar in {srcs}; do
                tar -xzf "$tar" -C {out} --strip-components={strip} './{prefix}' 2>/dev/null || true
            done
        """.format(
            out = out_dir.path,
            srcs = " ".join([f.path for f in ctx.files.data]),
            strip = strip_count,
            prefix = ctx.attr.strip_prefix,
        ),
        mnemonic = "SwigLibExtract",
        progress_message = "Extracting SWIG library files",
    )

    return [DefaultInfo(files = depset([out_dir]))]

swig_lib_deb = rule(
    implementation = _swig_lib_extract_impl,
    attrs = {
        "data": attr.label(
            mandatory = True,
            allow_files = True,
            doc = "The apt package data tar.gz containing SWIG library files",
        ),
        "strip_prefix": attr.string(
            default = "usr/share/swig4.0",
            doc = "Path prefix to strip when extracting (e.g., 'usr/share/swig4.0')",
        ),
    },
)
