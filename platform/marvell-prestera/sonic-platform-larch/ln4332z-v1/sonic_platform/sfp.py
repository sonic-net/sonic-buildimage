#!/usr/bin/env python
"""
Transceivers of the Lanaotek LN-4332Z1 (arm64-larch_ln4332z-v1).

Front panel: 32x QSFP28 (ports 1-32) and 2x SFP+ (ports 33-34), all reached
over the CP2112 USB-to-I2C bridges described in bridges.py (HW spec sheets
"Falcon_I2C1_SFP-I2C-Master" and "Falcon-I2C_SFP-IO-Master"):

  EEPROM bridge:  PCA9548 0x70 (U51) ch0-7 -> ports 5-12
                  PCA9548 0x71 (U50) ch0-3 -> ports 1-4,  ch4-7 -> ports 13-16
                  PCA9548 0x72 (U53) ch0-3 -> ports 17-20, ch4-7 -> ports 29-32
                  PCA9548 0x73 (U52) ch0-7 -> ports 21-28
                  PCA9548 0x74 (U55) ch0 -> SFP+1, ch1 -> SFP+2
                  PCA9555 0x20 (U54) SFP+ IO: port 0 = SFP+1, port 1 = SFP+2
                    bit0 TX_FAULT, bit1 TX_DIS, bit2 ABS, bit3 RS, bit4 RX_LOS
  IO bridge:      PCA9555 pair k = 0x20+2k (ModPrsL on port 0, IntL on port 1)
                  and 0x21+2k (ResetL on port 0, LPMode on port 1); pair 0 ->
                  ports 5-12, 1 -> ports 1-4,13-16, 2 -> ports 17-20,29-32,
                  3 -> ports 21-28 (bit = index within that group)

The muxes, expanders and optoe devices are instantiated by the platform init
script; here we only resolve sysfs paths (gpio-pca953x exports the expanders as
gpiochips) and read/write them.
"""

try:
    import glob
    import os
    import time
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform.bridges import bridge_bus, mux_child, gpiochip_base
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class SfpBoardConfig:
    SFP_TYPE = "SFP"
    QSFP_TYPE = "QSFP"

    PORT_START = 1
    PORT_END = 34
    QSFP_PORT_START = 1
    QSFP_PORT_END = 32
    SFP_PORT_START = 33
    SFP_PORT_END = 34


# panel port -> (PCA9548 address, channel) on the EEPROM bridge
EEPROM_MUX = {}
for _p in range(5, 13):
    EEPROM_MUX[_p] = (0x70, _p - 5)
for _p in range(1, 5):
    EEPROM_MUX[_p] = (0x71, _p - 1)
for _p in range(13, 17):
    EEPROM_MUX[_p] = (0x71, _p - 13 + 4)
for _p in range(17, 21):
    EEPROM_MUX[_p] = (0x72, _p - 17)
for _p in range(29, 33):
    EEPROM_MUX[_p] = (0x72, _p - 29 + 4)
for _p in range(21, 29):
    EEPROM_MUX[_p] = (0x73, _p - 21)
EEPROM_MUX[33] = (0x74, 0)
EEPROM_MUX[34] = (0x74, 1)

# QSFP panel port -> (expander pair 0..3, bit 0..7) on the IO bridge
QSFP_IO = {}
for _i, _p in enumerate(range(5, 13)):
    QSFP_IO[_p] = (0, _i)
for _i, _p in enumerate(list(range(1, 5)) + list(range(13, 17))):
    QSFP_IO[_p] = (1, _i)
for _i, _p in enumerate(list(range(17, 21)) + list(range(29, 33))):
    QSFP_IO[_p] = (2, _i)
for _i, _p in enumerate(range(21, 29)):
    QSFP_IO[_p] = (3, _i)

# gpio offsets inside a PCA9555 (16 lines): port 0 = 0..7, port 1 = 8..15
QSFP_PRS_CHIP, QSFP_INT_CHIP, QSFP_RST_CHIP, QSFP_LP_CHIP = 0, 0, 1, 1
QSFP_PRS_PORT, QSFP_INT_PORT, QSFP_RST_PORT, QSFP_LP_PORT = 0, 1, 0, 1

