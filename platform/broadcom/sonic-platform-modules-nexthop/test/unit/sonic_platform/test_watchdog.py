import pytest

from unittest.mock import patch, ANY

_FAKE_FPGA_PCI_ADDR = "FAKE_FPGA_PCI_ADDR"
_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET = 0x28
_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_BIT = 4
_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG = 0x1E0
_FAKE_WATCHDOG_COUNTER_MSI_REG = 0x1D8


@pytest.fixture
def watchdog_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from sonic_platform import watchdog

    yield watchdog


class TestWatchdogHelpers:
    """Tests for module-level helpers (register access)."""

    @pytest.fixture(scope="function", autouse=True)
    def setup(self, watchdog_module):
        self.watchdog_module = watchdog_module

    def test_read_watchdog_counter_register(self):
        with patch.object(
            self.watchdog_module.fpga_lib, "read_32", autospec=True
        ) as mock_read_32:
            self.watchdog_module._read_watchdog_counter_register(
                _FAKE_FPGA_PCI_ADDR, _FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG
            )
            mock_read_32.assert_called_once_with(
                pci_address=_FAKE_FPGA_PCI_ADDR,
                offset=_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG,
            )

    def test_read_watchdog_counter_enable(self):
        with (
            patch.object(self.watchdog_module.fpga_lib, "read_32", autospec=True),
            patch.object(
                self.watchdog_module.fpga_lib, "get_field", autospec=True
            ) as mock_get_field,
        ):
            mock_get_field.return_value = 1
            assert self.watchdog_module._read_watchdog_counter_enable(
                _FAKE_FPGA_PCI_ADDR, _FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG
            )

    def test_update_watchdog_countdown_value(self):
        with (
            patch.object(
                self.watchdog_module.fpga_lib, "read_32", autospec=True
            ) as mock_read_32,
            patch.object(
                self.watchdog_module.fpga_lib, "write_32", autospec=True
            ) as mock_write_32,
            patch.object(
                self.watchdog_module.fpga_lib, "overwrite_field", autospec=True
            ) as mock_overwrite_field,
        ):
            self.watchdog_module._update_watchdog_countdown_value(
                _FAKE_FPGA_PCI_ADDR,
                milliseconds=10,
                reg_offset=_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG,
            )

            mock_overwrite_field.assert_called_once_with(
                reg_val=mock_read_32.return_value, bit_range=(0, 23), field_val=10
            )
            mock_write_32.assert_called_once_with(
                pci_address=_FAKE_FPGA_PCI_ADDR,
                offset=_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG,
                val=mock_overwrite_field.return_value,
            )

    @pytest.mark.parametrize("is_enable,expected_field_val", [(True, 1), (False, 0)])
    def test_toggle_watchdog_counter_enable(self, is_enable, expected_field_val):
        with (
            patch.object(self.watchdog_module.fpga_lib, "read_32", autospec=True),
            patch.object(
                self.watchdog_module.fpga_lib, "write_32", autospec=True
            ) as mock_write_32,
            patch.object(
                self.watchdog_module.fpga_lib, "overwrite_field", autospec=True
            ) as mock_overwrite_field,
        ):
            self.watchdog_module._toggle_watchdog_counter_enable(
                _FAKE_FPGA_PCI_ADDR, is_enable, _FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG
            )
            mock_overwrite_field.assert_called_once_with(
                reg_val=ANY, bit_range=(31, 31), field_val=expected_field_val
            )
            mock_write_32.assert_called_once_with(
                pci_address=_FAKE_FPGA_PCI_ADDR,
                offset=_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG,
                val=mock_overwrite_field.return_value,
            )

    @pytest.mark.parametrize("is_enable,expected_field_val", [(True, 1), (False, 0)])
    def test_toggle_watchdog_reboot(self, is_enable, expected_field_val):
        with (
            patch.object(self.watchdog_module.fpga_lib, "read_32", autospec=True),
            patch.object(
                self.watchdog_module.fpga_lib, "write_32", autospec=True
            ) as mock_write_32,
            patch.object(
                self.watchdog_module.fpga_lib, "overwrite_field", autospec=True
            ) as mock_overwrite_field,
        ):
            self.watchdog_module._toggle_watchdog_reboot(
                _FAKE_FPGA_PCI_ADDR,
                is_enable,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_BIT,
            )
            mock_overwrite_field.assert_called_once_with(
                reg_val=ANY,
                bit_range=(
                    _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_BIT,
                    _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_BIT,
                ),
                field_val=expected_field_val,
            )
            mock_write_32.assert_called_once_with(
                pci_address=_FAKE_FPGA_PCI_ADDR,
                offset=_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
                val=mock_overwrite_field.return_value,
            )

    @pytest.mark.parametrize("control_reg_bit", [0, 4, 31])
    def test_toggle_watchdog_reboot_uses_the_configured_bit(self, control_reg_bit):
        """The control bit comes from pddf-device.json, not a fixed position."""
        with (
            patch.object(self.watchdog_module.fpga_lib, "read_32", autospec=True),
            patch.object(self.watchdog_module.fpga_lib, "write_32", autospec=True),
            patch.object(
                self.watchdog_module.fpga_lib, "overwrite_field", autospec=True
            ) as mock_overwrite_field,
        ):
            self.watchdog_module._toggle_watchdog_reboot(
                _FAKE_FPGA_PCI_ADDR,
                True,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
                control_reg_bit,
            )
            mock_overwrite_field.assert_called_once_with(
                reg_val=ANY,
                bit_range=(control_reg_bit, control_reg_bit),
                field_val=1,
            )


