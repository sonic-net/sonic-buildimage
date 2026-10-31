"""Unit tests for liquid-cooling detection (sonic_platform/cooling.py).

Self-contained by design: cooling.py is loaded directly by path rather than
imported as sonic_platform.cooling, because sonic_platform/__init__.py pulls in
sonic_platform_base and its dependencies. No EEPROM or BMC hardware is required
either.

Model numbers below are synthetic except where a real shape is needed to pin
down a grammar rule.
"""

import importlib.util
import os

import pytest

_TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
_COMMON_DIR = os.path.dirname(_TESTS_DIR)
COOLING_PATH = os.path.join(_COMMON_DIR, "sonic_platform", "cooling.py")


def _load():
    spec = importlib.util.spec_from_file_location("nexthop_cooling_under_test",
                                                  COOLING_PATH)
    assert spec is not None and spec.loader is not None, \
        f"cannot load {COOLING_PATH}"
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


cooling = _load()


# --------------------------------------------------------------------------
# Model segment extraction
# --------------------------------------------------------------------------

@pytest.mark.parametrize("product_name,expected", [
    ("NH-1234L-64R",        "1234L"),
    ("NHX-1234CBL-64R-S-R", "1234CBL"),
    ("NH-1234NPL-48C16R",   "1234NPL"),
    ("NH-AB800-F",          "AB800"),
    ("  nh-1234l-64r  ",    "1234L"),
])
def test_model_segment(product_name, expected):
    assert cooling.model_segment(product_name) == expected


@pytest.mark.parametrize("product_name", [
    "", None, "GARBAGE", "NH", "NH-", "-1234L-64R", "XYZ-1234L-64R",
])
def test_model_segment_rejects_malformed(product_name):
    assert cooling.model_segment(product_name) is None


# --------------------------------------------------------------------------
# Liquid marker
# --------------------------------------------------------------------------

@pytest.mark.parametrize("product_name", [
    "NH-1234L-64R",          # bare liquid marker
    "NH-1234NL-64C",         # one connectivity indicator before the marker
    "NH-1234NPL-48C16R",     # two connectivity indicators
    "NH-1234CBL-64R",        # decoder-ring shape
    "NHX-1234L-64R",         # customer marker is irrelevant
    "NH-1234L-64R-S",        # Lite sub variant
    "NH-1234L-64R-S-R",      # reverse airflow: 'R' must not read as air
    "  nh-1234l-64r  ",      # case and whitespace
])
def test_liquid_model_numbers(product_name):
    result, reason = cooling.liquid_from_model_number(product_name)
    assert result is True, f"{product_name}: {reason}"


@pytest.mark.parametrize("product_name", [
    "NH-1234-64P",
    "NH-1234-F",             # legacy airflow suffix, still air
    "NH-1234C-64C",          # connectivity but no liquid marker
    "NH-1234-32P-S",
    "NH-AB800-F",            # legacy 2-alpha + 3-digit base
    "NHX-1234-128P",
])
def test_air_model_numbers(product_name):
    result, reason = cooling.liquid_from_model_number(product_name)
    assert result is False, f"{product_name}: {reason}"


def test_legacy_liquid_suffix_is_not_liquid():
    """Older naming put the marker in its own segment ('NH-1234-L'). The model
    segment is then '1234', which carries no marker."""
    assert cooling.liquid_from_model_number("NH-1234-L")[0] is False


def test_trailing_l_in_a_legacy_base_does_not_false_positive():
    """A 2-alpha + 3-digit base containing L must not read as liquid -- this is
    why the marker is matched at the end of the segment rather than anywhere
    inside it."""
    assert cooling.liquid_from_model_number("NH-LW800-F")[0] is False


@pytest.mark.parametrize("product_name", [
    "", None, "GARBAGE", "NH", "NH-B27",
    "NH-12-64R",             # malformed base
    "NH-1234XL-64R",         # unknown connectivity letter
])
def test_unrecognised_is_none_not_false(product_name):
    """None is kept distinct from False so the caller can log an unrecognised
    name instead of silently reporting air cooled."""
    result, reason = cooling.liquid_from_model_number(product_name)
    assert result is None, f"{product_name!r} -> {result} ({reason})"
    assert reason


def test_reason_always_populated():
    for name in ("NH-1234L-64R", "NH-1234-64P", "GARBAGE", ""):
        assert cooling.liquid_from_model_number(name)[1]


# --------------------------------------------------------------------------
# platform.json override
# --------------------------------------------------------------------------

@pytest.mark.parametrize("data,expected", [
    ({"chassis": {"liquid_cooled": True}},  True),
    ({"chassis": {"liquid_cooled": False}}, False),
    ({"chassis": {}},                       None),
    ({},                                    None),
    (None,                                  None),
    ({"chassis": {"liquid_cooled": "true"}}, None),   # string is not a bool
    ({"chassis": {"liquid_cooled": 1}},      None),   # int is not a bool
])
def test_liquid_from_platform_json(data, expected):
    assert cooling.liquid_from_platform_json(data) is expected


