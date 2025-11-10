#!/usr/bin/env python

import os
import subprocess  # James
import json
import time
import sonic_platform
from sonic_py_common import logger
from swsscommon.swsscommon import ConfigDBConnector
import utilities_common.cli as clicommon

SYSLOG_IDENTIFIER = "poecfg"
log = logger.Logger(SYSLOG_IDENTIFIER)


def get_priority_value(priority):  # James
    priority_mapping = {
        "critical": "1",
        "high": "2",
        "low": "3",
        "0xff": "0xff"  # 0xff means don't change the setting
    }
    return priority_mapping.get(priority, None)


def parse_keyword(keyword, string):  # James
    if string:
        # Parse the value of keyword from the string
        index = string.find(keyword)
        if index != -1:
            start_index = index + len(keyword)
            end_index = string.find("\n", start_index)
            result = string[start_index:end_index].strip()
            # click.echo("result: {}\n".format(result))
        else:
            log.log_error("Unable to find the result in the output.")
            return None

        # click.echo("string: {}\n".format(string.strip()))
        return result
    else:
        return None


def run_command(command):  # James
    try:
        output = subprocess.check_output(command.split(), stderr=subprocess.STDOUT, universal_newlines=True)
        # command, shell=True, stderr=subprocess.DEVNULL, universal_newlines=True)
        # return output.strip()
    except subprocess.CalledProcessError as e:
        print("Command failed:")
        print(e.output)
        return None, None

    if output:
        # Parse the value of "result" from the output
        keyword = "result "
        index = output.find(keyword)
        if index != -1:
            start_index = index + len(keyword)
            end_index = output.find("\n", start_index)
            result = output[start_index:end_index].strip()
            # click.echo("result: {}\n".format(result))
        else:
            log.log_error("Unable to find the result in the output.")
            return None, None

    # click.echo("output: {}\n".format(output.strip()))
    return result, output.strip()


def run_lldpcli_config_port_command(interface, enabled):
    # lldpcli configure port Etherent40 dot3 power pse supported enabled powerpairs signal class class-4 type 2  source primary priority low requested 28000 allocated 15000

    cmd = "lldpcli configure port %s dot3 power pse supported %s powerpairs signal class class-4" % (
        interface, enabled)
    # print(cmd)
    try:
        result = subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT, universal_newlines=True)
        # print(result.strip())
        return result.strip()
    except subprocess.CalledProcessError as e:
        log.log_error("Error running run_lldpcli_neighbor_command command:", e)
        return None


def run_lldpcli_show_port_command(interface):
    # lldpcli configure port Etherent40 dot3 power pse supported enabled powerpairs signal class class-4 type 2  source primary priority low requested 28000 allocated 15000

    cmd = "lldpcli show in port %s de" % (interface)
    # print(cmd)
    try:
        result = subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT, universal_newlines=True)
        # print(result.strip())
        return result.strip()
    except subprocess.CalledProcessError as e:
        log.log_error("Error running run_lldpcli_neighbor_command command:", e)
        return None


