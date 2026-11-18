#!/usr/bin/env python

#############################################################################
# Ciena ADM1266 PMBus Voltage Sensor
#
# Two ADM1266 Super-Sequencers on Europa PMBus (I2C) provide detailed
# voltage-rail monitoring for J2C+ core and auxiliary/BMC supplies:
#
#   ADM1266 #1 — I2C address 0x48
#   ADM1266 #2 — I2C address 0x49
#
# Each device has 17 input channels:
#   VH1-VH4   — High-voltage sense inputs (up to ~14 V)
#   VP1-VP13  — Precision sense inputs    (up to ~4.4 V)
#
# PMBus access:
#   1. Write PAGE (cmd 0x00) to select the channel
#   2. Read READ_VOUT (cmd 0x8B) — returns a 16-bit word
#   3. Read VOUT_MODE (cmd 0x20) for the exponent  (LINEAR16)
#
# VOUT_MODE returns a signed 5-bit exponent N (bits [4:0]):
#   Voltage (V) = mantissa × 2^N
#
# PMBus page mapping for ADM1266:
#   Page 0  = VH1     Page 4  = VP1     Page 10 = VP7
#   Page 1  = VH2     Page 5  = VP2     Page 11 = VP8
#   Page 2  = VH3     Page 6  = VP3     Page 12 = VP9
#   Page 3  = VH4     Page 7  = VP4     Page 13 = VP10
#                      Page 8  = VP5     Page 14 = VP11
#                      Page 9  = VP6     Page 15 = VP12
#                                        Page 16 = VP13
#
# SONiC VoltageSensorBase expects get_value() to return millivolts (mV).
#
# ADM1266 Channel Allocation
# =============================================
#
# ADM1266 #1 (0x48) — J2C Core Voltages:
#   VH1  (p0)  MAIN_PSU1_12V             12.0 V
#   VH2  (p1)  J2C_3P3V_FILTERED         3.3  V
#   VH3  (p2)  MAIN_12V0                 12.0  V
#   VH4  (p3)  MAIN_5V0                   5.0  V
#   VP1  (p4)  J2C_CORE                   0.8  V
#   VP2  (p5)  J2C_ANALOG_0P8V           0.8  V
#   VP3  (p6)  J2C_1V_PLL                 1.0  V
#   VP4  (p7)  J2C_TVDD_0P8V             0.8  V
#   VP5  (p8)  J2C_1P2V                   1.2  V
#   VP6  (p9)  J2C_3P3V                   3.3  V
#   VP7  (p10) J2C_1P8V                   1.8  V
#   VP8  (p11) J2C_ANALOG_1P5V           1.5  V
#   VP9  (p12) J2C_ANALOG_3P3V           3.3  V
#   VP10 (p13) J2C_2P5V                   2.5  V
#   VP11 (p14) RETIMER_0P9V              0.9  V
#   VP12 (p15) RETIMER_1P8V              1.8  V
#   VP13 (p16) RETIMER_0P8V              0.8  V
#
# ADM1266 #2 (0x49) — Auxiliary / BMC Voltages:
#   VH1  (p0)  MAIN_PSU2_12V             12.0  V
#   VH2  (p1)  BMC_3P3V                   3.3  V
#   VH3  (p2)  MAIN_3P3V_STBY             3.3  V
#   VH4  (p3)  BMC_1P8V                   1.8  V
#   VP1  (p4)  BMC_1P0V                   1.0  V
#   VP2  (p5)  BMC_1P2V                   1.2  V
#   VP3  (p6)  BMC_0P9V                   0.9  V
#   VP4  (p7)  MAIN_3P3V                  3.3  V
#   VP5  (p8)  DDR4_VTT                   0.6  V
#   VP6  (p9)  DDR4_1P2V                  1.2  V
#   VP7  (p10) CPU_1P0V                   1.0  V
#   VP8  (p11) CPU_1P8V                   1.8  V
#   VP9  (p12) CPU_0P9V                   0.9  V
#   VP10 (p13) SOC_1P2V                   1.2  V
#   VP11 (p14) CPU_VNN                    1.0  V  (approx)
#   VP12 (p15) SOC_1P05V                  1.05 V
#   VP13 (p16) (unused or spare)          N/A
#############################################################################

import logging
import subprocess

try:
    from sonic_platform_base.sensor_base import VoltageSensorBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = logging.getLogger(__name__)

# PMBus commands
_PMBUS_PAGE      = 0x00
_PMBUS_VOUT_MODE = 0x20
_PMBUS_READ_VOUT = 0x8B

# I2C bus for Europa PMBus
_I2C_BUS = 3


def _i2c_set_page(bus, addr, page):
    """Set PMBus PAGE register via i2cset."""
    try:
        cmd = ["i2cset", "-y", "-f", str(bus), str(addr),
               str(_PMBUS_PAGE), str(page), "b"]
        result = subprocess.run(cmd, capture_output=True, timeout=2)
        return result.returncode == 0
    except Exception as e:
        logger.debug("i2cset PAGE failed (bus=%d addr=0x%02x page=%d): %s",
                     bus, addr, page, e)
        return False


def _i2c_read_word(bus, addr, cmd):
    """Read a 16-bit PMBus word via i2cget -w (little-endian)."""
    try:
        proc = subprocess.run(
            ["i2cget", "-y", "-f", str(bus), str(addr), str(cmd), "w"],
            capture_output=True, text=True, timeout=2
        )
        if proc.returncode == 0 and proc.stdout.strip():
            return int(proc.stdout.strip(), 16)
    except Exception as e:
        logger.debug("i2cget word failed (bus=%d addr=0x%02x cmd=0x%02x): %s",
                     bus, addr, cmd, e)
    return None