def test_platform_json_key_matches_existing_convention():
    """The field name is shared with the other AST2700 BMC platform in-tree;
    changing it would silently stop honouring existing device directories."""
    assert cooling.PLATFORM_JSON_KEY == "liquid_cooled"


# --------------------------------------------------------------------------
# Resolution order
# --------------------------------------------------------------------------

LIQUID_JSON = {"chassis": {"liquid_cooled": True}}
AIR_JSON = {"chassis": {"liquid_cooled": False}}


def test_platform_json_overrides_model_number():
    result, reason, status = cooling.resolve_is_liquid_cooled(LIQUID_JSON, "NH-1234-64P")
    assert result is True
    assert status == cooling.STATUS_DISAGREE
    assert "platform.json declares" in reason
    assert "no liquid marker" in reason      # the disagreement is reported


def test_platform_json_can_override_to_air():
    result, _, status = cooling.resolve_is_liquid_cooled(AIR_JSON, "NH-1234L-64R")
    assert result is False
    # The dangerous direction: a liquid chassis reported as air cooled. It must
    # come back flagged so the caller logs it instead of failing silently.
    assert status == cooling.STATUS_DISAGREE


def test_platform_json_used_when_model_number_unusable():
    """This is what lets lab and simulation work on a box whose IDPROM carries
    no usable system model number."""
    result, reason, status = cooling.resolve_is_liquid_cooled(LIQUID_JSON, "NH-B27")
    assert result is True
    assert status == cooling.STATUS_DECLARED
    assert "but" not in reason               # nothing to disagree with


def test_agreement_does_not_report_a_mismatch():
    _, reason, status = cooling.resolve_is_liquid_cooled(LIQUID_JSON, "NH-1234L-64R")
    assert status == cooling.STATUS_DECLARED
    assert "but" not in reason


@pytest.mark.parametrize("product_name,expected", [
    ("NH-1234L-64R", True),
    ("NH-1234-64P",  False),
])
def test_falls_back_to_model_number(product_name, expected):
    result, _, status = cooling.resolve_is_liquid_cooled(None, product_name)
    assert result is expected
    assert status == cooling.STATUS_PARSED


@pytest.mark.parametrize("product_name", ["", "NH-B27", "GARBAGE", None])
def test_unrecognised_defaults_to_false_and_says_so(product_name):
    """False is the permissive direction, so the reason must make the default
    visible for the caller to log."""
    result, reason, status = cooling.resolve_is_liquid_cooled(None, product_name)
    assert result is False
    assert status == cooling.STATUS_DEFAULT
    assert "defaulting to" in reason


# --------------------------------------------------------------------------
# Status drives logging
# --------------------------------------------------------------------------
# The caller picks a log level from the status. These tests pin that contract,
# because the failure they guard against is invisible in the returned result:
# a disagreement used to be detected but never logged, since the caller matched
# on the reason text and the mismatch message did not carry the phrase it
# looked for.


def _log_level_for(platform_data, product_name):
    """Mirror of Chassis.is_liquid_cooled()'s log-level decision."""
    result, _, status = cooling.resolve_is_liquid_cooled(platform_data,
                                                        product_name)
    if status in cooling.UNTRUSTWORTHY_STATUSES:
        return "error"
    if result:
        return "notice"
    return None


@pytest.mark.parametrize("platform_data,product_name,expected", [
    # Both disagreement directions must be loud.
    (AIR_JSON,    "NH-1234L-64R", "error"),
    (LIQUID_JSON, "NH-1234-64R",  "error"),
    # No usable model number is equally untrustworthy.
    (None,        "GARBAGE",      "error"),
    (None,        "NH-B27",       "error"),
    # Agreement, and a clean decision either way, stay quiet. An air-cooled B27
    # is the common case and must not log an error on every call.
    (LIQUID_JSON, "NH-1234L-64R", "notice"),
    (AIR_JSON,    "NH-1234-64R",  None),
    (None,        "NH-1234L-64R", "notice"),
    (None,        "NH-1234-64R",  None),
])
def test_log_level_follows_status(platform_data, product_name, expected):
    assert _log_level_for(platform_data, product_name) == expected


def test_every_status_is_covered_by_the_log_rule():
    """A new status must be classified as trustworthy or not, not silently
    fall through to no log."""
    all_statuses = {cooling.STATUS_DECLARED, cooling.STATUS_PARSED,
                    cooling.STATUS_DISAGREE, cooling.STATUS_DEFAULT}
    assert set(cooling.UNTRUSTWORTHY_STATUSES) <= all_statuses
    assert len(all_statuses) == 4


def test_disagreement_reason_names_both_sources():
    """The error log is the only artefact an on-call engineer sees, so it has to
    say what disagreed, not just that something did."""
    _, reason, status = cooling.resolve_is_liquid_cooled(AIR_JSON, "NH-1234L-64R")
    assert status == cooling.STATUS_DISAGREE
    assert "platform.json declares" in reason
    assert "liquid marker" in reason
