#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import traceback

from typing import TYPE_CHECKING, Dict, List, Optional, Any, Union

from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object

if TYPE_CHECKING:
    from sonic_platform_base.fan_base import Fan

from sonic_platform.thermal_infos import FanDrawerInfo, ThermalInfo
from sonic_platform.syslog import SYSLOG_IDENTIFIER_THERMAL, NhLoggerMixin

# Default range of fan speed (percentage) that PID controller can produce.
FAN_MIN_SPEED: float = 30.0
FAN_MAX_SPEED: float = 100.0

class FanException(Exception):
    """Base exception class for fan-related errors."""
    pass

def set_all_fan_speeds(logger: NhLoggerMixin, fans: List['Fan'], speed: float) -> None:
    """
    Set speed for all fans.

    Args:
        logger: Logger instance for logging messages
        fans: List of Fan objects to set speed for
        speed: Target fan speed percentage (0-100)
    """
    if not fans:
        logger.log_error("No fans available to set speed")
        raise FanException("No fans available to set speed")
    success_count = 0
    for i, fan in enumerate(fans):
        try:
            result = fan.set_speed(speed)
            if result:
                success_count += 1
            else:
                logger.log_warning(f"Failed to set speed {speed:.1f}% for fan {i} (fan may not be present)")
        except Exception as e:
            logger.log_error(f"Exception setting speed {speed:.1f}% for fan {i}: {e}")
            logger.log_error(f"Traceback:\n{traceback.format_exc()}")
            raise

    logger.log_info(f"Applied speed {speed:.1f}% to {success_count}/{len(fans)} fans")

@thermal_json_object('fan.set_speed')
class FanSetSpeedAction(ThermalPolicyActionBase, NhLoggerMixin):
    """Thermal action to set fan speed to a specific percentage."""

    JSON_FIELD_SPEED: str = 'speed'

    def __init__(self) -> None:
        """Initialize FanSetSpeedAction."""
        ThermalPolicyActionBase.__init__(self)
        NhLoggerMixin.__init__(self, SYSLOG_IDENTIFIER_THERMAL)
        self._speed: Optional[int] = None
        self.log_debug("Initialized")

    def load_from_json(self, set_speed_json: Dict[str, Any]) -> None:
        """
        Load configuration from JSON.

        Args:
            set_speed_json: JSON object with 'speed' field (0-100)

        Raises:
            KeyError: If 'speed' field is missing
            ValueError: If speed value is invalid
        """
        try:
            self._speed = int(set_speed_json[self.JSON_FIELD_SPEED])
            self.log_info(f"Loaded with speed: {self._speed}%")
        except (KeyError, ValueError, TypeError) as e:
            self.log_error(f"Failed to load from JSON: {e}")
            raise

    def execute(self, thermal_info_dict: Dict[str, Any]) -> None:
        """
        Set speed for all present fans.

        Args:
            thermal_info_dict: Dictionary containing thermal information
        """
        fan_drawer_info = thermal_info_dict.get(FanDrawerInfo.INFO_TYPE)
        set_all_fan_speeds(self, fan_drawer_info.get_fans(), self._speed)

