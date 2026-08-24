import pytest
from copy import deepcopy


@pytest.fixture(scope='function')
def data(request):
    base_data = {
        "sonic-device_metadata:sonic-device_metadata": {
            "sonic-device_metadata:DEVICE_METADATA": {
                "localhost": {
                    "switch_type": "dpu"
                }
            }
        }
    }

    return base_data


dash_thresholds = [
    'free', 'FREE',
    'used', 'USED',
    'percentage', 'PERCENTAGE'
]


@pytest.fixture(params=dash_thresholds)
def threshold(request):
    return request.param


dash_resources = [
    'vnet',
    'eni',
    'eni_ether_address_map',
    'ipv4_inbound_routing',
    'ipv6_inbound_routing',
    'ipv4_outbound_routing',
    'ipv6_outbound_routing',
    'ipv4_pa_validation',
    'ipv6_pa_validation',
    'ipv4_outbound_ca_to_pa',
    'ipv6_outbound_ca_to_pa',
    'ipv4_acl_group',
    'ipv6_acl_group'
]


@pytest.fixture(params=dash_resources)
def resource(request):
    return request.param


class TestDashCrm:

    def test_dash_crm_valid_data(self, yang_model, data, resource, threshold):
        data["sonic-crm:sonic-crm"] = {
            "sonic-crm:CRM": {
                "Config": {
                    f"dash_{resource}_high_threshold": 95,
                    f"dash_{resource}_low_threshold": 70,
                    f"dash_{resource}_threshold_type": threshold
                }
            }
        }

        yang_model.load_data(data)

    @pytest.mark.parametrize(
        "high, low, error_message", [
            (-1, 70, 'Value "-1" is out of type uint16 min/max bounds'),
            (100, -70, 'Value "-70" is out of type uint16 min/max bounds'),
            (10, 70, 'high_threshold should be more than low_threshold')]
        )
    def test_dash_crm_thresholds(self, yang_model, data, resource, threshold, high, low, error_message):
        data["sonic-crm:sonic-crm"] = {
            "sonic-crm:CRM": {
                "Config": {
                    f"dash_{resource}_high_threshold": high,
                    f"dash_{resource}_low_threshold": low,
                    f"dash_{resource}_threshold_type": threshold
                }
            }
        }

        yang_model.load_data(data, error_message)

    @pytest.mark.parametrize(
        "high, low, th_type, error_message", [
            (100, 70, 'wrong', 'Unsatisfied pattern'),
            (110, 20, 'percentage', 'Must condition')]
        )
    def test_dash_crm_threshold_type(self, yang_model, data, resource, high, low, th_type, error_message):
        data["sonic-crm:sonic-crm"] = {
            "sonic-crm:CRM": {
                "Config": {
                    f"dash_{resource}_high_threshold": high,
                    f"dash_{resource}_low_threshold": low,
                    f"dash_{resource}_threshold_type": th_type
                }
            }
        }

        yang_model.load_data(data, error_message)
