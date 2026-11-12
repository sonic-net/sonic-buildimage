"""
CP2112 USB-to-I2C bridges of the Lanaotek LN-4332Z1 (arm64-larch_ln4332z-v1).

The carrier has three Silabs CP2112 bridges, one per USB host of the SolidRun
CEx7 CN913x module (HW spec "Hardware Block Diagram"):

  kind      USB host (xHCI)      devices behind it
  "sys"     cp0  0xf2510000      PCA9548 0x70: ch0 PSU A (PMBus 0x5b), ch1 PSU B
                                 (PMBus 0x5b), ch2 core VR, ch3 LM75 0x48/0x49
                                 (board temp), ch4/ch5 98CX8522 I2C;
                                 PCA9548 0x71: ch0 SyncE 8A34002
  "io"      cp1  0xf4500000      PCA9555 0x20/0x22/0x24/0x26 QSFP ModPrsL (port 0)
                                 and IntL (port 1); 0x21/0x23/0x25/0x27 ResetL
                                 (port 0) and LPMode (port 1)
  "eeprom"  cp2  0xf6500000      PCA9548 0x70-0x73 QSFP28 EEPROMs, 0x74 SFP+
                                 EEPROMs, PCA9555 0x20 SFP+ control/status

The i2c adapter numbers are assigned when hid-cp2112 binds, so they are
resolved at run time from the USB topology. Only known addresses may be
touched through these bridges: a transfer to an unknown address can time out
and wedge the bridge until the board is powered off.
"""

import glob
import os

CP2112_ID = ("10c4", "ea90")

BRIDGE_XHCI = {
    "sys": "f2510000.usb3",
    "io": "f4500000.usb3",
    "eeprom": "f6500000.usb3",
}

_cache = {}


def _read(path):
    try:
        with open(path) as f:
            return f.read().strip()
    except OSError:
        return None


def bridge_bus(kind):
    """i2c adapter number of the CP2112 bridge `kind`, or None when absent."""
    if kind in _cache and os.path.exists("/sys/class/i2c-adapter/i2c-%d" % _cache[kind]):
        return _cache[kind]
    xhci = BRIDGE_XHCI[kind]
    for dev in glob.glob("/sys/bus/usb/devices/*-*"):
        if ":" in os.path.basename(dev):
            continue
        if (_read(os.path.join(dev, "idVendor")), _read(os.path.join(dev, "idProduct"))) != CP2112_ID:
            continue
        if xhci not in os.path.realpath(dev):
            continue
        adapters = glob.glob(os.path.join(dev, "*:1.0", "0003:*", "i2c-*"))
        if adapters:
            bus = int(os.path.basename(adapters[0]).replace("i2c-", ""))
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
    if len(chips) != 1:
        return None
    base = _read(os.path.join(chips[0], "base"))
    return int(base) if base is not None else None
