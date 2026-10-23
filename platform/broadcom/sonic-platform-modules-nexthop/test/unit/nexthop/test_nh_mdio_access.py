"""Unit tests for the gearbox MDIO access library
(nh-5010/agera2_mdio/nh_mdio_access.c).
"""

import ctypes
import os
import re
import subprocess

import pytest

_CWD = os.path.dirname(os.path.abspath(__file__))
MDIO_SRC_DIR = os.path.abspath(os.path.join(_CWD, "../../../nh-5010/agera2_mdio"))

SUCCESS = 0
FAILURE = -1
INVALID_PARAMETER = -5

PHY_ADDR = 0x1F
SOME_REG = (7 << 16) | 2


def _max_buses():
    """Track the library's bus count rather than restating it."""
    src = open(os.path.join(MDIO_SRC_DIR, "nh_mdio_access.c")).read()
    return int(re.search(r"#define\s+MDIO_MAX_BUSES\s+(\d+)", src).group(1))


MAX_BUSES = _max_buses()
ABSENT_BUS = MAX_BUSES - 1


def _ctx(bus, addr=PHY_ADDR):
    """Pack an address the way gearbox_config.json does: addr high, bus low."""
    return (addr << 16) | bus


@pytest.fixture(scope="module")
def mdio():
    subprocess.run(["make", "-C", MDIO_SRC_DIR], check=True,
                   stdout=subprocess.DEVNULL)

    lib = ctypes.CDLL(os.path.join(MDIO_SRC_DIR, "libgbsyncdaccess.so"))
    sig = [ctypes.c_uint64, ctypes.c_uint32, ctypes.c_uint32,
           ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    for fn in (lib.mdio_read, lib.mdio_write):
        fn.restype = ctypes.c_int32
        fn.argtypes = sig
    return lib


def _u32(*values):
    return (ctypes.c_uint32 * max(len(values), 1))(*values)


def test_zero_register_count_is_rejected(mdio):
    assert mdio.mdio_read(_ctx(0), PHY_ADDR, SOME_REG, 0, _u32(0)) == INVALID_PARAMETER
    assert mdio.mdio_write(_ctx(0), PHY_ADDR, SOME_REG, 0, _u32(0)) == INVALID_PARAMETER


def test_null_payload_is_rejected(mdio):
    null = ctypes.cast(None, ctypes.POINTER(ctypes.c_uint32))

    assert mdio.mdio_read(_ctx(0), PHY_ADDR, SOME_REG, 1, null) == INVALID_PARAMETER
    assert mdio.mdio_write(_ctx(0), PHY_ADDR, SOME_REG, 1, null) == INVALID_PARAMETER


def test_bus_beyond_the_fd_table_is_rejected(mdio):
    """Out of range must be refused outright, never folded onto another bus.

    bus_fds[] is indexed by the bus, so an unchecked index reads past the
    table, and masking it into range would quietly drive a different PHY.
    """
    assert mdio.mdio_read(_ctx(MAX_BUSES), PHY_ADDR, SOME_REG, 1, _u32(0)) == INVALID_PARAMETER
    assert mdio.mdio_write(_ctx(MAX_BUSES), PHY_ADDR, SOME_REG, 1, _u32(0)) == INVALID_PARAMETER
    assert mdio.mdio_read(_ctx(MAX_BUSES + 5), PHY_ADDR, SOME_REG, 1, _u32(0)) == INVALID_PARAMETER


def test_absent_bus_node_fails(mdio):
    """In range but with no sysfs node: a plain failure, not a bad status.

    The build host has no /sys/class/mdio_bus at all, so any in-range bus is
    absent here; on a DUT every one of them exists.
    """
    assert mdio.mdio_read(_ctx(ABSENT_BUS), PHY_ADDR, SOME_REG, 1, _u32(0)) == FAILURE
    assert mdio.mdio_write(_ctx(ABSENT_BUS), PHY_ADDR, SOME_REG, 1, _u32(0)) == FAILURE
