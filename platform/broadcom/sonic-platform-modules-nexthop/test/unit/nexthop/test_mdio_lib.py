# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import pytest


@pytest.fixture(scope="function", autouse=True)
def mdio_lib():
    from nexthop import mdio_lib

    yield mdio_lib


class FakeClock:
    def __init__(self):
        self.now = 0.0

    def __call__(self):
        return self.now

    def sleep(self, seconds):
        self.now += seconds


class FakeMdioFpga:
    """FPGA Clause 22 MDIO master fronting a set of PHY register files.

    `phys` maps phy address -> {reg: value}. Reads of absent PHYs return 0xFFFF,
    as an undriven MDIO bus does. Read-data-valid is asserted after `valid_after`
    polls (0 = immediately); a None value never asserts it.
    """

    def __init__(self, phys, valid_after=0):
        self.phys = phys
        self.valid_after = valid_after
        self.writes = []
        self._pending = None
        self._polls = 0

    def read32(self, offset):
        assert offset == 0x4, f"unexpected read of offset 0x{offset:X}"
        if self._pending is None or self.valid_after is None:
            return 0
        self._polls += 1
        if self._polls <= self.valid_after:
            return 0
        return 0x80000000 | self._pending

    def write32(self, offset, value):
        assert offset == 0x0, f"unexpected write of offset 0x{offset:X}"
        self.writes.append(value)
        if value == 0:
            return
        phy = (value >> 24) & 0x1F
        reg = (value >> 16) & 0x1F
        data = value & 0xFFFF
        regs = self.phys.get(phy)
        if value & (1 << 31):
            if regs is not None:
                self.on_phy_write(phy, reg, data)
        elif value & (1 << 30):
            self._polls = 0
            self._pending = 0xFFFF if regs is None else regs.get(reg, 0)

    def on_phy_write(self, phy, reg, data):
        self.phys[phy][reg] = data


class FakeMarvellSwitch(FakeMdioFpga):
    """Multi-chip-mode Marvell switch behind a bridge PHY.

    `internal` maps (device, reg) -> value. A write to SMI Command executes the
    indirect op immediately and clears SMIBusy, unless `stuck_busy` is set, in
    which case SMIBusy never clears, as seen when the BMC is the platform owner.
    """

    def __init__(self, bridge_phy, internal, stuck_busy=False, valid_after=0):
        super().__init__({bridge_phy: {0x0: 0x0, 0x1: 0x0}}, valid_after)
        self.bridge = bridge_phy
        self.internal = internal
        self.stuck_busy = stuck_busy
        self.smi_ops = []

    def on_phy_write(self, phy, reg, data):
        regs = self.phys[phy]
        if reg != 0x0:
            regs[reg] = data
            return
        op = (data >> 10) & 0x3
        device = (data >> 5) & 0x1F
        ireg = data & 0x1F
        self.smi_ops.append((op, device, ireg, regs[0x1]))
        if self.stuck_busy:
            regs[0x0] = data
            return
        if op == 0x2:
            regs[0x1] = self.internal.get((device, ireg), 0)
        elif op == 0x1:
            self.internal[(device, ireg)] = regs[0x1]
        regs[0x0] = data & ~(1 << 15)


def make_master(mdio_lib, fpga, clock=None):
    clock = clock or FakeClock()
    return mdio_lib.FpgaMdioC22Master(fpga.read32, fpga.write32, sleep=clock.sleep, clock=clock), clock


# --- FpgaMdioC22Master ---


def test_control_word_encoding_matches_cf2_transcript(mdio_lib):
    fpga = FakeMdioFpga({0x1E: {0x0: 0x0}})
    master, _ = make_master(mdio_lib, fpga)
    master.write(0x1E, 0x0, 0x9B9A)
    master.read(0x1E, 0x1)
    # Values observed on CF2 88E6191X hardware.
    assert fpga.writes == [0x9E009B9A, 0x0, 0x5E010000, 0x0]


def test_read_returns_16bit_data_when_valid(mdio_lib):
    fpga = FakeMdioFpga({0x05: {0x3: 0x1920}})
    master, _ = make_master(mdio_lib, fpga)
    assert master.read(0x05, 0x3) == 0x1920


def test_read_polls_until_valid(mdio_lib):
    fpga = FakeMdioFpga({0x05: {0x3: 0xABCD}}, valid_after=3)
    master, clock = make_master(mdio_lib, fpga)
    assert master.read(0x05, 0x3) == 0xABCD
    assert clock.now > 0


def test_read_times_out_and_still_rearms(mdio_lib):
    fpga = FakeMdioFpga({0x05: {0x3: 0xABCD}}, valid_after=None)
    master, _ = make_master(mdio_lib, fpga)
    with pytest.raises(mdio_lib.MdioTimeoutError):
        master.read(0x05, 0x3)
    assert fpga.writes[-1] == 0


def test_read_of_absent_phy_returns_all_ones(mdio_lib):
    fpga = FakeMdioFpga({})
    master, _ = make_master(mdio_lib, fpga)
    assert master.read(0x1E, 0x0) == 0xFFFF


