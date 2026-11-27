"""Record verified submodule commits for OCI image provenance.

The generated files identify the source revisions in image package manifests.
"""

def _identities(ctx):
    helper = ctx.path(ctx.attr._helper)
    ctx.watch(helper)
    ctx.watch(ctx.workspace_root.get_child("MODULE.bazel"))
    ctx.watch(ctx.workspace_root.get_child(".gitmodules"))
    outputs = []
    for label, name in ctx.attr.modules.items():
        selected = ctx.path(label)
        ctx.watch(selected)
        result = ctx.execute(["python3", helper, "--root", ctx.workspace_root, name, selected])
        if result.return_code:
            fail(result.stderr)
        receipt = json.decode(result.stdout)
        for path in receipt["watch"]:
            ctx.watch(path)
        output = name + ".MODULE.bazel"
        ctx.file(output, receipt["manifest"], executable = False)
        outputs.append(output)
    ctx.file("BUILD.bazel", "exports_files(%s)\n" % json.encode(outputs))

gitlink_identities = repository_rule(
    implementation = _identities,
    attrs = {
        "modules": attr.label_keyed_string_dict(mandatory = True),
        "_helper": attr.label(default = "//tools/bazel:submodule_provenance.py"),
    },
    local = True,
)
