"""Exercise the public management library against production SONiC models."""

from pathlib import Path
import sys

from sonic_yang import SonicYang
from sonic_yang_ext import SonicYangException


def main():
    if sys.version_info[:2] != (3, 13):
        raise AssertionError(f"Expected Python 3.13, got {sys.version}")
    model_dir = Path(sys.argv[1])
    models = {path.stem for path in model_dir.glob("*.yang")}
    if not {"sonic-port", "sonic-vlan", "sonic-syslog"} <= models:
        raise AssertionError("The declared production model payload is missing")

    manager = SonicYang(str(model_dir), print_log_enabled=False)
    if manager.loadYangModel() is not True or set(manager.yangFiles) != models:
        raise AssertionError("Management did not load every declared model")
    for table, module in {"PORT": "sonic-port", "VLAN": "sonic-vlan", "SYSLOG_SERVER": "sonic-syslog"}.items():
        if manager.confDbYangMap[table]["module"] != module:
            raise AssertionError(f"Incorrect compiled schema mapping for {table}")

    config = {
        "SYSLOG_SERVER": {
            "192.0.2.1": {"port": "514", "protocol": "udp", "severity": "info"},
        },
    }
    manager.loadData(config, quiet=True)
    manager.validate_data_tree()
    if manager.getData()["SYSLOG_SERVER"] != config["SYSLOG_SERVER"]:
        raise AssertionError("ConfigDB data changed during the YANG roundtrip")
    if manager.tablesWithOutYang:
        raise AssertionError("ConfigDB input bypassed the production schema")

    # The path mixin uses jsonpointer and the compiled SONiC list schema.
    config_path = "/SYSLOG_SERVER/192.0.2.1/port"
    xpath = manager.configdb_path_to_xpath(config_path)
    if manager.xpath_to_configdb_path(xpath) != config_path:
        raise AssertionError(f"ConfigDB/schema path did not roundtrip: {xpath}")

    manager.root.free()
    manager.root = None
    invalid = {"SYSLOG_SERVER": {"192.0.2.1": {"port": "70000"}}}
    try:
        manager.loadData(invalid, quiet=True)
        manager.validate_data_tree()
    except SonicYangException:
        pass
    else:
        raise AssertionError("Production inet:port-number accepted port 70000")
    print(f"Loaded {len(models)} production models; translated and validated ConfigDB data and paths")


if __name__ == "__main__":
    main()