def power():

    VOLTAGE_110V_MIN = 90
    VOLTAGE_110V_MAX = 132
    VOLTAGE_220V_MIN = 180
    VOLTAGE_220V_MAX = 264
    VOLTAGE_110_1_VALUE = 810  # 110V for one bank
    VOLTAGE_110_2_VALUE = 1810  # 110V for two banks
    VOLTAGE_220_1_VALUE = 1410  # 220V for one bank
    VOLTAGE_220_2_VALUE = 2400  # 220V for two banks

    POWER_BANK_1 = 0X1
    POWER_BANK_2 = 0X2
    POWER_BANK_3 = 0X3

    platform_chassis = None
    try:
        platform_chassis = sonic_platform.platform.Platform().get_chassis()
    except Exception as e:
        log.log_error(
            "Failed to instantiate Chassis due to {}".format(repr(e)))

    if not platform_chassis:
        log.log_error("no platform_chassis")
        return False

    psu_list = platform_chassis.get_all_psus()
    psu_count = 0  # Count PSU number when two PSUs are existing, need to set bank 3
    psu_110v_count = 0  # Check if any 110V PSU in the system. If any 110V PSU, then the power value of bank 3 follows 110V's value

    for psu in psu_list:
        if psu.get_presence():
            try:
                name = psu.get_name()
                voltage = psu.get_voltage_vin()
                # log.log_error("{}- voltage: {}".format(psu, voltage))
                # print("name: {}  voltage: {}".format(name, voltage))

                # Decide voltage
                if (voltage >= VOLTAGE_110V_MIN and voltage <= VOLTAGE_110V_MAX):
                    voltage_value = VOLTAGE_110_1_VALUE
                    psu_110v_count = 1
                elif (voltage >= VOLTAGE_220V_MIN and voltage <= VOLTAGE_220V_MAX):
                    voltage_value = VOLTAGE_220_1_VALUE
                else:
                    log.log_error(
                        "poe_cfg_init.py - wrong voltage: {}".format(voltage))
                    continue

            except:
                log.log_error(
                    "poe_cfg_init.py(Exception) - {}- voltage: {}".format(name, voltage))
                pass

    if (psu_110v_count != 0):
        voltage_value_1 = VOLTAGE_110_1_VALUE
        voltage_value_2 = VOLTAGE_110_2_VALUE
    else:
        voltage_value_1 = VOLTAGE_220_1_VALUE
        voltage_value_2 = VOLTAGE_220_2_VALUE

    cmd = "poetool mgmt set_power_banks 1 %s 570 440 0xa" % (
        voltage_value_1)
    log.log_error(cmd)
    result, output_str = run_command(cmd)
    if (result == None or int(result) != 0):
        result, output_str = run_command(cmd)
        log.log_error("try it again: " + result)

    cmd = "poetool mgmt set_power_banks 2 %s 570 440 0xa" % (
        voltage_value_1)
    log.log_error(cmd)
    result, output_str = run_command(cmd)
    if (result == None or int(result) != 0):
        result, output_str = run_command(cmd)
        log.log_error("try it again: " + result)

    cmd = "poetool mgmt set_power_banks 3 %s 570 440 0xa" % (
        voltage_value_2)
    log.log_error(cmd)
    result, output_str = run_command(cmd)
    if (result == None or int(result) != 0):
        result, output_str = run_command(cmd)
        log.log_error("try it again: " + result)