@thermal_json_object('fan.set_max_speed')
class FanSetMaxSpeedAction(ThermalPolicyActionBase, NhLoggerMixin):
    """Thermal action to set fan speed to a specific percentage."""

    JSON_FIELD_MAX_SPEED: str = 'max_speed'

    def __init__(self) -> None:
        """Initialize FanSetSpeedAction."""
        ThermalPolicyActionBase.__init__(self)
        NhLoggerMixin.__init__(self, SYSLOG_IDENTIFIER_THERMAL)
        self._max_speed: Optional[float] = None
        self.log_debug("Initialized")

    def load_from_json(self, set_max_speed_json: Dict[str, Any]) -> None:
        """
        Load configuration from JSON.

        Args:
            set_max_speed_json: JSON object with 'max_speed' field (0-100)

        Raises:
            KeyError: If 'max_speed' field is missing
            ValueError: If max_speed value is invalid
        """
        try:
            self._max_speed = float(set_max_speed_json[self.JSON_FIELD_MAX_SPEED])
            self._validate_json(set_max_speed_json)
            self.log_info(f"Loaded with max_speed: {self._max_speed}%")
        except (KeyError, ValueError, TypeError) as e:
            self.log_error(f"Failed to load from JSON: {e}")
            raise

    def _validate_json(self, set_max_speed_json: Dict[str, Any]) -> None:
        """
        Validate loaded JSON configuration.

        Raises:
            ValueError: If configuration is invalid
        """
        if self._max_speed is None:
            raise ValueError("No max_speed defined in JSON policy file")
        if self._max_speed < FAN_MIN_SPEED or self._max_speed > FAN_MAX_SPEED:
            raise ValueError(f"Max speed {self._max_speed} is out of range [{FAN_MIN_SPEED}, {FAN_MAX_SPEED}]")

    def execute(self, thermal_info_dict: Dict[str, Any]) -> None:
        """
        Set maximum speed for all present fans.

        Args:
            thermal_info_dict: Dictionary containing thermal information
        """
        fan_drawer_info = thermal_info_dict.get(FanDrawerInfo.INFO_TYPE)
        if not fan_drawer_info:
            raise ValueError("No fan drawer info available in thermal_info_dict")

        fans = fan_drawer_info.get_fans()
        if not fans:
            self.log_error("No fans available to set max_speed")
            raise FanException("No fans available to set max_speed")

        for fan in fans:
            fan.set_max_speed(self._max_speed)
        self.log_info(f"Applied max_speed {self._max_speed:.1f}% to {len(fans)} fans")

