from __future__ import print_function

import re


try:
    STRING_TYPES = (basestring,)
except NameError:
    STRING_TYPES = (str,)


def validate_asn(value):
    """Return a valid decimal BGP ASN or fail template rendering."""
    if (not isinstance(value, STRING_TYPES)
            or re.match(r'\A[0-9]{1,10}\Z', value) is None):
        raise ValueError("Invalid BGP ASN")

    asn = int(value)
    if not 0 < asn <= 0xffffffff:
        raise ValueError("Invalid BGP ASN")

    return value
