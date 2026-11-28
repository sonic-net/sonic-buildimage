"""Link the public FIB C API against the runtime library published by VS."""

from importlib import import_module

IMAGES = ("docker-fpm-frr",)
RUNTIME = {
    "fib.tar": "//src/libraries/sonic-fib:runtime",
    # This SDK fixture supplies the headers for the independent public API consumer.
    "fib-development.tar": "//src/libraries/sonic-fib:development",
}
DEBUG = {"fib-symbols.tar": "//src/libraries/sonic-fib:runtime.debug_symbols"}


def verify(directory, *, debug):
    fib = import_module("src.libraries.sonic-fib.verify_runtime")
    return fib.verify(directory / "fib.tar", directory / "fib-development.tar",
                      directory / "fib-symbols.tar" if debug else None)
