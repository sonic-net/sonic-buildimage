#!/usr/bin/env python


import subprocess
import json
import time
from swsscommon.swsscommon import ConfigDBConnector
from sonic_py_common import logger

SYSLOG_IDENTIFIER = "poelldp"
log = logger.Logger(SYSLOG_IDENTIFIER)

# Global dictionary to store power data for each port
port_power_data = {}
allocated_values = {}
requested_values = {}
lldp_power_sm = {}

# PoE configuration  - Customziation
# key is logical interface name
# value 1 is the PoE physical port
# value 2 is the PoE type(NA/AT/BT)
poe_config_dict = {
    "Ethernet0": ["0", "AT"],
    "Ethernet1": ["1", "AT"],
    "Ethernet2": ["2", "AT"],
    "Ethernet3": ["3", "AT"],
    "Ethernet4": ["4", "AT"],
    "Ethernet5": ["5", "AT"],
    "Ethernet6": ["6", "AT"],
    "Ethernet7": ["7", "AT"],
    "Ethernet8": ["8", "AT"],
    "Ethernet9": ["9", "AT"],
    "Ethernet10": ["10", "AT"],
    "Ethernet11": ["11", "AT"],
    "Ethernet12": ["12", "AT"],
    "Ethernet13": ["13", "AT"],
    "Ethernet14": ["14", "AT"],
    "Ethernet15": ["15", "AT"],
    "Ethernet16": ["16", "AT"],
    "Ethernet17": ["17", "AT"],
    "Ethernet18": ["18", "AT"],
    "Ethernet19": ["19", "AT"],
    "Ethernet20": ["20", "AT"],
    "Ethernet21": ["21", "AT"],
    "Ethernet22": ["22", "AT"],
    "Ethernet23": ["23", "AT"],
    "Ethernet24": ["24", "AT"],
    "Ethernet25": ["25", "AT"],
    "Ethernet26": ["26", "AT"],
    "Ethernet27": ["27", "AT"],
    "Ethernet28": ["28", "AT"],
    "Ethernet29": ["29", "AT"],
    "Ethernet30": ["30", "AT"],
    "Ethernet31": ["31", "AT"],
    "Ethernet32": ["32", "BT"],
    "Ethernet33": ["33", "BT"],
    "Ethernet34": ["34", "BT"],
    "Ethernet35": ["35", "BT"],
    "Ethernet36": ["36", "BT"],
    "Ethernet37": ["37", "BT"],
    "Ethernet38": ["38", "BT"],
    "Ethernet39": ["39", "BT"],
    "Ethernet40": ["40", "BT"],
    "Ethernet41": ["41", "BT"],
    "Ethernet42": ["42", "BT"],
    "Ethernet43": ["43", "BT"],
    "Ethernet44": ["44", "BT"],
    "Ethernet45": ["45", "BT"],
    "Ethernet46": ["46", "BT"],
    "Ethernet47": ["47", "BT"],
    "Ethernet48": ["48", "NA"],
    "Ethernet49": ["49", "NA"],
    "Ethernet50": ["50", "NA"],
    "Ethernet51": ["51", "NA"],
    "Ethernet52": ["52", "NA"],
    "Ethernet53": ["53", "NA"],
}


def run_lldpcli_neighbor_command():
    try:
        result = subprocess.run(['lldpcli', '-f', 'json', 'show',
                                'ne', 'de'], capture_output=True, text=True, check=True)
        return result.stdout
    except subprocess.CalledProcessError as e:
        print("Error running run_lldpcli_neighbor_command command:", e)
        return None


def run_lldpcli_config_port_command(interface, pri, req, alloc):
    # lldpcli configure port Etherent40 dot3 power pse supported enabled powerpairs signal class class-4 type 2  source primary priority low requested 28000 allocated 15000

    cmd = "lldpcli configure port %s dot3 power pse supported enabled powerpairs signal class class-4 type 2  source primary priority %s requested %s allocated %s" % (
        interface, pri, req, alloc)
    # print(cmd)
    try:
        result = subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT, universal_newlines=True)
        # print(result.strip())
        return result.strip()
    except subprocess.CalledProcessError as e:
        print("Error running run_lldpcli_neighbor_command command:", e)
        return None


def run_command(command):
    try:
        output = subprocess.check_output(command.split(), stderr=subprocess.STDOUT, universal_newlines=True)
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
            print("Unable to find the result in the output.")
            return None, None

    # click.echo("output: {}\n".format(output.strip()))
    return result, output.strip()


def run_port_lldp_pd_req_cmd(port_name, single, dual_a, dual_b, auto_class, cable_len, l2cfg):

    # Here is the command to set port params
    # poetool port set_lldp_pd_req <PortNum> <PDReqPowerSingle> <PDReqPowerDualA> <PDReqPowerDualB> <Autoclass> <CableLength> <L2CFG>

    port_num = poe_config_dict[port_name][0]

    cmd = "sudo poetool port set_lldp_pd_req %s %s %s %s %s %s %s" % (
        port_num, single, dual_a, dual_b, auto_class, cable_len, l2cfg)
    # print(cmd)
    result, output_str = run_command(cmd)
    # click.echo("result: {}\n".format(result))
    return result, output_str


