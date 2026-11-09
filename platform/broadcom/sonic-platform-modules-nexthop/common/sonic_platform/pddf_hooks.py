"""
Nexthop PDDF override hooks.

Discovered by pddfparse (in sonic-platform-pddf-base) via a single import at
the stable path ``sonic_platform.pddf_hooks.PlatformHooks``. See
sonic_platform_pddf_base/pddf_platform_hooks.py for the contract.

Dispatches the ``decoder`` id named in each ``CHILD_CARDS`` entry to the
vendor-specific decode implementations.
"""

from sonic_platform_pddf_base.pddf_platform_hooks import PddfPlatformHooks

from nexthop import fern


class PlatformHooks(PddfPlatformHooks):
    def decode_eeprom(self, decoder: str, eeprom_bytes: bytes, slot: int) -> dict:
        # `decoder` names the decoder family only; the EEPROM is self-describing
        # (it carries its own format version in a header byte), so fern.decode
        # reads that version and dispatches the parse internally.
        if decoder == "fern":
            return fern.decode(eeprom_bytes, slot)
        raise NotImplementedError(decoder)
