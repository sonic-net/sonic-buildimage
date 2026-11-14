#!/usr/bin/env python
"""
Transceivers of the Larch LN-4348Y-8Z (arm64-larch_ln4348y-8z-v1).

Front panel: 48x SFP28 (ports 1-48) and 8x QSFP28 (ports 49-56), all reached
over the CP2112 USB-to-I2C bridges described in bridges.py (HW spec sheet
"Optical DDM manage").

The spec numbers the SFP28 control/DDM signals 0-47 ("DDM port" d); the
relation to the panel (section 6 of the sheet, via the SerDes lane) is
  panel 1-16  -> d = panel + 15
  panel 17-32 -> d = panel - 17
  panel 33-48 -> d = panel - 1
For the QSFP28 cages the DDM index is q = panel - 49.

  EEPROM bridge:  PCA9548 0x70 + d/8, channel d%8 -> SFP28 EEPROM (optoe2)
  IO bridge:      PCA9555 0x20 + d/16 line d%16 -> ABS (module absent, high)
                  PCA9555 0x23 + d/16 line d%16 -> RX_LOS
                  PCA9555 0x26 line d/8 -> TX_FAULT of the 8-port group,
                               line 8 + d/8 -> RS (rate select) of the group
  QSFP bridge:    PCA9548 0x70 channel q -> QSFP28 EEPROM (optoe1)
                  PCA9555 0x20 line q -> ModPrsL, line 8+q -> IntL
                  PCA9555 0x21 line q -> ResetL,  line 8+q -> LPMode

The muxes, expanders and optoe devices are instantiated by the platform init
script; here we only resolve sysfs paths (gpio-pca953x exports the expanders
as gpiochips) and read/write them.
"""

try:
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
    PORT_END = 56
    SFP_PORT_START = 1
    SFP_PORT_END = 48
    QSFP_PORT_START = 49
    QSFP_PORT_END = 56


def ddm_index(panel):
    """SFP28 DDM/control signal index 0-47 of panel port 1-48."""
    if panel <= 16:
        return panel + 15
    if panel <= 32:
        return panel - 17
    return panel - 1


def qsfp_index(panel):
    return panel - 49


SFP_ABS_ADDR, SFP_RXLOS_ADDR, SFP_GROUP_ADDR = 0x20, 0x23, 0x26
QSFP_MUX_ADDR, QSFP_PRS_INT_ADDR, QSFP_RST_LP_ADDR = 0x70, 0x20, 0x21

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
    def _is_qsfp(self):
        return self.type == SfpBoardConfig.QSFP_TYPE

    def _resolve_eeprom(self):
        if self._is_qsfp():
            bus = mux_child(bridge_bus("qsfp"), QSFP_MUX_ADDR, qsfp_index(self.port_num))
        else:
            d = ddm_index(self.port_num)
            bus = mux_child(bridge_bus("eeprom"), 0x70 + d // 8, d % 8)
        if bus is None:
            self.eeprom_path = None
            return None
        # keep the path even when the optoe device is not there (yet): the
        # optoe base class opens it and handles OSError itself, a None path
        # makes it raise TypeError instead
        self.eeprom_path = "/sys/bus/i2c/devices/%d-0050/eeprom" % bus
        return self.eeprom_path

    def _line(self, kind, addr, offset, direction=None):
        """sysfs GPIO dir of line `offset` of the PCA9555 at `addr` on bridge `kind`."""
        key = (kind, addr, offset)
        if key in self._gpio and os.path.exists(self._gpio[key]):
            return self._gpio[key]
        base = gpiochip_base(bridge_bus(kind), addr)
        if base is None:
            return None
        gdir = _gpio_path(base + offset, direction)
        if gdir:
            self._gpio[key] = gdir
        return gdir

    def _qsfp_line(self, addr, port, direction=None):
        return self._line("qsfp", addr, 8 * port + qsfp_index(self.port_num), direction)

    def _sfp_line(self, addr_base, direction=None):
        d = ddm_index(self.port_num)
        return self._line("io", addr_base + d // 16, d % 16, direction)

    # ---- SfpBase API -------------------------------------------------
    def get_eeprom_path(self):
        if self.eeprom_path is None or not os.path.exists(self.eeprom_path):
            self._resolve_eeprom()
        return self.eeprom_path if self.eeprom_path else "/sys/bus/i2c/devices/none-0050/eeprom"

    def get_optoe_type(self):
        return 1 if self._is_qsfp() else 2

    def get_name(self):
        return ("QSFP28 %d" if self._is_qsfp() else "SFP28 %d") % self.port_num

    def get_position_in_parent(self):
        return self.port_num

    def is_replaceable(self):
        return True

    def get_presence(self):
        if self._is_qsfp():
            v = _gpio_get(self._qsfp_line(QSFP_PRS_INT_ADDR, 0))
        else:
            v = _gpio_get(self._sfp_line(SFP_ABS_ADDR))
        if v is None:
            # expander not available: fall back to "EEPROM readable"
            path = self.get_eeprom_path()
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
        v = _gpio_get(self._qsfp_line(QSFP_RST_LP_ADDR, 0))
        return bool(v is not None and not v)     # ResetL low = in reset

    def reset(self):
        if not self._is_qsfp():
            return False
        gdir = self._qsfp_line(QSFP_RST_LP_ADDR, 0, "high")
        if not _gpio_set(gdir, False):
            return False
        time.sleep(0.5)
        return _gpio_set(gdir, True)

    def get_lpmode(self):
        if not self._is_qsfp():
            return False
        return bool(_gpio_get(self._qsfp_line(QSFP_RST_LP_ADDR, 1)))

    def set_lpmode(self, lpmode):
        if not self._is_qsfp():
            return False
        return _gpio_set(self._qsfp_line(QSFP_RST_LP_ADDR, 1, "low"), lpmode)

    def get_rx_los(self):
        if self._is_qsfp():
            return SfpOptoeBase.get_rx_los(self)
        v = _gpio_get(self._sfp_line(SFP_RXLOS_ADDR))
        return None if v is None else [v]

    def get_tx_fault(self):
        if self._is_qsfp():
            return SfpOptoeBase.get_tx_fault(self)
        # one TX_FAULT line per 8-port group (spec: zTX_FLT0-5 on U141)
        v = _gpio_get(self._line("io", SFP_GROUP_ADDR, ddm_index(self.port_num) // 8))
        return None if v is None else [v]

    def get_interrupt(self):
        """True when the QSFP asserts IntL (active low)."""
        if not self._is_qsfp():
            return False
        v = _gpio_get(self._qsfp_line(QSFP_PRS_INT_ADDR, 1))
        return bool(v is not None and not v)

    def get_transceiver_info(self):
        info = SfpOptoeBase.get_transceiver_info(self)
        if info is not None and not info.get("hardware_rev"):
            info["hardware_rev"] = "N/A"
        return info
