from sonic_py_common import device_info


def get_gearbox_sensors_config():
    """Read gearbox sensor polling defaults from platform.json."""
    try:
        platform_json = device_info.get_platform_json_data()
    except OSError:
        return {}

    if not platform_json or "gearbox_sensors" not in platform_json:
        return {}

    sensors = platform_json["gearbox_sensors"]
    return {
        "GEARBOX_SENSORS": {
            "GEARBOX_SENSORS_POLLER_INTERVAL": {
                "interval": sensors["poll_interval"]
            },
            "GEARBOX_SENSORS_POLLER_STATUS": {
                "admin_status": sensors["poll_admin_status"]
            }
        }
    }
