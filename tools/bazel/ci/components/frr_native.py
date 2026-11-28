"""Exercise FRR and its SNMP module against the packages installed by VS."""

from importlib import import_module

IMAGES = ("docker-fpm-frr",)
RUNTIME = {
    "frr.tar": "//src/sonic-frr:frr_layout",
    "frr-snmp.tar": "//src/sonic-frr:frr_snmp_layout",
}
DEBUG = {
    "frr-symbols.tar": "//src/sonic-frr:frr_elf.debug_symbols",
    "frr-snmp-symbols.tar": "//src/sonic-frr:frr_snmp_elf.debug_symbols",
}


def verify(directory, *, debug):
    frr = import_module("src.sonic-frr.verify_runtime")
    return {
        "frr": frr.verify(directory / "frr.tar", directory / "frr-symbols.tar" if debug else None, "frr"),
        "snmp": frr.verify(directory / "frr-snmp.tar", directory / "frr-snmp-symbols.tar" if debug else None, "frr-snmp"),
    }
