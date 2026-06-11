load("@rules_python//python:defs.bzl", "PyInfo")
load("@tar.bzl", _mutate = "mutate", "tar")
load("@bazel_skylib//rules:write_file.bzl", "write_file")

def _export_pyinfo(ctx):
    files = []
    for dep in ctx.attr.srcs:
        files.append(dep[PyInfo].transitive_sources)
    return DefaultInfo(files = depset([], transitive = files))

export_py_info = rule(
    implementation = _export_pyinfo,
    doc = "Export `PyInfo.transitive_sources` as DefaultInfo.",
    attrs = {
        "srcs": attr.label_list(
            providers = [PyInfo],
        ),
    },
)

def site_packages(name, srcs, mutate = None, **kwargs):
    """
    Conveninece macro to create a tar with a bunch of python dependencies.
    srcs must export the required files with `PyInfo`.

    Args:
        name: Target name (also used as the name of the tar).
        srcs: List of targets that export `PyInfo` (e.g. `py_library` targets).
        mutate: Not allowed. If you want to use `mutate`, chances are you're better off using `export_py_info` and `tar` directly.
        **kwargs: Additional keyword arguments forwarded to the underlying `tar` rule.
    """
    if mutate != None:
        fail("mutate is not allowed in site_packages. If you want to use `mutate`, chances are you're better off using `export_py_info` and `tar` directly.")

    export_py_info(
        name = name + "_info",
        srcs = srcs,
    )
    script_name = name + "_awk"
    write_file(
        name = script_name,
        out = name + ".awk",
        content = """
@include "default"

# Skip install directories that we don't need.
# Keeping these in causes duplicate path conflicts in several versions of docker.
/^(install\\/|install\\/lib\\/|install\\/lib\\/python3.11\\/) uid/ {
  next
}

# Skip the install/bin/ directory entry itself.
/^install\\/bin\\/ / {
  next
}

{
  # Place binaries from install/bin/ under ./usr/bin/.
  if (sub("install/bin/", "./usr/bin/")) {
    # matched and replaced, done
  } else {
    # Remove prefixes for third party dependencies.
    sub("install/lib/python3.11/site-packages/", "");

    # Add everything, first and third party code, into ./usr/lib/python3/dist-packages.
    # We sohuld abstract this into the interface of site_packages when we have the need to.
    sub(/^/, "./usr/lib/python3/dist-packages/")
  }
}
""".split("\n") 
    )
    tar(
        name = name,
        srcs = [":" + name + "_info"],
        mutate = _mutate(
            awk_script = script_name,
        ),
        **kwargs
    )