@thermal_json_object('thermal.control_algo')
class ThermalControlAlgorithmAction(ThermalPolicyActionBase, NhLoggerMixin):
    """PID-based thermal control algorithm using multiple thermal domains."""

    def __init__(self) -> None:
        """Initialize thermal control algorithm action."""
        ThermalPolicyActionBase.__init__(self)
        NhLoggerMixin.__init__(self, SYSLOG_IDENTIFIER_THERMAL)
        self._pidDomains: Optional[Dict[str, Dict[str, float]]] = None
        self._constants: Optional[Dict[str, Any]] = None
        self._fan_limits: Optional[Dict[str, Union[int, float]]] = None
        self._pidControllers: Dict[str, 'PIDController'] = {}
        self._extra_setpoint_margin: Dict[str, float] = {}

        self.log_debug("Initialized")

    def load_from_json(self, algo_json: Dict[str, Any]) -> None:
        """
        Load PID configuration from JSON.

        Args:
            algo_json: JSON object with pid_domains, constants, and fan_limits

        Raises:
            KeyError: If required JSON fields are missing
            ValueError: If JSON validation fails
        """
        try:
            self._pidDomains = algo_json['pid_domains']
            self._constants = algo_json['constants']
            self._fan_limits = algo_json['fan_limits']
        except KeyError as e:
            self.log_error(f"Missing required fields in JSON: {e}")
            raise
        except Exception as e:
            self.log_error(f"Failed to load from JSON: {e}")
            raise

        self.log_info(f"Initialized with {len(self._pidDomains)} PID domains")
        self.log_debug(f"PID domains: {list(self._pidDomains.keys())}")
        self.log_debug(f"Constants: {self._constants}")
        self.log_debug(f"Fan limits: {self._fan_limits}")
        try:
            self.validate_json()
        except ValueError as e:
            self.log_error(f"Invalid thermal control algorithm JSON: {e}")
            raise

    def validate_json(self) -> None:
        """
        Validate loaded JSON configuration.

        Raises:
            ValueError: If configuration is invalid
        """
        if not self._pidDomains:
            raise ValueError("No PID domains defined in JSON policy file")
        if not self._constants:
            raise ValueError("No constants defined in JSON policy file")
        if not self._fan_limits:
            raise ValueError("No fan limits defined in JSON policy file")
        min_limit = self._fan_limits.get('min')
        if min_limit is None:
            raise ValueError("No min fan limits defined in JSON policy file")
        if min_limit < FAN_MIN_SPEED or min_limit > FAN_MAX_SPEED:
            raise ValueError(f"Min fan limit {min_limit} is out of range [{FAN_MIN_SPEED}, {FAN_MAX_SPEED}]")
        if not self._constants.get('interval'):
            raise ValueError("Interval must be defined in JSON policy file")

    def execute(self, thermal_info_dict: Dict[str, Any]) -> None:
        """
        Execute PID thermal control algorithm. Sets fans to maximum on errors.

        Args:
            thermal_info_dict: Dictionary containing thermal information
        """
        try:
            self._execute_raise_on_error(thermal_info_dict)
        except Exception as e:
            self.log_error(f"Exception executing thermal control algorithm: {e}")
            self.log_error(f"Traceback:\n{traceback.format_exc()}")
            self.log_error(f"Setting fan speed to {FAN_MAX_SPEED}% (max)")
            self._set_all_fan_speeds(thermal_info_dict, FAN_MAX_SPEED)
            raise

    def _execute_raise_on_error(self, thermal_info_dict: Dict[str, Any]) -> None:
        """
        Execute PID algorithm, raising exceptions on errors.

        Args:
            thermal_info_dict: Dictionary containing thermal information
        """
        thermal_info = thermal_info_dict.get(ThermalInfo.INFO_TYPE)
        if not thermal_info:
            raise ValueError("No thermal info available in thermal_info_dict")
        fan_drawer_info = thermal_info_dict.get(FanDrawerInfo.INFO_TYPE)
        if not fan_drawer_info:
            raise ValueError("No fan drawer info available in thermal_info_dict")

        # Fan's max speed limit can change at runtime, so retrieve it every time
        fan_max_speed = self._get_fan_max_speed(fan_drawer_info)

        # Initialize PID controllers if needed
        if not self._pidControllers:
            dt = thermal_info.get_thermal_manager().get_interval()
            self._initialize_pid_controllers(dt, fan_max_speed)

        # Get all thermals and group by PID domain
        thermals = thermal_info.get_thermals()
        domain_thermals = self._group_thermals_by_domain(thermals)

        # Compute PID output for each domain
        pid_outputs = {}
        max_error_thermals = {}
        for domain, domain_thermal_list in domain_thermals.items():
            pid_output, max_error_thermal = self._compute_domain_pid_output(
                domain, domain_thermal_list, fan_max_speed
            )
            pid_outputs[domain] = pid_output
            max_error_thermals[domain] = max_error_thermal.get_name() if max_error_thermal else "None"

        if not pid_outputs:
            raise ValueError("No valid PID outputs computed, keeping current fan speeds")

        # Use maximum PID output to set fan speed
        max_output = max(pid_outputs.values())
        max_domain = max(pid_outputs, key=pid_outputs.get)

        # Convert PID output to fan speed percentage
        final_speed = self._convert_pid_output_to_speed(max_output, fan_max_speed)

        self.log_info(f"Max PID output: {max_output:.3f} from domain '{max_domain}', "
                      f"setting fan speed to {final_speed:.1f}%")

        # Set all fan speeds
        self._set_all_fan_speeds(thermal_info_dict, final_speed)

    def _initialize_pid_controllers(self, interval: int, fan_max_speed: float) -> None:
        """
        Initialize PID controllers for each domain.

        Args:
            interval: Control loop interval in seconds
            fan_max_speed: Maximum fan speed in percentage (0-100)

        Raises:
            ValueError: If interval doesn't match configuration
        """
        if interval != self._constants['interval']:
            # PID parameters are tuned for specific intervals
            raise ValueError(f"Interval {interval} does not match interval {self._constants.get('interval')} "
                             f"specified in JSON policy file")
        for domain, domain_config in self._pidDomains.items():
            controller = PIDController(
                domain=domain,
                interval=interval,
                proportional_gain=domain_config['KP'],
                integral_gain=domain_config['KI'],
                derivative_gain=domain_config['KD'],
                output_min=self._fan_limits.get('min', FAN_MIN_SPEED),
                output_max=fan_max_speed
            )
            self._pidControllers[domain] = controller
            self._extra_setpoint_margin[domain] = domain_config.get('extra_setpoint_margin', 0)
            self.log_info(f"Initialized PID controller for domain '{domain}'")
            if self._extra_setpoint_margin[domain]:
                self.log_notice(f"Extra setpoint margin for domain '{domain}': {self._extra_setpoint_margin[domain]}")

    def _group_thermals_by_domain(self, thermals: List[Any]) -> Dict[str, List[Any]]:
        """
        Group thermals by their PID domain.

        Args:
            thermals: List of thermal objects

        Returns:
            Dictionary mapping domain names to lists of thermal objects
        """
        domain_thermals = {}
        for thermal in thermals:
            if hasattr(thermal, 'is_controlled_by_pid'):
                if not thermal.is_controlled_by_pid():
                    continue
                domain = thermal.get_pid_domain()
                if domain and domain in self._pidControllers:
                    if domain not in domain_thermals:
                        domain_thermals[domain] = []
                    domain_thermals[domain].append(thermal)
            else:
                self.log_warning(f"Thermal {thermal.get_name()} does not define is_controlled_by_pid()")
        if not domain_thermals:
            raise ValueError("No thermals available for PID control")
        for domain, domain_thermals_list in domain_thermals.items():
            if not domain_thermals_list:
                raise ValueError(f"Domain '{domain}' has no thermals")
        self.log_debug(f"Grouped thermals by domain: {[(d, len(ts)) for d, ts in domain_thermals.items()]}")
        return domain_thermals

    def _compute_domain_pid_output(
        self, domain: str, domain_thermals: List[Any], fan_max_speed: float
    ) -> tuple[float, Any]:
        """
        Compute PID output using thermal with largest error in domain.

        Args:
            domain: PID domain name
            domain_thermals: List of thermal objects in this domain
            fan_max_speed: Maximum fan speed in percentage (0-100)

        Returns:
            Tuple of (PID output value, max error thermal object)
        """
        controller = self._pidControllers[domain]

        # Fan's max speed limit can change at runtime, so update it first
        controller.set_output_max(fan_max_speed)

        # Find thermal with largest error (current temp - setpoint)
        max_error = None
        max_error_thermal = None
        max_error_thermal_setpoint = None

        for thermal in domain_thermals:
            current_temp = thermal.get_temperature()
            if current_temp is None:
                # We may have no temperature reading if thermal is not present
                continue

            setpoint = thermal.get_pid_setpoint()
            if setpoint is None:
                # If the thermal was just unplugged, we may got the temperature, but not the setpoint
                continue

            error = current_temp - setpoint - self._extra_setpoint_margin[domain]

            if max_error is None or error > max_error:
                max_error = error
                max_error_thermal = thermal
                max_error_thermal_setpoint = setpoint

        if max_error_thermal is None:
            raise ValueError(f"No valid thermal found for domain '{domain}'")

        self.log_debug(f"Domain '{domain}': using thermal '{max_error_thermal.get_name()}' "
                       f"with error {max_error:.2f}°C (setpoint={max_error_thermal_setpoint:.2f}°C)")

        # Compute PID output using the largest error
        pid_output = controller.compute(max_error)
        return pid_output, max_error_thermal

    def _convert_pid_output_to_speed(self, pid_output: float, max_speed: float) -> float:
        """
        Convert PID output to fan speed percentage.

        Args:
            pid_output: Raw PID controller output
            max_speed: Maximum fan speed in percentage (0-100)

        Returns:
            Fan speed percentage saturated to configured limits
        """
        min_speed = self._fan_limits.get('min', FAN_MIN_SPEED)
        return max(min_speed, min(max_speed, pid_output))

    def _get_fan_max_speed(self, fan_drawer_info: FanDrawerInfo) -> float:
        """
        Gets fan speed limit that satisfy all fans.
        
        If the value is out of range ([self._fan_limits['min'], FAN_MAX_SPEED]),
        clamps it into the range.

        Args:
            thermal_info_dict: Dictionary containing thermal information

        Returns:
            Max speed for all fans, saturated to the configured limits
        """
        fans = fan_drawer_info.get_fans()
        if not fans:
            raise FanException("No fans available to get max speed")
        fan_max_speed = min(fan.get_max_speed() for fan in fans)
        fan_min_speed = self._fan_limits.get('min', FAN_MIN_SPEED)
        if fan_max_speed < fan_min_speed or fan_max_speed > FAN_MAX_SPEED:
            self.log_error(
                f"Fan max speed {fan_max_speed} is out of range [{fan_min_speed}, {FAN_MAX_SPEED}]. "
                "Clamping it into the range."
            )
            fan_max_speed = max(fan_min_speed, min(fan_max_speed, FAN_MAX_SPEED))
        return fan_max_speed

    def _set_all_fan_speeds(self, thermal_info_dict: Dict[str, Any], speed: float) -> None:
        """
        Set speed for all fans.

        Args:
            thermal_info_dict: Dictionary containing thermal information
            speed: Target fan speed percentage
        """
        set_all_fan_speeds(self, thermal_info_dict.get(FanDrawerInfo.INFO_TYPE).get_fans(), speed)