class TestWatchdogSimple:
    """Tests for the single-counter Watchdog class."""

    @pytest.fixture(scope="function", autouse=True)
    def setup(self, watchdog_module):
        self.watchdog = watchdog_module.WatchdogSimple(
            fpga_pci_addr=_FAKE_FPGA_PCI_ADDR,
            event_driven_power_cycle_control_reg_offset=_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
            event_driven_power_cycle_control_bit=_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_BIT,
            watchdog_counter_powercycle_reg=_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG,
        )

    @pytest.mark.parametrize("seconds", [-1, 0, 0x1000000 / 1_000])
    def test_arm_seconds_out_of_bound_error(self, seconds):
        actual_return_value = self.watchdog.arm(seconds)

        assert actual_return_value == -1

    def test_arm_should_update_counter(self, watchdog_module):
        timeout_seconds = 10
        with (
            patch.object(
                watchdog_module, "_update_watchdog_countdown_value", autospec=True
            ) as mock_update,
            patch.object(
                watchdog_module, "_toggle_watchdog_counter_enable", autospec=True
            ),
            patch.object(watchdog_module, "_toggle_watchdog_reboot", autospec=True),
        ):
            actual_return_value = self.watchdog.arm(timeout_seconds)

            assert actual_return_value == timeout_seconds
            mock_update.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR,
                milliseconds=timeout_seconds * 1_000,
                reg_offset=_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG,
            )

    def test_arm_should_enable_counter_reboot(self, watchdog_module):
        with (
            patch.object(
                watchdog_module, "_update_watchdog_countdown_value", autospec=True
            ),
            patch.object(
                watchdog_module, "_toggle_watchdog_counter_enable", autospec=True
            ) as mock_toggle_counter,
            patch.object(
                watchdog_module, "_toggle_watchdog_reboot", autospec=True
            ) as mock_toggle_reboot,
        ):
            self.watchdog.arm(10)

            mock_toggle_reboot.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR,
                True,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_BIT,
            )
            mock_toggle_counter.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR, True, _FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG
            )

    def test_disarm_stops_counter(self, watchdog_module):
        with (
            patch.object(
                watchdog_module, "_toggle_watchdog_counter_enable", autospec=True
            ) as mock_toggle_counter,
            patch.object(
                watchdog_module, "_toggle_watchdog_reboot", autospec=True
            ) as mock_toggle_reboot,
        ):
            actual_return_value = self.watchdog.disarm()

            assert actual_return_value
            mock_toggle_reboot.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR,
                False,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_BIT,
            )
            mock_toggle_counter.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR, False, _FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG
            )

    def test_disarm_returns_false_on_error(self, watchdog_module):
        with (
            patch.object(
                watchdog_module, "_toggle_watchdog_counter_enable", autospec=True
            ),
            patch.object(
                watchdog_module,
                "_toggle_watchdog_reboot",
                autospec=True,
                side_effect=Exception(),
            ),
        ):
            assert not self.watchdog.disarm()

    def test_get_remaining_time_when_not_armed(self, watchdog_module):
        with patch.object(
            watchdog_module,
            "_read_watchdog_counter_enable",
            autospec=True,
            return_value=0,
        ):
            assert self.watchdog.get_remaining_time() == -1

    def test_get_remaining_time_when_armed(self, watchdog_module):
        with (
            patch.object(
                watchdog_module,
                "_read_watchdog_counter_enable",
                autospec=True,
                return_value=1,
            ),
            patch.object(
                watchdog_module,
                "_read_watchdog_countdown_value_milliseconds",
                autospec=True,
                return_value=2_200,
            ),
        ):
            assert self.watchdog.get_remaining_time() == 2


