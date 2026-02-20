#!/usr/bin/env python3

import subprocess
import sys
import syslog
import yaml

PLATFORM_FOLDER = "/usr/share/sonic/platform"


def get_cmd_output(cmd: str) -> str:
    result = subprocess.run(["/bin/bash", "-c", cmd], capture_output=True)
    if result.returncode != 0:
        syslog.syslog(syslog.LOG_ERR, f"{cmd} -- command failed")
        sys.exit(1)

    return result.stdout.decode("utf-8").strip()


def get_pcie_variables(vars_filepath) -> dict[str, str]:
    """
    Reads a yaml file containing a list of variables in the format of (name, lookup_command) pairs.

    For example:
    - name: "foo_bus"
      lookup_command: "setpci -s 00:02.1 0x19.b"
    - name: "bar_bus"
      lookup_command: "echo 'e5'"
    - name: "baz_bdf"
      lookup_command: "setpci -s 00:02.2 0x19.b | xargs printf '0000:%s:00.0'"

    Returns a dict mapping name to the output of the lookup_command. These variables are intended
    to be used for feeding the jinja2 templates, e.g. pddf-device.json.j2 and pcie.yaml.j2, as PCIe
    buses of the devices behind root ports can only be determined after boot.
    """
    result = dict()

    with open(vars_filepath, "r") as f:
        config = yaml.safe_load(f)
        for entry in config:
            name = entry.get("name")
            command = entry.get("lookup_command")
            if not name or not command:
                syslog.syslog(
                    syslog.LOG_ERR,
                    f"{vars_filepath} -- invalid format: each entry must contain 'name' and 'lookup_command'",
                )
                exit(1)
            elif name in result:
                syslog.syslog(
                    syslog.LOG_ERR,
                    f"{vars_filepath} -- duplicate variable name '{name}'",
                )
                exit(1)
            value = get_cmd_output(command)
            result[name] = value
    return result


def get_cpu_card_fpga_bdf() -> str | None:
    return get_pcie_variables(f"{PLATFORM_FOLDER}/pcie-variables.yaml").get(
        "cpu_card_fpga_bdf"
    )


def get_switchcard_fpga_bdf() -> str | None:
    return get_pcie_variables(f"{PLATFORM_FOLDER}/pcie-variables.yaml").get(
        "switchcard_fpga_bdf"
    )
