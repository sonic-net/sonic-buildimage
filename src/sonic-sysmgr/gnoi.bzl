"""Utilities to build gnoi."""

_SONAME = "librebootgnoi.so.0"
_LIBRARY = _SONAME + ".0.0"

def gnoi_cc_protos(name):
    """Compile the vendored gNOI messages with the registry's Protobuf toolchain.

    Args:
        name: the cc_library to define.
    """
    sources = ["@gnoi_protos//:sources"]
    headers = ["@gnoi_protos//:headers"]

    # Preserve the librebootgnoi SONAME and layout in the sysmgr runtime tar.
    # Protobuf is supplied by the matching source-built runtime layer.
    native.cc_binary(
        name = _SONAME,
        srcs = sources + headers,
        linkopts = ["-Wl,-soname," + _SONAME],
        linkshared = True,
        visibility = ["//:__subpackages__"],
        # As with the legacy library, the executable supplies Protobuf symbols.
        deps = ["@gnoi_protos//:generated_headers"],
    )

    # Keep the existing package target while exposing the SONAME to runfiles.
    native.alias(
        name = _LIBRARY,
        actual = ":" + _SONAME,
        visibility = ["//:__subpackages__"],
    )

    native.cc_import(
        name = name + "_import",
        shared_library = ":" + _SONAME,
    )

    native.cc_library(
        name = name,
        hdrs = headers,
        deps = [
            ":" + name + "_import",
            "@gnoi_protos//:generated_headers",
            "@sonic_protobuf//:libprotobuf",
        ],
    )
