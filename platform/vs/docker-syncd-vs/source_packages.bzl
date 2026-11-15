"""Public source package inputs shared by the VS Syncd and gearbox images.

Their Dockerfiles and Make dependency closures are identical for the selected
DASH profile. Component owners retain all compilation/install definitions.
"""

SOURCE_PACKAGES = {
    "p4lang-pi": "@p4lang_pi//:runtime",
    "p4lang-bmv2": "@p4lang_bmv2//:runtime",
    "p4lang-p4c": "@p4c//:p4c_pkg",
    "libsai": "@sonic_dash//:runtime",
    "libswsscommon": "@sonic_swss_common//dist:libswsscommon_pkg",
    "libsairedis": "@sonic_sairedis//lib:libsairedis_pkg",
    "libsaimetadata": "@sonic_sairedis//meta:libsaimetadata_pkg",
    "libsaivs": "@sonic_sairedis//vslib:libsaivs_pkg",
    "syncd-vs": "@sonic_sairedis//syncd:syncd_pkg",
    "openssh-client": "@sonic_fips//openssh:openssh_client_pkg",
    "libnl-3-dev": "@libnl3//:libnl-3-dev_files",
    "libnl-route-3-dev": "@libnl3//:libnl-route-3-dev_files",
}

OWNER_METADATA = [
    "@sonic_swss_common//:runtime_packages.json",
    "@sonic_sairedis//:runtime_packages.json",
    "@sonic_fips//:runtime_packages.json",
    "@libnl3//:runtime_packages.json",
    "@p4lang_pi//:runtime_packages.json",
    "@p4lang_bmv2//:runtime_packages.json",
    "@p4c//:runtime_packages.json",
    "@sonic_dash//:runtime_packages.json",
]

SOURCE_MODULES = {
    "sonic-swss-common": "@syncd_source_modules//:sonic-swss-common.MODULE.bazel",
    "sonic-sairedis": "@syncd_source_modules//:sonic-sairedis.MODULE.bazel",
    "sonic-fips": "@syncd_source_modules//:sonic-fips.MODULE.bazel",
    "libnl3": "@syncd_source_modules//:libnl3.MODULE.bazel",
    "p4lang-pi": "@syncd_source_modules//:p4lang-pi.MODULE.bazel",
    "p4lang-bmv2": "@syncd_source_modules//:p4lang-bmv2.MODULE.bazel",
    "p4c": "@syncd_source_modules//:p4c.MODULE.bazel",
    "sonic-dash": "@syncd_source_modules//:sonic-dash.MODULE.bazel",
}

# Installed native/debug guarantees apply to both images' identical packages.
NATIVE_POLICY = {
    "debug_forwarding": {
        "usr/bin/pi_CLI_dummy": {
            "provider": "usr/lib/x86_64-linux-gnu/libpicli.so.0",
            "symbol": "main",
            "source": "main.c",
        },
    },
    "required_packages": [
        "syncd-vs",
        "libsairedis",
        "libsaimetadata",
        "libsaivs",
        "libswsscommon",
        "openssh-client",
        "p4lang-pi",
        "p4lang-bmv2",
        "p4lang-p4c",
        "libsai",
    ],
    "required_sonames": {
        "libsairedis": "libsairedis.so.0",
        "libsaimetadata": "libsaimetadata.so.0",
        "libsaivs": "libsaivs.so.0",
        "libswsscommon": "libswsscommon.so.0",
    },
    "gap_packages": [],
    "local_dependency_pattern": "lib(?:sai|swsscommon|MdioIpcClient|pi)",
    "require_source": True,
    "require_imported_symbols": False,
}
