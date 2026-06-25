"""A `gzip` rule that compresses a single file using the hermetic pigz binary."""

def _gzip_impl(ctx):
    out = ctx.actions.declare_file(ctx.label.name)
    # pigz only writes the compressed stream to stdout (-c); redirect it to the
    # declared output.
    #
    # --no-name omits the original filename/mtime from the gzip header,
    # keeping the output reproducible.
    ctx.actions.run_shell(
        inputs = [ctx.file.src],
        outputs = [out],
        tools = [ctx.executable._compressor],
        command = '"$1" --no-name --stdout "$2" > "$3"',
        arguments = [ctx.executable._compressor.path, ctx.file.src.path, out.path],
        mnemonic = "Gzip",
        progress_message = "Gzipping %{label}",
    )
    return [DefaultInfo(files = depset([out]))]

gzip = rule(
    implementation = _gzip_impl,
    doc = "Gzip-compress a single file with the hermetic pigz binary. " +
          "The output file is named after the target.",
    attrs = {
        "src": attr.label(
            allow_single_file = True,
            mandatory = True,
            doc = "The file to compress.",
        ),
        "_compressor": attr.label(
            default = Label("@pigz"),
            executable = True,
            cfg = "exec",
            doc = "A gzip-compatible compressor binary (defaults to @pigz).",
        ),
    },
)