# Panel ports whose ModPrsL and IntL are swapped on the board against the HW
# spec (V2.4, U59 0x24): on PORT32 ZQSFP_M_PRS23 is on IO1_7 and
# ZQSFP_M_INT23 on IO0_7. Measured with a module plugged in and pulled out.
QSFP_PRS_INT_SWAPPED = {32}

SFP_IO_ADDR = 0x20
SFP_TXFAULT_BIT, SFP_TXDIS_BIT, SFP_ABS_BIT, SFP_RS_BIT, SFP_RXLOS_BIT = 0, 1, 2, 3, 4

GPIO_SYSFS = "/sys/class/gpio"


def _read(path):
    try:
        with open(path) as f:
            return f.read().strip()
    except OSError:
        return None


def _gpio_path(gpio_num, direction=None):
    """Export a sysfs GPIO (if needed) and return its directory, or None.

    direction: None = leave as is (read the line level), "high"/"low" = make
    it an output driving that level unless it already is an output.
    """
    if gpio_num is None:
        return None
    gdir = os.path.join(GPIO_SYSFS, "gpio%d" % gpio_num)
    if not os.path.exists(gdir):
        try:
            with open(os.path.join(GPIO_SYSFS, "export"), "w") as f:
                f.write(str(gpio_num))
        except OSError:
            return None
        for _ in range(20):
            if os.path.exists(os.path.join(gdir, "value")):
                break
            time.sleep(0.05)
    if direction in ("high", "low"):
        try:
            if _read(os.path.join(gdir, "direction")) != "out":
                with open(os.path.join(gdir, "direction"), "w") as f:
                    f.write(direction)
        except OSError:
            pass
    return gdir if os.path.exists(os.path.join(gdir, "value")) else None


def _gpio_get(gdir):
    v = _read(os.path.join(gdir, "value")) if gdir else None
    return None if v is None else v == "1"


def _gpio_set(gdir, value):
    if gdir is None:
        return False
    try:
        with open(os.path.join(gdir, "value"), "w") as f:
            f.write("1" if value else "0")
        return True
    except OSError:
        return False


