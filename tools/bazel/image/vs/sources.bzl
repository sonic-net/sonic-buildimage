"""Mainline VS host package installation interfaces, in installer order.

APT payloads precede these source/prebuilt packages. FIPS replaces its package
families after the ordinary host packages, and Python FIPS is installed last.
Python distributions are provided separately by //tools/bazel/image/vs/python.
"""

HOST_SOURCE_PAYLOADS = {
    "initramfs-tools-core": "//src/initramfs-tools:initramfs-tools-core.runtime",
    "initramfs-tools": "//src/initramfs-tools:initramfs-tools.runtime",
    "socat": "@socat//:socat_pkg",
    "sedutil": "@sedutil//:sedutil.runtime",
    "sonic-nettools": "//src/sonic-nettools:nettools.runtime",
    "libtac2": "@pam-tacplus//:libtac2.runtime",
    "libpam-tacplus": "@pam-tacplus//:libpam-tacplus.runtime",
    "libnss-tacplus": "@libnss_tacplus//:libnss-tacplus.runtime",
    "bash-tacplus": "@bash_tacplus//:bash-tacplus.runtime",
    "audisp-tacplus": "@audisp_tacplus//:audisp-tacplus.runtime",
    "libpam-radius-auth": "//src/radius/pam:libpam-radius-auth.runtime",
    "libnss-radius": "//src/radius/nss:libnss-radius.runtime",
    "ifupdown2": "@ifupdown2//:ifupdown2.runtime",
    "libyang3": "@libyang//:libyang_pkg",
    "python3-libyang": "@libyang_python//:libyang_pkg",
    "bash": "@sonic_bash//:bash.runtime",
    "sonic-host-services-data": "@sonic_host_services//:sonic-host-services-data.runtime",
    "sonic-device-data": "//src/sonic-device-data:vs_runtime_tar",
    "sonic-utilities-data": "@sonic_utilities//:sonic-utilities-data.runtime",
    "makedumpfile": "@makedumpfile//:makedumpfile.runtime",
    "kdump-tools": "@kdump_tools//:kdump-tools.runtime",
    "sonic-host-services-rs": "@sonic_host_services//:sonic-host-services-rs.runtime",
    # rules/swss-common.mk RDEPENDS and sonic-utilities DEBS_DEPENDS.
    "libnl-3-200": "@libnl3//:libnl-3-runtime_files",
    "libnl-genl-3-200": "@libnl3//:libnl-genl-3-runtime_files",
    "libnl-nf-3-200": "@libnl3//:libnl-nf-3-runtime_files",
    "libnl-route-3-200": "@libnl3//:libnl-route-3-runtime_files",
    "libnl-cli-3-200": "@libnl3//:libnl-cli-3-runtime_files",
    "libswsscommon": "@sonic_swss_common//dist:libswsscommon_pkg",
    "python3-swsscommon": "@sonic_swss_common//pyext:swsscommon_pkg",
    "sonic-db-cli": "@sonic_swss_common//dist:sonic-db-cli_pkg",
    "sonic-rsyslog-plugin": "//src/sonic-eventd:rsyslog_plugin_runtime",
    "monit": "@monit//:monit.runtime",
    "rasdaemon": "@rasdaemon//:rasdaemon.runtime",
    "sonic-ctrmgrd-rs": "//src/sonic-ctrmgrd-rs:ctrmgrd_rs.runtime",
    "systemd-sonic-generator": "//src/systemd-sonic-generator:systemd-sonic-generator.runtime",
    "syslog-counter": "//src/syslog-counter:syslog_counter.runtime",
}

# Original DEB controls are already exported by these prebuilt/source adapters.
HOST_DIRECT_CONTROLS = {
    "//src/radius/pam:libpam-radius-auth.control": "libpam-radius-auth",
    "@pam-tacplus//:libtac2.control": "libtac2",
    "@pam-tacplus//:libpam-tacplus.control": "libpam-tacplus",
    "@libnss_tacplus//:libnss-tacplus.control": "libnss-tacplus",
    "@bash_tacplus//:bash-tacplus.control": "bash-tacplus",
    "@audisp_tacplus//:audisp-tacplus.control": "audisp-tacplus",
    "@sedutil//:sedutil.control": "sedutil",
    "@sonic_bash//:bash.control": "bash",
    "@ifupdown2//:ifupdown2.control": "ifupdown2",
    "@monit//:monit.control": "monit",
}

# Project the real DEBIAN trees from these source owners; the flat 'controls'
# output group contains only control fields and is insufficient for postinst.
HOST_SOURCE_METADATA = {
    "//src/radius/pam:runtime_metadata": ["libpam-radius-auth"],
    "@bash_tacplus//:runtime_metadata": ["bash-tacplus"],
    "//src/initramfs-tools:initramfs-tools-core.runtime_metadata": ["initramfs-tools-core"],
    "//src/initramfs-tools:initramfs-tools.runtime_metadata": ["initramfs-tools"],
    "//src/radius/nss:runtime_packages.json": ["libnss-radius"],
    "@socat//:runtime_packages.json": ["socat"],
    "@rasdaemon//:runtime_packages.json": ["rasdaemon"],
    "//src/sonic-nettools:runtime_packages.json": ["sonic-nettools"],
    "//src/systemd-sonic-generator:systemd-sonic-generator.runtime_metadata": ["systemd-sonic-generator"],
    "//src/sonic-eventd:bazel/rsyslog_plugin.runtime_packages.json": ["sonic-rsyslog-plugin"],
    "@libnl3//:runtime_packages.json": ["libnl-3-200", "libnl-genl-3-200", "libnl-nf-3-200", "libnl-route-3-200", "libnl-cli-3-200"],
    "@libyang//:runtime_packages.json": ["libyang3"],
    "@libyang_python//:runtime_packages.json": ["python3-libyang"],
    "@sonic_host_services//:sonic-host-services-data.runtime_metadata": ["sonic-host-services-data"],
    "//src/sonic-device-data:runtime_packages.json": ["sonic-device-data"],
    "@sonic_utilities//:sonic-utilities-data.runtime_metadata": ["sonic-utilities-data"],
    "@makedumpfile//:makedumpfile.runtime_metadata": ["makedumpfile"],
    "@kdump_tools//:runtime_packages.json": ["kdump-tools"],
    "@sonic_host_services//:sonic-host-services-rs.runtime_metadata": ["sonic-host-services-rs"],
    "@sonic_swss_common//:runtime_packages.json": ["libswsscommon", "python3-swsscommon", "sonic-db-cli"],
    "//src/sonic-ctrmgrd-rs:runtime_packages.json": ["sonic-ctrmgrd-rs"],
    "//src/syslog-counter:runtime_packages.json": ["syslog-counter"],
}

# The default Make installation contract; each adapter must use a native producer.
FIPS_HOST_PACKAGES = [
    "openssl", "libssl3t64", "libssl-dev", "symcrypt-openssl",
    "ssh", "openssh-client", "openssh-server", "openssh-sftp-server",
    "libk5crypto3", "libkrb5support0", "libkrb5-3", "libgssapi-krb5-2",
    "python3.13", "python3.13-minimal", "libpython3.13",
    "libpython3.13-minimal", "libpython3.13-stdlib",
]
FIPS_HOST_PAYLOADS = {package: "//src/sonic-fips:" + package + ".runtime" for package in FIPS_HOST_PACKAGES}
FIPS_HOST_CONTROLS = {"//src/sonic-fips:" + package + ".control": package for package in FIPS_HOST_PACKAGES}
HOST_PYTHON_RUNTIME = "//tools/bazel/image/vs/python:runtime"
