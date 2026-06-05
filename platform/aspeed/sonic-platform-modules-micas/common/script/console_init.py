#!/usr/bin/env python3

import json
import os
import shutil
import sys
import subprocess
import tempfile
import stat

INIT_CFG_FILE = "/etc/sonic/init_cfg.json"
CONFIG_DB_FILE = "/etc/sonic/config_db.json"

CONSOLE_CONFIG = {
    "CONSOLE_PORT": {
        "0": {
            "baud_rate": "115200",
            "flow_control": "0",
            "remote_device": "CPU",
            "escape_char": "a"
        }
    },
    "CONSOLE_SWITCH": {
        "console_mgmt": {
            "enabled": "yes"
        }
    }
}

def load_json(path):
    with open(path, "r") as f:
        return json.load(f)

def atomic_write_json(path, data):
    dir_name = os.path.dirname(path)
    fd, temp_path = tempfile.mkstemp(dir=dir_name)

    try:
        with os.fdopen(fd, "w") as tmp:
            json.dump(data, tmp, indent=4)
            tmp.write("\n")
        shutil.move(temp_path, path)
        # Maintain the same permissions as the original file (usually 644)
        os.chmod(path, 0o644)
    except Exception:
        if os.path.exists(temp_path):
            os.remove(temp_path)
        raise

def is_config_already_present(config):
    return (
        "CONSOLE_SWITCH" in config and
        "CONSOLE_PORT" in config
    )

def configure_sonic_json():
    """Core logic for handling JSON configuration."""
    if os.path.exists(CONFIG_DB_FILE):
        print(f"{CONFIG_DB_FILE} already exists. System initialized, skipping JSON update.")
        return

    if not os.path.exists(INIT_CFG_FILE):
        print(f"ERROR: {INIT_CFG_FILE} does not exist")
        sys.exit(1)

    try:
        config = load_json(INIT_CFG_FILE)
    except Exception as e:
        print(f"ERROR: failed to parse JSON: {e}")
        sys.exit(1)

    if is_config_already_present(config):
        print("Console config already exists in init_cfg.json.")
        return

    modified = False

    if "CONSOLE_SWITCH" not in config:
        print("Adding CONSOLE_SWITCH...")
        config["CONSOLE_SWITCH"] = CONSOLE_CONFIG["CONSOLE_SWITCH"]
        modified = True

    if "CONSOLE_PORT" not in config:
        print("Adding CONSOLE_PORT...")
        config["CONSOLE_PORT"] = CONSOLE_CONFIG["CONSOLE_PORT"]
        modified = True
    else:
        for port, port_cfg in CONSOLE_CONFIG["CONSOLE_PORT"].items():
            if port not in config["CONSOLE_PORT"]:
                print(f"Adding missing CONSOLE_PORT {port}")
                config["CONSOLE_PORT"][port] = port_cfg
                modified = True

    if modified:
        try:
            atomic_write_json(INIT_CFG_FILE, config)
            print("Successfully updated init_cfg.json")
        except Exception as e:
            print(f"ERROR: failed to write JSON: {e}")
            sys.exit(1)

def adjust_picocom():
    """Use dpkg-divert to relocate native picocom and replace it with a custom script."""
    src_local = "/usr/local/bin/picocom.py"
    dest_sys = "/usr/bin/picocom"
    backup_sys = "/usr/bin/picocom.real"

    # Check if picocom.real already exists to avoid redundant operations
    if os.path.exists(backup_sys):
        print(f"{backup_sys} already exists. Skipping dpkg-divert and script installation.")
        return

    # 1. Execute dpkg-divert first. Let the package manager move the real picocom to .real
    dpkg_cmd = [
        "dpkg-divert",
        "--add",
        "--rename",
        "--divert",
        "/usr/bin/picocom.real",
        "/usr/bin/picocom",
    ]

    try:
        # If already diverted, dpkg-divert is idempotent by default (returns 0)
        subprocess.run(dpkg_cmd, check=True, stdout=subprocess.DEVNULL)
        print("dpkg-divert configuration ensured.")
    except subprocess.CalledProcessError as e:
        print(f"WARNING: dpkg-divert failed: {e}")
        # If it fails, decide whether to raise based on the situation; here we continue to try placing the script

    # 2. Place our custom script in the destination
    if os.path.exists(src_local):
        try:
            # Use copy2 instead of move, so the source file remains if debugging is needed
            shutil.copy2(src_local, dest_sys)
            # Ensure executable permissions (755)
            st = os.stat(dest_sys)
            os.chmod(dest_sys, 0o755)
            print(f"Installed custom picocom from {src_local} -> {dest_sys}")
        except Exception as e:
            print(f"ERROR: failed to copy {src_local} -> {dest_sys}: {e}")
    else:
        # If the target is already there, or the source file is missing, just print a prompt
        if not os.path.exists(dest_sys):
             print(f"{src_local} not found; skipping python script placement.")

def main():
    # 1. Pre-check: Must have Root privileges
    if os.geteuid() != 0:
        print("ERROR: This script must be run as root (or with sudo).")
        sys.exit(1)

    print("Starting initialization script...")

    # 2. Execute JSON configuration modification
    # sonic-switchcpu-console-init.sh will reset console again, so we don't need cofigure here.
    # configure_sonic_json()

    # 3. Regardless of JSON modification, ensure the underlying toolchain is in the correct state
    try:
        adjust_picocom()
    except Exception as e:
        print(f"WARNING: adjust_picocom encountered an error: {e}")

    print("All tasks completed!")

if __name__ == "__main__":
    main()
