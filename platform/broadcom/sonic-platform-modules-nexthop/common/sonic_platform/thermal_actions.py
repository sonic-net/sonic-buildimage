#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object

@thermal_json_object('fan.set_speed')
class FanSetSpeedAction(ThermalPolicyActionBase):
    # JSON field definition
    JSON_FIELD_SPEED = 'speed'

    def __init__(self):
        """
        Constructor of SetFanSpeedAction
        """
        self.speed = None
    
    def load_from_json(self, set_speed_json):
        """
        Construct FanSetSpeedAction via JSON. JSON example:
            {
                "type": "fan.set_speed"
                "speed": "100"
            }
        :param json_obj: A JSON object representing a FanSetSpeedAction action.
        :return:
        """
        self.speed = int(set_speed_json[FanSetSpeedAction.JSON_FIELD_SPEED])
    
    def execute(self, thermal_info_dict):
        """
        Set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import FanInfo
        fan_info = thermal_info_dict.get(FanInfo.INFO_TYPE)
        for fan in fan_info.get_fans():
            # No op if fan is not present
            fan.set_speed(self.speed)

@thermal_json_object('thermal.control_algo')
class ThermalControlAlgorithmAction(ThermalPolicyActionBase):
    def __init__(self):
        self.pidParams = None

        # For our use the process variable and setpoint are temperature
        # control variable is the fan speed (duty cycle between 0-100)
        self.pidController = None
    
    def load_from_json(self, algo_json):
        """
        Construct ThermalAlgorithmAction via JSON. JSON example:
            {
                "type": "thermal.control_algo",
                "pid_params": {
                    "setpoint": 85,
                    "Kp": 1.0,
                    "Ki": 1.0,
                    "Kd": 1.0,
                    "min_speed": 30,
                    "max_speed": 100
                }
            }
        :param json_obj: A JSON object representing a ThermalAlgorithmAction action.
        :return:
        """
        self.pid_kwargs = algo_json['pid_params']
        

    def execute(self, thermal_info_dict):
        """
        Run a single PID iteration and sets all fan speeds
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import ThermalInfo
        thermal_info = thermal_info_dict.get(ThermalInfo.INFO_TYPE)
        # Initialized here since we need interval from thermal manager
        if self.pidController == None:
            dt = thermal_info.get_thermal_manager().get_interval()
            self.pidController = FanPIDController(dt, **self.pid_kwargs)
        
        # Single PID to set all fans to same speed. PID uses average temp 
        # across all thermals as process variable
        thermals = thermal_info.get_thermals()
        valid_temps = [t.get_temperature() for t in thermals if t.get_temperature() is not None]
        
        if not valid_temps:
            # No valid temps, keep speeds same
            return
        avg_temp = sum(valid_temps) / len(valid_temps)
        speed = self.pidController.compute_speed(avg_temp)
        
        # Set all fan speed based on the one global PID control loop
        from .thermal_infos import FanInfo
        fan_info = thermal_info_dict.get(FanInfo.INFO_TYPE)
        for fan in fan_info.get_fans():
            # Returns False if unsuccessful
            fan.set_speed(speed)
    
class FanPIDController:
    """
    Fan PID controller. Setpoint and process variable are temp in Farenheit.
    Control variable is fan speed (duty cycle 0-100).
    """
    def __init__(self, dt: int, setpoint: float, Kp : float, Ki: float, Kd: float, min_speed: int, max_speed: int):
        self.dt = dt
        self.setpoint = setpoint
        self.Kp = Kp
        self.Ki = Ki
        self.Kd = Kd
        self.min_speed = min_speed
        self.max_speed = max_speed
        self.prev_speed = self.min_speed

        # Initialize error history
        self.prev_error = 0
        self.integral = 0
        
    def compute_speed(self, cur_temp : float):
        """
        Compute the control variable based on the process variable's
        Args:
            current_temp: Current measured temperature in Farenheit
        Returns:
            Fan speed as duty cycle (0-100)
        """
        # If the temp is larger than set point the error should be positive
        # i.e. increase fan speed
        error = cur_temp - self.setpoint
        
        P = self.Kp * error
        
        self.integral += error * self.dt
        I = self.Ki * self.integral
        
        derivative = (error - self.prev_error) / self.dt
        D = self.Kd * derivative
        
        # Save error for next iteration
        self.prev_error = error
        
        output = P + I + D

        # Apply limits
        speed = self.prev_speed + output
        speed = max(speed, self.min_speed)
        speed = min(speed, self.max_speed)
                
        self.prev_speed = speed

        return speed