def _i2c_read_byte(bus, addr, cmd):
    """Read a single PMBus byte via i2cget."""
    try:
        proc = subprocess.run(
            ["i2cget", "-y", "-f", str(bus), str(addr), str(cmd), "b"],
            capture_output=True, text=True, timeout=2
        )
        if proc.returncode == 0 and proc.stdout.strip():
            return int(proc.stdout.strip(), 16)
    except Exception as e:
        logger.debug("i2cget byte failed (bus=%d addr=0x%02x cmd=0x%02x): %s",
                     bus, addr, cmd, e)
    return None


def _decode_linear16(raw_word, exponent):
    """Decode a PMBus LINEAR16 VOUT value.

    LINEAR16: Voltage = mantissa × 2^exponent
    where mantissa is the unsigned 16-bit word and exponent comes
    from VOUT_MODE[4:0] as a signed 5-bit value.
    """
    # exponent is a 5-bit two's complement value
    if exponent >= 16:
        exponent -= 32
    return raw_word * (2.0 ** exponent)


def read_adm1266_voltage(bus, addr, page):
    """Read a voltage channel from an ADM1266 via PMBus.

    Args:
        bus  (int): I2C bus number.
        addr (int): 7-bit I2C address (e.g. 0x48, 0x49).
        page (int): PMBus page (0-16) selecting VH1-VH4/VP1-VP13.

    Returns:
        float: Voltage in volts, or None on failure.
    """
    # Set the page
    if not _i2c_set_page(bus, addr, page):
        return None

    # Read VOUT_MODE to get the exponent
    mode = _i2c_read_byte(bus, addr, _PMBUS_VOUT_MODE)
    if mode is None:
        return None

    # VOUT_MODE format: [7:5] = mode (should be 0b000 for LINEAR),
    #                   [4:0] = exponent (signed 5-bit)
    exponent = mode & 0x1F

    # Read READ_VOUT
    raw = _i2c_read_word(bus, addr, _PMBUS_READ_VOUT)
    if raw is None:
        return None

    return _decode_linear16(raw, exponent)


class Adm1266VoltageSensor(VoltageSensorBase):
    """
    Ciena voltage sensor backed by ADM1266 PMBus sequencer.

    Reads a single voltage rail from one of the two ADM1266 devices
    and reports it in millivolts (mV) for SONiC sensormond.
    """

    def __init__(self, name, bus, addr, page, nominal_mv,
                 warn_pct=5.0, crit_pct=10.0):
        """
        Args:
            name       (str):   Human-readable sensor name
            bus        (int):   I2C bus number
            addr       (int):   I2C device address (0x48 or 0x49)
            page       (int):   PMBus page (0-16) for the channel
            nominal_mv (float): Nominal rail voltage in mV (for thresholds)
            warn_pct   (float): Warning threshold percentage (default ±5%)
            crit_pct   (float): Critical threshold percentage (default ±10%)
        """
        VoltageSensorBase.__init__(self)
        self._name = name
        self._bus = bus
        self._addr = addr
        self._page = page
        self._nominal_mv = nominal_mv
        self._warn_pct = warn_pct
        self._crit_pct = crit_pct
        self._min_recorded = None
        self._max_recorded = None

    # ------------------------------------------------------------------
    # DeviceBase methods
    # ------------------------------------------------------------------

    def get_name(self):
        return self._name

    def get_presence(self):
        """Check if the ADM1266 is accessible on I2C."""
        try:
            proc = subprocess.run(
                ["i2cget", "-y", "-f", str(self._bus), str(self._addr),
                 "0x00", "b"],
                capture_output=True, timeout=2
            )
            return proc.returncode == 0
        except Exception:
            return False

    def get_model(self):
        return "ADM1266"

    def get_serial(self):
        return "N/A"

    def get_status(self):
        return self.get_presence()

    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return False

    # ------------------------------------------------------------------
    # VoltageSensorBase methods
    # ------------------------------------------------------------------

    def get_value(self):
        """Read the ADM1266 channel and return voltage in millivolts.

        Returns:
            A float: voltage in mV, or 0.0 on failure.
        """
        volts = read_adm1266_voltage(self._bus, self._addr, self._page)
        if volts is None:
            return 0.0

        mv = volts * 1000.0

        # Track min/max
        if self._min_recorded is None or mv < self._min_recorded:
            self._min_recorded = mv
        if self._max_recorded is None or mv > self._max_recorded:
            self._max_recorded = mv

        return float("{:.1f}".format(mv))

    def get_high_threshold(self):
        """High warning threshold in mV (nominal + warn_pct%)."""
        return self._nominal_mv * (1.0 + self._warn_pct / 100.0)

    def get_low_threshold(self):
        """Low warning threshold in mV (nominal - warn_pct%)."""
        return self._nominal_mv * (1.0 - self._warn_pct / 100.0)

    def get_high_critical_threshold(self):
        """High critical threshold in mV (nominal + crit_pct%)."""
        return self._nominal_mv * (1.0 + self._crit_pct / 100.0)

    def get_low_critical_threshold(self):
        """Low critical threshold in mV (nominal - crit_pct%)."""
        return self._nominal_mv * (1.0 - self._crit_pct / 100.0)

    def set_high_threshold(self, value):
        return False

    def set_low_threshold(self, value):
        return False

    def get_minimum_recorded(self):
        """Returns the minimum recorded voltage in mV."""
        if self._min_recorded is None:
            self.get_value()
        return self._min_recorded if self._min_recorded is not None else 0.0

    def get_maximum_recorded(self):
        """Returns the maximum recorded voltage in mV."""
        if self._max_recorded is None:
            self.get_value()
        return self._max_recorded if self._max_recorded is not None else 0.0
