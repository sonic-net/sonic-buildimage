"""Repository rule that imports a workspace-relative file as a Bazel source.

The SONiC base images live under `target/` (gitignored, not a Bazel package),
so they cannot be referenced by an in-tree label. `local_archive` symlinks such
a file into a generated repo and exposes it as `@<name>//:archive`.

`local = True` makes the rule re-run on every build (a cheap symlink), so the
import tracks the file even though `target/` is outside the module.
"""

def _local_archive_impl(rctx):
    archive_path = rctx.path(rctx.attr.archive)
    rctx.watch(archive_path) # Watch for any change in the file, such as Make building it again

    archive_path.exists or fail("Archive path {} doesn't exist".format(archive_path))

    rctx.symlink(archive_path, "archive.tar.gz")
    rctx.file(
        "BUILD.bazel",
        """
filegroup(
    name = "archive",
    srcs = ["archive.tar.gz"],
    visibility = ["//visibility:public"],
)
      """,
    )

local_archive = repository_rule(
    implementation = _local_archive_impl,
    attrs = {
        "archive": attr.label(mandatory = True),
    },
    local = True,
)
