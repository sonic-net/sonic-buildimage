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

    @pytest.mark.parametrize(
        "dpu_name, error_message", [
            ("str-8102-t1-dpu0", None),
            ("str-8102-t1-dpu0a", 'Value "str-8102-t1-dpu0a" does not satisfy the constraint "[a-zA-Z0-9-]+[0-9]"')]
        )
    def test_dpu_name(self, yang_model, dpu_name, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU": {
                    "DPU_LIST": [
                        {
                            "dpu_name": dpu_name,
                            "state": "up",
                            "local_port": "Ethernet0",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "midplane_ipv4": "169.254.200.245",
                            "dpu_id": "0",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080,
                            "orchagent_zmq_port": 50
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "local_port, error_message", [
            ("Ethernet0", None),
            ("EthernetXYZ284099", 'Invalid interface name length, it must not exceed 16 characters.')]
        )
    def test_dpu_local_port(self, yang_model, local_port, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU": {
                    "DPU_LIST": [
                        {
                            "dpu_name": "str-8102-t1-dpu0",
                            "state": "up",
                            "local_port": local_port,
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "midplane_ipv4": "169.254.200.245",
                            "dpu_id": "0",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080,
                            "orchagent_zmq_port": 50
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "vip_ipv4, error_message", [
            ("192.168.1.1", None),
            ("192.168.1.xyz", 'Value "192.168.1.xyz" does not satisfy the constraint')]
        )
    def test_dpu_vip_ipv4(self, yang_model, vip_ipv4, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU": {
                    "DPU_LIST": [
                        {
                            "dpu_name": "str-8102-t1-dpu0",
                            "state": "up",
                            "local_port": "Ethernet0",
                            "vip_ipv4": vip_ipv4,
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "midplane_ipv4": "169.254.200.245",
                            "dpu_id": "0",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080,
                            "orchagent_zmq_port": 50
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "vip_ipv6, error_message", [
            ("2001:db8::1", None),
            ("2001:db8::xyz", 'Value "2001:db8::xyz" does not satisfy the constraint')]
        )
    def test_dpu_vip_ipv6(self, yang_model, vip_ipv6, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU": {
                    "DPU_LIST": [
                        {
                            "dpu_name": "str-8102-t1-dpu0",
                            "state": "up",
                            "local_port": "Ethernet0",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": vip_ipv6,
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "midplane_ipv4": "169.254.200.245",
                            "dpu_id": "0",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080,
                            "orchagent_zmq_port": 50
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "pa_ipv4, error_message", [
            ("192.168.1.2", None),
            ("192.168.1.xyz", 'Value "192.168.1.xyz" does not satisfy the constraint')]
        )
    def test_dpu_pa_ipv4(self, yang_model, pa_ipv4, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU": {
                    "DPU_LIST": [
                        {
                            "dpu_name": "str-8102-t1-dpu0",
                            "state": "up",
                            "local_port": "Ethernet0",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": pa_ipv4,
                            "pa_ipv6": "2001:db8::2",
                            "midplane_ipv4": "169.254.200.245",
                            "dpu_id": "0",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080,
                            "orchagent_zmq_port": 50
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "pa_ipv6, error_message", [
            ("2001:db8::2", None),
            ("2001:db8::xyz", 'Value "2001:db8::xyz" does not satisfy the constraint')]
        )
    def test_dpu_pa_ipv6(self, yang_model, pa_ipv6, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU": {
                    "DPU_LIST": [
                        {
                            "dpu_name": "str-8102-t1-dpu0",
                            "state": "up",
                            "local_port": "Ethernet0",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": pa_ipv6,
                            "midplane_ipv4": "169.254.200.245",
                            "dpu_id": "0",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080,
                            "orchagent_zmq_port": 50
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "dpu_id, error_message", [
            ("0", None),
            ("xyz", 'Value "xyz" does not satisfy the constraint')]
        )
    def test_dpu_id(self, yang_model, dpu_id, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU": {
                    "DPU_LIST": [
                        {
                            "dpu_name": "str-8102-t1-dpu0",
                            "state": "up",
                            "local_port": "Ethernet0",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "midplane_ipv4": "169.254.200.245",
                            "dpu_id": dpu_id,
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080,
                            "orchagent_zmq_port": 50
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "gnmi_port, error_message", [
            (8080, None),
            (99999, 'Invalid value "99999" in "gnmi_port" element.')]
        )
    def test_dpu_gnmi_port(self, yang_model, gnmi_port, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU": {
                    "DPU_LIST": [
                        {
                            "dpu_name": "str-8102-t1-dpu0",
                            "state": "up",
                            "local_port": "Ethernet0",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "midplane_ipv4": "169.254.200.245",
                            "dpu_id": "0",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": gnmi_port,
                            "orchagent_zmq_port": 50
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "orchagent_zmq_port, error_message", [
            (50, None),
            (99999, 'Invalid value "99999" in "orchagent_zmq_port" element.')]
        )
    def test_dpu_orchagent_zmq_port(self, yang_model, orchagent_zmq_port, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DPU": {
                    "DPU_LIST": [
                        {
                            "dpu_name": "str-8102-t1-dpu0",
                            "state": "up",
                            "local_port": "Ethernet0",
                            "vip_ipv4": "192.168.1.1",
                            "vip_ipv6": "2001:db8::1",
                            "pa_ipv4": "192.168.1.2",
                            "pa_ipv6": "2001:db8::2",
                            "midplane_ipv4": "169.254.200.245",
                            "dpu_id": "0",
                            "vdpu_id": "vdpu0",
                            "gnmi_port": 8080,
                            "orchagent_zmq_port": orchagent_zmq_port
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "dpu_id, swbus_port, error_message", [
            (0, 23606, None),
            (1, 23607, None),
            (7, 23613, None)]
        )
    def test_remote_dpu_swbus_port(self, yang_model, dpu_id, swbus_port, error_message):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:REMOTE_DPU": {
                    "REMOTE_DPU_LIST": [
                        {
                            "dpu_name": f"str-8102-t1-dpu{dpu_id}",
                            "type": "xyz",
                            "pa_ipv4": "192.168.1.4",
                            "pa_ipv6": "2001:db8::4",
                            "npu_ipv4": "192.168.1.5",
                            "npu_ipv6": "2001:db8::5",
                            "dpu_id": dpu_id,
                            "swbus_port": swbus_port,
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data, error_message)

    def test_vdpu(self, yang_model):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:VDPU": {
                    "VDPU_LIST": [
                        {
                            "vdpu_id": "vdpu0",
                            "profile": "none",
                            "tier": "none",
                            "main_dpu_ids": ["str-8102-t1-dpu0"]
                        }
                    ]
                }
            }
        }
        yang_model.load_data(data)

    def test_dash_ha_global_config(self, yang_model):
        data = {
            "sonic-smart-switch:sonic-smart-switch": {
                "sonic-smart-switch:DASH_HA_GLOBAL_CONFIG": {
                    "global": {
                        "cp_data_channel_port": 11234,
                        "dp_channel_port": 11235,
                        "dp_channel_src_port_min": 11236,
                        "dp_channel_src_port_max": 11237,
                        "dp_channel_probe_interval_ms": 500,
                        "dp_channel_probe_fail_threshold": 3,
                        "dpu_bfd_probe_interval_in_ms": 500,
                        "dpu_bfd_probe_multiplier": 3
                    }
                }
            }
        }
        yang_model.load_data(data)
