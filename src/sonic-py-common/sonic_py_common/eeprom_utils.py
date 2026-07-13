"""
Utilities for safely decoding raw EEPROM text fields.

EEPROM data from PSU/Fan modules often contains non-UTF-8 padding bytes
(e.g., 0xFF, 0x00) that cause UnicodeDecodeError when decoded with the
default UTF-8 codec.  Use :func:`safe_decode_eeprom_text` instead of
``bytes.decode()`` in platform EEPROM drivers.
"""


def safe_decode_eeprom_text(raw, default="NA"):
    """
    Safely decode a raw EEPROM text field to a str.

    :param raw: The value to decode.  Accepts ``bytes``, ``bytearray``,
                ``str``, or ``None``.
    :param default: Value returned when *raw* is empty, ``None``, or cannot
                    be decoded.  Defaults to ``"NA"``.
    :returns: Decoded, stripped string or *default*.

    Behaviour:
    * Strips common EEPROM padding bytes (``0x00``, ``0xFF``) and ASCII
      spaces before and after the payload.
    * Attempts strict UTF-8 decoding first.
    * Falls back to latin-1 (which never raises) if UTF-8 fails.
    * Never raises a decoding exception; returns *default* on any error.
    """
    if raw is None:
        return default

    if isinstance(raw, str):
        result = raw.strip('\x00\xff ')
        return result if result else default

    if not isinstance(raw, (bytes, bytearray)):
        return default

    # Strip common EEPROM padding before decoding
    stripped = raw.strip(b'\x00\xff ')
    if not stripped:
        return default

    # Prefer strict UTF-8
    try:
        return stripped.decode('utf-8')
    except (UnicodeDecodeError, ValueError):
        pass

    # latin-1 is a lossless single-byte encoding that never raises
    try:
        return stripped.decode('latin-1')
    except Exception:
        return default
