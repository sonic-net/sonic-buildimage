import filecmp
import json
import os
import subprocess

import tests.common_utils as utils
from sonic_py_common.general import getstatusoutput_noshell
from unittest import TestCase

class TestCfgGen(TestCase):
    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.script_file = [utils.PYTHON_INTERPRETTER, os.path.join(self.test_dir, '..', 'sonic-cfggen')]
        self.t0_minigraph = os.path.join(self.test_dir, 't0-sample-graph.xml')
        self.t0_port_config = os.path.join(self.test_dir, 't0-sample-port-config.ini')
        self.output_file = os.path.join(self.test_dir, 'output')

    def tearDown(self):
        try:
            os.remove(self.output_file)
        except OSError:
            pass


    def run_script(self, argument, check_stderr=False, output_file=None):
#        print '\n    Running sonic-cfggen ' + argument

        if check_stderr:
            output = subprocess.check_output(self.script_file + argument, stderr=subprocess.STDOUT)
        else:
            output = subprocess.check_output(self.script_file + argument)

        if utils.PY3x:
            output = output.decode()
        if output_file:
            with open(output_file, 'w') as f:
                f.write(output)

        linecount = output.strip().count('\n')
        if linecount <= 0:
            print('    Output: ' + output.strip())
        else:
            print('    Output: ({0} lines, {1} bytes)'.format(linecount + 1, len(output)))
        return output

    def run_diff(self, file1, file2):
        _, output = getstatusoutput_noshell(['diff', '-u', file1, file2])
        return output

    def render_template(self, template, extra_data=None):
        template_dir = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-fpm-frr', "frr")
        conf_template = os.path.join(template_dir, template)
        constants = os.path.join(self.test_dir, 'data', 'constants.yml')
        cmd = ['-m', self.t0_minigraph, '-p', self.t0_port_config, '-y', constants, '-t', conf_template, '-T', template_dir]
        if extra_data:
            cmd = ['-a', json.dumps(extra_data)] + cmd
        return self.run_script(cmd, output_file=self.output_file)

    def run_case(self, template, target, extra_data=None):
        self.render_template(template, extra_data)
        original_filename = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, target)
        r = filecmp.cmp(original_filename, self.output_file)
        diff_output = self.run_diff(original_filename, self.output_file) if not r else ""

        return r, "Diff:\n" + diff_output

    def test_config_frr(self):
        self.assertTrue(*self.run_case('frr.conf.j2', 'frr.conf'))

    def test_bgpd_frr(self):
        self.assertTrue(*self.run_case('bgpd/bgpd.conf.j2', 'bgpd_frr.conf'))

    def test_bgpd_frr_graceful_restart(self):
        for device_type in ('ToRRouter', 'LeafRouter', 'BackEndLeafRouter',
                            'MgmtToRRouter', 'BmcMgmtToRRouter'):
            extra_data = {"DEVICE_METADATA": {"localhost": {"type": device_type}}}
            output = self.render_template('bgpd/bgpd.conf.j2', extra_data)
            self.assertIn(
                '  bgp graceful-restart restart-time 240\n'
                '  bgp graceful-restart\n'
                '  bgp graceful-restart preserve-fw-state\n'
                '  bgp graceful-restart select-defer-time 45\n',
                output, device_type)

    def test_bgpd_frr_graceful_restart_disabled(self):
        for device_type in ('ToRRouter', 'LeafRouter', 'BackEndLeafRouter',
                            'MgmtToRRouter', 'BmcMgmtToRRouter'):
            extra_data = {
                "DEVICE_METADATA": {"localhost": {"type": device_type}},
                "constants": {"bgp": {"graceful_restart": {"enabled": False}}}
            }
            output = self.render_template('bgpd/bgpd.conf.j2', extra_data)
            self.assertNotIn('  bgp graceful-restart\n', output, device_type)
            self.assertNotIn('  bgp graceful-restart ', output, device_type)

    def test_bgpd_frr_graceful_restart_excluded_roles(self):
        for device_type in ('SpineRouter', 'UpperRegionalHub'):
            extra_data = {"DEVICE_METADATA": {"localhost": {"type": device_type}}}
            output = self.render_template('bgpd/bgpd.conf.j2', extra_data)
            self.assertNotIn('  bgp graceful-restart\n', output, device_type)
            self.assertNotIn('  bgp graceful-restart ', output, device_type)
            if device_type == 'UpperRegionalHub':
                self.assertIn('  bgp graceful-restart-disable\n', output)

    def test_bgpd_frr_graceful_restart_timers(self):
        for device_type in ('MgmtToRRouter', 'BmcMgmtToRRouter'):
            extra_data = {
                "DEVICE_METADATA": {"localhost": {"type": device_type}},
                "constants": {"bgp": {"graceful_restart": {
                    "restart_time": 300,
                    "select_defer_time": 60
                }}}
            }
            output = self.render_template('bgpd/bgpd.conf.j2', extra_data)
            self.assertIn('  bgp graceful-restart restart-time 300\n', output, device_type)
            self.assertIn('  bgp graceful-restart select-defer-time 60\n', output, device_type)

    def test_zebra_frr(self):
        self.assertTrue(*self.run_case('zebra/zebra.conf.j2', 'zebra_frr.conf'))

    def test_tcp_vty_disabled(self):
        repo_root = os.path.join(self.test_dir, '..', '..', '..')

        supervisord_template = os.path.join(
            repo_root, 'dockers', 'docker-fpm-frr', 'frr',
            'supervisord', 'supervisord.conf.common.j2')
        with open(supervisord_template) as f:
            supervisord_config = f.read()
        daemon_commands = [
            line for line in supervisord_config.splitlines()
            if line.startswith('command=/usr/lib/frr/')
        ]
        self.assertTrue(daemon_commands)
        for command in daemon_commands:
            self.assertIn(' -P 0', command)

        readiness_script = os.path.join(
            repo_root, 'dockers', 'docker-fpm-frr', 'zsocket.sh')
        with open(readiness_script) as f:
            readiness_check = f.read()
        self.assertIn('vtysh -d zebra', readiness_check)
        self.assertNotIn('/dev/tcp', readiness_check)

        gobgp_dockerfile = os.path.join(
            repo_root, 'dockers', 'docker-fpm-gobgp', 'Dockerfile.j2')
        with open(gobgp_dockerfile) as f:
            gobgp_build = f.read()
        self.assertIn('s/"$/ -P 0"/', gobgp_build)

    def test_bgpd_frr_dualtor(self):
        extra_data = {"DEVICE_METADATA": {"localhost": {"subtype": "DualToR"}}}
        self.assertTrue(*self.run_case('bgpd/bgpd.conf.j2', 'bgpd_frr_dualtor.conf', extra_data=extra_data))

    def test_zebra_frr_dualtor(self):
        extra_data = {"DEVICE_METADATA": {"localhost": {"subtype": "DualToR"}}}
        self.assertTrue(*self.run_case('zebra/zebra.conf.j2', 'zebra_frr_dualtor.conf', extra_data=extra_data))

    def test_bgpd_frr_bmp(self):
        extra_data = {"FEATURE": {"frr_bmp": {"state": "enabled"}}}
        self.assertTrue(*self.run_case('bgpd/bgpd.conf.j2', 'bgpd_frr_bmp.conf', extra_data=extra_data))
