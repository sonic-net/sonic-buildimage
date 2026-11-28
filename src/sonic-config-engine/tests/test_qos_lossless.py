# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Verify lossless QoS settings using production cfggen rendering."""

import copy
import json
import os
import runpy
import subprocess
import sys
from pathlib import Path
from types import SimpleNamespace
from unittest import TestCase
from unittest.mock import patch


class TestQosLossless(TestCase):
    """Check CLI sample output and cache production templates for role checks."""

    test_dir = Path(__file__).resolve().parent
    repo_dir = test_dir.parents[2]
    backend_skus = ('Mellanox-SN4700-O10C88', 'Mellanox-SN4700-O10X88')

    @classmethod
    def setUpClass(cls):
        with patch.object(sys, 'path', [str(cls.test_dir.parent)] + sys.path):
            cls.cfggen = runpy.run_path(str(cls.test_dir.parent / 'sonic-cfggen'))
        cls.fixture = {}
        cls.cfggen['_process_json'](SimpleNamespace(
            json=[str(cls.test_dir / 'data/qos/leaf-remap.json')]), cls.fixture)
        cls.templates = {}

    def render_qos(self, sku, role='LeafRouter', remap=True, use_cli=False):
        data = {
            'DEVICE_METADATA': {'localhost': {'type': role, 'hwsku': sku}},
            'SYSTEM_DEFAULTS': {'tunnel_qos_remap': {
                'status': 'enabled' if remap else 'disabled'}},
        }
        if role == 'ToRRouter':
            data['DEVICE_METADATA']['localhost']['subtype'] = 'DualToR'
        template = self.repo_dir / 'device/mellanox/x86_64-mlnx_msn4700-r0' / sku / 'qos.json.j2'
        if use_cli:
            output = subprocess.check_output([
                sys.executable, str(self.test_dir.parent / 'sonic-cfggen'),
                '-j', str(self.test_dir / 'data/qos/leaf-remap.json'),
                '-a', json.dumps(data), '-t', str(template),
            ], env=dict(os.environ, PLATFORM='x86_64-mlnx_msn4700-r0'))
            return json.loads(output)

        if sku not in self.templates:
            with patch.dict(os.environ, PLATFORM='x86_64-mlnx_msn4700-r0'):
                env = self.cfggen['_get_jinja2_env']([
                    str(template.parent), str(self.repo_dir / 'files/build_templates')])
            self.templates[sku] = env.get_template(template.name)
        context = copy.deepcopy(self.fixture)
        self.cfggen['deep_update'](context, data)
        return json.loads(self.templates[sku].render(context))

    def assert_lossy_extra_queues(self, config):
        self.assertNotIn('AZURE_DUALTOR', config['PFC_PRIORITY_TO_PRIORITY_GROUP_MAP'])
        for port in ('Ethernet0', 'Ethernet4'):
            qos = config['PORT_QOS_MAP'][port]
            self.assertEqual(qos['pfc_enable'], '')
            self.assertEqual(qos['pfcwd_sw_enable'], '')
            self.assertEqual(qos['pfc_to_pg_map'], 'AZURE')
            for queue in ('2', '6'):
                self.assertEqual(config['QUEUE'][port + '|' + queue],
                                 {'scheduler': 'scheduler.0'})

    def test_empty_lossless_leaf_remap_sample(self):
        with (self.test_dir / 'sample_output/py3/qos-mellanox4700-o10-leaf-remap.json').open() as stream:
            expected = json.load(stream)
        for sku in self.backend_skus:
            with self.subTest(sku=sku):
                config = self.render_qos(sku, use_cli=True)
                self.assertEqual(config, expected)
                self.assert_lossy_extra_queues(config)
                # Confirm the ToR-facing port still takes the tunnel-remap path.
                self.assertEqual(config['DSCP_TO_TC_MAP']['AZURE']['2'], '2')
                self.assertEqual(config['PORT_QOS_MAP']['Ethernet4']['dscp_to_tc_map'],
                                 'AZURE_UPLINK')

    def test_empty_lossless_other_roles(self):
        for sku in self.backend_skus:
            for role, remap in (('ToRRouter', True), ('LeafRouter', False),
                                ('BackEndLeafRouter', False), ('BackEndLeafRouter', True)):
                with self.subTest(sku=sku, role=role, remap=remap):
                    self.assert_lossy_extra_queues(self.render_qos(sku, role, remap))

    def test_default_lossless_classes(self):
        for role, extra_port in (('LeafRouter', 'Ethernet0'), ('ToRRouter', 'Ethernet4')):
            for remap in (False, True):
                with self.subTest(role=role, remap=remap):
                    config = self.render_qos('Mellanox-SN4700-O32', role, remap)
                    self.assertEqual('AZURE_DUALTOR' in config['PFC_PRIORITY_TO_PRIORITY_GROUP_MAP'],
                                     remap)
                    for port in ('Ethernet0', 'Ethernet4'):
                        extra = remap and port == extra_port
                        qos = config['PORT_QOS_MAP'][port]
                        self.assertEqual(qos['pfc_enable'], '2,3,4,6' if extra else '3,4')
                        self.assertEqual(qos['pfcwd_sw_enable'], '3,4')
                        self.assertEqual(qos['pfc_to_pg_map'], 'AZURE_DUALTOR' if extra else 'AZURE')
                        for queue in ('2', '6'):
                            expected = {'scheduler': 'scheduler.0'}
                            if extra:
                                expected = {'scheduler': 'scheduler.1', 'wred_profile': 'AZURE_LOSSLESS'}
                            self.assertEqual(config['QUEUE'][port + '|' + queue], expected)