class PIDController(NhLoggerMixin):
    def __init__(self,
                 domain: str,
                 interval: int,
                 proportional_gain: float,
                 integral_gain: float,
                 derivative_gain: float,
                 output_min: float,
                 output_max: float) -> None:
        """
        Initialize PID controller.

        Args:
            domain: Thermal domain name for logging
            interval: Control loop interval in seconds
            proportional_gain: Kp gain
            integral_gain: Ki gain
            derivative_gain: Kd gain
            output_min: Minimum output value (fan speed %)
            output_max: Maximum output value (fan speed %)
        """
        super().__init__(SYSLOG_IDENTIFIER_THERMAL)

        self._domain = domain
        self._interval = interval

        # Gains
        self._kp = proportional_gain
        self._ki = integral_gain
        self._kd = derivative_gain

        self._output_min = output_min
        self._output_max = output_max

        # PID state variables
        # Pre-seed integral to adjust to the midpoint between min/max
        # This helps reduce the initial transient response
        self._integral = (output_min + output_max) / 2 / self._ki
        self._prev_error: float = 0
        self._first_run: bool = True
        self._last_output: float = (output_min + output_max) / 2  # Start at midpoint
        self._in_deadband: bool = False

        self.log_info(f"PIDController initialized for domain '{domain}': "
                      f"gains=[Kp={proportional_gain}, Ki={integral_gain}, Kd={derivative_gain}], "
                      f"output_range=[{output_min}, {output_max}], interval={interval}s")

    def log(self, priority: Any, msg: str, also_print_to_console: bool = False) -> None:
        super().log(priority, f"[{self._domain}] {msg}", also_print_to_console)

    def set_output_max(self, output_max: float) -> None:
        """Updates the maximum output value (fan speed %) as this can change at runtime."""
        if output_max < self._output_min:
            self.log_error(
                f"PIDController for domain '{self._domain}': "
                f"attempting to set output_max={output_max}, "
                f"which is below output_min={self._output_min}. "
                f"Ignoring."
            )
            return
        self._output_max = output_max
        self.log_info(f"PIDController for domain '{self._domain}': updated output_range=[{self._output_min}, {self._output_max}]")

    def compute(self, error: float) -> float:
        """
        Compute PID output.

        Args:
            error: Current error value (measured_value - setpoint)

        Returns:
            Saturated PID controller output
        """
        debug_params_strings = []
        kp, ki, kd = self._kp, self._ki, self._kd

        # Proportional term - current error
        proportional = error

        # Derivative term - rate of change of error
        if self._first_run:
            derivative = 0.0
            self._first_run = False
        else:
            derivative = (error - self._prev_error) / self._interval
            
        # Integral term - accumulated error over time
        integral = self._integral + error * self._interval
        
        # Calculate output
        output = kp * proportional + ki * integral + kd * derivative
        saturated_output = max(self._output_min, min(self._output_max, output))
        if saturated_output != output:
            debug_params_strings.append("output saturated")
        
        # Save state for next iteration
        # Only update integral if output is not saturated or if the error is helping to unsaturate
        self._prev_error = error
        if (output <= self._output_max or error < 0) and (output >= self._output_min or error > 0):
            self._integral = integral
        else:
            debug_params_strings.append("integral frozen")

        # Debug logging
        log_str = f"PID=[ %8.3f %8.3f %8.3f ]   =>   OUT=%8.3f" % (proportional, integral, derivative, output)
        if debug_params_strings:
            log_str += f"   ({', '.join(debug_params_strings)})"
        self.log_debug(log_str)

        return saturated_output
