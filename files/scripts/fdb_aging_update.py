#!/usr/bin/python3
# -*- coding: utf-8 -*-

import subprocess
import sys
import json

def modifySwssConfig(aging_time):
    result = subprocess.run(['docker', 'cp', 'swss:/etc/swss/config.d/switch.json', '/tmp'], capture_output=True, text=True)
    if result.returncode != 0:
        print("Failed to copy switch.json to host")
        sys.exit(1)
    with open('/tmp/switch.json', 'r') as f:
        data = json.load(f)
    print("Update switch.json with new fdb_aging_time")
    data[0]["SWITCH_TABLE:switch"]["fdb_aging_time"] = str(aging_time)
    with open('/tmp/switch.json', 'w') as f:
        json.dump(data, f,indent=4)

def applySwssConfig():
    result = subprocess.run(['docker', 'cp', '/tmp/switch.json', 'swss:/etc/swss/config.d/switch.json'], capture_output=True, text=True)
    if result.returncode != 0:
        print("Failed to copy switch.json to swss")
        sys.exit(1)
    print("Apply swssconfig switch.json ")
    result = subprocess.run(['docker', 'exec', 'swss', 'swssconfig', '/etc/swss/config.d/switch.json'], capture_output=True, text=True)
    if result.returncode != 0:
        print("Failed to execute swssconfig switch.json")
        sys.exit(1)

def main():
    if len(sys.argv) < 2:
        print("Usage: python fdb_aging_update.py <aging-time in seconds>")
        sys.exit(1)
    fdb_aging_time = sys.argv[1]
    print(f"FDB aging time config: {fdb_aging_time}")
    modifySwssConfig(fdb_aging_time)
    applySwssConfig()

if __name__ == "__main__":
    main()