def main():
    # PoE global configuration
    # preemptive priority
    tmp_file = "/etc/sonic/poe_preemptive_priority_tmp"
    cfg_file = "/etc/sonic/poe_preemptive_priority"

    if os.path.exists(cfg_file):
        value = 0
        if not os.path.exists(tmp_file):
            with open(tmp_file, 'w') as new_file:
                new_file.write("poe_preemptive_priority_tmp")
    else:
        value = 1
        if os.path.exists(tmp_file):
            # Delete the the file if it exists
            os.remove(tmp_file)

    IGNORANCE_MASK = '0'
    cmd = "sudo poetool system set_idv_mask %s %s" % (IGNORANCE_MASK, value)
    # print(cmd)
    result, output_str = run_command(cmd)
    if (result == None or int(result) != 0):
        result, output_str = run_command(cmd)
        log.log_error("try it again: " + result)

    # redundant mode
    tmp_file = "/etc/sonic/poe_redundant_tmp"
    cfg_file = "/etc/sonic/poe_redundant"

    if os.path.exists(cfg_file):
        value = 0
        if not os.path.exists(tmp_file):
            with open(tmp_file, 'w') as new_file:
                new_file.write("poe_redundant_tmp")
    else:
        value = 1
        if os.path.exists(tmp_file):
            # Delete the the file if it exists
            os.remove(tmp_file)

    FILE_PATH = "/sys/bus/i2c/devices/0-0033/psu_budget_mode"
    if not os.path.exists(FILE_PATH):
        log.log_error("FILE_PATH is not exsiting " + FILE_PATH)

    cmd = "sudo echo  %s >> %s" % (value, FILE_PATH)
    try:
        result = subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT, universal_newlines=True)
        # print(result.strip())
    except subprocess.CalledProcessError as e:
        log.log_error("Error running run_power_redundant_cmd command:" + e)

    # PoE port configuration
    CONFIG_DB_FILE = '/etc/sonic/config_db.json'
    with open(CONFIG_DB_FILE) as json_file:
        # Load the contents of the file
        data = json.load(json_file)

    port_dict = data['PORT']
    # config_db = ConfigDBConnector()
    # config_db.connect()
    # port_dict = config_db.get_table('PORT')
    # print(port_dict)
    # command = "poeutil port-priority Ethernet0 low"

    for key in port_dict:
        if port_dict[key]["poe_pri"] == "NA":
            continue  # Skips the non-poe ports

        # command = "poeutil port-priority %s %s" % (
        #    key, port_dict[key]["poe_pri"])
        # clicommon.run_command(command)
        # command = "poeutil power-inline %s %s %s" % (
        #    key, port_dict[key]["poe_status"], port_dict[key]["poe_maxpower"])
        # clicommon.run_command(command)

        port_num = port_dict[key]["lanes"]
        power_mode = port_dict[key]["poe_status"]
        max_power = int(float(port_dict[key]["poe_maxpower"]) * 10)

        pure_power_mode = "enable" if (power_mode == "enableDynamic" or
                                       power_mode == "enableStatic" or
                                       power_mode == "enable") else "disable"

        pure_power_calculation = "static" if (power_mode == "enableStatic" or
                                              power_mode == "disableStatic") else "disable"

        if pure_power_calculation == "static":
            cfg2 = 1
        else:
            cfg2 = 0

        if (int(port_num) <= 31):  # AT port, oper_mode is 9
            oper_mode = "9"
        else:
            oper_mode = "0"  # BT port, oper_mode is 0
        add_power = 0

        if (pure_power_mode == "enable" and int(max_power) != 0):
            cfg1 = 1
            oper_mode = 0x31  # According to 3.3.12 Set BT Port Reserve Power Request
        elif (pure_power_mode == "enable"):
            cfg1 = 1
        elif (pure_power_mode == "disable"):
            cfg1 = 0

        pri = get_priority_value(port_dict[key]["poe_pri"])

        cmd = "poetool port set_port_params %s %s %s %s %s %s" % (
            port_num, cfg1, cfg2, oper_mode, add_power, pri)
        result, output_str = run_command(cmd)
        if (result == None or int(result) != 0):
            result, output_str = run_command(cmd)
            log.log_error("try it again: " + result)

        if (pure_power_mode == "enable" and int(max_power) != 0):
            reserve_power = max_power
            pse_type = 1
            cmd = "poetool port set_port_reserve_power %s %s %s" % (
                port_num, reserve_power, pse_type)
            result, output_str = run_command(cmd)
            if (result == None or int(result) != 0):
                result, output_str = run_command(cmd)
                log.log_error("try it again: " + result)

            # print("cmd: ")
            # print(cmd)
            # print("result: ")
            # print(result)

        # print(key, port_dict[key]["poe_pri"])

    # Save setting and rest to make poe priority work properly
    time.sleep(1)
    cmd = "poetool system save_setting"
    result, output_str = run_command(cmd)
    if (result == None or int(result) != 0):
        result, output_str = run_command(cmd)
        log.log_error("try it again: " + result)
    time.sleep(2)
    cmd = "poetool system reset"
    result, output_str = run_command(cmd)
    if (result == None or int(result) != 0):
        result, output_str = run_command(cmd)
        log.log_error("try it again: " + result)

    while True:
        key = "Ethernet0"
        result = run_lldpcli_show_port_command(key)
        # print(result)
        match = parse_keyword(key, result)

        if match is not None:
            # print("Function returned:", result)
            time.sleep(2)
            break
        else:
            # print("Function returned None. Retrying in 5 seconds...")
            time.sleep(5)

    for key in port_dict:
        if port_dict[key]["poe_pri"] == "NA":
            continue  # Skips the non-poe ports

        state = port_dict[key]["poe_tlv"]
        # print(state)
        if (state == 'enable'):
            enabled = 'enabled'
        else:
            enabled = ''

        while True:
            result = run_lldpcli_config_port_command(key, enabled)
            # print(result)
            if result is not None:
                # print("Function returned:", result)
                break
            else:
                # print("Function returned None. Retrying in 5 seconds...")
                time.sleep(5)


if __name__ == '__main__':
    # power()
    main()
