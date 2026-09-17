import logging
import os
import sys

from sonic_py_common import syslogger

SYSLOG_IDENTIFIER = "hwsku-init"
LOG = syslogger.SysLogger(
    SYSLOG_IDENTIFIER, enable_runtime_config=False, log_level=logging.INFO
)

# hwsku-init redirects stdout to /var/log/hwsku-init.log, so every message is
# logged with also_print_to_console to land in both that file and syslog.

TH6_CONFIG = "th6.yaml"
PORT_CONFIG_FILE = "port_configuration.yaml"
SAI_PROFILE_FILE = "sai.profile"

# Each TH6 platform has a `th6` symlink to device/nexthop/common/th6, so the
# chip config is present inside the platform mount in swss.
TH6_DIR = "/usr/share/sonic/platform/th6"


def _get_output_name(sai_profile_path):
    """Derive the output filename from sai.profile."""
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
    """Generate the SAI init config. Returns False if none is available."""
    sai_profile_path = os.path.join(base_dir, SAI_PROFILE_FILE)
    output_path = None

    try:
        output_path = os.path.join(base_dir, _get_output_name(sai_profile_path))

        with open(os.path.join(base_dir, PORT_CONFIG_FILE)) as f:
            sku_content = f.read()
        with open(os.path.join(TH6_DIR, TH6_CONFIG)) as f:
            th6_content = f.read()

        # Write via a temporary file so a failed run leaves any previously
        # generated config intact to fall back on.
        tmp_path = f"{output_path}.tmp"
        with open(tmp_path, "w") as f:
            f.write(_stitch(sku_content, th6_content))
        os.replace(tmp_path, output_path)
    except Exception as e:
        LOG.log_warning(
            f"Failed to generate the SAI init config: {e}",
            also_print_to_console=True,
        )

        if output_path and os.path.isfile(output_path):
            LOG.log_warning(
                f"Falling back to the previously generated {output_path}, "
                "which may not match this image",
                also_print_to_console=True,
            )
            return True

        LOG.log_error(
            "No SAI init config is available: syncd will fail to start because "
            f"SAI_INIT_CONFIG_FILE in {sai_profile_path} names a file that does "
            "not exist",
            also_print_to_console=True,
        )
        return False

    LOG.log_info(f"Generated SAI init config {output_path}", also_print_to_console=True)
    return True


def __main__():
    if len(sys.argv) > 2:
        print(f"Usage: {sys.argv[0]} [base_dir]", file=sys.stderr)
        sys.exit(1)

    base_dir = (
        sys.argv[1]
        if len(sys.argv) == 2
        else os.path.dirname(os.path.abspath(__file__))
    )

    sys.exit(0 if run(base_dir) else 1)


if __name__ == "__main__":
    __main__()
