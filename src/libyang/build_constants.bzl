"""Constants derived from CMakeLists.txt for the libyang Bazel build."""

# Project metadata from CMakeLists.txt.
LIBYANG_DESCRIPTION = "libyang is YANG data modelling language parser and toolkit written (and providing API) in C."

# Version constants from CMakeLists.txt.
# LIBYANG_VERSION is the project release version.
# LIBYANG_SOVERSION_FULL is the shared library ABI version (used for .so naming).
LIBYANG_VERSION = "1.0.73"
LIBYANG_MAJOR_SOVERSION = "1"
LIBYANG_MINOR_SOVERSION = "2"
LIBYANG_MICRO_SOVERSION = "2"
LIBYANG_SOVERSION_FULL = LIBYANG_MAJOR_SOVERSION + "." + LIBYANG_MINOR_SOVERSION + "." + LIBYANG_MICRO_SOVERSION

# Install paths from CMakeLists.txt and GNUInstallDirs defaults on Debian.
# TODO(bazel-ready): Parameterize INSTALL_LIBDIR by architecture to support aarch64
#   (lib/x86_64-linux-gnu vs lib/aarch64-linux-gnu).
INSTALL_PREFIX = "/usr"
INSTALL_INCLUDEDIR = "include"
INSTALL_LIBDIR = "lib/x86_64-linux-gnu"

# Plugin directories from CMakeLists.txt.
PLUGINS_DIR = INSTALL_PREFIX + "/" + INSTALL_LIBDIR + "/libyang"
EXTENSIONS_PLUGINS_DIR = PLUGINS_DIR + "/extensions"
USER_TYPES_PLUGINS_DIR = PLUGINS_DIR + "/user_types"

# Non-generated public headers from CMakeLists.txt set(headers ...).
PUBLIC_HEADERS = [
    "src/dict.h",
    "src/extensions.h",
    "src/tree_data.h",
    "src/tree_schema.h",
    "src/user_types.h",
    "src/xml.h",
]

# C++ binding headers from swig/CMakeLists.txt.
CPP_SWIG_HEADERS = [
    "swig/cpp/src/Internal.hpp",
    "swig/cpp/src/Libyang.hpp",
    "swig/cpp/src/Tree_Data.hpp",
    "swig/cpp/src/Tree_Schema.hpp",
    "swig/cpp/src/Xml.hpp",
]
