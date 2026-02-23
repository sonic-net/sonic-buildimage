#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import click
import jinja2
import json
import os
import syslog

from nexthop import pcie_lib

PLATFORM_FOLDER = "/usr/share/sonic/platform"
PDDF_FOLDER = "/usr/share/sonic/platform/pddf"

DEFAULT_PCIE_VARS_FILEPATH = f"{PLATFORM_FOLDER}/pcie-variables.yaml"
DEFAULT_PLATFORM_JSON_FILEPATH = f"{PLATFORM_FOLDER}/platform.json"

DEFAULT_PDDF_DEVICE_JSON_TEMPLATE_FILEPATH = f"{PDDF_FOLDER}/pddf-device.json.j2"
DEFAULT_PDDF_DEVICE_JSON_OUTPUT_FILEPATH = f"{PDDF_FOLDER}/pddf-device.json"

DEFAULT_PCIE_YAML_TEMPLATE_FILEPATH = f"{PLATFORM_FOLDER}/pcie.yaml.j2"
DEFAULT_PCIE_YAML_OUTPUT_FILEPATH = f"{PLATFORM_FOLDER}/pcie.yaml"


def check_file_exists_if_not_default(filepath: str, default_filepath: str, param_name: str):
    if filepath != default_filepath and not os.path.isfile(filepath):
        raise click.BadParameter(f"{filepath} does not exist.", param_hint=param_name)

def generate_file_from_jinja2_template(
    template_filepath: str, variables: dict[str, str], output_filepath: str
):
    loader = jinja2.FileSystemLoader(os.path.dirname(template_filepath))
    # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
    j2_env = jinja2.Environment(loader=loader, keep_trailing_newline=True)
    template = j2_env.get_template(os.path.basename(template_filepath))
    # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
    output = template.render(variables)
    with open(output_filepath, "w") as f:
        f.write(output)
    syslog.syslog(syslog.LOG_INFO, f"Successfully generated {output_filepath}")

def get_model_name(platform_json_filepath):
    try:
        with open(platform_json_filepath, 'r') as file:
            platform_data = json.load(file)
            return platform_data["chassis"]["name"]
    except FileNotFoundError:
        syslog.syslog(syslog.LOG_ERR, f"Error: Platform JSON file not found at {platform_json_filepath}")
        return None
    except json.JSONDecodeError:
        syslog.syslog(syslog.LOG_ERR, f"Error: Invalid JSON format in {platform_json_filepath}")
        return None
    except KeyError as e:
        syslog.syslog(syslog.LOG_ERR, f"Error: Missing key {e} in platform JSON data from {platform_json_filepath}")
        return None


@click.group()
def cli():
    pass


# Computes variables from pcie-variables.yaml and feeds them to
# pddf-device.json.j2 to generate pddf-device.json.
#
# Original motivation: pddf-device.json contains hardcoded PCIe
# address of each FPGA, but the PCIe address of each FPGA can only be
# determined after boot. Therefore we use pcie-variables.yaml to get variables
# at runtime and fill them in pddf-device.json.j2 template.
@cli.command("pddf_device_json")
@click.option(
    "--template_filepath",
    type=click.Path(exists=False),
    default=DEFAULT_PDDF_DEVICE_JSON_TEMPLATE_FILEPATH,
    help="Filepath to the jinja2 template for generating pddf-device.json.",
)
@click.option(
    "--vars_filepath",
    type=click.Path(exists=False),
    default=DEFAULT_PCIE_VARS_FILEPATH,
    help="Filepath to the yaml file containing variables to be substituted in the template.",
)
@click.option(
    "--platform_json_filepath",
    type=click.Path(exists=False),
    default=DEFAULT_PLATFORM_JSON_FILEPATH,
    help="Filepath to the platform.json file.",
)
@click.option(
    "--output_filepath",
    type=click.Path(exists=False),
    default=DEFAULT_PDDF_DEVICE_JSON_OUTPUT_FILEPATH,
    help="Filepath to store the generated pddf-device.json. If the file already exists, it will be overwritten.",
)
def pddf_device_json(template_filepath, vars_filepath, platform_json_filepath, output_filepath):
    check_file_exists_if_not_default(template_filepath, DEFAULT_PDDF_DEVICE_JSON_TEMPLATE_FILEPATH, "--template_filepath")
    check_file_exists_if_not_default(vars_filepath, DEFAULT_PCIE_VARS_FILEPATH, "--vars_filepath")
    check_file_exists_if_not_default(platform_json_filepath, DEFAULT_PLATFORM_JSON_FILEPATH, "--platform_json_filepath")
    if not os.path.isfile(template_filepath) or not os.path.isfile(vars_filepath) or not os.path.isfile(platform_json_filepath):
        syslog.syslog(syslog.LOG_INFO, f"Skipping {output_filepath} generation")
        return
    vars = pcie_lib.get_pcie_variables(vars_filepath)

    model_name = get_model_name(platform_json_filepath)
    if model_name is None:
        syslog.syslog(syslog.LOG_ERR, f"Skipping {output_filepath} generation due to missing model name.")
        return

    vars["model_name"] = model_name
    generate_file_from_jinja2_template(template_filepath, vars, output_filepath)


# Computes variables from pcie-variables.yaml and feeds them to
# pcie.yaml.j2 to generate pcie.yaml.
#
# Original motivation: pcie.yaml contains hardcoded PCIe
# address of each device, but the PCIe bus of the devices behind
# root ports can only be determined after boot.Therefore we
# use pcie-variables.yaml to get variables at runtime and fill them
# in pcie.yaml.j2 template.
@cli.command("pcie_yaml")
@click.option(
    "--template_filepath",
    type=click.Path(exists=False),
    default=DEFAULT_PCIE_YAML_TEMPLATE_FILEPATH,
    help="Filepath to the jinja2 template for generating pcie.yaml.",
)
@click.option(
    "--vars_filepath",
    type=click.Path(exists=False),
    default=DEFAULT_PCIE_VARS_FILEPATH,
    help="Filepath to the yaml file containing variables to be substituted in the template.",
)
@click.option(
    "--output_filepath",
    type=click.Path(exists=False),
    default=DEFAULT_PCIE_YAML_OUTPUT_FILEPATH,
    help="Filepath to store the generated pcie.yaml. If the file already exists, it will be overwritten.",
)
def pcie_yaml(template_filepath, vars_filepath, output_filepath):
    check_file_exists_if_not_default(template_filepath, DEFAULT_PCIE_YAML_TEMPLATE_FILEPATH, "--template_filepath")
    check_file_exists_if_not_default(vars_filepath, DEFAULT_PCIE_VARS_FILEPATH, "--vars_filepath")
    if not os.path.isfile(template_filepath) or not os.path.isfile(vars_filepath):
        syslog.syslog(syslog.LOG_INFO, f"Skipping {output_filepath} generation")
        return
    vars = pcie_lib.get_pcie_variables(vars_filepath)
    generate_file_from_jinja2_template(template_filepath, vars, output_filepath)


if __name__ == "__main__":
    cli()
