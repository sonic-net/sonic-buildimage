#!/usr/bin/env python3

import json
import os
import shutil
import sys
import tempfile

INIT_CFG_FILE = "/etc/sonic/init_cfg.json"

CONFIG_DB_FILE = "/etc/sonic/config_db.json"

CONSOLE_CONFIG = {
    "CONSOLE_PORT": {
        str(i): {
            "baud_rate": "115200",
            "flow_control": "0"
        } for i in range(48)
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

    except Exception:
        if os.path.exists(temp_path):
            os.remove(temp_path)
        raise


def is_config_already_present(config):
    return (
        "CONSOLE_SWITCH" in config and
        "CONSOLE_PORT" in config
    )


def main():

    #
    # config_db.json exists
    # means system already initialized
    #
    if os.path.exists(CONFIG_DB_FILE):
        print(f"{CONFIG_DB_FILE} already exists")
        print("System already initialized.")
        return

    #
    # init_cfg.json must exist
    #
    if not os.path.exists(INIT_CFG_FILE):
        print(f"ERROR: {INIT_CFG_FILE} does not exist")
        sys.exit(1)

    #
    # load init_cfg.json
    #
    try:
        config = load_json(INIT_CFG_FILE)
    except Exception as e:
        print(f"ERROR: failed to parse JSON: {e}")
        sys.exit(1)

    #
    # config already contains console config
    #
    if is_config_already_present(config):
        print("Console config already exists.")
        return

    modified = False

    #
    # check CONSOLE_SWITCH
    #
    if "CONSOLE_SWITCH" not in config:
        print("Adding CONSOLE_SWITCH...")
        config["CONSOLE_SWITCH"] = CONSOLE_CONFIG["CONSOLE_SWITCH"]
        modified = True
    else:
        print("CONSOLE_SWITCH already exists")

    #
    # check CONSOLE_PORT
    #
    if "CONSOLE_PORT" not in config:
        print("Adding CONSOLE_PORT...")
        config["CONSOLE_PORT"] = CONSOLE_CONFIG["CONSOLE_PORT"]
        modified = True
    else:
        print("CONSOLE_PORT already exists")

        #
        # fill missing ports
        #
        for port, port_cfg in CONSOLE_CONFIG["CONSOLE_PORT"].items():
            if port not in config["CONSOLE_PORT"]:
                print(f"Adding missing CONSOLE_PORT {port}")
                config["CONSOLE_PORT"][port] = port_cfg
                modified = True

    if not modified:
        print("No changes needed.")
        return

    #
    # atomic write
    #
    try:
        atomic_write_json(INIT_CFG_FILE, config)
    except Exception as e:
        print(f"ERROR: failed to write JSON: {e}")
        sys.exit(1)

    print("Successfully updated init_cfg.json")


if __name__ == "__main__":
    main()
