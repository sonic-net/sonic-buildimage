import pytest


class TestSmartSwitch:

    def test_valid_data(self, yang_model):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:MID_PLANE_BRIDGE": {
                    "GLOBAL": {
                        "bridge": "bridge-midplane",
                        "ip_prefix": "169.254.200.254/24"
                    }
                },
                "sonic-smart-switch:DPUS": {
                    "DPUS_LIST": [
                        {
                            "dpu_name": "dpu0",
                            "midplane_interface": "dpu0"
                        },
                        {
                            "dpu_name": "dpu1",
                            "midplane_interface": "dpu1"
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data)

    @pytest.mark.parametrize(
        "bridge_name, error_message", [
            ("bridge-midplane", None),
            ("wrong_name", 'Value "wrong_name" does not satisfy the constraint "bridge-midplane"')]
        )
    def test_bridge_name(self, yang_model, bridge_name, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:MID_PLANE_BRIDGE": {
                    "GLOBAL": {
                        "bridge": bridge_name,
                        "ip_prefix": "169.254.200.254/24"
                    }
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "ip_prefix, error_message", [
            ("169.254.200.254/24", None),
            ("169.254.xyz.254/24", 'Value "169.254.xyz.254/24" does not satisfy the constraint')]
        )
    def test_bridge_ip_prefix(self, yang_model, ip_prefix, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:MID_PLANE_BRIDGE": {
                    "GLOBAL": {
                        "bridge": "bridge-midplane",
                        "ip_prefix": ip_prefix
                    }
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "dpu_name, error_message", [
            ("dpu0", None),
            ("xyz", 'Value "xyz" does not satisfy the constraint "dpu[0-9]+')]
        )
    def test_dpu_name(self, yang_model, dpu_name, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPUS": {
                    "DPUS_LIST": [
                        {
                            "dpu_name": dpu_name,
                            "midplane_interface": "dpu0"
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "midplane_interface, error_message", [
            ("dpu0", None),
            ("xyz", 'Value "xyz" does not satisfy the constraint "dpu[0-9]+')]
        )
    def test_dpu_midplane_interface(self, yang_model, midplane_interface, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPUS": {
                    "DPUS_LIST": [
                        {
                            "dpu_name": "dpu0",
                            "midplane_interface": midplane_interface
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)
