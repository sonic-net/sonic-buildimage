"""Helper for formatting mtree entries with common install paths."""

load("//:build_constants.bzl", "INSTALL_INCLUDEDIR", "INSTALL_LIBDIR", "INSTALL_PREFIX", "LIBYANG_MAJOR_SOVERSION", "LIBYANG_SOVERSION_FULL")

def pkg_path(path, **kwargs):
    """Format path template with common install variables.

    All format calls in mtree entries can use this instead of repeating
    prefix/libdir/includedir/version kwargs everywhere.

    Available placeholders: {prefix}, {libdir}, {includedir},
    {soversion_full}, {soversion_major}, plus any extra kwargs.
    """
    return path.format(
        prefix = INSTALL_PREFIX,
        libdir = INSTALL_LIBDIR,
        includedir = INSTALL_INCLUDEDIR,
        soversion_full = LIBYANG_SOVERSION_FULL,
        soversion_major = LIBYANG_MAJOR_SOVERSION,
        **kwargs
    )
