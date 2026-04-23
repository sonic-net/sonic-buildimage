"""Repository rule for libnl3 source archive.

Wraps http_archive to template the BUILD file with the correct
exec-root-relative repo path. This is needed because the BUILD file
uses -I copts for the linux-private headers (to shadow system headers),
and the -I path must include the repo's canonical name, which varies
depending on whether libnl3 is the main module or a dependency.
"""

def _libnl3_src_impl(repository_ctx):
    repository_ctx.download_and_extract(
        url = repository_ctx.attr.urls,
        sha256 = repository_ctx.attr.sha256,
        stripPrefix = repository_ctx.attr.strip_prefix,
    )

    for patch in repository_ctx.attr.patches:
        repository_ctx.patch(
            repository_ctx.path(patch),
            strip = 1,
        )

    repo_dir = "external/" + repository_ctx.name
    repository_ctx.template(
        "BUILD.bazel",
        repository_ctx.attr.build_file_template,
        substitutions = {
            "{REPO_DIR}": repo_dir,
        },
    )

libnl3_src = repository_rule(
    implementation = _libnl3_src_impl,
    attrs = {
        "urls": attr.string_list(mandatory = True),
        "sha256": attr.string(mandatory = True),
        "strip_prefix": attr.string(),
        "patches": attr.label_list(),
        "build_file_template": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
    },
)
