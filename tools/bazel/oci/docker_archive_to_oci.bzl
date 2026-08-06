"""Turn a docker-archive (`docker save` output) into an OCI image layout directory

rules_oci's `base` expects an OCI layout (`oci-layout`/`index.json`/`blobs`),
but the SONiC base images are produced by the legacy Make flow as docker-archive tarballs.

This wraps `docker_archive_to_oci_layout.py`in an action that emits a tree artifact
in exactly the shape `oci_image` produces.
"""

def _docker_archive_to_oci_layout_impl(ctx):
    layout = ctx.actions.declare_directory(ctx.label.name + "_layout")
    args = ctx.actions.args()
    args.add("--src", ctx.file.src)
    args.add("--out", layout.path)
    ctx.actions.run(
        inputs = [ctx.file.src],
        outputs = [layout],
        executable = ctx.executable._converter,
        arguments = [args],
        mnemonic = "DockerArchiveToOci",
        progress_message = "Converting %{label} docker-archive to OCI layout",
    )
    return [DefaultInfo(files = depset([layout]))]

docker_archive_to_oci_layout = rule(
    implementation = _docker_archive_to_oci_layout_impl,
    doc = "Convert a docker-archive tarball into an OCI image layout directory.",
    attrs = {
        "src": attr.label(
            allow_single_file = True,
            mandatory = True,
            doc = "docker-archive tarball (.tar or .tar.gz), e.g. from `docker save`.",
        ),
        "_converter": attr.label(
            default = Label("//tools/bazel/oci:docker_archive_to_oci_layout"),
            executable = True,
            cfg = "exec",
        ),
    },
)
