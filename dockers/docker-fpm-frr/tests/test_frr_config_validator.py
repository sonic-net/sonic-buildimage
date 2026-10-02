import pytest

from frr_config_validator import validate_hostname


@pytest.mark.parametrize("hostname", [
    "sonic", "test_hostname", "switch-1.example",
])
def test_valid_hostname(hostname):
    validate_hostname({"localhost": {"hostname": hostname}})


@pytest.mark.parametrize("hostname", [
    "", "switch one", "switch'one", "-switch", "switch-", "switch..name;",
    42, None,
])
def test_invalid_hostname(hostname):
    with pytest.raises(ValueError):
        validate_hostname({"localhost": {"hostname": hostname}})


@pytest.mark.parametrize("metadata", [None, {}, [], {"localhost": None}])
def test_invalid_metadata(metadata):
    with pytest.raises(ValueError):
        validate_hostname(metadata)
