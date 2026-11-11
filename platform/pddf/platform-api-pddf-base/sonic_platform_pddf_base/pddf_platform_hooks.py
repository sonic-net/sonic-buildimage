"""
PDDF platform-side vendor override surface.

Vendors that adopt the CHILD_CARDS feature override this class in their
platform package at the stable import path ``sonic_platform.pddf_hooks.PlatformHooks``.
pddfparse discovers it via a single import and calls only the hooks that the vendor's
pddf-device.json actually references.

"""


class ChildCardEepromUnprogrammed(Exception):
    """Raised by ``decode_eeprom`` when a child-card EEPROM has no decodable
    inventory at all -- i.e. it is blank/unprogrammed (or the card is absent).

    pddfparse.expand_child_cards treats this as "the child card is not
    present": it skips the CHILD_CARDS entry (that card's telemetry is
    unavailable) and logs an error, rather than hard-failing platform init.

    This is deliberately distinct from a *populated* EEPROM that is merely
    missing the requested record -- decoders should signal that as a hard
    error (e.g. KeyError), since it indicates a programming/hardware fault on
    an otherwise-present card.
    """


class PddfPlatformHooks:
    def decode_eeprom(self, decoder: str, eeprom_bytes: bytes, slot: int) -> dict:
        """Decode an EEPROM blob into a single identity record.

        The return dict's keys are vendor-defined; the only contract is that
        whatever keys appear in any ``variants[].match`` block in
        pddf-device.json's CHILD_CARDS entry must be present. The returned
        record is merged into the Jinja context for fragment rendering.

        Raise :class:`ChildCardEepromUnprogrammed` when the EEPROM carries no
        inventory at all so that pddfparse can skip the (absent/unprogrammed)
        card instead of failing platform init.
        """
        raise NotImplementedError(decoder)
