from __future__ import print_function

import re
from numbers import Integral


try:
    STRING_TYPES = (basestring,)
except NameError:
    STRING_TYPES = (str,)


def validate_asn(value):
    """Return a valid decimal BGP ASN or fail template rendering."""
    if isinstance(value, Integral) and not isinstance(value, bool):
        asn = value
        rendered_value = str(value)
    elif (isinstance(value, STRING_TYPES)
            and re.match(r'\A[0-9]{1,10}\Z', value) is not None):
        asn = int(value)
        rendered_value = value
    else:
        raise ValueError("Invalid BGP ASN")

    if not 0 < asn <= 0xffffffff:
        raise ValueError("Invalid BGP ASN")

    return rendered_value
