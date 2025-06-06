#!/usr/bin/env python
#
# main.py
#
# Command-line utility for interacting with environment sensors within SONiC
#
from __future__ import print_function

try:
    import click
    import os
    import sys
    import tabulate

    from sonic_platform.helper import Helper
    from sonic_platform.platform import Platform
    from sonic_platform.constants import *
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

UTIL_VERSION = "1.0"
NOT_AVAILABLE = "N/A"
RPM_UNIT = "r/min"
CUR_UNIT = "A"
VOL_UNIT = "V"
PWR_UNIT = "W"
TMP_UNIT = "C"


class Sensors(object):
    platform = Platform()
    helper = Helper()
    chassis = platform.get_chassis()

    @property
    def all(self):
        return self.fans, self.psus, self.temperature, self.voltage

    @property
    def fans(self):
        fan_status_dict = {
            # tuple[presence, status]
            (True, True): "Normal",
            (True, False): "Stuck",
            (False, True): "Absent",
            (False, False): "Absent",
        }
        table_header = ("Fan Units", "Status", "Speed", "Direction")
        table_data = []
        all_fans = self.chassis.get_all_fans()
        if not all_fans:
            return "No fans available on this platform"
        for fan in all_fans:
            fan_name = fan.get_name().title()
            fan_status = fan_status_dict[(fan.get_presence(), fan.get_status())]
            fan_speed = fan.get_speed()
            fan_direction = fan.get_direction().title()
            # Post processing
            if fan_speed != NOT_AVAILABLE:
                fan_speed = "{} {}".format(fan_speed, RPM_UNIT)
            # Appending data
            table_data.append((fan_name, fan_status, fan_speed, fan_direction))
        return tabulate.tabulate(tabular_data=table_data, headers=table_header)

    @property
    def psus(self):
        psu_status_dict = {
            # tuple[presence, status]
            (True, True): "Normal",
            (True, False): "Powerloss",
            (False, True): "Absent",
            (False, False): "Absent",
        }
        table_header = ("Power Supply Units", "Status",
                        "Current In", "Current Out",
                        "Voltage In", "Voltage Out",
                        "Power In", "Power Out",
                        "Temperature", "Model", "Serial")
        table_data = []
        all_psus = self.chassis.get_all_psus()
        if not all_psus:
            return "No power supply units available on this platform"
        for psu in all_psus:
            psu_name = psu.get_name()
            psu_status = psu_status_dict[(psu.get_presence(), psu.get_status())]
            psu_current_in = psu.get_current_in()
            psu_current_out = psu.get_current_out()
            psu_voltage_in = psu.get_voltage_in()
            psu_voltage_out = psu.get_voltage_out()
            psu_power_in = psu.get_power_in()
            psu_power_out = psu.get_power_out()
            # Post processing
            if psu_current_in != NOT_AVAILABLE:
                psu_current_in = "{} {}".format(psu_current_in, CUR_UNIT)
            if psu_current_out != NOT_AVAILABLE:
                psu_current_out = "{} {}".format(psu_current_out, CUR_UNIT)
            if psu_voltage_in != NOT_AVAILABLE:
                psu_voltage_in = "{} {}".format(psu_voltage_in, VOL_UNIT)
            if psu_voltage_out != NOT_AVAILABLE:
                psu_voltage_out = "{} {}".format(psu_voltage_out, VOL_UNIT)
            if psu_power_in != NOT_AVAILABLE:
                psu_power_in = "{} {}".format(psu_power_in, PWR_UNIT)
            if psu_power_out != NOT_AVAILABLE:
                psu_power_out = "{} {}".format(psu_power_out, PWR_UNIT)
            # Appending data
            table_data.append((psu_name, psu_status,
                               psu_current_in, psu_current_out,
                               psu_voltage_in, psu_voltage_out,
                               psu_power_in, psu_power_out))
        return tabulate.tabulate(tabular_data=table_data, headers=table_header)

    @property
    def temperature(self):
        thermal_status_dict = {
            # bool
            True: "Normal",
            False: "Abormal",
        }
        table_header = ("Thermal Sensors", "Status", "Temperature",
                        "High Threshold", "Critical High Threshold",
                        "Low Threshold", "Critical Low Threshold")
        table_data = []
        all_thermals = self.chassis.get_all_thermals()
        if not all_thermals:
            return "No thermal sensors available on this platform"
        for thermal in all_thermals:
            thermal_name = thermal.get_name()
            thermal_status = thermal_status_dict[thermal.get_status()]
            thermal_temperature = thermal.get_temperature()
            if thermal_temperature == NOT_AVAILABLE: continue
            thermal_temperature = "{} {}".format(thermal_temperature, TMP_UNIT)
            thermal_temperature_high_threshold = thermal.get_high_threshold()
            thermal_temperature_critical_high_threshold = thermal.get_high_critical_threshold()
            thermal_temperature_low_threshold = thermal.get_low_threshold()
            thermal_temperature_critical_low_threshold = thermal.get_low_critical_threshold()
            # Post processing
            if thermal_temperature_high_threshold != NOT_AVAILABLE:
                thermal_temperature_high_threshold = "{} {}".format(
                    thermal_temperature_high_threshold, TMP_UNIT
                )
            if thermal_temperature_critical_high_threshold != NOT_AVAILABLE:
                thermal_temperature_critical_high_threshold = "{} {}".format(
                    thermal_temperature_critical_high_threshold, TMP_UNIT
                )
            if thermal_temperature_low_threshold != NOT_AVAILABLE:
                thermal_temperature_low_threshold = "{} {}".format(
                    thermal_temperature_low_threshold, TMP_UNIT
                )
            if thermal_temperature_critical_low_threshold != NOT_AVAILABLE:
                thermal_temperature_critical_low_threshold = "{} {}".format(
                    thermal_temperature_critical_low_threshold, TMP_UNIT
                )
            # Appending data
            table_data.append((thermal_name, thermal_status, thermal_temperature,
                               thermal_temperature_high_threshold, thermal_temperature_critical_high_threshold,
                               thermal_temperature_low_threshold, thermal_temperature_critical_low_threshold))
        return tabulate.tabulate(tabular_data=table_data, headers=table_header)

    @property
    def voltage(self):
        voltage_sensor_status_dict = {
            # bool
            True: "Normal",
            False: "Abormal",
        }
        table_header = ("Voltage Sensors", "Status", "Value")
        table_data = []
        all_voltage_sensors = self.chassis.get_all_voltage_sensors()
        if not all_voltage_sensors:
            return "No voltage sensors available on this platform"
        for voltage_sensor in all_voltage_sensors:
            voltage_sensor_name = voltage_sensor.get_name()
            voltage_sensor_status = voltage_sensor_status_dict[voltage_sensor.get_status()]
            voltage_sensor_value = voltage_sensor.get_value()
            if voltage_sensor_value == NOT_AVAILABLE: continue
            voltage_sensor_value = "{} {}".format(voltage_sensor_value, VOL_UNIT)
            # Post processing
            # Appending data
            table_data.append((voltage_sensor_name, voltage_sensor_status, voltage_sensor_value))
        return tabulate.tabulate(tabular_data=table_data, headers=table_header)


