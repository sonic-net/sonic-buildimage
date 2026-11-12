"""
CP2112 USB-to-I2C bridges of the Larch LN-4348Y-8Z (arm64-larch_ln4348y-8z-v1).

The carrier has three Silabs CP2112 bridges on a USB 2.0 hub (1a86:8091) behind
the CPU module's second xHCI (cp0 0xf2510000); HW spec sheet "Optical DDM
manage":

  kind      COM Express USB   devices behind it
  "eeprom"  USB1 -> I2C0      PCA9548 0x70-0x75 (U84,U86,U85,U88,U90,U87):
                              SFP28 EEPROMs, DDM port d on mux 0x70 + d/8, ch d%8
  "io"      USB3 -> I2C1      PCA9555 0x20/0x21/0x22 ABS (ports 0-15/16-31/32-47),
                              0x23/0x24/0x25 RX_LOS, 0x26 TX_FAULT (port 0, one
                              bit per 8-port group) and RS (port 1)
  "qsfp"    USB4 -> I2C2      PCA9548 0x70 (U89) QSFP28 EEPROMs ch 0-7,
                              PCA9555 0x20 (U139) ModPrsL / IntL, 0x21 (U140)
                              ResetL / LPMode

Which hub port carries which bridge is not documented; the platform init
script probes each bridge once at safe addresses (the PCA9548 at 0x75 only
exists on the EEPROM bridge, the PCA9555 at 0x26 only on the IO bridge) and
records the result in a cache file that this module reads. Without the cache
the hub port order 1/2/3 = eeprom/io/qsfp is assumed.

Only known addresses may be touched through these bridges: a transfer to an
unknown address can time out and wedge the bridge until the board is powered
off.
"""

import glob
import os

CP2112_ID = ("10c4", "ea90")
CACHE = "/var/run/platform_cache/ln4348y-8z-v1/bridges"
DEFAULT_HUB_PORT = {"eeprom": ".1", "io": ".2", "qsfp": ".3"}

_cache = {}


def _read(path):
    try:
        with open(path) as f:
            return f.read().strip()
    except OSError:
        return None


def _cp2112_adapters():
    """{usb device name (e.g. '3-1.2'): i2c adapter number} for every CP2112."""
    found = {}
    for dev in glob.glob("/sys/bus/usb/devices/*-*"):
        name = os.path.basename(dev)
        if ":" in name:
            continue
        if (_read(os.path.join(dev, "idVendor")), _read(os.path.join(dev, "idProduct"))) != CP2112_ID:
            continue
        adapters = glob.glob(os.path.join(dev, "*:1.0", "0003:*", "i2c-*"))
        if adapters:
            found[name] = int(os.path.basename(adapters[0]).replace("i2c-", ""))
    return found


def bridge_bus(kind):
    """i2c adapter number of the CP2112 bridge `kind`, or None when absent."""
    if kind in _cache and os.path.exists("/sys/class/i2c-adapter/i2c-%d" % _cache[kind]):
        return _cache[kind]
    # 1. the mapping probed by the init script
    txt = _read(CACHE)
    if txt:
        for line in txt.splitlines():
            parts = line.split()
            if len(parts) == 2 and parts[0] == kind and parts[1].isdigit():
                bus = int(parts[1])
                if os.path.exists("/sys/class/i2c-adapter/i2c-%d" % bus):
                    _cache[kind] = bus
                    return bus
    # 2. fall back to the hub port order
    suffix = DEFAULT_HUB_PORT.get(kind)
    if suffix is None:
        return None
    for name, bus in _cp2112_adapters().items():
        if name.endswith(suffix):
            _cache[kind] = bus
            return bus
    return None


def mux_child(bus, mux_addr, channel):
    """Adapter number behind channel `channel` of the PCA9548 at `mux_addr` on `bus`."""
    if bus is None:
        return None
    link = "/sys/bus/i2c/devices/%d-%04x/channel-%d" % (bus, mux_addr, channel)
    if not os.path.exists(link):
        return None
    name = os.path.basename(os.path.realpath(link))
    return int(name.replace("i2c-", "")) if name.startswith("i2c-") else None


def gpiochip_base(bus, addr):
    """sysfs GPIO base of the pca953x expander at `addr` on `bus`, or None."""
    if bus is None:
        return None
    chips = glob.glob("/sys/bus/i2c/devices/%d-%04x/gpio/gpiochip*" % (bus, addr))
    if not chips:
        return None
    base = _read(os.path.join(chips[0], "base"))
    return int(base) if base and base.isdigit() else None
