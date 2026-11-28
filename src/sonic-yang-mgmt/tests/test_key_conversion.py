"""ConfigDB key reconstruction from typed YANG key values."""
import pytest

from sonic_yang_ext import SonicYangExtMixin


@pytest.mark.parametrize("entry, keys, expected", [
    ({"id": 0}, "id", "0"),
    ({"id": "0"}, "id", "0"),
    ({"id": 7}, "id", "7"),
    ({"first": 0, "second": 2}, "first second", "0|2"),
    ({
        "vrf_name": "default", "instance_name": "CORE", "addr_family": "ipv4",
        "src_protocol": "connected", "level": "level-2", "table_id": 0,
    }, "vrf_name instance_name addr_family src_protocol level table_id",
        "default|CORE|ipv4|connected|level-2|0"),
])
def test_create_key_preserves_zero(entry, keys, expected):
    key, fields = SonicYangExtMixin()._createKey(entry, keys)
    assert key == expected
    assert fields == {name: str(entry[name]) for name in keys.split()}


@pytest.mark.parametrize("entry", [{}, {"id": None}])
def test_create_key_rejects_missing_value(entry):
    with pytest.raises(Exception, match="^key id not found in entry$"):
        SonicYangExtMixin()._createKey(entry, "id")
