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
            ("wrong_name", 'Unsatisfied pattern')]
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
            ("169.254.xyz.254/24", 'Unsatisfied pattern')]
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
            ("xyz", 'Unsatisfied pattern')]
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
            ("xyz", 'Unsatisfied pattern')]
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

    @pytest.mark.parametrize(
        "port_name, error_message", [
            ("dpu0", None),
            ("dp0rt0", 'Unsatisfied pattern')]
        )
    def test_dpu_port_name(self, yang_model, port_name, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU_PORT": {
                    "DPU_PORT_LIST": [
                        {
                            "PORT_NAME": port_name,
                            "state": "up",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "vip_ipv4, error_message", [
            ("192.168.1.1", None),
            ("192.168.1.xyz", 'Unsatisfied pattern')]
        )
    def test_dpu_port_vip_ipv4(self, yang_model, vip_ipv4, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU_PORT": {
                    "DPU_PORT_LIST": [
                        {
                            "PORT_NAME": "dpu0",
                            "state": "up",
                            "vip_ipv4": vip_ipv4,
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "vip_ipv6, error_message", [
            ("2001:db8::1", None),
            ("2001:db8::xyz", 'Unsatisfied pattern')]
        )
    def test_dpu_port_vip_ipv6(self, yang_model, vip_ipv6, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU_PORT": {
                    "DPU_PORT_LIST": [
                        {
                            "PORT_NAME": "dpu0",
                            "state": "up",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": vip_ipv6,
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "pa_ipv4, error_message", [
            ("192.168.1.2", None),
            ("192.168.1.xyz", 'Unsatisfied pattern')]
        )
    def test_dpu_port_pa_ipv4(self, yang_model, pa_ipv4, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU_PORT": {
                    "DPU_PORT_LIST": [
                        {
                            "PORT_NAME": "dpu0",
                            "state": "up",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": pa_ipv4,
                            "pa_ipv6": "2001:db8::2",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "pa_ipv6, error_message", [
            ("2001:db8::2", None),
            ("2001:db8::xyz", 'Unsatisfied pattern')]
        )
    def test_dpu_port_pa_ipv6(self, yang_model, pa_ipv6, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU_PORT": {
                    "DPU_PORT_LIST": [
                        {
                            "PORT_NAME": "dpu0",
                            "state": "up",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": pa_ipv6,
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "gnmi_port, error_message", [
            (8080, None),
            (99999, 'Value "99999" is out of type uint16 min/max bounds')]
        )
    def test_dpu_port_gnmi(self, yang_model, gnmi_port, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU_PORT": {
                    "DPU_PORT_LIST": [
                        {
                            "PORT_NAME": "dpu0",
                            "state": "up",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": gnmi_port
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)
