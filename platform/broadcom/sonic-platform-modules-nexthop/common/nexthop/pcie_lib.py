#!/usr/bin/env python3

import functools
import subprocess
import yaml

PLATFORM_FOLDER = "/usr/share/sonic/platform"


@functools.cache
def get_cmd_output(cmd: str) -> str:
    result = subprocess.run(["/bin/bash", "-c", cmd], capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(f"'{cmd}' -- command failed")

    return result.stdout.decode("utf-8").strip()


def get_var_name_to_cmd_map(vars_filepath) -> dict[str, str]:
    """
    Reads a yaml file containing a list of variables in the format of (name, lookup_command) pairs.

    For example:
    - name: "foo_bus"
      lookup_command: "setpci -s 00:02.1 0x19.b"
    - name: "bar_bus"
      lookup_command: "echo 'e5'"
    - name: "baz_bdf"
      lookup_command: "setpci -s 00:02.2 0x19.b | xargs printf '0000:%s:00.0'"

    Returns a dict mapping the variable name to the lookup_command.
    """
    result = dict()

    with open(vars_filepath, "r") as f:
        config = yaml.safe_load(f)
        for entry in config:
            name = entry.get("name")
            cmd = entry.get("lookup_command")
            if not name or not cmd:
                raise ValueError(
                    f"{vars_filepath} -- invalid format: each entry must contain 'name' and 'lookup_command'"
                )
            elif name in result:
                raise ValueError(f"{vars_filepath} -- duplicate variable name '{name}'")
            result[name] = cmd

    return result


def get_pcie_variables(vars_filepath, vars_to_get: set[str] | None = None) -> dict[str, str]:
    """
    Reads a yaml file containing a list of variables in the format of (name, lookup_command) pairs.

    For example:
    - name: "foo_bus"
      lookup_command: "setpci -s 00:02.1 0x19.b"
    - name: "bar_bus"
      lookup_command: "echo 'e5'"
    - name: "baz_bdf"
      lookup_command: "setpci -s 00:02.2 0x19.b | xargs printf '0000:%s:00.0'"

    Returns a dict mapping the variable name to the output of the lookup_command.
    If `vars_to_get` is provided, only returns the variables in `vars_to_get`.
    Otherwise, returns all variables.

    These variables are intended to be used for feeding the jinja2 templates,
    e.g. pddf-device.json.j2 and pcie.yaml.j2, as PCIe buses of the devices
    behind root ports can only be determined after boot.
    """
    all_vars = get_var_name_to_cmd_map(vars_filepath)

    return {
        name: get_cmd_output(cmd)
        for name, cmd in all_vars.items() 
        if vars_to_get is None or name in vars_to_get
    }


def get_cpu_card_fpga_bdf(vars_filepath=f"{PLATFORM_FOLDER}/pcie-variables.yaml") -> str | None:
    return get_pcie_variables(vars_filepath, vars_to_get={"cpu_card_fpga_bdf"}).get(
        "cpu_card_fpga_bdf"
    )


def get_switchcard_fpga_bdf(vars_filepath=f"{PLATFORM_FOLDER}/pcie-variables.yaml") -> str | None:
    return get_pcie_variables(vars_filepath, vars_to_get={"switchcard_fpga_bdf"}).get(
        "switchcard_fpga_bdf"
    )
