"""Helpers for validating BGP configuration values."""

from __future__ import print_function

import re
from numbers import Integral


try:
    STRING_TYPES = (basestring,)
except NameError:
    STRING_TYPES = (str,)


_ASN_PATTERN = re.compile(r'\A[0-9]{1,10}\Z')
_MAX_ASN = 0xffffffff


def validate_asn(value):
    """Return *value* in its configuration representation when it is valid.

    BGP ASNs are represented as decimal strings in CONFIG_DB, while callers
    such as template filters may receive an integer. Both representations are
    accepted, but booleans are deliberately excluded even though ``bool`` is a
    subclass of ``int`` in Python.

    ``None`` and other sentinel values are not interpreted here. A caller that
    supports disabling BGP should handle that policy before invoking this
    validator.

    :raises ValueError: if *value* is not a decimal ASN in the 32-bit range.
    :return: the original decimal string, or the rendered integer string.
    """
    if isinstance(value, Integral) and not isinstance(value, bool):
        asn = int(value)
        rendered_value = str(value)
    elif (isinstance(value, STRING_TYPES)
          and _ASN_PATTERN.match(value) is not None):
        asn = int(value)
        rendered_value = value
    else:
        raise ValueError("Invalid BGP ASN")

    if not 0 < asn <= _MAX_ASN:
        raise ValueError("Invalid BGP ASN")

    return rendered_value
