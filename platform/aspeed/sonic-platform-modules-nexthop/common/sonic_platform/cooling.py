#
# cooling.py
#
# Liquid-cooling detection for NextHop BMC cards.
#
# Depends only on the standard library so the decision logic can be unit-tested
# without sonic_platform_base installed. All I/O -- reading platform.json and
# the IDPROM -- is done by the caller and passed in.
#

import re

# A BMC card can be installed in either an air- or liquid-cooled chassis, so
# cooling type cannot be a static property of the platform directory the way it
# is for a switch host. It is resolved at runtime from the system model number
# in the IDPROM, with platform.json available as an override.

# Model number grammar: <customer marker>-<model>-<ports>[-<sub variant>][-<R>]
# e.g. NH-1234L-64R-S-R. Only the model segment matters here; a trailing 'L'
# marks liquid cooling. The port descriptor, sub variant and reverse-airflow
# marker are deliberately ignored -- 'R' is overloaded across all three.
_PREFIX_RE = re.compile(r"^NH[A-Z]?$")

# <family/generation/flavour/tie-break><connectivity><liquid>
#   base          4 digits, or 2 letters + 3 digits for older names
#   connectivity  any of C/N/B/P
#   liquid        trailing L
_MODEL_RE = re.compile(
    r"^(?:[0-9]{4}|[A-Z]{2}[0-9]{3})[CNBP]*(?P<liquid>L?)$")

# platform.json key, matching the field the NVIDIA AST2700 BMC platform already
# uses (device/aspeed/arm64-aspeed_nvidia_ast2700_bmc-r0/platform.json).
PLATFORM_JSON_KEY = "liquid_cooled"

# How the decision was reached. Callers log from this rather than by matching on
# the reason text, so rewording a message cannot silently change log levels.
#   STATUS_DECLARED  platform.json declared it, and nothing contradicted it
#   STATUS_PARSED    decided from the model number
#   STATUS_DISAGREE  platform.json and the model number disagree
#   STATUS_DEFAULT   no usable model number, fell back to not liquid cooled
#
# DISAGREE and DEFAULT both mean the inputs cannot be trusted and must be
# logged at error level whichever way the result points: an air-cooled verdict
# skips the leak gate on a liquid chassis, and a liquid verdict arms it on an
# air-cooled one.
STATUS_DECLARED = "declared"
STATUS_PARSED = "parsed"
STATUS_DISAGREE = "disagree"
STATUS_DEFAULT = "default"

# Statuses the caller must log at error level.
UNTRUSTWORTHY_STATUSES = (STATUS_DISAGREE, STATUS_DEFAULT)


def model_segment(product_name):
    """
    Extract the model segment from a system model number.

    Args:
        product_name: Product Name string (ONIE TLV 0x21), e.g. "NH-1234L-64R".

    Returns:
        str or None: the model segment uppercased, or None if the name does not
                     have the expected shape.
    """
    if not product_name:
        return None
    parts = str(product_name).strip().upper().split("-")
    if len(parts) < 2 or not _PREFIX_RE.match(parts[0]):
        return None
    return parts[1] or None


def liquid_from_model_number(product_name):
    """
    Decide liquid cooling from a system model number.

    Args:
        product_name: Product Name string (ONIE TLV 0x21).

    Returns:
        tuple: (result, reason) where result is True, False, or None when the
               name does not match the naming convention. None is kept distinct
               from False so the caller can report an unrecognised name instead
               of silently treating it as air cooled.
    """
    if not product_name:
        return None, "no product name available"

    model = model_segment(product_name)
    if model is None:
        return None, f"cannot parse model segment from {product_name!r}"

    m = _MODEL_RE.match(model)
    if not m:
        return None, f"model segment {model!r} does not match the naming convention"

    if m.group("liquid"):
        return True, f"model segment {model!r} carries the liquid marker"
    return False, f"model segment {model!r} has no liquid marker"


def liquid_from_platform_json(platform_data):
    """
    Read an explicit liquid-cooling declaration from platform.json data.

    Returns:
        bool or None: the declared value, or None when absent or not a bool.
    """
    try:
        declared = (platform_data or {}).get("chassis", {}).get(PLATFORM_JSON_KEY)
    except AttributeError:
        return None
    return declared if isinstance(declared, bool) else None


def resolve_is_liquid_cooled(platform_data, product_name):
    """
    Decide whether this chassis is liquid cooled.

    Resolution order:
      1. platform.json "chassis.liquid_cooled" -- authoritative when declared.
         The B27 device directory deliberately does not declare it, since the
         same card ships in both chassis types; this exists for fixed-purpose
         SKU directories and for lab use.
      2. The system model number read from the IDPROM.

    An unrecognised model number resolves to False, matching the ChassisBase
    default. That is the permissive direction -- a liquid chassis reported as air
    cooled skips the leak gate -- so callers are expected to log the returned
    reason at error level rather than let it pass quietly.

    Args:
        platform_data: parsed platform.json dict, or None.
        product_name: Product Name string (ONIE TLV 0x21), or "".

    Returns:
        tuple: (is_liquid_cooled, reason, status) where status is one of
               STATUS_DECLARED, STATUS_PARSED, STATUS_DISAGREE or
               STATUS_DEFAULT. It is returned explicitly so callers decide how
               loudly to log from the status rather than by matching on the
               wording of the reason.
    """
    declared = liquid_from_platform_json(platform_data)
    parsed, reason = liquid_from_model_number(product_name)

    if declared is not None:
        if parsed is not None and parsed != declared:
            return declared, (
                f"platform.json declares {PLATFORM_JSON_KEY}={declared} but {reason}; using platform.json -- "
                "check the device directory or the IDPROM programming"), STATUS_DISAGREE
        return declared, f"platform.json declares {PLATFORM_JSON_KEY}={declared}", STATUS_DECLARED

    if parsed is None:
        return False, f"{reason}; defaulting to not liquid cooled", STATUS_DEFAULT

    return parsed, reason, STATUS_PARSED
