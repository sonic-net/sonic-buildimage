import pytest

from sonic_py_common.bgp import validate_asn


@pytest.mark.parametrize("value, expected", [
    ("1", "1"),
    ("0001", "0001"),
    ("4294967295", "4294967295"),
    (1, "1"),
    (4294967295, "4294967295"),
])
def test_validate_asn_accepts_valid_values(value, expected):
    assert validate_asn(value) == expected


@pytest.mark.parametrize("value", [
    None,
    True,
    False,
    "",
    "0",
    "4294967296",
    "1.0",
    "+1",
    " 1",
    "1 ",
    "1e3",
    "12345678901",
    "\u0661",
    b"1",
    -1,
    4294967296,
    1.0,
])
def test_validate_asn_rejects_invalid_values(value):
    with pytest.raises(ValueError):
        validate_asn(value)
