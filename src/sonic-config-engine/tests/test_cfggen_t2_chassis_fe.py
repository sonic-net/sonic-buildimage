import os
import subprocess
import tests.common_utils as utils

from unittest import TestCase


class TestCfgGenT2ChassisFe(TestCase):

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.script_file = [utils.PYTHON_INTERPRETTER, os.path.join(self.test_dir, '..', 'sonic-cfggen')]
        self.sample_graph_t2_chassis_fe = os.path.join(self.test_dir, 't2-chassis-fe-graph.xml')
        self.sample_graph_t2_chassis_fe_vni = os.path.join(self.test_dir, 't2-chassis-fe-graph-vni.xml')
        self.sample_graph_t2_chassis_fe_pc = os.path.join(self.test_dir, 't2-chassis-fe-graph-pc.xml')
        self.t2_chassis_fe_port_config = os.path.join(self.test_dir, 't2-chassis-fe-port-config.ini')

    def run_script(self, argument, check_stderr=False):
        print('\n    Running sonic-cfggen ' + ' '.join(argument))
        if check_stderr:
            output = subprocess.check_output(self.script_file + argument, stderr=subprocess.STDOUT)
        else:
            output = subprocess.check_output(self.script_file + argument)

        if utils.PY3x:
            output = output.decode()

        linecount = output.strip().count('\n')
        if linecount <= 0:
            print('    Output: ' + output.strip())
        else:
            print('    Output: ({0} lines, {1} bytes)'.format(linecount + 1, len(output)))
        return output

    def test_minigraph_t2_chassis_fe_type(self):
        argument = ['-m', self.sample_graph_t2_chassis_fe, '-p', self.t2_chassis_fe_port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'type\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'SpineChassisFrontendRouter')

    def test_minigraph_t2_chassis_fe_interfaces(self):
        argument = ['-m', self.sample_graph_t2_chassis_fe, '-p', self.t2_chassis_fe_port_config, '-v', "INTERFACE"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet8': {}, "
                "('Ethernet8', '172.16.0.9/30'): {}, "
                "'Ethernet0': {'vnet_name': 'VnetFE'}, "
                "('Ethernet4', '172.16.0.1/30'): {}, "
                "('Ethernet0', '192.168.0.2/30'): {}, "
                "'Ethernet4': {}}"
            )
        )
    def test_minigraph_t2_chassis_fe_pc_interfaces(self):
        argument = ['-m', self.sample_graph_t2_chassis_fe_pc, '-p', self.t2_chassis_fe_port_config, '-v', "PORTCHANNEL_INTERFACE"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'PortChannel8': {}, "
                "('PortChannel0', '192.168.0.2/30'): {}, "
                "('PortChannel4', '172.16.0.1/30'): {}, "
                "'PortChannel4': {}, "
                "('PortChannel8', '172.16.0.9/30'): {}, "
                "'PortChannel0': {'vnet_name': 'VnetFE'}}"
            )
        )

    # Test a minigraph file where VNI is not specified
    # Default VNI is 8000
    def test_minigraph_t2_chassis_fe_vnet_default(self):
        argument = ['-m', self.sample_graph_t2_chassis_fe, '-p', self.t2_chassis_fe_port_config, '-v', "VNET"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "{'VnetFE': {'vxlan_tunnel': 'TunnelInt', 'vni': 8000}}")

    # Test a minigraph file where VNI is specified
    def test_minigraph_t2_chassis_fe_vnet(self):
        argument = ['-m', self.sample_graph_t2_chassis_fe_vni, '-p', self.t2_chassis_fe_port_config, '-v', "VNET"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "{'VnetFE': {'vxlan_tunnel': 'TunnelInt', 'vni': 9000}}")

    def test_minigraph_t2_chassis_fe_vxlan(self):
        argument = ['-m', self.sample_graph_t2_chassis_fe, '-p', self.t2_chassis_fe_port_config, '-v', "VXLAN_TUNNEL"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "{'TunnelInt': {'src_ip': '4.0.0.0'}}")
