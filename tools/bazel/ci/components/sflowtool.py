"""Decode and forward the owner's IPv4 and IPv6 captures with the VS sflowtool."""

from tools.bazel.ci import installed_package_test

IMAGES = ("docker-sflow",)
RUNTIME = {
    "runtime.tar": "@sflowtool//:runtime",
    "metadata.json": "@sflowtool//:runtime_packages.json",
    "package_test.py": "@sflowtool//:bazel/package_test.py",
    "copyright": "@sflowtool//:COPYING",
}
DEBUG = {
    "symbols.tar": "@sflowtool//:runtime.debug_symbols",
    "unstripped": "@sflowtool//:sflowtool",
}


def verify(directory, *, debug):
    inputs = {"runtime": "runtime.tar", "metadata": "metadata.json", "copyright": "copyright"}
    if debug:
        inputs["unstripped"] = "unstripped"
    return installed_package_test.verify("sflowtool", directory, debug=debug, inputs=inputs)
