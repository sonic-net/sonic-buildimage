#
# leak_detect.py
#
# Leak-sensor definitions and GPIO value decoding for liquid-cooled Nexthop
# systems.
#
# The BMC reads leak state from two GPIO lines rather than over the FPGA. The
# switch card FPGA carries per-sensor detail, but it is unreachable from the BMC
# while the switch card is powered off -- which is exactly when a leak matters.
# The CPU card senses the leak sensors itself and drives two fault lines to BMC
# GPIOs, and both the sensing and those GPIOs stay up on CPU power.
#
# Depends only on the standard library so it can be unit-tested without
# sonic_platform_base installed. The GPIO access itself lives in
# liquid_cooling.py, which keeps this module hardware-free.
#
# Severity strings mirror LeakSeverity.value in sonic_platform_base's
# liquid_cooling_base. Duplicated so this module stays dependency-free; the unit
# tests assert they still match.
SEVERITY_MINOR = "MINOR"
SEVERITY_CRITICAL = "CRITICAL"

# --- GPIO lines -------------------------------------------------------------
# Lines are addressed by their device-tree gpio-line-names entry, not by
# chip+offset: libgpiod v2 dropped gpiofind and gpioinfo's output is not a
# stable parsing target, and a DTB without leak support then fails loudly
# instead of reading whichever signal happens to sit at a hardcoded offset.
#
# For reference, on the B27 BMC:
#   leak_fault_0 = AST2720 GPIOH3 = gpiochip1 offset 59
#   leak_fault_1 = AST2720 GPIOD4 = gpiochip1 offset 28
#
# Confirmed on hardware by injecting a leak upstream of the CPU card: a
# drip-tray leak asserts leak_fault_1 only, and a cold-plate leak asserts
# leak_fault_0 only.
LINE_DRIPTRAY = "leak_fault_1"
LINE_COLDPLATE = "leak_fault_0"

# Both lines are active high: asserted means leak. If a DTB ever declares them
# inverted, gpioinfo shows the line as active-low and libgpiod applies it.
LINE_ACTIVE_HIGH = True

# --- Sensor inventory -------------------------------------------------------
# Two sensors, because two GPIO lines are what the BMC can see. The physical
# sensors of the liquid loop are aggregated in hardware before they reach these
# lines: the drip-tray rope drives one, and the CPU and switch card cold-plate
# flex PCBs drive the other.
#
# Names are location-led so an operator reading an alert knows where to look;
# severity is a separate field and is not repeated in the name. The tray rope
# sees liquid pooling in the chassis, so it maps to CRITICAL.
LEAK_SENSORS = (
    {
        "key": "coldplate",
        "name": "coldplate_leak",
        "type": "FlexPCB",
        "location": "ColdPlate",
        "severity": SEVERITY_MINOR,
        "line": LINE_COLDPLATE,
        "description": "Flex PCB sensors on the CPU and switch card cold "
                       "plates, aggregated in hardware. Localized leak "
                       "detection at the cold plate connections.",
    },
    {
        "key": "driptray",
        "name": "driptray_leak",
        "type": "Rope",
        "location": "DripTray",
        "severity": SEVERITY_CRITICAL,
        "line": LINE_DRIPTRAY,
        "description": "Rope sensor along the chassis drip tray. Asserted when "
                       "coolant is pooling in the chassis.",
    },
)

SENSOR_KEYS = tuple(s["key"] for s in LEAK_SENSORS)
SEVERITY_BY_KEY = {s["key"]: s["severity"] for s in LEAK_SENSORS}
SENSOR_BY_KEY = {s["key"]: s for s in LEAK_SENSORS}
LINE_BY_KEY = {s["key"]: s["line"] for s in LEAK_SENSORS}


class LeakLineError(Exception):
    """A leak GPIO line could not be read, or its value was not understood."""


def line_for(key):
    """
    The gpio-line-name backing one sensor.

    Raises:
        KeyError: unknown sensor key.
    """
    if key not in LINE_BY_KEY:
        raise KeyError(f"unknown leak sensor {key!r}")
    return LINE_BY_KEY[key]


# Values libgpiod may print for a line level. v2 reports active/inactive by
# default and 0/1 under --numeric, and prints either a bare value or
# <name>=<value> depending on version and line count. Accepting all of these
# costs nothing and avoids the read breaking on a libgpiod upgrade.
_TRUE_TOKENS = frozenset(("1", "active", "true", "high"))
_FALSE_TOKENS = frozenset(("0", "inactive", "false", "low"))


def parse_line_value(output):
    """
    Decode one line's level from gpioget output.

    Handles a bare value ("1", "active") and the <name>=<value> form, with or
    without quotes around the name.

    Args:
        output: raw stdout from gpioget.

    Returns:
        bool: True when the line is asserted.

    Raises:
        LeakLineError: output was empty or not a value we recognise. Callers
                       must not read an unparseable value as "no leak".
    """
    if output is None:
        raise LeakLineError("no output from gpioget")

    token = output.strip().split()[-1] if output.strip() else ""
    if not token:
        raise LeakLineError("empty output from gpioget")

    # "<name>"=<value> -> <value>
    if "=" in token:
        token = token.rsplit("=", 1)[1]
    token = token.strip().strip('"').strip("'").lower()

    if token in _TRUE_TOKENS:
        return True
    if token in _FALSE_TOKENS:
        return False
    raise LeakLineError(f"unrecognised gpioget value {output.strip()!r}")


def is_leaking(output):
    """
    Whether a sensor is wet, from its line's gpioget output.

    Raises:
        LeakLineError: unparseable output.
    """
    asserted = parse_line_value(output)
    return asserted if LINE_ACTIVE_HIGH else not asserted
