import pytest
from nexthop import fpga_lib
from unittest.mock import patch, ANY, Mock

# Should match the watchdog mock in fixtures_unit_test.py
_FAKE_EVENT_DRIVEN_POWER_CYCLE_CONTROL_REG_OFFSET = 0x28
_FAKE_WATCHDOG_COUNTER_REG_OFFSET = 0x1E0


class TestWatchdog:
    @pytest.fixture(autouse=True)
    def setup(self, watchdog):
        self.watchdog = watchdog

    @pytest.fixture
    def mock_toggle_watchdog_reboot(self):
        """Mock for _toggle_watchdog_reboot."""
        return Mock()

    @pytest.fixture
    def mock_read_watchdog_counter_enable(self):
        """Mock for _read_watchdog_counter_enable."""
        return Mock()

    @pytest.fixture
    def mock_toggle_watchdog_counter_enable(self):
        """Mock for _toggle_watchdog_counter_enable."""
        return Mock()

    @pytest.fixture
    def mock_update_watchdog_countdown_value(self):
        """Mock for _update_watchdog_counter_value."""
        return Mock()

    @pytest.fixture
    def mock_read_watchdog_countdown_value_milliseconds(self):
        """Mock for _read_watchdog_countdown_value_milliseconds."""
        return Mock()

    @pytest.fixture
    def mock_read_watchdog_counter_register(self):
        """Mock for _read_watchdog_counter_register."""
        return Mock()

    @patch.object(fpga_lib, "read_32", autospec=True)
    def test_read_watchdog_counter_register(self, mock_read_32):
        self.watchdog._read_watchdog_counter_register()

        mock_read_32.assert_called_once_with(
            self.watchdog.fpga_pci_addr, _FAKE_WATCHDOG_COUNTER_REG_OFFSET
        )

    def test_read_watchdog_counter_enable(self, mock_read_watchdog_counter_register):
        self.watchdog._read_watchdog_counter_register = (
            mock_read_watchdog_counter_register
        )
        with patch.object(fpga_lib, "get_field", autospec=True) as mock_get_field:
            mock_get_field.return_value = 1
            assert self.watchdog._read_watchdog_counter_enable()

    @patch.object(fpga_lib, "read_32", autospec=True)
    @patch.object(fpga_lib, "write_32", autospec=True)
    @patch.object(fpga_lib, "overwrite_field", autospec=True)
    def test_update_watchdog_countdown_value(
        self, mock_overwrite_field, mock_write_32, mock_read_32
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
    def test_toggle_watchdog_counter_enable(self, is_enable, expected_field_val):
        with (
            patch.object(fpga_lib, "read_32", autospec=True),
            patch.object(fpga_lib, "write_32", autospec=True) as mock_write_32,
            patch.object(
                fpga_lib, "overwrite_field", autospec=True
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
    def test_toggle_watchdog_reboot(self, is_enable, expected_field_val):
        with (
            patch.object(fpga_lib, "read_32", autospec=True),
            patch.object(fpga_lib, "write_32", autospec=True) as mock_write_32,
            patch.object(
                fpga_lib, "overwrite_field", autospec=True
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

    def test_arm_should_enable_counter_and_reboot(
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
        mock_toggle_watchdog_reboot.assert_called_once()
        mock_toggle_watchdog_counter_enable.assert_called_once()

    def test_disarm(
        self, mock_toggle_watchdog_reboot, mock_toggle_watchdog_counter_enable
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
        mock_toggle_watchdog_reboot.assert_called_once()
        mock_toggle_watchdog_counter_enable.assert_called_once()

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