class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class implementation"""

    def __init__(self, index, sfp_type):
        assert SfpBoardConfig.PORT_START <= index <= SfpBoardConfig.PORT_END
        SfpOptoeBase.__init__(self)
        self.port_num = index
        self.type = sfp_type
        self.eeprom_path = None
        self._gpio = {}
        self._resolve_eeprom()

    # ---- helpers -----------------------------------------------------
    def _resolve_eeprom(self):
        mux_addr, channel = EEPROM_MUX[self.port_num]
        bus = mux_child(bridge_bus("eeprom"), mux_addr, channel)
        if bus is None:
            return None
        path = "/sys/bus/i2c/devices/%d-0050/eeprom" % bus
        self.eeprom_path = path if os.path.exists(path) else None
        return self.eeprom_path

    def _qsfp_gpio(self, chip, port, direction=None):
        """sysfs GPIO dir of this QSFP's line on the IO bridge expander."""
        key = (chip, port)
        if key in self._gpio and os.path.exists(self._gpio[key]):
            return self._gpio[key]
        pair, bit = QSFP_IO[self.port_num]
        base = gpiochip_base(bridge_bus("io"), 0x20 + 2 * pair + chip)
        if base is None:
            return None
        gdir = _gpio_path(base + 8 * port + bit, direction)
        if gdir:
            self._gpio[key] = gdir
        return gdir

    def _prs_port(self):
        return QSFP_INT_PORT if self.port_num in QSFP_PRS_INT_SWAPPED else QSFP_PRS_PORT

    def _int_port(self):
        return QSFP_PRS_PORT if self.port_num in QSFP_PRS_INT_SWAPPED else QSFP_INT_PORT

    def _sfp_gpio(self, bit, direction=None):
        key = ("sfp", bit)
        if key in self._gpio and os.path.exists(self._gpio[key]):
            return self._gpio[key]
        base = gpiochip_base(bridge_bus("eeprom"), SFP_IO_ADDR)
        if base is None:
            return None
        gdir = _gpio_path(base + 8 * (self.port_num - SfpBoardConfig.SFP_PORT_START) + bit, direction)
        if gdir:
            self._gpio[key] = gdir
        return gdir

    def _is_qsfp(self):
        return self.type == SfpBoardConfig.QSFP_TYPE

    # ---- SfpBase API -------------------------------------------------
    def get_eeprom_path(self):
        if self.eeprom_path is None or not os.path.exists(self.eeprom_path):
            self._resolve_eeprom()
        return self.eeprom_path

    def get_optoe_type(self):
        return 1 if self._is_qsfp() else 2

    def get_name(self):
        return ("QSFP28 %d" if self._is_qsfp() else "SFP+ %d") % self.port_num

    def get_position_in_parent(self):
        return self.port_num

    def is_replaceable(self):
        return True

    def get_presence(self):
        if self._is_qsfp():
            v = _gpio_get(self._qsfp_gpio(QSFP_PRS_CHIP, self._prs_port()))
        else:
            v = _gpio_get(self._sfp_gpio(SFP_ABS_BIT))
        if v is None:
            # expander not available: fall back to "EEPROM readable"
            path = self.get_eeprom_path()
            if path is None:
                return False
            try:
                with open(path, "rb") as f:
                    return len(f.read(1)) == 1
            except OSError:
                return False
        return not v      # ModPrsL / ABS are active low

    def get_status(self):
        return self.get_presence()

    def get_reset_status(self):
        if not self._is_qsfp():
            return False
        v = _gpio_get(self._qsfp_gpio(QSFP_RST_CHIP, QSFP_RST_PORT))
        return bool(v is not None and not v)     # ResetL low = in reset

    def reset(self):
        if not self._is_qsfp():
            return False
        gdir = self._qsfp_gpio(QSFP_RST_CHIP, QSFP_RST_PORT, "high")
        if not _gpio_set(gdir, False):
            return False
        time.sleep(0.5)
        return _gpio_set(gdir, True)

    def get_lpmode(self):
        if not self._is_qsfp():
            return False
        return bool(_gpio_get(self._qsfp_gpio(QSFP_LP_CHIP, QSFP_LP_PORT)))

    def set_lpmode(self, lpmode):
        if not self._is_qsfp():
            return False
        return _gpio_set(self._qsfp_gpio(QSFP_LP_CHIP, QSFP_LP_PORT, "low"), lpmode)

    def get_rx_los(self):
        if self._is_qsfp():
            return SfpOptoeBase.get_rx_los(self)
        v = _gpio_get(self._sfp_gpio(SFP_RXLOS_BIT))
        return None if v is None else [v]

    def get_tx_fault(self):
        if self._is_qsfp():
            return SfpOptoeBase.get_tx_fault(self)
        v = _gpio_get(self._sfp_gpio(SFP_TXFAULT_BIT))
        return None if v is None else [v]

    def get_tx_disable(self):
        if self._is_qsfp():
            return SfpOptoeBase.get_tx_disable(self)
        v = _gpio_get(self._sfp_gpio(SFP_TXDIS_BIT))
        return None if v is None else [v]

    def tx_disable(self, tx_disable):
        if self._is_qsfp():
            return SfpOptoeBase.tx_disable(self, tx_disable)
        return _gpio_set(self._sfp_gpio(SFP_TXDIS_BIT, "low"), tx_disable)

    def get_interrupt(self):
        """True when the QSFP asserts IntL (active low)."""
        if not self._is_qsfp():
            return False
        v = _gpio_get(self._qsfp_gpio(QSFP_INT_CHIP, self._int_port()))
        return bool(v is not None and not v)

    def get_transceiver_info(self):
        info = SfpOptoeBase.get_transceiver_info(self)
        if info is not None and not info.get("hardware_rev"):
            info["hardware_rev"] = "N/A"
        return info
