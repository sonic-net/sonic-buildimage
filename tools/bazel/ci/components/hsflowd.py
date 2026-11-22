"""Exercise the VS hsflowd and modules with the owner's private Redis and UDP probes."""

from tools.bazel.ci import sflow_owner

IMAGES = ("docker-sflow",)
RUNTIME = {
    "runtime.tar": "@hsflowd//:runtime",
    "metadata.json": "@hsflowd//:runtime_packages.json",
    "package_test.py": "@hsflowd//bazel:package_test.py",
    "changelog": "@hsflowd//:changelog",
    # Private Redis fixture and matching hiredis from the source owner's test closure.
    "redis.tar": "@hsflowd//:service_fixtures",
    "hiredis.tar": "@hsflowd_deps//libhiredis1.1.0:data",
}
DEBUG = {"symbols.tar": "@hsflowd//:runtime.debug_symbols"}


def verify(directory, *, debug):
    return sflow_owner.verify("hsflowd", directory, debug=debug, inputs={
        "runtime": "runtime.tar", "metadata": "metadata.json", "changelog": "changelog",
        "fixtures": "redis.tar", "hiredis": "hiredis.tar",
    })
