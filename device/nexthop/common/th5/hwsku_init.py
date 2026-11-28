import os
import sys
import logging

from jinja2 import Environment, FileSystemLoader
from sonic_py_common import syslogger

SYSLOG_IDENTIFIER = "hwsku-init"
LOG = syslogger.SysLogger(
    SYSLOG_IDENTIFIER, enable_runtime_config=False, log_level=logging.DEBUG
)

_syslog_log = LOG.log


def _log_with_console(priority, msg, also_print_to_console=False):
    return _syslog_log(priority, msg, also_print_to_console=True)


LOG.log = _log_with_console

CHIP_CONFIG_TEMPLATE = "chip_configuration.yaml.j2"
CHIP_CONFIG_DEFAULT = os.path.join("defaults", "chip_configuration.yaml")
PORT_CONFIG_FILE = "port_configuration.yaml"
SAI_PROFILE_FILE = "sai.profile"


def _get_output_name(base_dir):
    """Derive the output filename from sai.profile."""
    sai_profile_path = os.path.join(base_dir, SAI_PROFILE_FILE)
    with open(sai_profile_path) as f:
        for line in f:
            if line.startswith("SAI_INIT_CONFIG_FILE="):
                return os.path.basename(line.strip().split("=", 1)[1])
    raise RuntimeError(f"SAI_INIT_CONFIG_FILE not found in {sai_profile_path}")


def _stitch(sku_content, chip_content):
    """Concatenate per-SKU YAML with chip config."""
    if not sku_content.endswith("\n"):
        sku_content += "\n"
    return sku_content + chip_content


def run(base_dir):
    output_name = _get_output_name(base_dir)
    output_path = os.path.join(base_dir, output_name)
    port_config_path = os.path.join(base_dir, PORT_CONFIG_FILE)

    with open(port_config_path) as f:
        sku_content = f.read()

    try:
        # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
        env = Environment(loader=FileSystemLoader(base_dir))
        chip_content = env.get_template(CHIP_CONFIG_TEMPLATE).render()

    except Exception as e:
        LOG.log_error(f"Error generating {output_name}: {e}")
        LOG.log_info("Falling back to defaults...")
        with open(os.path.join(base_dir, CHIP_CONFIG_DEFAULT)) as f:
            chip_content = f.read()

    with open(output_path, "w") as f:
        f.write(_stitch(sku_content, chip_content))

    LOG.log_info(f"Generated {output_name}")


def __main__():
    if len(sys.argv) > 2:
        print(
            f"Usage: {sys.argv[0]} [base_dir]", file=sys.stderr
        )
        sys.exit(1)

    base_dir = sys.argv[1] if len(sys.argv) >= 2 else os.path.dirname(
        os.path.abspath(__file__)
    )

    run(base_dir)


if __name__ == "__main__":
    __main__()
