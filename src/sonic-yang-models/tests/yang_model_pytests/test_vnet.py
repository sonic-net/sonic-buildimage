import pytest


@pytest.mark.parametrize("relax_list_limit", [False, True], ids=["limit-enforced", "limit-relaxed"])
def test_vxlan_tunnel_list_max_elements(yang_model, relax_list_limit):
    # Relax competing constraints only in this test's context to isolate the leaf-list limit.
    deviations = """
        deviation "/svxlan:sonic-vxlan/svxlan:VXLAN_TUNNEL/svxlan:VXLAN_TUNNEL_LIST" {
            deviate replace {
                max-elements 3;
            }
        }
        deviation "/svnet:sonic-vnet/svnet:VNET/svnet:VNET_LIST" {
            deviate delete {
                must "count(/svxlan:sonic-vxlan/svxlan:VXLAN_TUNNEL/svxlan:VXLAN_TUNNEL_LIST" +
                     "[svxlan:name = current()/vxlan_tunnel_list]" +
                     "[not(contains(svxlan:src_ip, ':'))]) <= 1";
                must "count(/svxlan:sonic-vxlan/svxlan:VXLAN_TUNNEL/svxlan:VXLAN_TUNNEL_LIST" +
                     "[svxlan:name = current()/vxlan_tunnel_list]" +
                     "[contains(svxlan:src_ip, ':')]) <= 1";
            }
        }
    """
    if relax_list_limit:
        deviations += """
            deviation "/svnet:sonic-vnet/svnet:VNET/svnet:VNET_LIST/svnet:vxlan_tunnel_list" {
                deviate replace {
                    max-elements 3;
                }
            }
        """

    yang_model.ctx.parse_module_str("""
        module vnet-cardinality-test {
            yang-version 1.1;
            namespace "urn:sonic:vnet-cardinality-test";
            prefix vct;
            import sonic-vnet {
                prefix svnet;
            }
            import sonic-vxlan {
                prefix svxlan;
            }
    """ + deviations + "}", "yang")

    tunnels = [
        {"name": "vtep1", "src_ip": "192.0.2.1"},
        {"name": "vtep2", "src_ip": "2001:db8::1"},
        {"name": "vtep3", "src_ip": "192.0.2.2"},
    ]
    vnet = {
        "name": "Vnet1",
        "vxlan_tunnel_list": ["vtep1", "vtep2"],
        "vni": "10011",
    }
    data = {
        "sonic-vxlan:sonic-vxlan": {
            "VXLAN_TUNNEL": {"VXLAN_TUNNEL_LIST": tunnels},
        },
        "sonic-vnet:sonic-vnet": {
            "VNET": {"VNET_LIST": [vnet]},
        },
    }

    yang_model.load_data(data)
    vnet["vxlan_tunnel_list"].append("vtep3")

    if relax_list_limit:
        yang_model.load_data(data)
    else:
        with pytest.raises(Exception, match="Too many") as exc_info:
            yang_model._load_data(data)
        assert "vxlan_tunnel_list" in str(exc_info.value)
