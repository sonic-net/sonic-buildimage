"""Package an `oci_image` into the gzipped docker-archive the Make build consumes.

The Make-based build expects each container to land in `target/<name>.gz` as a
gzipped `docker save` archive.

This macro replicates that process in Bazel, creating intermediary targets when necessary.
"""

load("@bazel_skylib//rules:copy_file.bzl", "copy_file")
load("@rules_gzip//gzip/compress:defs.bzl", "gzip_compress")
load("@rules_oci//oci:defs.bzl", "oci_load")

def sonic_docker_archive(name, image, visibility = None):
    """Packages an `oci_image` into `target/<name>`, where the Make build expects it.

    The archive is always tagged `<name without .gz>:latest`,
    as expected by `sonic_debian_extension.j2`.

    For `name = "docker-orchagent.gz"`, useful targets include:

    - `:docker-orchagent.gz`, the compressed archive exported by the Make bridge.
    - `:docker-orchagent.load`, which loads the image into the local Docker engine.

    Args:
        name: File name of the archive, including the `.gz` suffix. Must match the
            name the Make build expects to find in `target/`.
        image: The `oci_image` to package.
        visibility: Visibility for the generated targets.
    """
    if not name.endswith(".gz"):
        fail("sonic_docker_archive: name must end in '.gz', got '%s'" % name)

    stem = name[:-len(".gz")]

    # Materialize this exact producer's layout alongside its archive on demand.
    native.filegroup(
        name = name + ".boot-oci",
        srcs = [image],
        tags = ["manual"],
        visibility = visibility,
    )

    oci_load(
        name = stem + ".load",
        image = image,
        repo_tags = [stem + ":latest"],
        visibility = visibility,
    )

    native.filegroup(
        name = stem + ".tar",
        srcs = [stem + ".load"],
        output_group = "tarball",
        visibility = visibility,
    )

    # gzip_compress names its output <input basename>.gz. Give each input the
    # expected stem so runtime/debug archives keep their Make-compatible names.
    copy_file(
        name = stem + ".gzip_input",
        src = stem + ".tar",
        out = stem + ".gzip_input/" + stem,
        allow_symlink = True,
        visibility = ["//visibility:private"],
    )

    gzip_compress(
        name = name,
        src = stem + ".gzip_input",
        level = 6,
        visibility = visibility,
    )
