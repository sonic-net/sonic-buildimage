import datetime
import pathlib
import pytest
import os
import tempfile

from unittest.mock import patch, ANY, create_autospec

_FAKE_FPGA_PCI_ADDR = "FAKE_FPGA_PCI_ADDR"
_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET = 0x28
_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG = 0x1E0
_FAKE_WATCHDOG_COUNTER_MSI_REG = 0x1D8


@pytest.fixture
def watchdog_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from sonic_platform import watchdog

    yield watchdog


class TestWatchdogHelpers:
    """Tests for module-level helpers (file pause + register access)."""

    @pytest.fixture(scope="function", autouse=True)
    def setup(self, watchdog_module):
        self.watchdog_module = watchdog_module

    def test_pause_watchdog_punching(self):
        expected_timestamp = 105
        with (
            tempfile.NamedTemporaryFile() as test_pause_file,
            patch.object(
                self.watchdog_module,
                "_WATCHDOG_PAUSE_FILE_PATH",
                test_pause_file.name,
            ),
            patch("time.time", return_value=100),
        ):
            self.watchdog_module._pause_watchdog_punching(datetime.timedelta(seconds=5))

            actual_timestamp = int(test_pause_file.read())
            assert actual_timestamp == expected_timestamp

    @pytest.mark.parametrize(
        "contents,now,expected",
        [
            ("200", 100, True),    # deadline ahead
            ("200", 200, False),   # deadline reached
            ("200", 300, False),   # deadline passed
            ("", 100, False),      # malformed -> arm rather than stay paused
            ("not-a-ts", 100, False),
        ],
    )
    def test_punching_paused_honours_the_deadline(self, contents, now, expected):
        with tempfile.TemporaryDirectory() as tmpdir:
            pause_file = pathlib.Path(tmpdir) / "watchdog.pause"
            pause_file.write_text(contents)
            with (
                patch.object(
                    self.watchdog_module, "_WATCHDOG_PAUSE_FILE_PATH", pause_file
                ),
                patch("time.monotonic", return_value=now),
            ):
                assert self.watchdog_module._punching_paused() is expected

    def test_punching_paused_is_false_without_a_pause_file(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            pause_file = pathlib.Path(tmpdir) / "watchdog.pause"
            with patch.object(
                self.watchdog_module, "_WATCHDOG_PAUSE_FILE_PATH", pause_file
            ):
                assert self.watchdog_module._punching_paused() is False

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
            )
            mock_overwrite_field.assert_called_once_with(
                reg_val=ANY, bit_range=(4, 4), field_val=expected_field_val
            )
            mock_write_32.assert_called_once_with(
                pci_address=_FAKE_FPGA_PCI_ADDR,
                offset=_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
                val=mock_overwrite_field.return_value,
            )


@pytest.fixture
def mock_pause_watchdog_punching(watchdog_module):
    """Mock for _pause_watchdog_punching."""
    return create_autospec(watchdog_module._pause_watchdog_punching)


@pytest.fixture
def mock_unpause_watchdog_punching(watchdog_module):
    """Mock for _unpause_watchdog_punching."""
    return create_autospec(watchdog_module._unpause_watchdog_punching)


class TestWatchdogSimple:
    """Tests for the single-counter Watchdog class."""

    @pytest.fixture(scope="function", autouse=True)
    def setup(
        self,
        watchdog_module,
        mock_pause_watchdog_punching,
        mock_unpause_watchdog_punching,
    ):
        watchdog_module._pause_watchdog_punching = mock_pause_watchdog_punching
        watchdog_module._unpause_watchdog_punching = mock_unpause_watchdog_punching
        with tempfile.NamedTemporaryFile() as test_pause_file:
            watchdog_module._WATCHDOG_PAUSE_FILE_PATH = test_pause_file.name

        self.watchdog = watchdog_module.WatchdogSimple(
            fpga_pci_addr=_FAKE_FPGA_PCI_ADDR,
            event_driven_power_cycle_control_reg_offset=_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
            watchdog_counter_powercycle_reg=_FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG,
        )

    def test_arm_from_daemon_skips_while_punching_is_paused(self, watchdog_module):
        with (
            patch.object(watchdog_module, "_punching_paused", return_value=True),
            patch.object(self.watchdog, "_do_real_arm") as mock_do_real_arm,
        ):
            assert self.watchdog.arm_from_daemon() == 0
        mock_do_real_arm.assert_not_called()

    def test_arm_from_daemon_arms_once_the_pause_has_expired(self, watchdog_module):
        with (
            patch.object(watchdog_module, "_punching_paused", return_value=False),
            patch.object(self.watchdog, "_do_real_arm") as mock_do_real_arm,
        ):
            self.watchdog.arm_from_daemon()
        mock_do_real_arm.assert_called_once_with(
            watchdog_module._WATCHDOG_PUNCH_DAEMON_ARM_SECONDS
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
            )
            mock_toggle_counter.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR, True, _FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG
            )

    def test_arm_should_pause_punching(
        self, watchdog_module, mock_pause_watchdog_punching
    ):
        with (
            patch.object(
                watchdog_module, "_update_watchdog_countdown_value", autospec=True
            ),
            patch.object(
                watchdog_module, "_toggle_watchdog_counter_enable", autospec=True
            ),
            patch.object(watchdog_module, "_toggle_watchdog_reboot", autospec=True),
        ):
            self.watchdog.arm(1)

            mock_pause_watchdog_punching.assert_called_once_with(
                datetime.timedelta(seconds=1)
            )

    def test_arm_should_unpause_punching_on_error(
        self, mock_unpause_watchdog_punching
    ):
        mock_do_real_arm = create_autospec(self.watchdog._do_real_arm)
        mock_do_real_arm.return_value = -1
        self.watchdog._do_real_arm = mock_do_real_arm

        self.watchdog.arm(1)

        mock_unpause_watchdog_punching.assert_called_once()

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
            )
            mock_toggle_counter.assert_called_once_with(
                _FAKE_FPGA_PCI_ADDR, False, _FAKE_WATCHDOG_COUNTER_POWERCYCLE_REG
            )

    def test_disarm_unpause_punching(
        self, watchdog_module, mock_unpause_watchdog_punching
    ):
        with (
            patch.object(
                watchdog_module, "_toggle_watchdog_counter_enable", autospec=True
            ),
            patch.object(watchdog_module, "_toggle_watchdog_reboot", autospec=True),
        ):
            actual_return_value = self.watchdog.disarm()

            assert actual_return_value
            mock_unpause_watchdog_punching.assert_called_once()

    def test_disarm_fails_do_not_resume_punching(
        self, watchdog_module, mock_unpause_watchdog_punching
    ):
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
            actual_return_value = self.watchdog.disarm()

            assert not actual_return_value
            mock_unpause_watchdog_punching.assert_not_called()

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
    def setup(
        self,
        watchdog_module,
        mock_pause_watchdog_punching,
        mock_unpause_watchdog_punching,
    ):
        watchdog_module._pause_watchdog_punching = mock_pause_watchdog_punching
        watchdog_module._unpause_watchdog_punching = mock_unpause_watchdog_punching
        with tempfile.NamedTemporaryFile() as test_pause_file:
            watchdog_module._WATCHDOG_PAUSE_FILE_PATH = test_pause_file.name

        self.watchdog = watchdog_module.Watchdog(
            fpga_pci_addr=_FAKE_FPGA_PCI_ADDR,
            event_driven_power_cycle_control_reg_offset=_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
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
