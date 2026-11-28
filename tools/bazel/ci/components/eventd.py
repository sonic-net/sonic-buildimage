"""Check the event service package installed through the VS config-engine layer."""
from importlib import import_module
from pathlib import Path

IMAGES = ("docker-eventd",)
RUNTIME = {"eventd-runtime.tar": "//src/sonic-eventd:eventd_runtime"}
DEBUG = {"eventd-debug.tar": "//src/sonic-eventd:eventd_runtime.debug_symbols"}


def verify(directory, *, debug):
    checker = import_module("src.sonic-eventd.bazel.verify_packages")
    return checker.verify(
        Path(checker.__file__).resolve().parents[1],
        {"eventd": (directory / "eventd-runtime.tar", directory / "eventd-debug.tar" if debug else None)},
        "amd64",
    )
