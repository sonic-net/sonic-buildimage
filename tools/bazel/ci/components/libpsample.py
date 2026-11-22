"""Test the VS psample CLI and library with the owner's netlink fixture and C SDK."""

from tools.bazel.ci import installed_package_test

IMAGES = ("docker-sflow",)
RUNTIME = {
    "runtime.tar": "@libpsample//:runtime",
    "metadata.json": "@libpsample//:runtime_packages.json",
    "package_test.py": "@libpsample//:bazel/package_test.py",
    "copyright": "@libpsample//:bazel/copyright",
    "psample.8": "@libpsample//:man/psample.8",
    "mnl.tar": "@psample_deps//libmnl0:data",
    # Source owner test fixtures. The SDK consumer loads the VS shared library.
    "netlink.so": "@libpsample//:netlink_fixture.so",
    "sdk-probe": "@libpsample//:sdk_probe",
}
DEBUG = {"symbols.tar": "@libpsample//:runtime.debug_symbols"}


def verify(directory, *, debug):
    return installed_package_test.verify("libpsample", directory, debug=debug, executables=("sdk",), inputs={
        "runtime": "runtime.tar", "metadata": "metadata.json", "copyright": "copyright",
        "manual": "psample.8", "mnl": "mnl.tar", "fixture": "netlink.so", "sdk": "sdk-probe",
    })
