import os
import sys
import logging

from jinja2 import Environment, FileSystemLoader, select_autoescape
from swsscommon import swsscommon
from sonic_py_common import syslogger

SYSLOG_IDENTIFIER = "hwsku-init"
LOG = syslogger.SysLogger(
    SYSLOG_IDENTIFIER, enable_runtime_config=False, log_level=logging.DEBUG
)

_syslog_log = LOG.log


def _log_with_console(priority, msg, also_print_to_console=False):
    return _syslog_log(priority, msg, also_print_to_console=True)


LOG.log = _log_with_console

TH6_TEMPLATE = "th6.yaml.j2"
PORT_CONFIG_FILE = "port_configuration.yaml"
SAI_PROFILE_FILE = "sai.profile"

# Each TH6 platform has a `th6` symlink to device/nexthop/common/th6, so the
# templates and defaults are present inside the platform mount in swss.
TH6_DIR = "/usr/share/sonic/platform/th6"


def has_vxlan_tunnel():
    """Check if VXLAN_TUNNEL is configured in CONFIG_DB.

    Returns:
        True if VXLAN_TUNNEL table exists and has entries, False otherwise.
    """
    db = swsscommon.DBConnector("CONFIG_DB", 0)
    tbl = swsscommon.Table(db, "VXLAN_TUNNEL")
    keys = tbl.getKeys()
    result = len(keys) > 0
    LOG.log_info(
        f"VXLAN_TUNNEL from CONFIG_DB: "
        f"{'present' if result else 'absent'} ({len(keys)} entries)"
    )
    return result


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
    if not os.path.isdir(TH6_DIR):
        raise RuntimeError(f"th6 directory not found at {TH6_DIR}")
    default_th6_path = os.path.join(TH6_DIR, "defaults", "th6.yaml")

    output_name = _get_output_name(base_dir)
    output_path = os.path.join(base_dir, output_name)
    port_config_path = os.path.join(base_dir, PORT_CONFIG_FILE)

    with open(port_config_path) as f:
        sku_content = f.read()

    try:
        # Determine template variables from CONFIG_DB
        variables = {}
        try:
            vxlan_present = has_vxlan_tunnel()
        except Exception as e:
            LOG.log_warning(
                f"Failed to read VXLAN_TUNNEL from CONFIG_DB: {e}, "
                "assuming no VXLAN"
            )
            vxlan_present = False
        if not vxlan_present:
            variables["sai_stats_disable_mask"] = "0x200"

        # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
        env = Environment(
            loader=FileSystemLoader(TH6_DIR),
            autoescape=select_autoescape(["html", "xml"]),
        )
        th6_content = env.get_template(TH6_TEMPLATE).render(**variables)

    except Exception as e:
        LOG.log_error(f"Error generating {output_name}: {e}")
        LOG.log_info("Falling back to defaults...")
        with open(default_th6_path) as f:
            th6_content = f.read()

    with open(output_path, "w") as f:
        f.write(_stitch(sku_content, th6_content))

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
