"""Import only symbols that match files deployed in the final runtime image."""

def _imported_debug_symbols_impl(ctx):
    if not ctx.file.runtime.is_directory:
        fail("runtime must be one complete OCI layout directory")
    output = ctx.actions.declare_file(ctx.label.name + ".tar")
    receipt = ctx.actions.declare_file(ctx.label.name + ".json")
    args = ctx.actions.args()
    args.add("--runtime", ctx.file.runtime.path)
    args.add("--expected-platform", ctx.attr.expected_platform)
    args.add_all(ctx.files.tars, before_each = "--tar")
    args.add_all(ctx.attr.required_paths, before_each = "--required-path")
    args.add("--output", output)
    args.add("--receipt", receipt)
    ctx.actions.run(
        executable = ctx.executable._matcher,
        arguments = [args],
        inputs = [ctx.file.runtime] + ctx.files.tars,
        outputs = [output, receipt],
        mnemonic = "MatchImportedDebugSymbols",
        progress_message = "Matching imported debug symbols to %{label}'s runtime",
    )
    return [DefaultInfo(files = depset([output])), OutputGroupInfo(receipt = depset([receipt]))]

imported_debug_symbols_layer = rule(
    implementation = _imported_debug_symbols_impl,
    attrs = {
        "runtime": attr.label(mandatory = True, allow_single_file = True, doc = "Final runtime OCI image, before adding debug layers."),
        "tars": attr.label_list(allow_files = True, doc = "Candidate imported debug archives. Matching regular ELF files retain their owner and mode."),
        "expected_platform": attr.string(mandatory = True, doc = "Required OCI os/architecture, for example linux/amd64."),
        "required_paths": attr.string_list(doc = "Runtime paths that must have matching symbols, preventing silent coverage loss."),
        "_matcher": attr.label(default = "//tools/bazel/oci:imported_symbols", executable = True, cfg = "exec"),
    },
    doc = "Select build-ID/debuglink matched companions and their validated DWZ supplements. Does not compile or install packages.",
)
