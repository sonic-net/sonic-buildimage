#!/usr/bin/env python3
"""
Sync REST API certificate CNs from running config to CONFIG_DB.
Only adds missing CNs, doesn't overwrite existing ones.
"""

import json
import subprocess


def get_running_config_cns():
    """Get CNs from running config (ground truth)."""
    cmd = ["sonic-cfggen", "-d", "--print-data"]
    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    config = json.loads(result.stdout)

    # Extract client_crt_cname value and parse CNs
    cname_str = config.get("RESTAPI", {}).get("certs", {}).get("client_crt_cname", "")
    return set(cn.strip() for cn in cname_str.split(',') if cn.strip())


def get_config_db_cns():
    """Get current CNs from CONFIG_DB."""
    cmd = ["sonic-db-cli", "CONFIG_DB", "HGET", "RESTAPI|certs", "client_crt_cname"]
    result = subprocess.run(cmd, capture_output=True, text=True, check=True)

    cname_str = result.stdout.strip()
    if not cname_str:
        return set()

    return set(cn.strip() for cn in cname_str.split(',') if cn.strip())


def add_missing_cns(existing_cns, missing_cns):
    """Add missing CNs to CONFIG_DB's client_crt_cname field."""
    # Combine existing and missing CNs
    all_cns = existing_cns | missing_cns

    # Create comma-separated string
    new_value = ','.join(sorted(all_cns))

    # Update CONFIG_DB
    cmd = ["sonic-db-cli", "CONFIG_DB", "HSET", "RESTAPI|certs", "client_crt_cname", new_value]
    subprocess.run(cmd, check=True)

    print(f"  Updated client_crt_cname to: {new_value}")


def main():
    # A: Get ground truth from running config
    print("Getting CNs from running config...")
    running_cns = get_running_config_cns()
    print(f"  Running config CNs: {running_cns}")

    # B: Get current CNs from CONFIG_DB
    print("\nGetting CNs from CONFIG_DB...")
    config_db_cns = get_config_db_cns()
    print(f"  CONFIG_DB CNs: {config_db_cns}")

    # Find diff: CNs in A but not in B
    missing_cns = running_cns - config_db_cns

    if missing_cns:
        print(f"\nMissing CNs in CONFIG_DB: {missing_cns}")
        print("Adding missing CNs...")
        add_missing_cns(config_db_cns, missing_cns)
        print("\nSync complete!")
    else:
        print("\nNo missing CNs. CONFIG_DB is up to date.")


if __name__ == "__main__":
    main()
