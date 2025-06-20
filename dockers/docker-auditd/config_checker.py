#!/usr/bin/env python3

import json
import subprocess
import sys
from sonic_py_common import logger as log

logger = log.Logger()


# Configuration file paths
RULES_DIR = "/etc/audit/rules.d/"
SYSLOG_CONF = "/etc/audit/plugins.d/syslog.conf"
AUDIT_CONF = "/etc/audit/auditd.conf"
AUDIT_SERVICE = "/lib/systemd/system/auditd.service"
CONFIG_FILES = "/usr/share/sonic/auditd_config_files/"

# Expected hash values
CONFIG_HASHES = {
    "rules": {
        "default": "3042c67383dccff079755c5f3daa2820e2ca392a",
        "nokia": "fffb00199104c493d04a621b257c579c48d77225"
    },
    "auditd_conf": "7cdbd1450570c7c12bdc67115b46d9ae778cbd76"
}

# Command definitions
RULES_HASH_CMD = f"sh -c \"find {RULES_DIR} -name '*.rules' -type f | sort | xargs cat 2>/dev/null | sha1sum\""
AUDIT_CONF_HASH_CMD = "cat {} | sha1sum".format(AUDIT_CONF)


def run_command(cmd):
    logger.log_debug("Running command: {}".format(cmd))
    p = subprocess.Popen(cmd,
                         text=True,
                         shell=True, # nosemgrep
                         executable='/bin/bash',
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    output, error = p.communicate()
    if p.returncode == 0:
        return p.returncode, output
    logger.log_error("Command failed: {} (Return code: {})".format(cmd, p.returncode))
    logger.log_error("Error: {}".format(error))
    return p.returncode, error


def get_hwsku():
    hwsku_cmd = "sonic-cfggen -d -v DEVICE_METADATA.localhost.hwsku"
    rc, out = run_command(hwsku_cmd)
    return out.rstrip('\n')


def is_auditd_rules_configured():
    hwsku = get_hwsku()
    if "Nokia-7215" in hwsku or "Nokia-M0-7215" in hwsku:
        EXPECTED_HASH = CONFIG_HASHES["rules"]["nokia"]
    else:
        EXPECTED_HASH = CONFIG_HASHES["rules"]["default"]

    rc, out = run_command(RULES_HASH_CMD)
    is_configured = EXPECTED_HASH in out
    logger.log_info("auditd rules have {} configured (Hash: {})".format(
        "already" if is_configured else "not",
        out.strip())
    )
    return is_configured


def is_syslog_conf_configured():
    rc, out = run_command("grep '^active = yes' {}".format(SYSLOG_CONF))
    is_configured = rc == 0
    logger.log_info("syslog.conf has {} configured".format("already" if is_configured else "not"))
    return is_configured


def is_auditd_conf_configured():
    rc, out = run_command(AUDIT_CONF_HASH_CMD)
    is_configured = CONFIG_HASHES["auditd_conf"] in out
    logger.log_info("auditd.conf has {} configured (Hash: {})".format(
        "already" if is_configured else "not",
        out.strip())
    )
    return is_configured


def is_auditd_service_configured():
    rc, out = run_command("grep '^CPUQuota=10%' {}".format(AUDIT_SERVICE))
    is_configured = rc == 0
    logger.log_info("auditd.service has {} configured".format("already" if is_configured else "not"))
    return is_configured


def check_rules_syntax():
    logger.log_info("Checking auditd rules syntax...")
    rc, out = run_command("auditctl -R /etc/audit/audit.rules")
    if rc != 0:
        logger.log_error("auditctl -R failed: {}".format(out))
        return False
    logger.log_info("Auditd rules syntax check passed")
    return True


def main():
    is_configured = True
    hwsku = get_hwsku()

    # Check rules configuration
    if not is_auditd_rules_configured():
        logger.log_info("Updating auditd rules...")
        run_command("rm -f {}/*.rules".format(RULES_DIR))
        run_command("cp {}/*.rules {}".format(CONFIG_FILES, RULES_DIR))
        if "Nokia-7215" in hwsku or "Nokia-M0-7215" in hwsku:
            logger.log_info("Installing Nokia-specific rules")
            run_command("cp {}/32bit/*.rules {}".format(CONFIG_FILES, RULES_DIR))
        is_configured = False

    # Check syslog configuration
    if not is_syslog_conf_configured():
        logger.log_info("Updating syslog.conf...")
        run_command("sed -i 's/^active = no/active = yes/' {}".format(SYSLOG_CONF))
        is_configured = False

    # Check auditd configuration
    if not is_auditd_conf_configured():
        logger.log_info("Updating auditd.conf...")
        run_command("cp {}/auditd.conf {}".format(CONFIG_FILES, AUDIT_CONF))
        is_configured = False

    # Check service configuration
    if not is_auditd_service_configured():
        logger.log_info("Updating auditd.service...")
        run_command(r"""sed -i '/\[Service\]/a CPUQuota=10%' {}""".format(AUDIT_SERVICE))
        is_configured = False

    # If configuration has been modified, restart service
    if not is_configured:
        logger.log_info("Configuration changed, restarting auditd service...")
        run_command("nsenter --target 1 --pid --mount --uts --ipc --net systemctl daemon-reload")
        run_command("nsenter --target 1 --pid --mount --uts --ipc --net systemctl restart auditd")
        logger.log_info("auditd service restart completed")

        # check rules syntax by reload all rules file
        if not check_rules_syntax():
            logger.log_error("auditd rules syntax check failed")
            # Exit with non-zero status to indicate failure
            sys.exit(1)
    else:
        logger.log_info("No configuration changes needed")

    logger.log_info("auditd configuration check completed")


if __name__ == "__main__":
    main()
