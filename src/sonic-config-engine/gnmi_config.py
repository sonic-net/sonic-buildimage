from __future__ import print_function

import sys


def set_gnmi_defaults(data, configdb):
    """Provision an explicit loopback profile when enabling unconfigured gNMI."""
    state = data.get("FEATURE", {}).get("gnmi", {}).get("state")
    if state not in ("enabled", "always_enabled") or "GNMI" in data:
        return False
    if configdb.get_table("GNMI"):
        return False

    device_metadata = data.get("DEVICE_METADATA", {})
    metadata = dict(configdb.get_entry("DEVICE_METADATA", "localhost"))
    metadata.update(device_metadata.get("localhost", {}))
    x509 = dict(configdb.get_entry("DEVICE_METADATA", "x509"))
    x509.update(device_metadata.get("x509", {}))
    if x509 or metadata.get("type") == "SmartSwitchDPU" or metadata.get("switch_type") == "dpu":
        return False

    data["GNMI"] = {"gnmi": {"port": "8080", "user_auth": "password,jwt"}}
    return True


def migrate_gnmi_defaults(configdb):
    data = {"FEATURE": {"gnmi": configdb.get_entry("FEATURE", "gnmi")}}
    if not set_gnmi_defaults(data, configdb):
        return False
    configdb.mod_config({"GNMI": data["GNMI"]})
    return True


if __name__ == "__main__":
    from swsscommon.swsscommon import ConfigDBPipeConnector

    configdb = ConfigDBPipeConnector(use_unix_socket_path=True)
    configdb.connect(False)
    if migrate_gnmi_defaults(configdb):
        print("Provisioned GNMI loopback TCP defaults with explicit password/JWT authentication",
              file=sys.stderr)
