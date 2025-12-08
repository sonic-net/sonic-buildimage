#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the thermal_actions.py module.
These tests run in isolation from the SONiC environment using pytest:
python -m pytest test/unit/sonic_platform/test_thermal.py -v
"""

import importlib.util
import os
import sys
import types
from unittest.mock import Mock, call, patch

import pytest


@pytest.fixture(scope="session", autouse=True)
def setup_external_mocks():
    """Set up mocks for external SONiC dependencies only."""

    # Mock the thermal_json_object decorator
    def mock_thermal_json_object(name):
        def decorator(cls):
            return cls
        return decorator

    # Mock ThermalPolicyActionBase
    class MockThermalPolicyActionBase:
        def __init__(self):
            pass

    # Mock SysLogger
    class MockSysLogger:
        def __init__(self, *args, **kwargs):
            # Methods as mocks so tests can assert calls
            self.log_info = Mock()
            self.log_error = Mock()
            self.log_warning = Mock()
            self.log_debug = Mock()
            self.log = Mock()

    # Create mock modules for external SONiC dependencies
    mock_thermal_action_base = Mock()
    mock_thermal_action_base.ThermalPolicyActionBase = MockThermalPolicyActionBase

    mock_thermal_json_object_module = Mock()
    mock_thermal_json_object_module.thermal_json_object = mock_thermal_json_object

    mock_syslogger = Mock()
    mock_syslogger.SysLogger = MockSysLogger

    # Create mock modules for the local dependencies
    mock_thermal_infos = Mock()
    mock_thermal_infos.FanDrawerInfo = type('FanDrawerInfo', (), {'INFO_TYPE': 'fan_drawer_info'})
    mock_thermal_infos.ThermalInfo = type('ThermalInfo', (), {'INFO_TYPE': 'thermal_info'})

    mock_syslog = Mock()
    mock_syslog.SYSLOG_IDENTIFIER_THERMAL = "nh_thermal"
    mock_syslog.NhLoggerMixin = MockSysLogger
    # Build mock for swsscommon package and submodule
    mock_swsscommon_pkg = types.ModuleType('swsscommon')
    mock_swsscommon_sub = types.ModuleType('swsscommon.swsscommon')

    class MockSonicV2Connector:
        CONFIG_DB = 4
        STATE_DB = 6
        RETURN_GET_ALL = {}
        def __init__(self, *args, **kwargs):
            pass
        def connect(self, db):
            pass
        def get_redis_client(self, db):
            return object()
        def get_all(self, db, key):
            return self.RETURN_GET_ALL
        def close(self, db):
            pass

    class MockTable:
        MOCK_PORT_KEYS = []
        MOCK_PORT_DATA = {}
        def __init__(self, db_connector, table_name):
            self.table_name = table_name
        def getKeys(self):
            return list(self.MOCK_PORT_KEYS)
        def get(self, intf_name):
            data = self.MOCK_PORT_DATA.get(intf_name)
            return (True, data) if data is not None else (False, [])

    setattr(mock_swsscommon_sub, 'SonicV2Connector', MockSonicV2Connector)
    setattr(mock_swsscommon_sub, 'Table', MockTable)
    setattr(mock_swsscommon_sub, 'CFG_PORT_TABLE_NAME', 'PORT')
    setattr(mock_swsscommon_pkg, 'swsscommon', mock_swsscommon_sub)

    # Build other external dependency mocks
    mock_fpga_lib = types.SimpleNamespace()
    def _mock_read_32(addr, reg):
        raise PermissionError("not root")
    setattr(mock_fpga_lib, 'read_32', _mock_read_32)

    mock_thermal_base_module = types.ModuleType('sonic_platform_base.thermal_base')
    class _ThermalBase:
        def __init__(self):
            pass
    setattr(mock_thermal_base_module, 'ThermalBase', _ThermalBase)

    mock_pddf_thermal_module = types.ModuleType('sonic_platform_pddf_base.pddf_thermal')
    class _PddfThermal:
        def __init__(self, *args, **kwargs):
            pass
        def get_temperature(self):
            return None
    setattr(mock_pddf_thermal_module, 'PddfThermal', _PddfThermal)


    # Mock all dependencies that aren't available in test environment
    with patch.dict('sys.modules', {
        # External SONiC dependencies for thermal_actions
        'sonic_platform_base.sonic_thermal_control.thermal_action_base': mock_thermal_action_base,
        'sonic_platform_base.sonic_thermal_control.thermal_json_object': mock_thermal_json_object_module,
        'sonic_platform_base.sonic_thermal_control.thermal_info_base': Mock(),
        'sonic_platform_base.fan_base': Mock(),
        'sonic_py_common.syslogger': mock_syslogger,
        # External SONiC dependencies for thermal.py
        'swsscommon': mock_swsscommon_pkg,
        'swsscommon.swsscommon': mock_swsscommon_sub,
        'nexthop.fpga_lib': mock_fpga_lib,
        'sonic_platform_base.thermal_base': mock_thermal_base_module,
        'sonic_platform_pddf_base.pddf_thermal': mock_pddf_thermal_module,
        # Local dependencies
        'sonic_platform.thermal_infos': mock_thermal_infos,
        'sonic_platform.syslog': mock_syslog,
    }):
        yield


@pytest.fixture(scope="session")
def thermal_actions_module():
    """Import the actual thermal_actions module using normal Python imports."""
    test_dir = os.path.dirname(os.path.realpath(__file__))
    thermal_actions_path = os.path.join(test_dir, "../../../common/sonic_platform/thermal_actions.py")

    spec = importlib.util.spec_from_file_location("thermal_actions", thermal_actions_path)
    thermal_actions = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(thermal_actions)

    return thermal_actions

@pytest.fixture(scope="session")
def thermal_module():
    """Import the actual thermal module (thermal.py) using normal Python imports."""
    test_dir = os.path.dirname(os.path.realpath(__file__))
    thermal_path = os.path.join(test_dir, "../../../common/sonic_platform/thermal.py")

    spec = importlib.util.spec_from_file_location("thermal", thermal_path)
    thermal = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(thermal)

    return thermal



class TestPIDController:
    """Test class for PIDController functionality."""

    @pytest.fixture
    def pid_controller(self, thermal_actions_module):
        """Fixture providing a PIDController instance for testing."""
        return thermal_actions_module.PIDController(
            domain="test_domain",
            interval=5,
            proportional_gain=2.0,
            integral_gain=0.5,
            derivative_gain=0.1,
            output_min=30.0,
            output_max=100.0
        )

    def _calculate_expected_output(self, kp, ki, kd, proportional, integral, derivative, output_min, output_max):
        """
        Calculate expected PID output with saturation.

        Args:
            kp, ki, kd: PID gains
            proportional, integral, derivative: PID terms
            output_min, output_max: Output limits

        Returns:
            Expected output value after saturation
        """
        raw_output = kp * proportional + ki * integral + kd * derivative
        return max(output_min, min(output_max, raw_output))

    def _calculate_initial_integral(self, output_min, output_max, ki):
        """Calculate the pre-seeded integral value."""
        return (output_min + output_max) / 2 / ki

    def _check_anti_windup_condition(self, output, error, output_min, output_max):
        """
        Check if integral should be updated based on anti-windup logic.

        Returns:
            True if integral should be updated, False if frozen
        """
        return (output <= output_max or error < 0) and (output >= output_min or error > 0)

    def test_pid_controller_initialization(self, pid_controller):
        """Test PID controller initialization with various parameters."""
        assert pid_controller._domain == "test_domain"
        assert pid_controller._interval == 5
        assert pid_controller._kp == 2.0
        assert pid_controller._ki == 0.5
        assert pid_controller._kd == 0.1
        assert pid_controller._output_min == 30.0
        assert pid_controller._output_max == 100.0
        assert pid_controller._first_run is True
        assert pid_controller._prev_error == 0.0
        # Integral should be pre-seeded to midpoint
        expected_integral = (30.0 + 100.0) / 2 / 0.5  # (min + max) / 2 / Ki
        assert pid_controller._integral == expected_integral

    @pytest.mark.parametrize("kp,ki,kd,output_min,output_max", [
        (2.0, 1.0, 0.5, 20.0, 90.0),
        (0.5, 0.1, 0.05, 40.0, 100.0),
        (1.5, 2.0, 1.0, 30.0, 80.0),
    ])
    def test_pid_controller_custom_parameters(self, thermal_actions_module, kp, ki, kd, output_min, output_max):
        """Test PID controller initialization with various parameter combinations."""
        controller = thermal_actions_module.PIDController(
            domain="custom_domain",
            interval=10,
            proportional_gain=kp,
            integral_gain=ki,
            derivative_gain=kd,
            output_min=output_min,
            output_max=output_max
        )

        assert controller._kp == kp
        assert controller._ki == ki
        assert controller._kd == kd
        assert controller._output_min == output_min
        assert controller._output_max == output_max

    def test_pid_controller_compute_first_run(self, pid_controller):
        """Test PID controller computation on first run with mathematical validation."""
        error = 5.0
        kp, ki, kd = 2.0, 0.5, 0.1
        interval = 5
        output_min, output_max = 30.0, 100.0

        # Calculate expected values using helper methods
        initial_integral = self._calculate_initial_integral(output_min, output_max, ki)
        proportional = error
        derivative = 0.0  # First run
        integral_calculation = initial_integral + error * interval

        expected_output = self._calculate_expected_output(
            kp, ki, kd, proportional, integral_calculation, derivative, output_min, output_max
        )

        output = pid_controller.compute(error)

        # Verify exact calculation - no tolerance for error
        assert output == expected_output, f"Expected {expected_output}, got {output}"

        # Verify state changes
        assert pid_controller._first_run is False
        assert pid_controller._prev_error == error

        # Check integral anti-windup logic
        should_update_integral = self._check_anti_windup_condition(
            expected_output, error, output_min, output_max
        )
        if should_update_integral:
            assert pid_controller._integral == integral_calculation, \
                f"Expected integral {integral_calculation}, got {pid_controller._integral}"
        else:
            assert pid_controller._integral == initial_integral, \
                f"Expected integral {initial_integral}, got {pid_controller._integral}"

    def test_pid_controller_compute_subsequent_runs(self, pid_controller):
        """Test PID controller computation on subsequent runs with mathematical validation."""
        # First run
        error1 = 5.0
        output1 = pid_controller.compute(error1)

        # Second run calculation
        error2 = 8.0
        kp, ki, kd = 2.0, 0.5, 0.1
        interval = 5
        output_min, output_max = 30.0, 100.0

        # State after first run
        prev_integral = pid_controller._integral

        proportional = error2
        derivative = (error2 - error1) / interval
        integral_calculation = prev_integral + error2 * interval

        expected_output = self._calculate_expected_output(
            kp, ki, kd, proportional, integral_calculation, derivative, output_min, output_max
        )

        output2 = pid_controller.compute(error2)

        # Verify exact calculation (accounting for saturation)
        assert output2 == expected_output, f"Expected {expected_output}, got {output2}"

        # Verify state is updated
        assert pid_controller._prev_error == error2
        assert pid_controller._first_run is False

    def test_pid_controller_output_saturation(self, pid_controller):
        """Test PID controller output saturation validation."""
        output_min, output_max = 30.0, 100.0

        # Test saturation to maximum
        large_error = 50.0

        # Run multiple times to build up integral
        for i in range(5):
            output = pid_controller.compute(large_error)
            if i == 4:  # Last iteration
                # Should be saturated to max
                assert output == output_max, f"Expected saturation to {output_max}, got {output}"

        # Reset controller for minimum saturation test
        pid_controller_min = pid_controller.__class__(
            domain="test_domain",
            interval=5,
            proportional_gain=2.0,
            integral_gain=0.5,
            derivative_gain=0.1,
            output_min=output_min,
            output_max=output_max
        )

        # Test saturation to minimum
        large_negative_error = -50.0

        # Run multiple times to reduce integral
        for i in range(5):
            output = pid_controller_min.compute(large_negative_error)
            if i == 4:  # Last iteration
                # Should be saturated to min
                assert output == output_min, f"Expected saturation to {output_min}, got {output}"

    def test_pid_controller_integral_anti_windup(self, pid_controller):
        """Test integral anti-windup mechanism validation."""
        output_max = 100.0
        large_error = 100.0

        # First run to establish baseline
        output1 = pid_controller.compute(large_error)
        integral_after_first = pid_controller._integral

        # Second run - should saturate and freeze integral
        output2 = pid_controller.compute(large_error)
        integral_after_second = pid_controller._integral

        # Both outputs should be saturated to max
        assert output1 == output_max, f"First output should be saturated to {output_max}, got {output1}"
        assert output2 == output_max, f"Second output should be saturated to {output_max}, got {output2}"

        # When saturated, integral should be frozen (anti-windup)
        # The anti-windup condition should prevent integral from growing
        # Since we have large positive error and saturated output, integral should be frozen
        assert integral_after_second == integral_after_first, \
            f"Integral should be frozen due to anti-windup: {integral_after_first} vs {integral_after_second}"

    def test_pid_controller_zero_error_steady_state(self, pid_controller):
        """Test PID controller behavior with zero error (steady state)."""
        error = 0.0
        kp, ki, kd = 2.0, 0.5, 0.1
        output_min, output_max = 30.0, 100.0

        # Calculate expected values using helper methods
        initial_integral = self._calculate_initial_integral(output_min, output_max, ki)
        proportional = error
        derivative = 0.0  # First run
        integral_after = initial_integral + error * 5  # No change with zero error

        expected_output = self._calculate_expected_output(
            kp, ki, kd, proportional, integral_after, derivative, output_min, output_max
        )

        output = pid_controller.compute(error)

        assert output == expected_output, f"Expected {expected_output}, got {output}"

    def test_pid_controller_oscillating_error(self, pid_controller):
        """Test PID controller with oscillating error pattern."""
        errors = [5.0, -3.0, 7.0, -2.0, 4.0]
        outputs = []

        for i, error in enumerate(errors):
            output = pid_controller.compute(error)
            outputs.append(output)

            # Verify output is always within bounds
            assert 30.0 <= output <= 100.0, f"Output {output} out of bounds at step {i}"

        # Verify derivative calculation for second run
        # derivative = (errors[1] - errors[0]) / interval = (-3.0 - 5.0) / 5 = -1.6
        # This should contribute to reducing the output
        assert outputs[1] < outputs[0], "Negative derivative should reduce output"

    def test_pid_controller_step_response(self, pid_controller):
        """Test PID controller step response with precise validation."""
        # Step input: sudden change from 0 to 10
        step_error = 10.0

        # First run with step
        output1 = pid_controller.compute(step_error)

        # Calculate expected first response
        kp, ki, kd = 2.0, 0.5, 0.1
        initial_integral = (30.0 + 100.0) / 2 / ki  # 130.0

        proportional = step_error  # 10.0
        derivative = 0.0  # First run
        integral = initial_integral + step_error * 5  # 130.0 + 50.0 = 180.0

        expected_output1 = kp * proportional + ki * integral + kd * derivative
        expected_output1 = 2.0 * 10.0 + 0.5 * 180.0 + 0.1 * 0.0  # 20.0 + 90.0 + 0.0 = 110.0

        # Should be saturated to max
        assert output1 == 100.0, f"Step response should saturate to 100.0, got {output1}"

        # Second run with same error (steady state)
        output2 = pid_controller.compute(step_error)

        # Should still be saturated
        assert output2 == 100.0, f"Continued step should remain saturated, got {output2}"

    def test_pid_controller_zero_gains(self, thermal_actions_module):
        """Test PID controller with zero gains (except integral to avoid division by zero)."""
        # Create controller with zero proportional and derivative gains, small integral gain
        controller = thermal_actions_module.PIDController(
            domain="zero_test",
            interval=5,
            proportional_gain=0.0,
            integral_gain=0.001,  # Small but non-zero to avoid division by zero
            derivative_gain=0.0,
            output_min=30.0,
            output_max=100.0
        )

        error = 10.0
        output = controller.compute(error)

        # With zero Kp and Kd, output should be driven only by integral term
        # Initial integral = (30 + 100) / 2 / 0.001 = 65000
        # After first run: integral = 65000 + 10 * 5 = 65050 (integral is updated)
        # Output = 0 * 10 + 0.001 * 65050 + 0 * 0 = 65.05
        expected_output = 0.001 * (65000 + 10 * 5)  # 65.05
        assert output == expected_output, f"Zero Kp/Kd should result in integral-driven output {expected_output}, got {output}"

    def test_pid_controller_very_small_interval(self, thermal_actions_module):
        """Test PID controller with very small interval."""
        controller = thermal_actions_module.PIDController(
            domain="small_interval_test",
            interval=0.1,  # Very small interval
            proportional_gain=1.0,
            integral_gain=0.1,
            derivative_gain=0.01,
            output_min=30.0,
            output_max=100.0
        )

        # First run
        error1 = 5.0
        output1 = controller.compute(error1)

        # Second run - derivative should be large due to small interval
        error2 = 6.0
        output2 = controller.compute(error2)

        # Derivative = (6.0 - 5.0) / 0.1 = 10.0
        # This should significantly affect the output
        derivative_contribution = 0.01 * 10.0  # 0.1

        # Verify both outputs are within bounds
        assert 30.0 <= output1 <= 100.0
        assert 30.0 <= output2 <= 100.0

    def test_pid_controller_large_interval(self, thermal_actions_module):
        """Test PID controller with large interval."""
        controller = thermal_actions_module.PIDController(
            domain="large_interval_test",
            interval=100,  # Very large interval
            proportional_gain=1.0,
            integral_gain=0.1,
            derivative_gain=1.0,
            output_min=30.0,
            output_max=100.0
        )

        # First run
        error1 = 5.0
        output1 = controller.compute(error1)

        # Second run - derivative should be small due to large interval
        error2 = 10.0
        output2 = controller.compute(error2)

        # Derivative = (10.0 - 5.0) / 100 = 0.05
        derivative_contribution = 1.0 * 0.05  # 0.05

        # Verify both outputs are within bounds
        assert 30.0 <= output1 <= 100.0
        assert 30.0 <= output2 <= 100.0

    def test_pid_controller_extreme_output_limits(self, thermal_actions_module):
        """Test PID controller with extreme output limits."""
        # Very narrow range
        controller_narrow = thermal_actions_module.PIDController(
            domain="narrow_test",
            interval=5,
            proportional_gain=1.0,
            integral_gain=0.1,
            derivative_gain=0.01,
            output_min=49.9,
            output_max=50.1
        )

        error = 1.0
        output = controller_narrow.compute(error)

        # Should be within the narrow range
        assert 49.9 <= output <= 50.1, f"Output {output} outside narrow range [49.9, 50.1]"

        # Very wide range
        controller_wide = thermal_actions_module.PIDController(
            domain="wide_test",
            interval=5,
            proportional_gain=1.0,
            integral_gain=0.1,
            derivative_gain=0.01,
            output_min=0.0,
            output_max=1000.0
        )

        error = 1.0
        output_wide = controller_wide.compute(error)

        # Should be within the wide range
        assert 0.0 <= output_wide <= 1000.0, f"Output {output_wide} outside wide range [0.0, 1000.0]"

    def test_pid_controller_floating_point_arithmetic(self, thermal_actions_module):
        """Test PID controller with floating point arithmetic."""
        kp, ki, kd = 1.234567, 0.987654, 0.123456
        output_min, output_max = 25.5, 95.7
        interval = 3

        controller = thermal_actions_module.PIDController(
            domain="precision_test",
            interval=interval,
            proportional_gain=kp,
            integral_gain=ki,
            derivative_gain=kd,
            output_min=output_min,
            output_max=output_max
        )

        # Use precise floating point values
        error1 = 3.141592653589793

        # Calculate expected values using helper methods
        initial_integral = self._calculate_initial_integral(output_min, output_max, ki)
        proportional = error1
        derivative = 0.0  # First run
        integral = initial_integral + error1 * interval

        expected_output = self._calculate_expected_output(
            kp, ki, kd, proportional, integral, derivative, output_min, output_max
        )

        output1 = controller.compute(error1)

        # Verify exact calculation - no tolerance for error
        assert output1 == expected_output, f"Expected {expected_output}, got {output1}"

        # Second run with different precise value
        error2 = 2.718281828459045
        output2 = controller.compute(error2)

        # Verify output is within bounds
        assert output_min <= output2 <= output_max, f"Output {output2} outside bounds [{output_min}, {output_max}]"


class TestFanSetSpeedAction:
    """Test class for FanSetSpeedAction functionality."""

    @pytest.fixture
    def fan_set_speed_action(self, thermal_actions_module):
        """Fixture providing a FanSetSpeedAction instance."""
        return thermal_actions_module.FanSetSpeedAction()

    @pytest.fixture
    def mock_fans(self):
        """Fixture providing mock fan objects."""
        fans = []
        for i in range(3):
            fan = Mock()
            fan.set_speed = Mock(return_value=True)
            fans.append(fan)
        return fans

    @pytest.fixture
    def mock_thermal_info_dict(self, thermal_actions_module, mock_fans):
        """Fixture providing mock thermal info dictionary."""
        fan_drawer_info = Mock()
        fan_drawer_info.get_fans = Mock(return_value=mock_fans)

        return {
            thermal_actions_module.FanDrawerInfo.INFO_TYPE: fan_drawer_info
        }

    def test_fan_set_speed_action_initialization(self, fan_set_speed_action):
        """Test FanSetSpeedAction initialization."""
        assert fan_set_speed_action._speed is None

    def test_fan_set_speed_action_load_from_json_valid(self, fan_set_speed_action):
        """Test loading valid JSON configuration."""
        json_config = {'speed': 75}
        fan_set_speed_action.load_from_json(json_config)
        assert fan_set_speed_action._speed == 75

    def test_fan_set_speed_action_load_from_json_invalid(self, fan_set_speed_action):
        """Test loading invalid JSON configuration."""
        with pytest.raises(KeyError):
            fan_set_speed_action.load_from_json({})  # Missing speed field

    def test_fan_set_speed_action_execute(self, fan_set_speed_action, mock_thermal_info_dict, mock_fans):
        """Test FanSetSpeedAction execution."""
        # Configure action
        fan_set_speed_action.load_from_json({'speed': 75})

        # Execute action
        fan_set_speed_action.execute(mock_thermal_info_dict)

        # Verify all fans were set to correct speed
        for fan in mock_fans:
            fan.set_speed.assert_called_once_with(75)


class TestFanSetMaxSpeedAction:
    """Test class for FanSetMaxSpeedAction functionality."""

    @pytest.fixture
    def fan_set_max_speed_action(self, thermal_actions_module):
        """Fixture providing a FanSetMaxSpeedAction instance."""
        return thermal_actions_module.FanSetMaxSpeedAction()

    @pytest.fixture
    def mock_fans(self):
        """Fixture providing mock fan objects."""
        fans = []
        for _ in range(3):
            fan = Mock()
            fan.set_speed = Mock(return_value=True)
            fans.append(fan)
        return fans

    @pytest.fixture
    def mock_thermal_info_dict(self, thermal_actions_module, mock_fans):
        """Fixture providing mock thermal info dictionary."""
        fan_drawer_info = Mock()
        fan_drawer_info.get_fans = Mock(return_value=mock_fans)
        
        return {
            thermal_actions_module.FanDrawerInfo.INFO_TYPE: fan_drawer_info
        }

    def test_fan_set_max_speed_action_initialization(self, fan_set_max_speed_action):
        """Test FanSetMaxSpeedAction initialization."""
        assert fan_set_max_speed_action._max_speed is None

    def test_fan_set_max_speed_action_load_from_json_valid(self, fan_set_max_speed_action):
        """Test loading valid JSON configuration."""
        json_config = {'max_speed': 75}
        fan_set_max_speed_action.load_from_json(json_config)
        assert fan_set_max_speed_action._max_speed == 75

    def test_fan_set_max_speed_action_load_from_json_invalid(self, fan_set_max_speed_action):
        """Test loading invalid JSON configuration."""
        with pytest.raises(KeyError):
            fan_set_max_speed_action.load_from_json({})  # Missing max_speed field

    def test_fan_set_max_speed_action_load_from_json_range_validation(self, fan_set_max_speed_action):
        """Test max_speed range validation during JSON loading."""
        with pytest.raises(ValueError, match="Max speed 5.0 is out of range"):
            fan_set_max_speed_action.load_from_json({'max_speed': 5})
        
        with pytest.raises(ValueError, match="Max speed 105.0 is out of range"):
            fan_set_max_speed_action.load_from_json({'max_speed': 105})

    def test_fan_set_max_speed_action_execute(self, fan_set_max_speed_action, mock_thermal_info_dict, mock_fans):
        """Test FanSetMaxSpeedAction execution."""
        # Configure action
        fan_set_max_speed_action.load_from_json({'max_speed': 75})
        
        # Execute action
        fan_set_max_speed_action.execute(mock_thermal_info_dict)
        
        # Verify all fans were set to correct max speed
        for fan in mock_fans:
            fan.set_max_speed.assert_called_once_with(75)


class TestThermalControlAlgorithmAction:
    """Test class for ThermalControlAlgorithmAction functionality."""

    @pytest.fixture
    def thermal_control_action(self, thermal_actions_module):
        """Fixture providing a ThermalControlAlgorithmAction instance."""
        return thermal_actions_module.ThermalControlAlgorithmAction()

    @pytest.fixture
    def valid_json_config(self):
        """Fixture providing valid JSON configuration for thermal control."""
        return {
            'pid_domains': {
                'cpu': {'KP': 1.0, 'KI': 0.5, 'KD': 0.1},
            },
            'constants': {
                'interval': 5,
                'extra_setpoint_margin': {'cpu': 2.0}
            },
            'fan_limits': {
                'min': 30
            }
        }

    def test_thermal_control_action_initialization(self, thermal_control_action):
        """Test ThermalControlAlgorithmAction initialization."""
        assert thermal_control_action._pidDomains is None
        assert thermal_control_action._constants is None
        assert thermal_control_action._fan_limits is None
        assert thermal_control_action._pidControllers == {}

    def test_thermal_control_action_load_from_json_valid(self, thermal_control_action, valid_json_config):
        """Test loading valid JSON configuration."""
        thermal_control_action.load_from_json(valid_json_config)

        assert thermal_control_action._pidDomains == valid_json_config['pid_domains']
        assert thermal_control_action._constants == valid_json_config['constants']
        assert thermal_control_action._fan_limits == valid_json_config['fan_limits']

    def test_thermal_control_action_load_from_json_missing_fields(self, thermal_control_action, valid_json_config):
        """Test loading JSON configuration with missing required fields."""
        invalid_config = valid_json_config.copy()
        del invalid_config['pid_domains']

        with pytest.raises(KeyError):
            thermal_control_action.load_from_json(invalid_config)

    def test_thermal_control_action_pid_controller_creation(self, thermal_control_action, valid_json_config):
        """Test that PID controllers are created correctly from JSON config."""
        thermal_control_action.load_from_json(valid_json_config)

        # Initialize PID controllers
        thermal_control_action._initialize_pid_controllers(interval=5, fan_max_speed=100)

        # Verify controller was created for the domain
        assert 'cpu' in thermal_control_action._pidControllers
        controller = thermal_control_action._pidControllers['cpu']

        # Verify controller parameters match JSON config
        assert controller._kp == 1.0
        assert controller._ki == 0.5
        assert controller._kd == 0.1
        assert controller._output_min == 30
        assert controller._output_max == 100
        assert controller._interval == 5

    def test_thermal_control_action_interval_mismatch(self, thermal_control_action, valid_json_config):
        """Test that mismatched intervals raise ValueError."""
        thermal_control_action.load_from_json(valid_json_config)

        # Try to initialize with different interval
        with pytest.raises(ValueError, match="Interval 10 does not match interval 5"):
            thermal_control_action._initialize_pid_controllers(interval=10, fan_max_speed=100)

    def test_thermal_control_action_convert_pid_output_to_speed(self, thermal_control_action, valid_json_config):
        """Test PID output to fan speed conversion with precise validation."""
        thermal_control_action.load_from_json(valid_json_config)

        # Test various PID outputs
        test_cases = [
            (25.0, 30),    # Below min, should clamp to min
            (30.0, 30),    # At min
            (65.0, 65),    # In range
            (100.0, 100),  # At max
            (105.0, 100),  # Above max, should clamp to max
        ]

        for pid_output, expected_speed in test_cases:
            speed = thermal_control_action._convert_pid_output_to_speed(pid_output, max_speed=100)
            assert speed == expected_speed, f"PID output {pid_output} should convert to {expected_speed}, got {speed}"

    def test_thermal_control_action_multiple_domains(self, thermal_actions_module):
        """Test thermal control action with multiple PID domains."""
        action = thermal_actions_module.ThermalControlAlgorithmAction()

        multi_domain_config = {
            'pid_domains': {
                'cpu': {'KP': 1.0, 'KI': 0.5, 'KD': 0.1},
                'switch': {'KP': 2.0, 'KI': 1.0, 'KD': 0.2},
                'ambient': {'KP': 0.5, 'KI': 0.25, 'KD': 0.05},
            },
            'constants': {
                'interval': 5,
                'extra_setpoint_margin': {'cpu': 2.0, 'switch': 3.0, 'ambient': 1.0}
            },
            'fan_limits': {
                'min': 35
            }
        }

        action.load_from_json(multi_domain_config)
        action._initialize_pid_controllers(interval=5, fan_max_speed=95)

        # Verify all controllers were created
        assert len(action._pidControllers) == 3
        assert 'cpu' in action._pidControllers
        assert 'switch' in action._pidControllers
        assert 'ambient' in action._pidControllers

        # Verify each controller has correct parameters
        cpu_controller = action._pidControllers['cpu']
        assert cpu_controller._kp == 1.0 and cpu_controller._ki == 0.5 and cpu_controller._kd == 0.1

        switch_controller = action._pidControllers['switch']
        assert switch_controller._kp == 2.0 and switch_controller._ki == 1.0 and switch_controller._kd == 0.2

        ambient_controller = action._pidControllers['ambient']
        assert ambient_controller._kp == 0.5 and ambient_controller._ki == 0.25 and ambient_controller._kd == 0.05

    def test_thermal_control_action_fan_limits_validation(self, thermal_control_action):
        """Test fan limits validation during JSON loading."""
        # Test invalid fan limits (out of range)
        invalid_config = {
            'pid_domains': {'cpu': {'KP': 1.0, 'KI': 0.5, 'KD': 0.1}},
            'constants': {'interval': 5, 'extra_setpoint_margin': {'cpu': 2.0}},
            'fan_limits': {'min': 5}  # Out of valid range
        }

        with pytest.raises(ValueError, match="Min fan limit 5 is out of range"):
            thermal_control_action.load_from_json(invalid_config)

    def test_thermal_control_action_missing_interval(self, thermal_control_action):
        """Test that missing interval raises ValueError."""
        invalid_config = {
            'pid_domains': {'cpu': {'KP': 1.0, 'KI': 0.5, 'KD': 0.1}},
            'constants': {'extra_setpoint_margin': {'cpu': 2.0}},  # Missing interval
            'fan_limits': {'min': 30}
        }

        with pytest.raises(ValueError, match="Interval must be defined"):
            thermal_control_action.load_from_json(invalid_config)


class TestSetAllFanSpeedsErrorCases:
    """Test class for set_all_fan_speeds error cases."""

    @pytest.fixture
    def mock_logger(self):
        """Fixture providing a mock logger."""
        logger = Mock()
        logger.log_error = Mock()
        logger.log_warning = Mock()
        return logger

    @pytest.fixture
    def thermal_actions_module_for_function(self, thermal_actions_module):
        """Fixture providing access to the thermal_actions module for function testing."""
        return thermal_actions_module

    def test_set_all_fan_speeds_no_fans_available(self, thermal_actions_module_for_function, mock_logger):
        """Test set_all_fan_speeds with empty fan list."""
        empty_fans = []

        with pytest.raises(thermal_actions_module_for_function.FanException, match="No fans available to set speed"):
            thermal_actions_module_for_function.set_all_fan_speeds(mock_logger, empty_fans, 50.0)

        # Verify error was logged
        mock_logger.log_error.assert_called_once_with("No fans available to set speed")

    def test_set_all_fan_speeds_fan_set_speed_returns_false(self, thermal_actions_module_for_function, mock_logger):
        """Test set_all_fan_speeds when fan.set_speed returns False."""
        # Create mock fans that return False from set_speed
        mock_fans = []
        for i in range(3):
            fan = Mock()
            fan.set_speed = Mock(return_value=False)
            mock_fans.append(fan)

        speed = 75.0

        # Should not raise exception, but should log warnings
        thermal_actions_module_for_function.set_all_fan_speeds(mock_logger, mock_fans, speed)

        # Verify all fans were called
        for i, fan in enumerate(mock_fans):
            fan.set_speed.assert_called_once_with(speed)

        # Verify warnings were logged for each fan
        expected_calls = [
            call(f"Failed to set speed {speed:.1f}% for fan {i} (fan may not be present)")
            for i in range(3)
        ]
        mock_logger.log_warning.assert_has_calls(expected_calls)

    def test_set_all_fan_speeds_fan_set_speed_raises_exception(self, thermal_actions_module_for_function, mock_logger):
        """Test set_all_fan_speeds when fan.set_speed raises an exception."""
        # Create mock fan that raises exception
        mock_fan = Mock()
        test_exception = RuntimeError("Hardware failure")
        mock_fan.set_speed = Mock(side_effect=test_exception)
        mock_fans = [mock_fan]

        speed = 60.0

        # Should re-raise the exception
        with pytest.raises(RuntimeError, match="Hardware failure"):
            thermal_actions_module_for_function.set_all_fan_speeds(mock_logger, mock_fans, speed)

        # Verify fan was called
        mock_fan.set_speed.assert_called_once_with(speed)

        # Verify error was logged
        mock_logger.log_error.assert_any_call(f"Exception setting speed {speed:.1f}% for fan 0: {test_exception}")
        # Also verify traceback was logged (we can't easily test the exact traceback content)
        assert any("Traceback:" in str(call) for call in mock_logger.log_error.call_args_list)

    def test_set_all_fan_speeds_mixed_success_and_failure(self, thermal_actions_module_for_function, mock_logger):
        """Test set_all_fan_speeds with mix of successful and failed fan operations."""
        # Create mix of fans: some succeed, some fail, some raise exceptions
        mock_fans = []

        # Fan 0: Success
        fan0 = Mock()
        fan0.set_speed = Mock(return_value=True)
        mock_fans.append(fan0)

        # Fan 1: Returns False
        fan1 = Mock()
        fan1.set_speed = Mock(return_value=False)
        mock_fans.append(fan1)

        # Fan 2: Success
        fan2 = Mock()
        fan2.set_speed = Mock(return_value=True)
        mock_fans.append(fan2)

        # Fan 3: Raises exception
        fan3 = Mock()
        fan3.set_speed = Mock(side_effect=IOError("I/O error"))
        mock_fans.append(fan3)

        speed = 80.0

        # Should raise exception from fan3
        with pytest.raises(IOError, match="I/O error"):
            thermal_actions_module_for_function.set_all_fan_speeds(mock_logger, mock_fans, speed)

        # Verify all fans up to the failing one were called
        fan0.set_speed.assert_called_once_with(speed)
        fan1.set_speed.assert_called_once_with(speed)
        fan2.set_speed.assert_called_once_with(speed)
        fan3.set_speed.assert_called_once_with(speed)

        # Verify warning was logged for fan1 (returned False)
        mock_logger.log_warning.assert_called_with(f"Failed to set speed {speed:.1f}% for fan 1 (fan may not be present)")

        # Verify error was logged for fan3 (raised exception)
        mock_logger.log_error.assert_any_call(f"Exception setting speed {speed:.1f}% for fan 3: I/O error")

    def test_set_all_fan_speeds_none_fans_list(self, thermal_actions_module_for_function, mock_logger):
        """Test set_all_fan_speeds with None as fan list."""
        with pytest.raises(thermal_actions_module_for_function.FanException, match="No fans available to set speed"):
            thermal_actions_module_for_function.set_all_fan_speeds(mock_logger, None, 50.0)

        # Verify error was logged
        mock_logger.log_error.assert_called_once_with("No fans available to set speed")


class TestSfpThermalGetPidSetpoint:
    """Test class for SfpThermal get_pid_setpoint invalid setpoint handling."""

    @pytest.fixture
    def mock_sfp(self):
        """Fixture providing a mock SFP object."""
        sfp = Mock()
        sfp.get_name = Mock(return_value="sfp1")
        return sfp

    @pytest.fixture
    def mock_thermal_syslogger(self):
        """Fixture providing a mock thermal syslogger."""
        mock_syslogger = Mock()
        mock_syslogger.log_warning = Mock()
        return mock_syslogger

    @pytest.fixture
    def sfp_thermal(self, mock_sfp, mock_thermal_syslogger):
        """Fixture providing a mock SfpThermal instance."""
        # Create a mock SfpThermal class that mimics the real behavior
        class MockSfpThermal:
            MIN_VALID_SETPOINT = 30.0
            DEFAULT_SETPOINT = 65.0

            def __init__(self, sfp, thermal_syslogger):
                self._sfp = sfp
                self._invalid_setpoint_logged = False
                self._thermal_syslogger = thermal_syslogger
                self._parent_setpoint = None  # Will be set by tests

            def get_name(self):
                return f"Transceiver {self._sfp.get_name().capitalize()}"

            def get_pid_setpoint(self):
                """Mock implementation of SfpThermal.get_pid_setpoint logic."""
                setpoint = self._parent_setpoint  # Simulates parent class call
                if setpoint is None:
                    return setpoint
                # Setpoint cannot be guaranteed on pluggables - some modules may have invalid values such as 0.
                # For these cases, use a default setpoint.
                if setpoint < self.MIN_VALID_SETPOINT:
                    if not self._invalid_setpoint_logged:
                        self._thermal_syslogger.log_warning(f"Invalid setpoint {setpoint:.1f} for {self.get_name()}, "
                                                          f"using default setpoint {self.DEFAULT_SETPOINT:.1f}")
                        self._invalid_setpoint_logged = True
                    return self.DEFAULT_SETPOINT
                return setpoint

        # Create instance
        sfp_thermal = MockSfpThermal(mock_sfp, mock_thermal_syslogger)
        return sfp_thermal

    def test_sfp_thermal_get_pid_setpoint_valid_setpoint(self, sfp_thermal):
        """Test SfpThermal get_pid_setpoint with valid setpoint."""
        # Set parent setpoint to a valid value
        sfp_thermal._parent_setpoint = 75.0
        setpoint = sfp_thermal.get_pid_setpoint()
        assert setpoint == 75.0
        # Should not log any warning for valid setpoint
        assert not sfp_thermal._invalid_setpoint_logged

    def test_sfp_thermal_get_pid_setpoint_none_setpoint(self, sfp_thermal):
        """Test SfpThermal get_pid_setpoint when parent returns None."""
        # Set parent setpoint to None
        sfp_thermal._parent_setpoint = None
        setpoint = sfp_thermal.get_pid_setpoint()
        assert setpoint is None
        # Should not log any warning for None setpoint
        assert not sfp_thermal._invalid_setpoint_logged

    def test_sfp_thermal_get_pid_setpoint_invalid_setpoint_below_minimum(self, sfp_thermal):
        """Test SfpThermal get_pid_setpoint with setpoint below minimum."""
        # Set parent setpoint to invalid value
        invalid_setpoint = 15.0  # Below MIN_VALID_SETPOINT (30.0)
        sfp_thermal._parent_setpoint = invalid_setpoint

        setpoint = sfp_thermal.get_pid_setpoint()

        # Should return default setpoint
        assert setpoint == sfp_thermal.DEFAULT_SETPOINT

        # Should log warning
        expected_warning = (f"Invalid setpoint {invalid_setpoint:.1f} for {sfp_thermal.get_name()}, "
                          f"using default setpoint {sfp_thermal.DEFAULT_SETPOINT:.1f}")
        sfp_thermal._thermal_syslogger.log_warning.assert_called_once_with(expected_warning)

        # Should mark as logged
        assert sfp_thermal._invalid_setpoint_logged

    def test_sfp_thermal_get_pid_setpoint_invalid_setpoint_zero(self, sfp_thermal):
        """Test SfpThermal get_pid_setpoint with zero setpoint."""
        # Set parent setpoint to zero
        invalid_setpoint = 0.0
        sfp_thermal._parent_setpoint = invalid_setpoint

        setpoint = sfp_thermal.get_pid_setpoint()

        # Should return default setpoint
        assert setpoint == sfp_thermal.DEFAULT_SETPOINT

        # Should log warning
        expected_warning = (f"Invalid setpoint {invalid_setpoint:.1f} for {sfp_thermal.get_name()}, "
                          f"using default setpoint {sfp_thermal.DEFAULT_SETPOINT:.1f}")
        sfp_thermal._thermal_syslogger.log_warning.assert_called_once_with(expected_warning)

    def test_sfp_thermal_get_pid_setpoint_invalid_setpoint_negative(self, sfp_thermal):
        """Test SfpThermal get_pid_setpoint with negative setpoint."""
        # Set parent setpoint to negative value
        invalid_setpoint = -5.0
        sfp_thermal._parent_setpoint = invalid_setpoint

        setpoint = sfp_thermal.get_pid_setpoint()

        # Should return default setpoint
        assert setpoint == sfp_thermal.DEFAULT_SETPOINT

        # Should log warning
        expected_warning = (f"Invalid setpoint {invalid_setpoint:.1f} for {sfp_thermal.get_name()}, "
                          f"using default setpoint {sfp_thermal.DEFAULT_SETPOINT:.1f}")
        sfp_thermal._thermal_syslogger.log_warning.assert_called_once_with(expected_warning)

    def test_sfp_thermal_get_pid_setpoint_warning_logged_only_once(self, sfp_thermal):
        """Test that invalid setpoint warning is logged only once."""
        invalid_setpoint = 10.0
        sfp_thermal._parent_setpoint = invalid_setpoint

        # Call multiple times
        setpoint1 = sfp_thermal.get_pid_setpoint()
        setpoint2 = sfp_thermal.get_pid_setpoint()
        setpoint3 = sfp_thermal.get_pid_setpoint()

        # All should return default setpoint
        assert setpoint1 == sfp_thermal.DEFAULT_SETPOINT
        assert setpoint2 == sfp_thermal.DEFAULT_SETPOINT
        assert setpoint3 == sfp_thermal.DEFAULT_SETPOINT

        # Warning should be logged only once
        assert sfp_thermal._thermal_syslogger.log_warning.call_count == 1

        # Flag should be set
        assert sfp_thermal._invalid_setpoint_logged

    def test_sfp_thermal_get_pid_setpoint_boundary_conditions(self, sfp_thermal):
        """Test SfpThermal get_pid_setpoint at boundary conditions."""
        # Test exactly at minimum valid setpoint
        boundary_setpoint = sfp_thermal.MIN_VALID_SETPOINT  # 30.0
        sfp_thermal._parent_setpoint = boundary_setpoint

        setpoint = sfp_thermal.get_pid_setpoint()

        # Should return the boundary setpoint (valid)
        assert setpoint == boundary_setpoint

        # Should not log warning
        sfp_thermal._thermal_syslogger.log_warning.assert_not_called()
        assert not sfp_thermal._invalid_setpoint_logged

        # Test just below minimum valid setpoint
        just_below_boundary = sfp_thermal.MIN_VALID_SETPOINT - 0.1  # 29.9

        # Reset the logged flag for this test
        sfp_thermal._invalid_setpoint_logged = False
        sfp_thermal._thermal_syslogger.reset_mock()


class TestPortIndexMapper:
    def test_get_interface_name_picks_lowest_and_ignores_invalid(self, thermal_module):
        """Verify PortIndexMapper builds mapping and picks lowest Ethernet name for same index."""
        sw = sys.modules['swsscommon.swsscommon']
        # Prepare mock PORT table data
        sw.Table.MOCK_PORT_KEYS = ['Ethernet4', 'Ethernet0', 'NotAnEthernet']
        sw.Table.MOCK_PORT_DATA = {
            'Ethernet4': [('index', '1')],
            'Ethernet0': [('index', '1')],
            'NotAnEthernet': [('index', '1')],
        }
        # Reset singleton to rebuild mapping
        thermal_module.PortIndexMapper._instance = None
        mapper = thermal_module.PortIndexMapper()

        assert mapper.get_interface_name(1) == 'Ethernet0'
        assert mapper.get_interface_name(2) is None


class TestSfpThermal:
    @pytest.fixture
    def pddf_platform(self):
        # Provide minimal PLATFORM data to avoid None .lower() in PidThermalMixin
        return types.SimpleNamespace(data={'PLATFORM': {
            'nexthop_thermal_xcvr_setpoint_override': None,
            'nexthop_thermal_xcvr_pid_domain': 'none'
        }})

    def test_default_setpoint_when_thresholds_unavailable(self, thermal_module, pddf_platform):
        """When thresholds are not yet available but SFP is present, default setpoint is used."""
        sw = sys.modules['swsscommon.swsscommon']
        sw.SonicV2Connector.RETURN_GET_ALL = {}

        sfp = Mock()
        sfp.get_name.return_value = 'sfp1'
        sfp.get_presence.return_value = True
        sfp.get_position_in_parent.return_value = 1

        with patch.object(thermal_module.PortIndexMapper, 'get_interface_name', return_value='Ethernet0'):
            sfp_th = thermal_module.SfpThermal(sfp, pddf_platform)
            setpoint = sfp_th.get_pid_setpoint()
            assert setpoint == thermal_module.SfpThermal.DEFAULT_SETPOINT

    def test_invalid_computed_setpoint_logs_once_and_uses_default(self, thermal_module, pddf_platform):
        """If computed setpoint < MIN_VALID_SETPOINT, fallback to default and log once."""
        sw = sys.modules['swsscommon.swsscommon']
        # temphighwarning - margin (10) => 25 < 30 -> invalid
        sw.SonicV2Connector.RETURN_GET_ALL = {'temphighwarning': '35'}

        sfp = Mock()
        sfp.get_name.return_value = 'sfp2'
        sfp.get_presence.return_value = True
        sfp.get_position_in_parent.return_value = 2

        with patch.object(thermal_module.PortIndexMapper, 'get_interface_name', return_value='Ethernet4'):
            sfp_th = thermal_module.SfpThermal(sfp, pddf_platform)
            logger = thermal_module.thermal_syslogger
            before = getattr(logger, 'log_warning').call_count

            sp1 = sfp_th.get_pid_setpoint()
            assert sp1 == thermal_module.SfpThermal.DEFAULT_SETPOINT
            assert getattr(logger, 'log_warning').call_count == before + 1

            # Second call should not log again
            sp2 = sfp_th.get_pid_setpoint()
            assert sp2 == thermal_module.SfpThermal.DEFAULT_SETPOINT
            assert getattr(logger, 'log_warning').call_count == before + 1

    def test_thresholds_parsing_and_cache(self, thermal_module, pddf_platform):
        """State DB threshold values are parsed to float and cached for THRESHOLDS_CACHE_INTERVAL_SEC."""
        sw = sys.modules['swsscommon.swsscommon']
        sw.SonicV2Connector.RETURN_GET_ALL = {
            'temphighwarning': '75.0',
            'templowwarning': '10.5',
            'temphighalarm': '90',
            'templowalarm': '5',
            'irrelevant': 'N/A',
        }

        sfp = Mock()
        sfp.get_name.return_value = 'sfp3'
        sfp.get_presence.return_value = True
        sfp.get_position_in_parent.return_value = 3

        with patch.object(thermal_module.PortIndexMapper, 'get_interface_name', return_value='Ethernet8'):
            sfp_th = thermal_module.SfpThermal(sfp, pddf_platform)

            # First fetch reads from DB and caches
            assert sfp_th.get_high_threshold() == 75.0
            assert sfp_th.get_low_threshold() == 10.5
            assert sfp_th.get_high_critical_threshold() == 90.0
            assert sfp_th.get_low_critical_threshold() == 5.0

            # Change underlying DB data; cache should prevent update immediately
            sw.SonicV2Connector.RETURN_GET_ALL = {
                'temphighwarning': '10',
                'templowwarning': '1',
                'temphighalarm': '20',
                'templowalarm': '0',
            }
            # Values should remain cached (unchanged)
            assert sfp_th.get_high_threshold() == 75.0
            assert sfp_th.get_low_threshold() == 10.5



