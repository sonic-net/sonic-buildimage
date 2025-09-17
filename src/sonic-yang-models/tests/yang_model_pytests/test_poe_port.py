import pytest


class TestPoePort:

    def test_valid_data(self, yang_model):
        data = {
            "sonic-port:sonic-port": {
                "sonic-port:PORT": {
                    "PORT_LIST": [
                    {
                        "admin_status": "up",
                        "alias": "eth8",
                        "description": "Ethernet0",
                        "lanes": "65",
                        "mtu": 9000,
                        "name": "Ethernet0",
                        "speed": 25000
                    }
                    ]
                }
            },
            "sonic-poe:sonic-poe": {
                "sonic-poe:POE_PORT": {
                    "POE_PORT_LIST": [
                        {
                            "ifname": "Ethernet0",
                            "enabled": "enable",
                            "pwr_limit": "50",
                            "priority": "high"
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data)

    @pytest.mark.parametrize(
        "enabled, error_message", [
            ("enable", None),
            ("disable", None),
            ("xyz", 'Invalid value "xyz" in "enabled" element.')]
        )
    def test_enabled_field(self, yang_model, enabled, error_message):
        data = {
            "sonic-port:sonic-port": {
                "sonic-port:PORT": {
                    "PORT_LIST": [
                    {
                        "admin_status": "up",
                        "alias": "eth8",
                        "description": "Ethernet0",
                        "lanes": "65",
                        "mtu": 9000,
                        "name": "Ethernet0",
                        "speed": 25000
                    }
                    ]
                }
            },
            "sonic-poe:sonic-poe": {
                "sonic-poe:POE_PORT": {
                    "POE_PORT_LIST": [
                        {
                            "ifname": "Ethernet0",
                            "enabled": enabled,
                            "pwr_limit": "50",
                            "priority": "high"
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "priority, error_message", [
            ("low", None),
            ("high", None),
            ("crit", None),
            ("xyz", 'Invalid value "xyz" in "priority" element.')]
        )
    def test_priority_field(self, yang_model, priority, error_message):
        data = {
            "sonic-port:sonic-port": {
                "sonic-port:PORT": {
                    "PORT_LIST": [
                    {
                        "admin_status": "up",
                        "alias": "eth8",
                        "description": "Ethernet0",
                        "lanes": "65",
                        "mtu": 9000,
                        "name": "Ethernet0",
                        "speed": 25000
                    }
                    ]
                }
            },
            "sonic-poe:sonic-poe": {
                "sonic-poe:POE_PORT": {
                    "POE_PORT_LIST": [
                        {
                            "ifname": "Ethernet0",
                            "enabled": "enable",
                            "pwr_limit": "50",
                            "priority": priority
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)
