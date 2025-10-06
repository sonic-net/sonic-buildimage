import json
import os
import shutil
import subprocess
import re

from unittest import TestCase
import tests.common_utils as utils
from sonic_py_common.general import getstatusoutput_noshell, getstatusoutput_noshell_pipe


class TestJ2Files(TestCase):
    def setUp(self):
        self.yang = utils.YangWrapper()
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.script_file = [utils.PYTHON_INTERPRETTER, os.path.join(self.test_dir, '..', 'sonic-cfggen')]
        self.simple_minigraph = os.path.join(self.test_dir, 'simple-sample-graph.xml')
        self.port_data = os.path.join(self.test_dir, 'sample-port-data.json')
        self.ztp = os.path.join(self.test_dir, "sample-ztp.json")
        self.ztp_inband = os.path.join(self.test_dir, "sample-ztp-inband.json")
        self.ztp_ip = os.path.join(self.test_dir, "sample-ztp-ip.json")
        self.ztp_inband_ip = os.path.join(self.test_dir, "sample-ztp-inband-ip.json")
        self.t0_minigraph = os.path.join(self.test_dir, 't0-sample-graph.xml')
        self.t0_minigraph_syslog = os.path.join(self.test_dir, 't0-sample-graph-syslog.xml')
        self.t0_minigraph_secondary_subnets = os.path.join(self.test_dir, 't0-sample-graph-secondary-subnets.xml')
        self.t0_minigraph_common_dhcp_relay = os.path.join(self.test_dir, 't0-sample-graph-common-dhcp-relay.xml')
        self.t0_mvrf_minigraph = os.path.join(self.test_dir, 't0-sample-graph-mvrf.xml')
        self.t0_minigraph_nomgmt = os.path.join(self.test_dir, 't0-sample-graph-nomgmt.xml')
        self.t0_minigraph_two_mgmt = os.path.join(self.test_dir, 't0-sample-graph-two-mgmt.xml')
        self.t0_mvrf_minigraph_nomgmt = os.path.join(self.test_dir, 't0-sample-graph-mvrf-nomgmt.xml')
        self.pc_minigraph = os.path.join(self.test_dir, 'pc-test-graph.xml')
        self.t0_port_config = os.path.join(self.test_dir, 't0-sample-port-config.ini')
        self.t0_port_config_tiny = os.path.join(self.test_dir, 't0-sample-port-config-tiny.ini')
        self.t1_ss_port_config = os.path.join(self.test_dir, 't1-ss-sample-port-config.ini')
        self.t1_ss_dpu_port_config = os.path.join(self.test_dir, 't1-ss-dpu-sample-port-config.ini')
        self.l1_l3_port_config = os.path.join(self.test_dir, 'l1-l3-sample-port-config.ini')
        self.t0_7050cx3_port_config = os.path.join(self.test_dir, 't0_7050cx3_d48c8_port_config.ini')
        self.t1_mlnx_minigraph = os.path.join(self.test_dir, 't1-sample-graph-mlnx.xml')
        self.mlnx_port_config = os.path.join(self.test_dir, 'sample-port-config-mlnx.ini')
        self.dell6100_t0_minigraph = os.path.join(self.test_dir, 'sample-dell-6100-t0-minigraph.xml')
        self.arista7050_t0_minigraph = os.path.join(self.test_dir, 'sample-arista-7050-t0-minigraph.xml')
        self.arista7800r3_48cq2_lc_t2_minigraph = os.path.join(self.test_dir, 'sample-arista-7800r3-48cq2-lc-t2-minigraph.xml')
        self.arista7800r3_48cqm2_lc_t2_minigraph = os.path.join(self.test_dir, 'sample-arista-7800r3-48cqm2-lc-t2-minigraph.xml')
        self.arista7800r3a_36dm2_c36_lc_t2_minigraph = os.path.join(self.test_dir, 'sample-arista-7800r3a-36dm2-c36-lc-t2-minigraph.xml')
        self.arista7800r3a_36dm2_d36_lc_t2_minigraph = os.path.join(self.test_dir, 'sample-arista-7800r3a-36dm2-d36-lc-t2-minigraph.xml')
        self.multi_asic_minigraph = os.path.join(self.test_dir, 'multi_npu_data', 'sample-minigraph.xml')
        self.multi_asic_port_config = os.path.join(self.test_dir, 'multi_npu_data', 'sample_port_config-0.ini')
        self.dell9332_t1_minigraph = os.path.join(self.test_dir, 'sample-dell-9332-t1-minigraph.xml')
        self.radv_test_minigraph = os.path.join(self.test_dir, 'radv-test-sample-graph.xml')
        self.no_ip_helper_minigraph = os.path.join(self.test_dir, 't0-sample-no-ip-helper-graph.xml')
        self.nokia_ixr7250e_36x100g_t2_minigraph = os.path.join(self.test_dir, 'sample-nokia-ixr7250e-36x100g-t2-minigraph.xml')
        self.nokia_ixr7250e_36x400g_t2_minigraph = os.path.join(self.test_dir, 'sample-nokia-ixr7250e-36x400g-t2-minigraph.xml')
        self.t2_sample_graph_chassis_packet = os.path.join(self.test_dir, 'sample-chassis-packet-lc-graph.xml')
        self.output_file = os.path.join(self.test_dir, 'output')
        os.environ["CFGGEN_UNIT_TESTING"] = "2"

    def run_script(self, argument, output_file=None):
        print('CMD: sonic-cfggen ', argument)
        output = subprocess.check_output(self.script_file + argument)

        if utils.PY3x:
            output = output.decode()
        if output_file:
            with open(output_file, 'w') as f:
                f.write(output)

        return output

    def run_diff(self, file1, file2):
        _, output = getstatusoutput_noshell(['diff', '-u', file1, file2])
        return output

    def create_machine_conf(self, platform, vendor):
        file_exist = True
        dir_exist = True
        mode = {'arista': 'aboot',
                'dell': 'onie',
                'mellanox': 'onie'
               }
        echo_cmd1 = ["echo", '{}_platform={}'.format(mode[vendor], platform)]
        echo_cmd2 = ["sudo", "tee", "-a", "/host/machine.conf"]
        if not os.path.exists('/host/machine.conf'):
            file_exist = False
            if not os.path.isdir('/host'):
                dir_exist = False
                subprocess.call(['sudo', 'mkdir', '/host'])
            subprocess.call(['sudo', 'touch', '/host/machine.conf'])
            getstatusoutput_noshell_pipe(echo_cmd1, echo_cmd2)

        return file_exist, dir_exist

    def remove_machine_conf(self, file_exist, dir_exist):
        if not file_exist:
            subprocess.call(['sudo', 'rm', '-f', '/host/machine.conf'])

        if not dir_exist:
            subprocess.call(['sudo', 'rmdir', '/host'])

    def modify_cable_len(self, base_file, file_dir):
        input_file = os.path.join(file_dir, base_file)
        with open(input_file, 'r') as ifd:
            object = json.load(ifd)
            if 'CABLE_LENGTH' in object and 'AZURE' in object['CABLE_LENGTH']:
                for key in object['CABLE_LENGTH']['AZURE']:
                    object['CABLE_LENGTH']['AZURE'][key] = '300m'
        prefix, extension = base_file.split('.')
        output_file = '{}_300m.{}'.format(prefix, extension)
        out_file_path = os.path.join(file_dir, output_file)
        with open(out_file_path, 'w') as wfd:
            json.dump(object, wfd, indent=4)
        return output_file

    def test_interfaces(self):
        interfaces_template = os.path.join(self.test_dir, '..', '..', '..', 'files', 'image_config', 'interfaces', 'interfaces.j2')

        # ZTP enabled
        argument = ['-m', self.t0_minigraph_nomgmt, '-p', self.t0_port_config_tiny, '-j', self.ztp, '-j', self.port_data, '-a', '{\"hwaddr\":\"e4:1d:2d:a5:f3:ad\"}', '-t', interfaces_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'interfaces_nomgmt_ztp'), self.output_file))

        argument = ['-m', self.t0_minigraph_nomgmt, '-p', self.t0_port_config_tiny, '-j', self.ztp_inband, '-j', self.port_data, '-a', '{\"hwaddr\":\"e4:1d:2d:a5:f3:ad\"}', '-t', interfaces_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'interfaces_nomgmt_ztp_inband'), self.output_file))

        argument = ['-m', self.t0_minigraph_nomgmt, '-p', self.t0_port_config_tiny, '-j', self.ztp_ip, '-j', self.port_data, '-a', '{\"hwaddr\":\"e4:1d:2d:a5:f3:ad\"}', '-t', interfaces_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'interfaces_nomgmt_ztp_ip'), self.output_file))

        argument = ['-m', self.t0_minigraph_nomgmt, '-p', self.t0_port_config_tiny, '-j', self.ztp_inband_ip, '-j', self.port_data, '-a', '{\"hwaddr\":\"e4:1d:2d:a5:f3:ad\"}', '-t', interfaces_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'interfaces_nomgmt_ztp_inband_ip'), self.output_file))

        # ZTP disabled, MGMT_INTERFACE defined
        argument = ['-m', self.t0_minigraph, '-p', self.t0_port_config, '-a', '{\"hwaddr\":\"e4:1d:2d:a5:f3:ad\"}', '-t', interfaces_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'interfaces'), self.output_file))
        
        # ZTP disabled, MGMT_INTERFACE defined, SYSLOG_SERVER defined
        argument = ['-m', self.t0_minigraph_syslog, '-p', self.t0_port_config, '-a', '{\"hwaddr\":\"e4:1d:2d:a5:f3:ad\"}', '-t', interfaces_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'interfaces_syslog'), self.output_file))

        argument = ['-m', self.t0_mvrf_minigraph, '-p', self.t0_port_config, '-a', '{\"hwaddr\":\"e4:1d:2d:a5:f3:ad\"}', '-t', interfaces_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'mvrf_interfaces'), self.output_file))

        argument = ['-m', self.t0_minigraph_two_mgmt, '-p', self.t0_port_config, '-a', '{\"hwaddr\":\"e4:1d:2d:a5:f3:ad\"}', '-t', interfaces_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'two_mgmt_interfaces'), self.output_file), self.output_file)

        # ZTP disabled, no MGMT_INTERFACE defined
        argument = ['-m', self.t0_minigraph_nomgmt, '-p', self.t0_port_config, '-a', '{\"hwaddr\":\"e4:1d:2d:a5:f3:ad\"}', '-t', interfaces_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'interfaces_nomgmt'), self.output_file))

        argument = ['-m', self.t0_mvrf_minigraph_nomgmt, '-p', self.t0_port_config, '-a', '{\"hwaddr\":\"e4:1d:2d:a5:f3:ad\"}', '-t', interfaces_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'mvrf_interfaces_nomgmt'), self.output_file))


    def test_ports_json(self):
        ports_template = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-orchagent', 'ports.json.j2')
        argument = ['-m', self.simple_minigraph, '-p', self.t0_port_config, '-t', ports_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'ports.json'), self.output_file))

    def test_dhcp_relay(self):
        # Test generation of wait_for_intf.sh
        dhc_sample_data = os.path.join(self.test_dir, "dhcp-relay-sample.json")
        template_path = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-dhcp-relay', 'wait_for_intf.sh.j2')
        argument = ['-m', self.t0_minigraph, '-j', dhc_sample_data, '-p', self.t0_port_config, '-t', template_path]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'wait_for_intf.sh'), self.output_file))

        template_path = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-dhcp-relay', 'docker-dhcp-relay.supervisord.conf.j2')
        argument = ['-m', self.t0_minigraph_common_dhcp_relay, '-p', self.t0_port_config, '-t', template_path]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'docker-dhcp-relay.supervisord.conf'), self.output_file))

        # Test generation of docker-dhcp-relay.supervisord.conf when a vlan is missing ip/ipv6 helpers
        template_path = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-dhcp-relay',
                                     'docker-dhcp-relay.supervisord.conf.j2')
        argument = ['-m', self.no_ip_helper_minigraph, '-p', self.t0_port_config, '-t', template_path]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR,
                                               'docker-dhcp-relay-no-ip-helper.supervisord.conf'), self.output_file))

        # Test generation of docker-dhcp-relay.supervisord.conf when a vlan has secondary subnets specified
        template_path = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-dhcp-relay',
                                     'docker-dhcp-relay.supervisord.conf.j2')
        argument = ['-m', self.t0_minigraph_secondary_subnets, '-p', self.t0_port_config, '-t', template_path]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR,
                                               'docker-dhcp-relay-secondary-subnets.supervisord.conf'), self.output_file))

    def test_radv(self):
        # Test generation of radvd.conf with multiple ipv6 prefixes
        template_path = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-router-advertiser', 'radvd.conf.j2')
        argument = ['-m', self.radv_test_minigraph, '-p', self.t0_port_config, '-t', template_path]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'radvd.conf'), self.output_file))

    def test_lldp(self):
        lldpd_conf_template = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-lldp', 'lldpd.conf.j2')

        expected_mgmt_ipv4 = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'lldp_conf', 'lldpd-ipv4-iface.conf')
        expected_mgmt_ipv6 = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'lldp_conf', 'lldpd-ipv6-iface.conf')
        expected_mgmt_ipv4_and_ipv6 = expected_mgmt_ipv4

        # Test generation of lldpd.conf if IPv4 and IPv6 management interfaces exist
        mgmt_iface_ipv4_and_ipv6_json = os.path.join(self.test_dir, "data", "lldp", "mgmt_iface_ipv4_and_ipv6.json")
        argument = ['-j', mgmt_iface_ipv4_and_ipv6_json, '-t', lldpd_conf_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(expected_mgmt_ipv4_and_ipv6, self.output_file))

        # Test generation of lldpd.conf if management interface IPv4 only exist
        mgmt_iface_ipv4_json = os.path.join(self.test_dir, "data", "lldp", "mgmt_iface_ipv4.json")
        argument = ['-j', mgmt_iface_ipv4_json, '-t', lldpd_conf_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(expected_mgmt_ipv4, self.output_file))

        # Test generation of lldpd.conf if Management interface IPv6 only exist
        mgmt_iface_ipv6_json = os.path.join(self.test_dir, "data", "lldp", "mgmt_iface_ipv6.json")
        argument = ['-j', mgmt_iface_ipv6_json, '-t', lldpd_conf_template]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(expected_mgmt_ipv6, self.output_file))

    def test_ipinip(self):
        ipinip_file = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-orchagent', 'ipinip.json.j2')
        argument = ['-m', self.t0_minigraph, '-p', self.t0_port_config, '-t', ipinip_file]
        self.run_script(argument, output_file=self.output_file)

        sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'ipinip.json')
        assert utils.cmp(sample_output_file, self.output_file), self.run_diff(sample_output_file, self.output_file)

    def test_ipinip_subnet_decap_enable(self):
        ipinip_file = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-orchagent', 'ipinip.json.j2')
        extra_data = {"SUBNET_DECAP": {"AZURE": {"status": "enable"}}}
        argument = ['-m', self.t0_minigraph, '-p', self.t0_port_config, '-a', json.dumps(extra_data), '-t', ipinip_file]
        self.run_script(argument, output_file=self.output_file)

        sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'ipinip_subnet_decap_enable.json')
        assert utils.cmp(sample_output_file, self.output_file), self.run_diff(sample_output_file, self.output_file)

    def test_l2switch_template(self):
        argument = ['-k', 'Mellanox-SN2700', '--preset', 'l2', '-p', self.t0_port_config]
        output = self.run_script(argument)
        output_json = json.loads(output)

        sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'l2switch.json')
        with open(sample_output_file) as sample_output_fd:
            sample_output_json = json.load(sample_output_fd)

        self.assertTrue(json.dumps(sample_output_json, sort_keys=True) == json.dumps(output_json, sort_keys=True))

        template_dir = os.path.join(self.test_dir, '..', 'data', 'l2switch.j2')
        argument = ['-t', template_dir, '-k', 'Mellanox-SN2700', '-p', self.t0_port_config]
        output = self.run_script(argument)
        output_json = json.loads(output)

        self.assertTrue(json.dumps(sample_output_json, sort_keys=True) == json.dumps(output_json, sort_keys=True))

    def test_l1_ports_template(self):
        argument = ['-k', '32x1000Gb', '--preset', 'l1', '-p', self.l1_l3_port_config]
        self.assertTrue(self.yang.validate(argument))
        output = self.run_script(argument)
        output_json = json.loads(output)

        sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'l1_intfs.json')
        with open(sample_output_file) as sample_output_fd:
            sample_output_json = json.load(sample_output_fd)

        self.assertTrue(json.dumps(sample_output_json, sort_keys=True) == json.dumps(output_json, sort_keys=True))

        template_dir = os.path.join(self.test_dir, '..', 'data', 'l1intf.j2')
        argument = ['-t', template_dir, '-k', '32x1000Gb', '-p', self.l1_l3_port_config]
        output = self.run_script(argument)
        output_json = json.loads(output)

        self.assertTrue(json.dumps(sample_output_json, sort_keys=True) == json.dumps(output_json, sort_keys=True))

    def test_l3_ports_template(self):
        argument = ['-k', '32x1000Gb', '--preset', 'l3', '-p', self.l1_l3_port_config]
        self.assertTrue(self.yang.validate(argument))
        output = self.run_script(argument)
        output_json = json.loads(output)

        sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'l3_intfs.json')
        with open(sample_output_file) as sample_output_fd:
            sample_output_json = json.load(sample_output_fd)

        self.assertTrue(json.dumps(sample_output_json, sort_keys=True) == json.dumps(output_json, sort_keys=True))

        template_dir = os.path.join(self.test_dir, '..', 'data', 'l3intf.j2')
        argument = ['-t', template_dir, '-k', '32x1000Gb', '-p', self.l1_l3_port_config]
        output = self.run_script(argument)
        output_json = json.loads(output)

        self.assertTrue(json.dumps(sample_output_json, sort_keys=True) == json.dumps(output_json, sort_keys=True))

    def test_l2switch_template_dualtor(self):
        extra_args = {
            "is_dualtor": True,
            "uplinks": [
                "Ethernet24", "Ethernet28", "Ethernet32", "Ethernet36",
                "Ethernet88", "Ethernet92", "Ethernet96", "Ethernet100"
            ],
            "downlinks": [
                "Ethernet0", "Ethernet4", "Ethernet8", "Ethernet12",
                "Ethernet16", "Ethernet20", "Ethernet40", "Ethernet44",
                "Ethernet48", "Ethernet52", "Ethernet56", "Ethernet60",
                "Ethernet64", "Ethernet68", "Ethernet72", "Ethernet76",
                "Ethernet80", "Ethernet84", "Ethernet104", "Ethernet108",
                "Ethernet112", "Ethernet116", "Ethernet120", "Ethernet124"
            ]
        }
        argument = ['-a', json.dumps(extra_args), '-k', 'Arista-7050CX3-32S-D48C8', '--preset', 'l2', '-p', self.t0_7050cx3_port_config]
        self.assertTrue(self.yang.validate(argument))
        output = self.run_script(argument)
        output_json = json.loads(output)

        sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'l2switch_dualtor.json')
        with open(sample_output_file) as sample_output_fd:
            sample_output_json = json.load(sample_output_fd)
        self.maxDiff = None
        self.assertEqual(sample_output_json, output_json)

    def test_t1_smartswitch_template(self):
        argument = ['-k', 'SSwitch-32x1000Gb', '--preset', 't1-smartswitch', '-p', self.t1_ss_port_config]
        self.assertTrue(self.yang.validate(argument))
        output = self.run_script(argument)
        output_json = json.loads(output)

        sample_output_file = os.path.join(self.test_dir, 'sample_output', 't1-smartswitch.json')
        with open(sample_output_file) as sample_output_fd:
            sample_output_json = json.load(sample_output_fd)

        self.assertTrue(json.dumps(sample_output_json, sort_keys=True) == json.dumps(output_json, sort_keys=True))

    def test_t1_smartswitch_dpu_template(self):
        argument = ['-k', 'SS-DPU-1x400Gb', '--preset', 't1-smartswitch', '-p', self.t1_ss_dpu_port_config]
        self.assertTrue(self.yang.validate(argument))
        output = self.run_script(argument)
        output_json = json.loads(output)

        sample_output_file = os.path.join(self.test_dir, 'sample_output', 't1-smartswitch-dpu.json')
        with open(sample_output_file) as sample_output_fd:
            sample_output_json = json.load(sample_output_fd)

        self.assertTrue(json.dumps(sample_output_json, sort_keys=True) == json.dumps(output_json, sort_keys=True))

    def test_qos_arista7050_render_template(self):
        self._test_qos_render_template('arista', 'x86_64-arista_7050_qx32s', 'Arista-7050-QX-32S', 'sample-arista-7050-t0-minigraph.xml', 'qos-arista7050.json')

    def do_test_qos_and_buffer_lc_render_template(self, platform, vendor, hwsku, minigraph, qos_sample_output, buffer_sample_output, multi_asic):
        dir_path = os.path.join(self.test_dir, '..', '..', '..', 'device', vendor, platform, hwsku)

        if multi_asic == 1:
            # for asic0
            dir_path = os.path.join(dir_path, '0')

        qos_file = os.path.join(dir_path, 'qos.json.j2')
        buffer_file = os.path.join(dir_path, 'buffers.json.j2')
        port_config_ini_file = os.path.join(dir_path, 'port_config.ini')

        # copy qos_config.j2 and buffer_config.j2 to have all templates in one directory
        qos_config_file = os.path.join(self.test_dir, '..', '..', '..', 'files', 'build_templates', 'qos_config.j2')
        shutil.copy2(qos_config_file, dir_path)
        buffer_config_file = os.path.join(self.test_dir, '..', '..', '..', 'files', 'build_templates', 'buffers_config.j2')
        shutil.copy2(buffer_config_file, dir_path)

        for template_file, cfg_file, sample_output_file in [(qos_file, 'qos_config.j2', qos_sample_output),
                                                            (buffer_file, 'buffers_config.j2', buffer_sample_output) ]:
            argument = ['-m', minigraph, '-p', port_config_ini_file, '-t', template_file]
            self.run_script(argument, output_file=self.output_file)

            # cleanup
            cfg_file_new = os.path.join(dir_path, cfg_file)
            os.remove(cfg_file_new)

            sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, sample_output_file)
            assert utils.cmp(sample_output_file, self.output_file), self.run_diff(sample_output_file, self.output_file)

    def test_qos_and_buffer_arista7800r3_48cq2_lc_render_template(self):
        self.do_test_qos_and_buffer_lc_render_template('x86_64-arista_7800r3_48cq2_lc', 'arista', 'Arista-7800R3-48CQ2-C48',\
                                                        self.arista7800r3_48cq2_lc_t2_minigraph, 'qos-arista7800r3-48cq2-lc.json',\
                                                        'buffer-arista7800r3-48cq2-lc.json', 0)

    def test_qos_and_buffer_arista7800r3_48cqm2_lc_render_template(self):
        self.do_test_qos_and_buffer_lc_render_template('x86_64-arista_7800r3_48cqm2_lc', 'arista', 'Arista-7800R3-48CQM2-C48',\
                                                        self.arista7800r3_48cqm2_lc_t2_minigraph, 'qos-arista7800r3-48cqm2-lc.json',\
                                                        'buffer-arista7800r3-48cqm2-lc.json', 0)

    def test_qos_and_buffer_arista7800r3a_36dm2_c36_render_template(self):
        self.do_test_qos_and_buffer_lc_render_template('x86_64-arista_7800r3a_36dm2_lc', 'arista', 'Arista-7800R3A-36DM2-C36',\
                                                       self.arista7800r3a_36dm2_c36_lc_t2_minigraph, 'qos-arista7800r3a-36dm2-c36-lc.json',\
                                                       'buffer-arista7800r3a-36dm2-c36-lc.json', 1)

    def test_qos_and_buffer_arista7800r3a_36dm2_d36_render_template(self):
        self.do_test_qos_and_buffer_lc_render_template('x86_64-arista_7800r3a_36dm2_lc', 'arista', 'Arista-7800R3A-36DM2-D36',\
                                                       self.arista7800r3a_36dm2_d36_lc_t2_minigraph, 'qos-arista7800r3a-36dm2-d36-lc.json',\
                                                       'buffer-arista7800r3a-36dm2-d36-lc.json', 1)

    def test_qos_and_buffer_nokia_ixr7250e_36x100g_render_template(self):
        self.do_test_qos_and_buffer_lc_render_template('x86_64-nokia_ixr7250e_36x400g-r0', 'nokia', 'Nokia-IXR7250E-36x100G',\
                                                       self.nokia_ixr7250e_36x100g_t2_minigraph, 'qos-nokia-ixr7250e-36x100g.json',\
                                                       'buffer-nokia-ixr7250e-36x100g.json', 1)

    def test_qos_and_buffer_nokia_ixr7250e_36x400g_render_template(self):
        self.do_test_qos_and_buffer_lc_render_template('x86_64-nokia_ixr7250e_36x400g-r0', 'nokia', 'Nokia-IXR7250E-36x400G',\
                                                       self.nokia_ixr7250e_36x400g_t2_minigraph, 'qos-nokia-ixr7250e-36x400g.json',\
                                                       'buffer-nokia-ixr7250e-36x400g.json', 1)

    def test_qos_dell9332_render_template(self):
        self._test_qos_render_template('dell', 'x86_64-dellemc_z9332f_d1508-r0', 'DellEMC-Z9332f-O32', 'sample-dell-9332-t1-minigraph.xml', 'qos-dell9332.json')

    def test_qos_dell6100_render_template(self):
        self._test_qos_render_template('dell', 'x86_64-dell_s6100_c2538-r0', 'Force10-S6100', 'sample-dell-6100-t0-minigraph.xml', 'qos-dell6100.json', copy_files=True)

    def test_qos_arista7260_render_template(self):
        self._test_qos_render_template('arista', 'x86_64-arista_7260cx3_64', 'Arista-7260CX3-C64', 'sample-arista-7260-t1-minigraph-remap-disabled.xml', 'qos-arista7260.json')

    def test_qos_arista7060x6_render_template(self):
        self._test_qos_render_template('arista', 'x86_64-arista_7060x6_16pe_384c_b', 'Arista-7060X6-16PE-384C-B-O128S2', 'sample-arista-7060x6-t0-minigraph.xml', 'qos-arista7060x6.json')

    def test_qos_arista7260t0_render_template(self):
        self._test_qos_render_template('arista', 'x86_64-arista_7260cx3_64', 'Arista-7260CX3-D92C16', 'sample-arista-7260-t0-minigraph.xml', 'qos-arista7260-t0.json')

    def _test_qos_render_template(self, vendor, platform, sku, minigraph, expected, copy_files=False):
        file_exist, dir_exist = self.create_machine_conf(platform, vendor)
        dir_path = os.path.join(self.test_dir, '..', '..', '..', 'device', vendor, platform, sku)

        if copy_files:
            self.copy_mmu_templates(dir_path, revert=False)

        qos_file = os.path.join(dir_path, 'qos.json.j2')
        port_config_ini_file = os.path.join(dir_path, 'port_config.ini')

        # copy qos_config.j2 to the SKU directory to have all templates in one directory
        qos_config_file = os.path.join(self.test_dir, '..', '..', '..', 'files', 'build_templates', 'qos_config.j2')
        shutil.copy2(qos_config_file, dir_path)

        minigraph = os.path.join(self.test_dir, minigraph)
        argument = ['-m', minigraph, '-p', port_config_ini_file, '-t', qos_file]
        self.run_script(argument, output_file=self.output_file)

        # cleanup
        qos_config_file_new = os.path.join(dir_path, 'qos_config.j2')
        os.remove(qos_config_file_new)
        if copy_files:
            self.copy_mmu_templates(dir_path, revert=True)

        self.remove_machine_conf(file_exist, dir_exist)

        sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, expected)
        assert utils.cmp(sample_output_file, self.output_file), self.run_diff(sample_output_file, self.output_file)

    def test_qos_dscp_remapping_render_template(self):
        if utils.PYvX_DIR != 'py3':
            # Skip on python2 as the change will not be backported to previous version
            return

        dir_paths = [
            '../../../device/arista/x86_64-arista_7050cx3_32s/Arista-7050CX3-32S-D48C8',
            '../../../device/arista/x86_64-arista_7260cx3_64/Arista-7260CX3-D108C8',
            '../../../device/arista/x86_64-arista_7260cx3_64/Arista-7260CX3-C64',
            '../../../device/arista/x86_64-arista_7050cx3_32s/Arista-7050CX3-32S-D48C8',
            '../../../device/arista/x86_64-arista_7260cx3_64/Arista-7260CX3-D108C8',
            '../../../device/arista/x86_64-arista_7260cx3_64/Arista-7260CX3-C64',
            '../../../device/mellanox/x86_64-mlnx_msn4600c-r0/Mellanox-SN4600C-C64',
            '../../../device/mellanox/x86_64-mlnx_msn4600c-r0/Mellanox-SN4600C-C64',
            '../../../device/mellanox/x86_64-mlnx_msn4600c-r0/Mellanox-SN4600C-D48C40',
            '../../../device/mellanox/x86_64-mlnx_msn4600c-r0/Mellanox-SN4600C-D48C40',
            '../../../device/arista/x86_64-arista_7050_qx32s/Arista-7050-QX-32S'
        ]
        sample_outputs = [
            'qos-arista7050cx3-dualtor.json',
            'qos-arista7260-dualtor.json',
            'qos-arista7260-t1.json',
            'qos-arista7050cx3-dualtor-remap-disabled.json',
            'qos-arista7260-dualtor-remap-disabled.json',
            'qos-arista7260-t1-remap-disabled.json',
            'qos-mellanox4600c-c64.json',
            'qos-mellanox4600c-c64-remap-disabled.json',
            'qos-mellanox4600c-d48c40-t0.json',
            'qos-mellanox4600c-d48c40-t0-remap-disabled.json',
            'qos-arista7050-t0-storage-backend.json'
        ]
        sample_minigraph_files = [
            'sample-arista-7050cx3-dualtor-minigraph.xml',
            'sample-arista-7260-dualtor-minigraph.xml',
            'sample-arista-7260-t1-minigraph.xml',
            'sample-arista-7050cx3-dualtor-minigraph-remap-disabled.xml',
            'sample-arista-7260-dualtor-minigraph-remap-disabled.xml',
            'sample-arista-7260-t1-minigraph-remap-disabled.xml',
            'sample-mellanox-4600c-t1-minigraph.xml',
            'sample-mellanox-4600c-t1-minigraph-remap-disabled.xml',
            'sample-mellanox-4600c-t0-minigraph.xml',
            'sample-mellanox-4600c-t0-minigraph-remap-disabled.xml',
            'sample-arista-7050-t0-storage-backend-minigraph.xml'
        ]

        for i, path in enumerate(dir_paths):
            device_template_path = os.path.join(self.test_dir, path)
            sample_output = sample_outputs[i]
            sample_minigraph_file = os.path.join(self.test_dir,sample_minigraph_files[i])
            qos_file = os.path.join(device_template_path, 'qos.json.j2')
            port_config_ini_file = os.path.join(device_template_path, 'port_config.ini')
            test_output = os.path.join(self.test_dir, 'output.json')

            # copy qos_config.j2 to the target directory to have all templates in one directory
            qos_config_file = os.path.join(self.test_dir, '..', '..', '..', 'files', 'build_templates', 'qos_config.j2')
            shutil.copy2(qos_config_file, device_template_path)

            argument = ['-m', sample_minigraph_file, '-p', port_config_ini_file, '-t', qos_file]
            self.run_script(argument, output_file=test_output)

            # cleanup
            qos_config_file_new = os.path.join(device_template_path, 'qos_config.j2')
            os.remove(qos_config_file_new)

            sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, sample_output)
            assert utils.cmp(sample_output_file, test_output)
            os.remove(test_output)

    def test_qos_smartswitch_render_template(self):
        if utils.PYvX_DIR != 'py3':
            # Skip on python2 as the change will not be backported to previous version
            return

        dir_paths = [
            '../../../device/mellanox/x86_64-mlnx_msn4700-r0/Mellanox-SN4700-O28',
            '../../../device/mellanox/x86_64-mlnx_msn4700-r0/Mellanox-SN4700-O28'
        ]
        sample_outputs = [
            'qos-mellanox4700-o28-t1-smartswitch.json',
            'qos-mellanox4700-o28-t1-smartswitch_dyn.json'
        ]
        sample_minigraph_files = [
            'sample-mellanox-4700-t1-minigraph-smartswitch.xml',
            'sample-mellanox-4700-t1-minigraph-smartswitch.xml'
        ]
        buffer_files = [
            'buffers.json.j2', # traditional buffer mode
            'buffers_dynamic.json.j2' # dynamic buffer mode
        ]

        for i, path in enumerate(dir_paths):
            device_template_path = os.path.join(self.test_dir, path)
            sample_output = sample_outputs[i]
            sample_minigraph_file = os.path.join(self.test_dir,sample_minigraph_files[i])
            qos_file = os.path.join(device_template_path, 'qos.json.j2')
            buf_file = os.path.join(device_template_path, buffer_files[i])
            hwsku_json_file = os.path.join(device_template_path, 'hwsku.json')
            plat_json_file = os.path.join(device_template_path, '../platform.json')
            test_output = os.path.join(self.test_dir, 'output.json')

            # copy qos_config.j2 & buffer_config.j2 to the target directory to have all templates in one directory
            qos_config_file = os.path.join(self.test_dir, '..', '..', '..', 'files', 'build_templates', 'qos_config.j2')
            shutil.copy2(qos_config_file, device_template_path)

            buf_config_file = os.path.join(self.test_dir, '..', '..', '..', 'files', 'build_templates', 'buffers_config.j2')
            shutil.copy2(buf_config_file, device_template_path)

            argument = ['-m', sample_minigraph_file, '-p', plat_json_file, '-S', hwsku_json_file, '-t', "{},config-db".format(qos_file), '-t', "{},config-db".format(buf_file), '--print-data']
            self.run_script(argument, output_file=test_output)

            # cleanup
            qos_config_file_new = os.path.join(device_template_path, 'qos_config.j2')
            os.remove(qos_config_file_new)
            buf_config_file_new = os.path.join(device_template_path, 'buffers_config.j2')
            os.remove(buf_config_file_new)

            sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, sample_output)
            assert utils.cmp_tables(sample_output_file, test_output)
            os.remove(test_output)

    def test_config_brcm_render_template(self):
        if utils.PYvX_DIR != 'py3':
            #Skip on python2 as the change will not be backported to previous version
            return

        config_bcm_sample_outputs = [
            'arista7050cx3-dualtor.config.bcm',
            'arista7260-dualtor.config.bcm',
            'arista7260-t1.config.bcm'
        ]
        sample_minigraph_files = [
            'sample-arista-7050cx3-dualtor-minigraph.xml',
            'sample-arista-7260-dualtor-minigraph.xml',
            'sample-arista-7260-t1-minigraph.xml'
        ]
        for i, config in enumerate(config_bcm_sample_outputs):
            device_template_path = os.path.join(self.test_dir, './data/j2_template')
            config_sample_output = config_bcm_sample_outputs[i]
            sample_minigraph_file = os.path.join(self.test_dir,sample_minigraph_files[i])
            port_config_ini_file = os.path.join(device_template_path, 'port_config.ini')
            config_bcm_file = os.path.join(device_template_path, 'config.bcm.j2')
            config_test_output = os.path.join(self.test_dir, 'config_output.bcm')

            argument = ['-m', sample_minigraph_file, '-p', port_config_ini_file, '-t', config_bcm_file]
            self.run_script(argument, output_file=config_test_output)

            #check output config.bcm
            config_sample_output_file = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, config_sample_output)
            assert utils.cmp(config_sample_output_file, config_test_output)
            os.remove(config_test_output)

    def copy_mmu_templates(self, dir_path, revert=False):
        files_to_copy = ['pg_profile_lookup.ini', 'qos.json.j2', 'buffers_defaults_t0.j2', 'buffers_defaults_t1.j2']

        for file_name in files_to_copy:
            src_file = os.path.join(dir_path, file_name)
            dst_file = os.path.join(self.test_dir, file_name)

            if not revert:
                shutil.copy2(src_file, dst_file)
            else:
                shutil.copy2(dst_file, src_file)
                os.remove(dst_file)

    def _test_buffers_render_template(self, vendor, platform, sku, minigraph, buffer_template, expected, copy_files=False):
        file_exist, dir_exist = self.create_machine_conf(platform, vendor)
        dir_path = os.path.join(self.test_dir, '..', '..', '..', 'device', vendor, platform, sku)

        if copy_files:
            self.copy_mmu_templates(dir_path, revert=False)

        buffers_file = os.path.join(dir_path, buffer_template)
        port_config_ini_file = os.path.join(dir_path, 'port_config.ini')

        # copy buffers_config.j2 to the SKU directory to have all templates in one directory
        buffers_config_file = os.path.join(self.test_dir, '..', '..', '..', 'files', 'build_templates', 'buffers_config.j2')
        shutil.copy2(buffers_config_file, dir_path)

        minigraph = os.path.join(self.test_dir, minigraph)
        argument = ['-m', minigraph, '-p', port_config_ini_file, '-t', buffers_file]
        self.run_script(argument, output_file=self.output_file)

        # cleanup
        buffers_config_file_new = os.path.join(dir_path, 'buffers_config.j2')
        os.remove(buffers_config_file_new)
        self.remove_machine_conf(file_exist, dir_exist)

        out_file_dir = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR)
        expected_files = [expected, self.modify_cable_len(expected, out_file_dir)]
        match = False
        diff = ''
        for out_file in expected_files:
            sample_output_file = os.path.join(out_file_dir, out_file)
            if utils.cmp(sample_output_file, self.output_file):
                match = True
                break
            else:
                diff = diff + str(self.run_diff(sample_output_file, self.output_file))

        os.remove(os.path.join(out_file_dir, expected_files[1]))
        if copy_files:
            self.copy_mmu_templates(dir_path, revert=True)

        assert match, diff

    def test_buffers_dell6100_render_template(self):
        self._test_buffers_render_template('dell', 'x86_64-dell_s6100_c2538-r0', 'Force10-S6100', 'sample-dell-6100-t0-minigraph.xml', 'buffers.json.j2', 'buffers-dell6100.json', copy_files=True)

    def test_buffers_mellanox2410_render_template(self):
        self._test_buffers_render_template('mellanox', 'x86_64-mlnx_msn2410-r0', 'ACS-MSN2410', 'sample-mellanox-2410-t1-minigraph.xml', 'buffers.json.j2', 'buffers-mellanox2410.json')

    def test_buffers_arista7260_render_template(self):
        self._test_buffers_render_template('arista', 'x86_64-arista_7260cx3_64', 'Arista-7260CX3-D92C16', 'sample-arista-7260-t0-minigraph.xml', 'buffers.json.j2', 'buffer-arista7260-t0.json')

    def test_buffers_mellanox2410_dynamic_render_template(self):
        self._test_buffers_render_template('mellanox', 'x86_64-mlnx_msn2410-r0', 'ACS-MSN2410', 'sample-mellanox-2410-t1-minigraph.xml', 'buffers_dynamic.json.j2', 'buffers-mellanox2410-dynamic.json')

    def test_extra_lossless_buffer_for_tunnel_remapping(self):
        if utils.PYvX_DIR != 'py3':
            # Skip on python2 as the change will not be backported to previous version
            return

        TEST_DATA = [
            # (vendor, platform, sku, minigraph, buffer_template, sample_output )
            ('arista', 'x86_64-arista_7050cx3_32s', 'Arista-7050CX3-32S-D48C8', 'sample-arista-7050cx3-dualtor-minigraph.xml', 'buffers.json.j2', 'buffer-arista7050cx3-dualtor.json'),
            ('arista', 'x86_64-arista_7050cx3_32s', 'Arista-7050CX3-32S-D48C8', 'sample-arista-7050cx3-dualtor-minigraph-remap-disabled.xml', 'buffers.json.j2', 'buffer-arista7050cx3-dualtor-remap-disabled.json'),
            ('arista', 'x86_64-arista_7260cx3_64', 'Arista-7260CX3-D108C8', 'sample-arista-7260-dualtor-minigraph.xml', 'buffers.json.j2', 'buffer-arista7260-dualtor.json'),
            ('arista', 'x86_64-arista_7260cx3_64', 'Arista-7260CX3-D108C8', 'sample-arista-7260-dualtor-minigraph-remap-disabled.xml', 'buffers.json.j2', 'buffer-arista7260-dualtor-remap-disabled.json'),
            ('arista', 'x86_64-arista_7260cx3_64', 'Arista-7260CX3-C64', 'sample-arista-7260-t1-minigraph.xml', 'buffers.json.j2', 'buffer-arista7260-t1.json'),
            ('arista', 'x86_64-arista_7260cx3_64', 'Arista-7260CX3-C64', 'sample-arista-7260-t1-minigraph-remap-disabled.xml', 'buffers.json.j2', 'buffer-arista7260-t1-remap-disabled.json'),
            ('mellanox', 'x86_64-mlnx_msn4600c-r0', 'Mellanox-SN4600C-C64', 'sample-mellanox-4600c-t1-minigraph.xml', 'buffers_dynamic.json.j2', 'buffers-mellanox4600c-t1-dynamic.json'),
            ('mellanox', 'x86_64-mlnx_msn4600c-r0', 'Mellanox-SN4600C-C64', 'sample-mellanox-4600c-t1-minigraph.xml', 'buffers.json.j2', 'buffers-mellanox4600c-t1.json'),
            ('mellanox', 'x86_64-mlnx_msn4600c-r0', 'Mellanox-SN4600C-C64', 'sample-mellanox-4600c-t1-minigraph-remap-disabled.xml', 'buffers_dynamic.json.j2', 'buffers-mellanox4600c-t1-dynamic-remap-disabled.json'),
            ('mellanox', 'x86_64-mlnx_msn4600c-r0', 'Mellanox-SN4600C-C64', 'sample-mellanox-4600c-t1-minigraph-remap-disabled.xml', 'buffers.json.j2', 'buffers-mellanox4600c-t1-remap-disabled.json'),
            ('mellanox', 'x86_64-mlnx_msn4600c-r0', 'Mellanox-SN4600C-D48C40', 'sample-mellanox-4600c-t0-minigraph.xml', 'buffers_dynamic.json.j2', 'buffers-mellanox4600c-t0-dynamic.json'),
            ('mellanox', 'x86_64-mlnx_msn4600c-r0', 'Mellanox-SN4600C-D48C40', 'sample-mellanox-4600c-t0-minigraph.xml', 'buffers.json.j2', 'buffers-mellanox4600c-t0.json'),
            ('mellanox', 'x86_64-mlnx_msn4600c-r0', 'Mellanox-SN4600C-D48C40', 'sample-mellanox-4600c-t0-minigraph-remap-disabled.xml', 'buffers_dynamic.json.j2', 'buffers-mellanox4600c-t0-dynamic-remap-disabled.json'),
            ('mellanox', 'x86_64-mlnx_msn4600c-r0', 'Mellanox-SN4600C-D48C40', 'sample-mellanox-4600c-t0-minigraph-remap-disabled.xml', 'buffers.json.j2', 'buffers-mellanox4600c-t0-remap-disabled.json')
         ]

        for test_data in TEST_DATA:
            self._test_buffers_render_template(vendor=test_data[0],
                                                platform=test_data[1],
                                                sku=test_data[2],
                                                minigraph=test_data[3],
                                                buffer_template=test_data[4],
                                                expected=test_data[5])

    def test_ipinip_multi_asic(self):
        ipinip_file = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-orchagent', 'ipinip.json.j2')
        argument = ['-m', self.multi_asic_minigraph, '-p', self.multi_asic_port_config, '-t', ipinip_file, '-n', 'asic0']
        print(argument)
        self.run_script(argument, output_file=self.output_file)
        sample_output_file = os.path.join(self.test_dir, 'multi_npu_data', utils.PYvX_DIR, 'ipinip.json')
        assert utils.cmp(sample_output_file, self.output_file), self.run_diff(sample_output_file, self.output_file)

    def test_swss_switch_render_template(self):
        switch_template = os.path.join(
            self.test_dir, '..', '..', '..', 'dockers', 'docker-orchagent',
            'switch.json.j2'
        )
        constants_yml = os.path.join(
            self.test_dir, '..', '..', '..', 'files', 'image_config',
            'constants', 'constants.yml'
        )
        test_list = {
            "t1": {
                "graph": self.t1_mlnx_minigraph,
                "port_config": self.mlnx_port_config,
                "output": "t1-switch.json"
            },
            "t0": {
                "graph": self.t0_minigraph,
                "port_config": self.t0_port_config,
                "output": "t0-switch.json"
            },
        }
        for _, v in test_list.items():
            argument = ["-m", v["graph"], "-p", v["port_config"], "-y", constants_yml, "-t", switch_template]
            sample_output_file = os.path.join(
                self.test_dir, 'sample_output', v["output"]
            )
            self.run_script(argument, output_file=self.output_file)
            assert utils.cmp(sample_output_file, self.output_file), self.run_diff(sample_output_file, self.output_file)

    def test_swss_switch_render_template_multi_asic(self):
        # verify the ECMP hash seed changes per namespace
        switch_template = os.path.join(
            self.test_dir, '..', '..', '..', 'dockers', 'docker-orchagent',
            'switch.json.j2'
        )
        constants_yml = os.path.join(
            self.test_dir, '..', '..', '..', 'files', 'image_config',
            'constants', 'constants.yml'
        )
        test_list = {
            "0": {
                "namespace_id": "1",
                "output": "t0-switch-masic1.json"
            },
            "1": {
                "namespace_id": "3",
                "output": "t0-switch-masic3.json"
            },
        }
        for _, v in test_list.items():
            os.environ["NAMESPACE_ID"] = v["namespace_id"]
            argument = ["-m", self.t1_mlnx_minigraph, "-y", constants_yml, "-t", switch_template]
            sample_output_file = os.path.join(
                self.test_dir, 'sample_output', v["output"]
            )
            self.run_script(argument, output_file=self.output_file)
            assert utils.cmp(sample_output_file, self.output_file), self.run_diff(sample_output_file, self.output_file)
        os.environ["NAMESPACE_ID"] = ""

    def test_swss_switch_render_template_t2(self):
        # verify the ECMP hash seed changes per namespace
        switch_template = os.path.join(
            self.test_dir, '..', '..', '..', 'dockers', 'docker-orchagent',
            'switch.json.j2'
        )
        constants_yml = os.path.join(
            self.test_dir, '..', '..', '..', 'files', 'image_config',
            'constants', 'constants.yml'
        )
        test_list = {
            "0": {
                "namespace_id": "1",
                "output": "t2-switch-masic1.json"
            },
            "1": {
                "namespace_id": "3",
                "output": "t2-switch-masic3.json"
            },
        }
        for _, v in test_list.items():
            os.environ["NAMESPACE_ID"] = v["namespace_id"]
            argument = ["-m", self.t2_sample_graph_chassis_packet, "-y", constants_yml, "-t", switch_template]
            sample_output_file = os.path.join(
                self.test_dir, 'sample_output', v["output"]
            )
            self.run_script(argument, output_file=self.output_file)
            assert utils.cmp(sample_output_file, self.output_file), self.run_diff(sample_output_file, self.output_file)
        os.environ["NAMESPACE_ID"] = ""

    def test_ndppd_conf(self):
        conf_template = os.path.join(self.test_dir, "ndppd.conf.j2")
        vlan_interfaces_json = os.path.join(self.test_dir, "data", "ndppd", "vlan_interfaces.json")
        expected = os.path.join(self.test_dir, "sample_output", utils.PYvX_DIR, "ndppd.conf")

        argument = ['-j', vlan_interfaces_json, '-t', conf_template]
        self.run_script(argument, output_file=self.output_file)
        assert utils.cmp(expected, self.output_file), self.run_diff(expected, self.output_file)

    def test_ntp_conf(self):
        conf_template = os.path.join(self.test_dir, "chrony.conf.j2")
        config_db_ntp_json = os.path.join(self.test_dir, "data", "ntp", "ntp_interfaces.json")
        expected = os.path.join(self.test_dir, "sample_output", utils.PYvX_DIR, "chrony.conf")

        argument = ['-j', config_db_ntp_json, '-t', conf_template]
        self.run_script(argument, output_file=self.output_file)
        assert utils.cmp(expected, self.output_file), self.run_diff(expected, self.output_file)

    def test_ntp_smartswitch_conf(self):
        conf_template = os.path.join(self.test_dir, "chrony.conf.j2")
        config_db_ntp_json = os.path.join(self.test_dir, "data", "ntp", "ntp_smartswitch_interfaces.json")
        expected = os.path.join(self.test_dir, "sample_output", utils.PYvX_DIR, "chrony_smartswitch.conf")

        argument = ['-j', config_db_ntp_json, '-t', conf_template]
        self.run_script(argument, output_file=self.output_file)
        assert utils.cmp(expected, self.output_file), self.run_diff(expected, self.output_file)

    def test_ntp_smartswitch_dpu_conf(self):
        conf_template = os.path.join(self.test_dir, "chrony.conf.j2")
        config_db_ntp_json = os.path.join(self.test_dir, "data", "ntp", "ntp_smartswitch_dpu_interfaces.json")
        expected = os.path.join(self.test_dir, "sample_output", utils.PYvX_DIR, "chrony_smartswitch_dpu.conf")

        argument = ['-j', config_db_ntp_json, '-t', conf_template]
        self.run_script(argument, output_file=self.output_file)
        assert utils.cmp(expected, self.output_file), self.run_diff(expected, self.output_file)

    def test_ntp_keys(self):
        conf_template = os.path.join(self.test_dir, "chrony.keys.j2")
        config_db_ntp_json = os.path.join(self.test_dir, "data", "ntp", "ntp_interfaces.json")
        expected = os.path.join(self.test_dir, "sample_output", utils.PYvX_DIR, "chrony.keys")

        argument = ['-j', config_db_ntp_json, '-t', conf_template]
        self.run_script(argument, output_file=self.output_file)
        assert utils.cmp(expected, self.output_file), self.run_diff(expected, self.output_file)

    def test_backend_acl_template_render(self):
        acl_template = os.path.join(
            self.test_dir, '..', '..', '..', 'files', 'build_templates',
            'backend_acl.j2'
        )
        test_list = {
            'single_vlan': {
                'input': 'single_vlan.json',
                'output': 'acl_single_vlan.json'
            },
            'multi_vlan': {
                'input': 'multi_vlan.json',
                'output': 'acl_multi_vlan.json'
            },
        }
        for _, v in test_list.items():
            input_file = os.path.join(
                self.test_dir, 'data', 'backend_acl', v['input']
            )
            argument = ["-j", input_file, "-t", acl_template]
            sample_output_file = os.path.join(
                self.test_dir, 'data', 'backend_acl', v['output']
            )
            self.run_script(argument, output_file=self.output_file)
            assert utils.cmp(sample_output_file, self.output_file), self.run_diff(sample_output_file, self.output_file)

    def test_dns_template_render(self):
        conf_template = os.path.join(self.test_dir, '..', '..', '..', 'files', 'image_config', 'resolv-config', 'resolv.conf.j2')
        static_dns_conf = os.path.join(self.test_dir, "data", "dns", "static_dns.json")
        expected = os.path.join(self.test_dir, "data", "dns", "resolv.conf")

        argument = ['-j', static_dns_conf, '-t', conf_template]
        self.run_script(argument, output_file=self.output_file)
        assert utils.cmp(expected, self.output_file), self.run_diff(expected, self.output_file)

    def test_buffers_edgezone_aggregator_render_template(self):
        self._test_buffers_render_template('arista', 'x86_64-arista_7060_cx32s', 'Arista-7060CX-32S-D48C8', 'sample-arista-7060-t0-minigraph.xml', 'buffers.json.j2', 'buffer-arista7060-t0.json')

    def test_rsyslog_conf(self):
        if utils.PYvX_DIR != 'py3':
            # Skip on python2 as the change will not be backported to previous version
            return

        conf_template = os.path.join(self.test_dir, '..', '..', '..', 'files', 'image_config', 'rsyslog',
                                     'rsyslog.conf.j2')
        config_db_json = os.path.join(self.test_dir, "data", "rsyslog", "config_db.json")
        additional_data = "{\"udp_server_ip\": \"1.1.1.1\", \"hostname\": \"kvm-host\"}"

        argument = ['-j', config_db_json, '-t', conf_template, '-a', additional_data]
        self.run_script(argument, output_file=self.output_file)
        with open(self.output_file) as file:
            pattern = r'^action.*Device="eth0".*'
            for line in file:
                assert not bool(re.match(pattern, line.strip())), "eth0 is not allowed in Mgfx device"
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, 'rsyslog.conf'),
                                  self.output_file))

    def test_rsyslog_conf_docker0_ip(self):
        if utils.PYvX_DIR != 'py3':
            # Skip on python2 as the change will not be backported to previous version
            return

        conf_template = os.path.join(self.test_dir, '..', '..', '..', 'files', 'image_config', 'rsyslog',
                                     'rsyslog.conf.j2')
        config_db_json = os.path.join(self.test_dir, "data", "rsyslog", "config_db.json")
        additional_data = "{\"udp_server_ip\": \"1.1.1.1\", \"hostname\": \"kvm-host\", " + \
                          "\"docker0_ip\": \"2.2.2.2\"}"

        argument = ['-j', config_db_json, '-t', conf_template, '-a', additional_data]
        self.run_script(argument, output_file=self.output_file)
        self.assertTrue(utils.cmp(os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR,
                                               'rsyslog_with_docker0.conf'), self.output_file))

    def tearDown(self):
        os.environ["CFGGEN_UNIT_TESTING"] = ""
        try:
            os.remove(self.output_file)
        except OSError:
            pass