@pytest.mark.parametrize("phy, reg, data", [(0x20, 0, 0), (0, 0x20, 0), (0, 0, 0x10000)])
def test_write_rejects_out_of_range(mdio_lib, phy, reg, data):
    master, _ = make_master(mdio_lib, FakeMdioFpga({}))
    with pytest.raises(ValueError):
        master.write(phy, reg, data)


# --- MarvellSmi ---


def make_smi(mdio_lib, switch, **kwargs):
    master, clock = make_master(mdio_lib, switch)
    return mdio_lib.MarvellSmi(master, switch.bridge, sleep=clock.sleep, clock=clock, **kwargs)


def test_smi_read_issues_read_command_and_returns_data(mdio_lib):
    switch = FakeMarvellSwitch(0x1E, {(0x1C, 0x1A): 0x0101})
    smi = make_smi(mdio_lib, switch)
    assert smi.read(0x1C, 0x1A) == 0x0101
    assert switch.smi_ops == [(0x2, 0x1C, 0x1A, 0x0)]
    # SMI Command word as seen on the wire in the hardware transcript.
    assert 0x9E009B9A in switch.writes


def test_smi_write_loads_data_then_command(mdio_lib):
    switch = FakeMarvellSwitch(0x1E, {})
    smi = make_smi(mdio_lib, switch)
    smi.write(0x1C, 0x1A, 0x0100)
    assert switch.smi_ops == [(0x1, 0x1C, 0x1A, 0x0100)]
    assert switch.internal[(0x1C, 0x1A)] == 0x0100
    assert 0x9E010100 in switch.writes  # data reg first
    assert 0x9E00979A in switch.writes  # then command


def test_smi_busy_never_clears_raises(mdio_lib):
    switch = FakeMarvellSwitch(0x1E, {(0x1C, 0x1A): 0x0101}, stuck_busy=True)
    smi = make_smi(mdio_lib, switch, timeout_s=0.01)
    with pytest.raises(mdio_lib.SmiTimeoutError):
        smi.read(0x1C, 0x1A)


def test_read_switch_id_masks_revision(mdio_lib):
    switch = FakeMarvellSwitch(0x1E, {(0x00, 0x03): 0x1923})
    smi = make_smi(mdio_lib, switch)
    assert smi.read_switch_id(port0_device=0x00) == 0x1920


def test_absent_switch_raises_no_device_not_timeout(mdio_lib):
    switch = FakeMarvellSwitch(0x1E, {})
    switch.phys = {}  # nothing answers at the bridge address
    smi = make_smi(mdio_lib, switch)
    with pytest.raises(mdio_lib.SmiNoDeviceError):
        smi.read_switch_id(port0_device=0x00)
    assert not issubclass(mdio_lib.SmiNoDeviceError, TimeoutError)


def test_read_scratch_byte_uses_pointer_without_update(mdio_lib):
    class ScratchSwitch(FakeMarvellSwitch):
        def on_phy_write(self, phy, reg, data):
            super().on_phy_write(phy, reg, data)
            if self.smi_ops and self.smi_ops[-1][:3] == (0x1, 0x1C, 0x1A):
                pointer_word = self.smi_ops[-1][3]
                assert pointer_word & (1 << 15) == 0, "Update bit must be clear"
                self.internal[(0x1C, 0x1A)] = pointer_word | self.scratch[pointer_word >> 8]

    switch = ScratchSwitch(0x1E, {})
    switch.scratch = {0: 0x01, 1: 0x04}
    smi = make_smi(mdio_lib, switch)
    assert smi.read_scratch_byte(0) == 0x01
    assert smi.read_scratch_byte(1) == 0x04
    assert smi.read_config_version() == "1.4"


# --- fpga_register_io ---


def test_fpga_register_io_offsets_from_block_base(mdio_lib, monkeypatch):
    from nexthop import fpga_lib

    calls = []
    monkeypatch.setattr(fpga_lib, "name_to_bdf", lambda name, cfg=None: "0000:e3:00.0")
    monkeypatch.setattr(fpga_lib, "read_32", lambda bdf, off: calls.append(("r", bdf, off)) or 0x80001234)
    monkeypatch.setattr(fpga_lib, "write_32", lambda bdf, off, val: calls.append(("w", bdf, off, val)))

    read32, write32 = mdio_lib.fpga_register_io("CPUCARD_FPGA", 0xF0)
    write32(0x0, 0x5E000000)
    assert read32(0x4) == 0x80001234
    assert calls == [("w", "0000:e3:00.0", 0xF0, 0x5E000000), ("r", "0000:e3:00.0", 0xF4)]


def test_fpga_register_io_unknown_fpga(mdio_lib, monkeypatch):
    from nexthop import fpga_lib

    monkeypatch.setattr(fpga_lib, "name_to_bdf", lambda name, cfg=None: None)
    with pytest.raises(RuntimeError):
        mdio_lib.fpga_register_io("NOPE", 0xF0)
