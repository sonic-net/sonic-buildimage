"""Fixed artifacts selected by the mainline FIPS host installer recipe."""
def _impl(ctx):
    records = json.decode(ctx.read(ctx.attr.manifest))
    for record in records:
        ctx.download(url = record["url"], output = record["filename"], sha256 = record["sha256"])
    ctx.file("BUILD.bazel", 'exports_files(' + repr([r["filename"] for r in records]) + ', visibility = ["//visibility:public"])\n')

fips_host_archives = repository_rule(implementation = _impl, attrs = {"manifest": attr.label(allow_single_file = True, mandatory = True)})
