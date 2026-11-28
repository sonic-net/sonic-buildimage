"""Validate the Rust listener package installed in the VS config-engine image."""
from importlib import import_module

IMAGES = ("docker-config-engine-trixie",)
RUNTIME = {"listener.tar": "//src/sonic-supervisord-utilities-rs:runtime"}
DEBUG = {"listener-symbols.tar": "//src/sonic-supervisord-utilities-rs:runtime.debug_symbols"}


def verify(directory, *, debug):
    checker = import_module("src.sonic-supervisord-utilities-rs.bazel.package_test")
    return checker.verify(directory / "listener.tar", directory / "listener-symbols.tar" if debug else None)
