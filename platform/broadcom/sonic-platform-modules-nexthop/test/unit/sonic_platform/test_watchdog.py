import datetime
import pytest
import os
import tempfile

from unittest.mock import patch, ANY, create_autospec

_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET = 0x28
_FAKE_WATCHDOG_COUNTER_REG_OFFSET = 0x1E0


@pytest.fixture
def watchdog_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from sonic_platform import watchdog

    yield watchdog


class TestWatchdogHelpers:

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


@pytest.fixture
def mock_pause_watchdog_punching(watchdog_module):
    """Mock for _pause_watchdog_punching."""
    return create_autospec(watchdog_module._pause_watchdog_punching)


@pytest.fixture
def mock_unpause_watchdog_punching(watchdog_module):
    """Mock for _unpause_watchdog_punching."""
    return create_autospec(watchdog_module._unpause_watchdog_punching)


class TestWatchdogAPI:
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
            fpga_pci_addr="FAKE_FPGA_PCI_ADDR",
            event_driven_power_cycle_control_reg_offset=_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
            watchdog_counter_reg_offset=_FAKE_WATCHDOG_COUNTER_REG_OFFSET,
        )

    @pytest.fixture
    def mock_toggle_watchdog_reboot(self):
        """Mock for _toggle_watchdog_reboot."""
        return create_autospec(self.watchdog._toggle_watchdog_reboot)

    @pytest.fixture
    def mock_read_watchdog_counter_enable(self):
        """Mock for _read_watchdog_counter_enable."""
        return create_autospec(self.watchdog._read_watchdog_counter_enable)

    @pytest.fixture
    def mock_toggle_watchdog_counter_enable(self):
        """Mock for _toggle_watchdog_counter_enable."""
        return create_autospec(self.watchdog._toggle_watchdog_counter_enable)

    @pytest.fixture
    def mock_update_watchdog_countdown_value(self):
        """Mock for _update_watchdog_counter_value."""
        return create_autospec(self.watchdog._update_watchdog_countdown_value)

    @pytest.fixture
    def mock_read_watchdog_countdown_value_milliseconds(self):
        """Mock for _read_watchdog_countdown_value_milliseconds."""
        return create_autospec(
            self.watchdog._read_watchdog_countdown_value_milliseconds
        )

    @pytest.fixture
    def mock_read_watchdog_counter_register(self):
        """Mock for _read_watchdog_counter_register."""
        return create_autospec(self.watchdog._read_watchdog_counter_register)

    @pytest.fixture
    def mock_do_real_arm(self):
        """Mock for _do_real_arm."""
        return create_autospec(self.watchdog._do_real_arm)

    def test_read_watchdog_counter_register(self, watchdog_module):
        # Set up
        with patch.object(watchdog_module.fpga_lib, "read_32", autospec=True) as mock_read_32:
            # Act
            self.watchdog._read_watchdog_counter_register()
            # Assert
            mock_read_32.assert_called_once_with(
                self.watchdog.fpga_pci_addr, _FAKE_WATCHDOG_COUNTER_REG_OFFSET
            )

    def test_read_watchdog_counter_enable(self, watchdog_module, mock_read_watchdog_counter_register):
        self.watchdog._read_watchdog_counter_register = (
            mock_read_watchdog_counter_register
        )
        with patch.object(watchdog_module.fpga_lib, "get_field", autospec=True) as mock_get_field:
            mock_get_field.return_value = 1
            assert self.watchdog._read_watchdog_counter_enable()


    def test_update_watchdog_countdown_value(self, watchdog_module):
        # Set up
        with (
            patch.object(watchdog_module.fpga_lib, "read_32", autospec=True) as mock_read_32,
            patch.object(watchdog_module.fpga_lib, "write_32", autospec=True) as mock_write_32,
            patch.object(
                watchdog_module.fpga_lib, "overwrite_field", autospec=True
            ) as mock_overwrite_field,
        ):
            # Act
            self.watchdog._update_watchdog_countdown_value(10)

            # Assert
            mock_overwrite_field.assert_called_once_with(
                mock_read_32.return_value, (0, 23), 10
            )
            mock_write_32.assert_called_once_with(
                self.watchdog.fpga_pci_addr,
                _FAKE_WATCHDOG_COUNTER_REG_OFFSET,
                mock_overwrite_field.return_value,
            )

    @pytest.mark.parametrize("is_enable,expected_field_val", [(True, 1), (False, 0)])
    def test_toggle_watchdog_counter_enable(self, watchdog_module, is_enable, expected_field_val):
        with (
            patch.object(watchdog_module.fpga_lib, "read_32", autospec=True),
            patch.object(watchdog_module.fpga_lib, "write_32", autospec=True) as mock_write_32,
            patch.object(
                watchdog_module.fpga_lib, "overwrite_field", autospec=True
            ) as mock_overwrite_field,
        ):
            self.watchdog._toggle_watchdog_counter_enable(is_enable)
            mock_overwrite_field.assert_called_once_with(
                ANY, (31, 31), expected_field_val
            )
            mock_write_32.assert_called_once_with(
                self.watchdog.fpga_pci_addr,
                _FAKE_WATCHDOG_COUNTER_REG_OFFSET,
                mock_overwrite_field.return_value,
            )

    @pytest.mark.parametrize("is_enable,expected_field_val", [(True, 1), (False, 0)])
    def test_toggle_watchdog_reboot(self, watchdog_module, is_enable, expected_field_val):
        with (
            patch.object(watchdog_module.fpga_lib, "read_32", autospec=True),
            patch.object(watchdog_module.fpga_lib, "write_32", autospec=True) as mock_write_32,
            patch.object(
                watchdog_module.fpga_lib, "overwrite_field", autospec=True
            ) as mock_overwrite_field,
        ):
            self.watchdog._toggle_watchdog_reboot(is_enable)
            mock_overwrite_field.assert_called_once_with(
                ANY, (4, 4), expected_field_val
            )
            mock_write_32.assert_called_once_with(
                self.watchdog.fpga_pci_addr,
                _FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET,
                mock_overwrite_field.return_value,
            )

    @pytest.mark.parametrize("seconds", [-1, 0, 0x1000000 / 1_000])
    def test_arm_seconds_out_of_bound_error(self, seconds):
        actual_return_value = self.watchdog.arm(seconds)

        assert actual_return_value == -1

    def test_arm_should_update_counter(
        self,
        mock_update_watchdog_countdown_value,
        mock_toggle_watchdog_reboot,
        mock_toggle_watchdog_counter_enable,
    ):
        # Set up
        timeout_seconds = 10
        expected_return_value = 10
        self.watchdog._update_watchdog_countdown_value = (
            mock_update_watchdog_countdown_value
        )
        self.watchdog._toggle_watchdog_reboot = mock_toggle_watchdog_reboot
        self.watchdog._toggle_watchdog_counter_enable = (
            mock_toggle_watchdog_counter_enable
        )

        # Act
        actual_return_value = self.watchdog.arm(timeout_seconds)

        # Assert
        assert (
            actual_return_value == expected_return_value
        ), f"Expected {expected_return_value}, got {actual_return_value}"
        mock_update_watchdog_countdown_value.assert_called_once_with(
            milliseconds=timeout_seconds * 1_000
        )

    def test_arm_should_enable_counter_reboot(
        self,
        mock_update_watchdog_countdown_value,
        mock_toggle_watchdog_reboot,
        mock_toggle_watchdog_counter_enable,
    ):
        # Set up
        timeout = 10
        self.watchdog._update_watchdog_countdown_value = (
            mock_update_watchdog_countdown_value
        )
        self.watchdog._toggle_watchdog_reboot = mock_toggle_watchdog_reboot
        self.watchdog._toggle_watchdog_counter_enable = (
            mock_toggle_watchdog_counter_enable
        )

        # Act
        self.watchdog.arm(timeout)

        # Assert
        mock_toggle_watchdog_reboot.assert_called_once_with(True)
        mock_toggle_watchdog_counter_enable.assert_called_once_with(True)

    def test_arm_should_pause_punching(
        self,
        mock_update_watchdog_countdown_value,
        mock_toggle_watchdog_reboot,
        mock_toggle_watchdog_counter_enable,
        mock_pause_watchdog_punching,
    ):
        # Set up
        timeout = 1
        self.watchdog._update_watchdog_countdown_value = (
            mock_update_watchdog_countdown_value
        )
        self.watchdog._toggle_watchdog_reboot = mock_toggle_watchdog_reboot
        self.watchdog._toggle_watchdog_counter_enable = (
            mock_toggle_watchdog_counter_enable
        )

        # Act
        self.watchdog.arm(timeout)

        # Assert
        mock_pause_watchdog_punching.assert_called_once_with(
            datetime.timedelta(seconds=1)
        )

    def test_arm_should_unpause_punching_on_error(
        self,
        mock_do_real_arm,
        mock_unpause_watchdog_punching,
    ):
        # Set up
        mock_do_real_arm.return_value = -1
        self.watchdog._do_real_arm = mock_do_real_arm

        # Act
        self.watchdog.arm(1)

        # Assert
        mock_unpause_watchdog_punching.assert_called_once()

    def test_disarm_stops_counter(
        self,
        mock_toggle_watchdog_reboot,
        mock_toggle_watchdog_counter_enable,
    ):
        # Set up
        self.watchdog._toggle_watchdog_reboot = mock_toggle_watchdog_reboot
        self.watchdog._toggle_watchdog_counter_enable = (
            mock_toggle_watchdog_counter_enable
        )

        # Act
        actual_return_value = self.watchdog.disarm()

        # Assert
        assert actual_return_value
        mock_toggle_watchdog_reboot.assert_called_once_with(False)
        mock_toggle_watchdog_counter_enable.assert_called_once_with(False)

    def test_disarm_unpause_punching(
        self,
        mock_toggle_watchdog_reboot,
        mock_toggle_watchdog_counter_enable,
        mock_unpause_watchdog_punching,
    ):
        # Set up
        self.watchdog._toggle_watchdog_reboot = mock_toggle_watchdog_reboot
        self.watchdog._toggle_watchdog_counter_enable = (
            mock_toggle_watchdog_counter_enable
        )

        # Act
        actual_return_value = self.watchdog.disarm()

        # Assert
        assert actual_return_value
        mock_unpause_watchdog_punching.assert_called_once()

    def test_disarm_fails_do_not_resume_punching(
        self,
        mock_toggle_watchdog_reboot,
        mock_toggle_watchdog_counter_enable,
        mock_unpause_watchdog_punching,
    ):
        # Set up
        mock_toggle_watchdog_reboot.side_effect = Exception()
        self.watchdog._toggle_watchdog_reboot = mock_toggle_watchdog_reboot
        self.watchdog._toggle_watchdog_counter_enable = (
            mock_toggle_watchdog_counter_enable
        )

        # Act
        actual_return_value = self.watchdog.disarm()

        # Assert
        assert not actual_return_value
        mock_unpause_watchdog_punching.assert_not_called()

    def test_get_remaining_time_when_not_armed(self, mock_read_watchdog_counter_enable):
        # Set up
        mock_read_watchdog_counter_enable.return_value = 0
        self.watchdog._read_watchdog_counter_enable = mock_read_watchdog_counter_enable

        # Act
        actual_return_value = self.watchdog.get_remaining_time()

        # Assert
        assert actual_return_value == -1

    def test_get_remaining_time_when_armed(
        self,
        mock_read_watchdog_countdown_value_milliseconds,
        mock_read_watchdog_counter_enable,
    ):
        # Set up
        mock_read_watchdog_counter_enable.return_value = 1
        mock_read_watchdog_countdown_value_milliseconds.return_value = 2_200
        self.watchdog._read_watchdog_counter_enable = mock_read_watchdog_counter_enable
        self.watchdog._read_watchdog_countdown_value_milliseconds = (
            mock_read_watchdog_countdown_value_milliseconds
        )

        # Act
        actual_return_value = self.watchdog.get_remaining_time()

        # Assert
        assert actual_return_value == 2
