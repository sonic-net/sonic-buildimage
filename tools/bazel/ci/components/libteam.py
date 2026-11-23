"""Test libteam's installed SDK and its VS runtime and symbol TARs."""
from importlib import import_module

IMAGES = ("docker-teamd", "docker-orchagent")
RUNTIME = {
    "libteam.tar": "//src/libteam:libteam_runtime",
    "libteamdctl.tar": "//src/libteam:libteamdctl_runtime",
    "libteam-utils.tar": "//src/libteam:libteam_utils_runtime",
    # Auxiliary SDK fixtures used to compile the public C consumer against VS.
    "libteam-development.tar": "//src/libteam:libteam_dev",
    "libnl-development.tar": "@libnl3//:libnl-3-dev_files",
}
DEBUG = {
    "libteam-symbols.tar": "//src/libteam:libteam_runtime.debug_symbols",
    "libteamdctl-symbols.tar": "//src/libteam:libteamdctl_runtime.debug_symbols",
    "libteam-utils-symbols.tar": "//src/libteam:libteam_utils_runtime.debug_symbols",
}


def verify(directory, *, debug):
    checker = import_module("src.libteam.verify_runtime")
    archives = {
        name.replace("-", "_"): (directory / (name + ".tar"), directory / (name + "-symbols.tar") if debug else None)
        for name in ("libteam", "libteamdctl", "libteam-utils")
    }
    return checker.verify(archives, directory / "libteam-development.tar", directory / "libnl-development.tar")
