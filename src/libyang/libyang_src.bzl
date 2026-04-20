"""Repository rule for libyang source archive.

Wraps http_archive to template the BUILD file with the correct
exec-root-relative repo path. This is needed because the BUILD file
uses -I copts that must include the repo's canonical name, which varies
depending on whether libyang is the main module or a dependency.
"""

def _libyang_src_impl(repository_ctx):
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

    # Copy extra files (e.g. .bzl helpers, test data) into the fetched repo
    # so that load() statements and file references in BUILD.bazel resolve.
    # Label.name includes the path relative to the package, e.g.
    # "//:bazel_tests/foo.txt" has name "bazel_tests/foo.txt".
    for extra_file in repository_ctx.attr.extra_files:
        repository_ctx.symlink(
            repository_ctx.path(extra_file),
            extra_file.name,
        )

libyang_src = repository_rule(
    implementation = _libyang_src_impl,
    attrs = {
        "urls": attr.string_list(mandatory = True),
        "sha256": attr.string(mandatory = True),
        "strip_prefix": attr.string(),
        "patches": attr.label_list(),
        "build_file_template": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
        "extra_files": attr.label_list(
            allow_files = True,
            doc = "Extra files to copy into the fetched repo root (e.g. .bzl helpers, test data).",
        ),
    },
)