def run_port_port_status_cmd(port_name):

    # Here is the command to get port poe status
    # admin@sonic:/usr/local/bin$ sudo poetool port get_lldp_pse_data 2
    # Code 69 get_lldp_pse_data, result 0
    # pseAllocatedPwrSingleOrAltA  255
    # pseAllocatedPwrAltB  0
    # pseMaxPwr  300
    # assignedClassPrimary  4
    # assignedClassSecondary  12
    # layer2Execution  0
    # layer2Usage  1
    # ieeeBTPwrBitsExt1514  1
    # ieeeBTPwrBitsExt1110  1
    # cableLength  10
    # layer2_cfg  1

    port_num = poe_config_dict[port_name][0]

    cmd = "sudo poetool port get_lldp_pse_data %s" % (port_num)
    # print(cmd)
    result, output_str = run_command(cmd)
    # click.echo("result: {}\n".format(result))
    return result, output_str


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
            print("Unable to find the result in the output.")
            return None

        # click.echo("string: {}\n".format(string.strip()))
        return result
    else:
        return None


def parse_lldpcli_output(output):
    try:
        # open DB
        config_db = ConfigDBConnector()
        config_db.connect()
        port_dict = config_db.get_table('PORT')

        lldp_data = json.loads(output)
        interfaces = lldp_data['lldp']['interface']

        # print("lldp_debug: {}".format(lldp_debug))

        # Store dictionary keys in a list
        key_list = list(lldp_power_sm.keys())

        for key in key_list:
            if (lldp_debug != "0"):
                print("key: {} value: {}".format(key, lldp_power_sm[key]))
            is_found = 0
            for interface in interfaces:
                # print("interface: {}".format(interface))a
                if key in interface:
                    # print("xxxx")
                    is_found = 1
                    break
            # reset all parameters
            if (is_found == 0):
                del lldp_power_sm[key]
                if key in allocated_values:
                    del allocated_values[key]
                if key in requested_values:
                    del requested_values[key]
                if (lldp_debug != "0"):
                    print("reset intf: {}".format(key))

        for interface in interfaces:
            if (lldp_debug != "0"):
                # print(interface)
                pass
            for intf in interface:
                if (lldp_debug != "0"):
                    print(intf)
                if intf not in poe_config_dict:
                    if (lldp_debug != "0"):
                        print("intf not in poe_config_dict")
                    pass
                elif intf == "eth0":
                    if (lldp_debug != "0"):
                        print("intf == eth0")
                    pass
                elif port_dict[intf]["poe_status"] != "enableDynamic" and port_dict[intf]["poe_status"] != "enableStatic":
                    if (lldp_debug != "0"):
                        print("port_dict[intf][poe_status] is noe eanble")
                    pass  # Skip the poe-disabled ports
                elif port_dict[intf]["poe_tlv"] != "enable":
                    if (lldp_debug != "0"):
                        print("port_dict[intf][poe_tlv] is noe eanble")
                    pass  # Skip the poe_tlv-disabled ports
                elif port_dict[intf]["poe_maxpower"] != "0" and port_dict[intf]["poe_maxpower"] != "0.0":
                    if (lldp_debug != "0"):
                        print("port_dict[intf][poe_maxpower] is noe zero")
                    pass  # Skip the static-power ports
                else:
                    if 'port' not in interface[intf]:
                        if (lldp_debug != "0"):
                            print("port not in interface[intf]")
                        pass

                        port_power_data[intf] = {
                            'supported': False,
                            'enabled': False,
                            'device-type': '',
                            'paircontrol': False,
                            'pairs': '',
                            'class': '',
                            'power-type': '',
                            'source': '',
                            'priority': '',
                            'requested': '0',
                            'allocated': ''
                        }
                    elif 'power' not in interface[intf]['port']:
                        if (lldp_debug != "0"):
                            print("power not in interface[intf]")
                        pass

                        port_power_data[intf] = {
                            'supported': False,
                            'enabled': False,
                            'device-type': '',
                            'paircontrol': False,
                            'pairs': '',
                            'class': '',
                            'power-type': '',
                            'source': '',
                            'priority': '',
                            'requested': '0',
                            'allocated': ''
                        }
                    else:
                        # print(interface)
                        port_data = interface[intf]['port']['power']
                        # print("port_data: {}", port_data)
                        port_power_data[intf] = {
                            'supported': port_data.get('supported', False),
                            'enabled': port_data.get('enabled', False),
                            'device-type': port_data.get('device-type', ''),
                            'paircontrol': port_data.get('paircontrol', False),
                            'pairs': port_data.get('pairs', ''),
                            'class': port_data.get('class', ''),
                            'power-type': port_data.get('power-type', ''),
                            'source': port_data.get('source', ''),
                            'priority': port_data.get('priority', ''),
                            'requested': port_data.get('requested', '0'),
                            'allocated': port_data.get('allocated', '')
                        }

                    if (port_power_data[intf]['requested'] == '0'):
                        # no requested power, nothing to do
                        if (lldp_debug != "0"):
                            print("power reuqest is zero")
                        pass

                        # Check the poe attribute in lldp-med
                        if 'lldp-med' not in interface[intf]:
                            if (lldp_debug != "0"):
                                print("lldp-med not in interface[intf]")
                            continue
                        elif 'poe' not in interface[intf]['lldp-med']:
                            if (lldp_debug != "0"):
                                print("lldp-med not in interface[intf]")
                            continue
                        else:
                            lldp_med_poe = interface[intf]['lldp-med']['poe']

                            port_power_data[intf]['device-type'] = lldp_med_poe['device-type']
                            port_power_data[intf]['requested'] = lldp_med_poe['power']

                    port_name = intf
                    single = int(
                        int(port_power_data[intf]['requested'])/100)

                    if port_power_data[intf]['device-type'] != 'PD':
                        if (lldp_debug != "0"):
                            print("not a PD")
                        pass  # Support PD only as the switch is a PSE
                    elif intf in allocated_values and allocated_values[intf] == port_power_data[intf]['requested']:
                        if (lldp_debug != "0"):
                            print(" Bingo: {}", intf)
                        pass  # The request has been handled
                        # log.log_error(" Bingo: {}", intf)
                    elif intf in requested_values and requested_values[intf] == port_power_data[intf]['requested'] and lldp_power_sm[intf] == 0:
                        if (lldp_debug != "0"):
                            print(" Bingo2: {}".format(intf))
                        pass  # The request has been handled
                        # log.log_error(" Bingo2: {}", intf)
                    else:
                        # 1. call poetool to requst power
                        requested_values[intf] = port_power_data[intf]['requested']
                        if (intf not in lldp_power_sm or lldp_power_sm[intf] == 0):
                            # print("0xffff")
                            lldp_power_sm[intf] = 1
                            port_name = intf
                            # the requested power in lldpd is milliwatt(0.001w) and in poetool is 0.1w
                            single = int(
                                int(port_power_data[intf]['requested'])/100)
                            dual_a = 0
                            dual_b = 0
                            auto_class = 0
                            cable_len = 0
                            l2cfg = 0xff

                            result, output_str = run_port_lldp_pd_req_cmd(
                                port_name, single, dual_a, dual_b, auto_class, cable_len, l2cfg)

                            if result is None or int(result) != 0:
                                print("run_port_lldp_pd_req_cmd fails")
                                continue
                            else:
                                pass

                        # 2. call poetool to cofirm alloacted power
                        if (lldp_power_sm[intf] == 1):
                            lldp_power_sm[intf] = 2
                            pass
                        else:
                            result, output_str = run_port_port_status_cmd(
                                port_name)
                            # print(result)
                            # print(output_str)
                            if (result == None):
                                print(port_name)
                                result, output_str = run_port_port_status_cmd(
                                    port_name)
                                print("try again: ")
                                print(result)
                                continue

                            keyword = "pseAllocatedPwrSingleOrAltA"
                            allocated_power = parse_keyword(
                                keyword, output_str)
                            # print(allocated_power)
                            keyword = "layer2Execution"
                            layer2Execution = parse_keyword(
                                keyword, output_str)
                            # print(layer2Execution)
                            keyword = "layer2Usage"
                            layer2Usage = parse_keyword(keyword, output_str)
                            # print(layer2Usage)

                            # print(single)

                            if (layer2Usage == '1' or layer2Usage == '2' or layer2Usage == '3' or layer2Usage == '4'):
                                # print("xxx")
                                # 3. update allocated power to lldpd
                                port_pri = port_dict[intf]["poe_pri"]
                                run_lldpcli_config_port_command(
                                    intf, port_pri, int(single) * 100, int(allocated_power) * 100)
                                # 4. Save allocated value to the global variable
                                allocated_values[intf] = allocated_power
                                lldp_power_sm[intf] = 0
                                pass
                            else:
                                # print("yyy")
                                pass

    except (KeyError, json.JSONDecodeError) as e:
        print("Error parsing lldpcli output:", e)


if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        lldp_debug = sys.argv[1]
    else:
        lldp_debug = "0"

    while True:
        # log.log_error("while True")
        lldpcli_output = run_lldpcli_neighbor_command()
        if lldpcli_output:
            # log.log_error("parse_lldpcli_output")
            parse_lldpcli_output(lldpcli_output)
            if (lldp_debug != "0"):
                # print(port_power_data)
                print(allocated_values)
                print("=============")
        time.sleep(2)
        # log.log_error("time sleep")