# ==================== CLI commands and groups ====================


# This is our main entrypoint - the main 'environment' command
@click.group()
def cli():
    """environment - Command line utility for fans, power, temperature, voltage reading"""
    pass


# 'version' subcommand
@cli.command()
def version():
    """Display version info"""
    click.echo("environment version {0}".format(UTIL_VERSION))


# 'show' subgroup
@cli.group()
@click.pass_context
def show(ctx):
    # type: (click.Context) -> None
    """Display status of platform environment"""

    if os.geteuid() != 0:
        print("Root privileges are required for this operation")
        sys.exit(1)

    ctx.ensure_object(Sensors)


# 'environment' subcommand
@show.command()
@click.pass_context
def all(ctx):
    # type: (click.Context) -> None
    """Display Platform environment data"""
    sensors = ctx.obj # type: Sensors
    all_sensors = sensors.all
    splitter = "=" * max(map(len, "\n".join(all_sensors).splitlines()))
    click.echo(("\n{}\n".format(splitter)).join(all_sensors))

@show.command()
@click.pass_context
def fans(ctx):
    # type: (click.Context) -> None
    """Display Platform environment fans"""
    sensors = ctx.obj # type: Sensors
    click.echo(sensors.fans)

@show.command()
@click.pass_context
def power(ctx):
    # type: (click.Context) -> None
    """Display Platform environment power"""
    sensors = ctx.obj # type: Sensors
    click.echo(sensors.psus)

@show.command()
@click.pass_context
def temperature(ctx):
    # type: (click.Context) -> None
    """Display Platform environment temperature"""
    sensors = ctx.obj # type: Sensors
    click.echo(sensors.temperature)

@show.command()
@click.pass_context
def voltage(ctx):
    # type: (click.Context) -> None
    """Display Platform environment voltage"""
    sensors = ctx.obj # type: Sensors
    click.echo(sensors.voltage)

if __name__ == "__main__":
    cli()
