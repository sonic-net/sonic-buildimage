"""Unit tests for how sonic-cfggen's FormatConverter writes Python bools."""
import os
import sys
import unittest
from importlib.machinery import SourceFileLoader

test_dir = os.path.dirname(os.path.abspath(__file__))
sonic_cfggen_dir = os.path.dirname(test_dir)
if sonic_cfggen_dir not in sys.path:
    sys.path.insert(0, sonic_cfggen_dir)

sonic_cfggen = sys.modules.get('sonic_cfggen')
if sonic_cfggen is None:
    sonic_cfggen = SourceFileLoader("sonic_cfggen", os.path.join(sonic_cfggen_dir, "sonic-cfggen")).load_module()
    sys.modules['sonic_cfggen'] = sonic_cfggen

FormatConverter = sonic_cfggen.FormatConverter


class TestFormatConverterBool(unittest.TestCase):

    def test_output_to_db_lowercases_bools(self):
        # This is what --write-to-db hands to mod_config. Without the conversion,
        # ConfigDBConnector would store str(True), which is "True".
        data = {
            'MGMT_VRF_CONFIG': {'vrf_global': {'mgmtVrfEnabled': True}},
            'FEATURE': {'bgp': {'has_global_scope': False, 'has_per_asic_scope': 'True'}},
            'PORT': {'Ethernet0': {'lanes': '0,1', 'list_field': [True, 'x']}},
            'lowercase_table': {'skip': True},
        }
        self.assertEqual(FormatConverter.output_to_db(data), {
            'MGMT_VRF_CONFIG': {'vrf_global': {'mgmtVrfEnabled': 'true'}},
            'FEATURE': {'bgp': {'has_global_scope': 'false', 'has_per_asic_scope': 'True'}},
            'PORT': {'Ethernet0': {'lanes': '0,1', 'list_field': ['true', 'x']}},
        })

    def test_output_to_db_does_not_modify_input(self):
        data = {'MGMT_VRF_CONFIG': {'vrf_global': {'mgmtVrfEnabled': True}}}
        FormatConverter.output_to_db(data)
        self.assertIs(data['MGMT_VRF_CONFIG']['vrf_global']['mgmtVrfEnabled'], True)

    def test_to_serialized_lowercases_bools_in_place(self):
        # sonic-utilities calls to_serialized() for its side effect and ignores the return value.
        data = {'VLAN': {'Vlan10': {'static_anycast_gateway': True, 'vlanid': '10'}},
                'FEATURE': {'lldp': {'has_global_scope': 'True'}}}
        FormatConverter.to_serialized(data)
        self.assertEqual(data, {'VLAN': {'Vlan10': {'static_anycast_gateway': 'true', 'vlanid': '10'}},
                                'FEATURE': {'lldp': {'has_global_scope': 'True'}}})

    def test_to_serialized_keeps_non_bool_values(self):
        self.assertEqual(FormatConverter.to_serialized({'T': {'k': {'n': 1, 's': 'TrUE'}}}),
                         {'T': {'k': {'n': 1, 's': 'TrUE'}}})


if __name__ == '__main__':
    unittest.main()