class TestWatchdog:
    """Tests for the 2-counter Watchdog class (MSI + power cycle)."""

    @pytest.fixture(scope="function", autouse=True)
    def setup(self, watchdog_module):
        self.watchdog = watchdog_module.Watchdog(
            fpga_pci_addr=_FAKE_FPGA_PCI_ADDR,
            event_driven_power_cycle_control_reg_offset=_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
            event_driven_power_cycle_control_bit=_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_BIT,
            watchdog_counter_powercycle_reg=_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG,
            watchdog_counter_msi_reg=_FAKE_WATCHDOG_COUNTER_MSI_REG,
        )

    def test_attributes(self):
        """Verify the 2-counter Watchdog stores both register offsets."""
        assert self.watchdog.watchdog_counter_msi_reg == _FAKE_WATCHDOG_COUNTER_MSI_REG
        assert (
            self.watchdog.watchdog_counter_powercycle_reg
            == _FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG
        )

    def test_arm_writes_both_counters(self, watchdog_module):
        """arm() should arm both watchdog counters and enable reboot."""
        timeout_seconds = 120

        with (
            patch.object(
                watchdog_module, "_update_watchdog_countdown_value", autospec=True
            ) as mock_update,
            patch.object(
                watchdog_module, "_toggle_watchdog_counter_enable", autospec=True
            ) as mock_toggle_counter,
            patch.object(
                watchdog_module, "_toggle_watchdog_reboot", autospec=True
            ) as mock_toggle_reboot,
        ):
            result = self.watchdog.arm(timeout_seconds)

            assert result == timeout_seconds
            # Power cycle counter armed with the fixed timeout
            mock_update.assert_any_call(
                _FAKE_FPGA_PCI_ADDR,
                milliseconds=watchdog_module._WATCHDOG_POWER_CYCLE_TIMEOUT_SECONDS
                * 1_000,
                reg_offset=_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG,
            )
            # MSI counter armed with the requested timeout
            mock_update.assert_any_call(
                _FAKE_FPGA_PCI_ADDR,
                milliseconds=timeout_seconds * 1_000,
                reg_offset=_FAKE_WATCHDOG_COUNTER_MSI_REG,
            )
            mock_toggle_counter.assert_any_call(
                _FAKE_FPGA_PCI_ADDR, True, _FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG
            )
            mock_toggle_counter.assert_any_call(
                _FAKE_FPGA_PCI_ADDR, True, _FAKE_WATCHDOG_COUNTER_MSI_REG
            )
            mock_toggle_reboot.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR,
                True,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_BIT,
            )

    def test_disarm_disables_both_counters(self, watchdog_module):
        """disarm() should disable both watchdog counters and disable reboot."""
        with (
            patch.object(
                watchdog_module, "_toggle_watchdog_counter_enable", autospec=True
            ) as mock_toggle_counter,
            patch.object(
                watchdog_module, "_toggle_watchdog_reboot", autospec=True
            ) as mock_toggle_reboot,
        ):
            assert self.watchdog.disarm() is True

            mock_toggle_counter.assert_any_call(
                _FAKE_FPGA_PCI_ADDR, False, _FAKE_WATCHDOG_COUNTER_MSI_REG
            )
            mock_toggle_counter.assert_any_call(
                _FAKE_FPGA_PCI_ADDR, False, _FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG
            )
            mock_toggle_reboot.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR,
                False,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_BIT,
            )

    def test_is_armed_checks_msi_counter(self, watchdog_module):
        """is_armed() should query the MSI counter."""
        with patch.object(
            watchdog_module,
            "_read_watchdog_counter_enable",
            autospec=True,
            return_value=1,
        ) as mock_read_enable:
            assert self.watchdog.is_armed() is True
            mock_read_enable.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR, _FAKE_WATCHDOG_COUNTER_MSI_REG
            )

    def test_get_remaining_time_from_msi_counter(self, watchdog_module):
        """get_remaining_time() should read from the MSI counter."""
        with (
            patch.object(
                watchdog_module,
                "_read_watchdog_counter_enable",
                autospec=True,
                return_value=1,
            ),
            patch.object(
                watchdog_module,
                "_read_watchdog_countdown_value_milliseconds",
                autospec=True,
                return_value=5_000,
            ) as mock_read_countdown,
        ):
            result = self.watchdog.get_remaining_time()

            assert result == 5
            mock_read_countdown.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR, _FAKE_WATCHDOG_COUNTER_MSI_REG
            )
