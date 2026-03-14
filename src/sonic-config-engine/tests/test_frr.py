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

    def run_case(self, template, target, extra_data=None):
        template_dir = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-fpm-frr', "frr")
        conf_template = os.path.join(template_dir, template)
        constants = os.path.join(self.test_dir, '..', '..', '..', 'files', 'image_config', 'constants', 'constants.yml')
        cmd = ['-m', self.t0_minigraph, '-p', self.t0_port_config, '-y', constants, '-t', conf_template, '-T', template_dir]
        if extra_data:
            cmd = ['-a', json.dumps(extra_data)] + cmd
        self.run_script(cmd, output_file=self.output_file)

        original_filename = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, target)
        r = filecmp.cmp(original_filename, self.output_file)
        diff_output = self.run_diff(original_filename, self.output_file) if not r else ""

        return r, "Diff:\n" + diff_output

    def test_config_frr(self):
        self.assertTrue(*self.run_case('frr.conf.j2', 'frr.conf'))

    def test_bgpd_frr(self):
        self.assertTrue(*self.run_case('bgpd/bgpd.conf.j2', 'bgpd_frr.conf'))

    def test_zebra_frr(self):
        self.assertTrue(*self.run_case('zebra/zebra.conf.j2', 'zebra_frr.conf'))

    def test_bgpd_frr_dualtor(self):
        extra_data = {"DEVICE_METADATA": {"localhost": {"subtype": "DualToR"}}}
        self.assertTrue(*self.run_case('bgpd/bgpd.conf.j2', 'bgpd_frr_dualtor.conf', extra_data=extra_data))

    def test_zebra_frr_dualtor(self):
        extra_data = {"DEVICE_METADATA": {"localhost": {"subtype": "DualToR"}}}
        self.assertTrue(*self.run_case('zebra/zebra.conf.j2', 'zebra_frr_dualtor.conf', extra_data=extra_data))

    def test_bgpd_frr_bmp(self):
        extra_data = {"FEATURE": {"frr_bmp": {"state": "enabled"}}}
        self.assertTrue(*self.run_case('bgpd/bgpd.conf.j2', 'bgpd_frr_bmp.conf', extra_data=extra_data))