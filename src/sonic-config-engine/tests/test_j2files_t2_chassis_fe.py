import filecmp
import json
import os
import shutil
import subprocess
from unittest import TestCase
import tests.common_utils as utils
from sonic_py_common.general import getstatusoutput_noshell


class TestJ2FilesT2ChassisFe(TestCase):
    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.script_file = [utils.PYTHON_INTERPRETTER, os.path.join(self.test_dir, '..', 'sonic-cfggen')]
        self.t2_chassis_fe_minigraph = os.path.join(self.test_dir, 't2-chassis-fe-graph.xml')
        self.t2_chassis_fe_vni_minigraph = os.path.join(self.test_dir, 't2-chassis-fe-graph-vni.xml')
        self.t2_chassis_fe_pc_minigraph = os.path.join(self.test_dir, 't2-chassis-fe-graph-pc.xml')
        self.t2_chassis_fe_port_config = os.path.join(self.test_dir, 't2-chassis-fe-port-config.ini')
        self.output_file = os.path.join(self.test_dir, 'output')

    def tearDown(self):
        try:
            os.remove(self.output_file)
        except OSError:
            pass

    def run_script(self, argument, output_file=None):
        print('CMD: sonic-cfggen ' + ' '.join(argument))
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

    def run_case(self, minigraph, template, target):
        template_dir = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-fpm-frr', "frr")
        conf_template = os.path.join(template_dir, template)
        constants = os.path.join(self.test_dir, '..', '..', '..', 'files', 'image_config', 'constants', 'constants.yml')
        cmd = ["-m", minigraph, "-p", self.t2_chassis_fe_port_config, "-y", constants, "-t", conf_template, "-T", template_dir]
        self.run_script(cmd, output_file=self.output_file)

        original_filename = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, target)
        r = filecmp.cmp(original_filename, self.output_file)
        diff_output = self.run_diff(original_filename, self.output_file) if not r else ""

        return r, "Diff:\n" + diff_output

    # Test zebra.conf in FRR docker for a T2 chassis frontend (fe)
    def test_t2_chassis_fe_zebra_frr(self):
        self.assertTrue(*self.run_case(self.t2_chassis_fe_minigraph, 'zebra/zebra.conf.j2', 't2-chassis-fe-zebra.conf'))

    # Test zebra.conf in FRR docker for a T2 chassis frontend (fe) switch with specified VNI
    def test_t2_chassis_fe_vni_zebra_frr(self):
        self.assertTrue(*self.run_case(self.t2_chassis_fe_vni_minigraph, 'zebra/zebra.conf.j2', 't2-chassis-fe-vni-zebra.conf'))

    # Test bgpd.conf in FRR docker for a T2 chassis frontend (fe)
    def test_t2_chassis_frontend_bgpd_frr(self):
        self.assertTrue(*self.run_case(self.t2_chassis_fe_minigraph, 'bgpd/bgpd.conf.j2', 't2-chassis-fe-bgpd.conf'))

