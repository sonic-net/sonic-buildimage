def _extract_tar_impl(ctx):
    tar_file = ctx.path(ctx.attr.archive)

    ctx.extract(
        archive = tar_file,
    )

    ctx.file("BUILD.bazel", ctx.read(ctx.attr.build_file))

extract_tar = repository_rule(
    implementation = _extract_tar_impl,
    doc = "Repository rule to extract tars with a BUILD file. Useful when we want to extract a dependency resolved with rules_distroless and need to glob the contents.",
    attrs = {
        "archive": attr.label(
            allow_single_file = True,
            doc = "Archive to extract. Please note that this rule runs at repository time, so aliases won't be resolved",
            mandatory = True,
        ),
        "build_file": attr.label(
            allow_single_file = True,
            mandatory = True,
        ),
    },
)
