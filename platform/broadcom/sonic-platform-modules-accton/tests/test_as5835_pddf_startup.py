"""Exercise AS5835 context loading with real PDDF initialization and file I/O."""
import builtins
import glob
import json
import os
from pathlib import Path
import re
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

from platform_test_utils import DEVICE, SOURCE, definitions
from test_as5835_pddf import load_file, PDDF


class As5835PddfStartup(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.platform = 'x86_64-accton_as5835_54x-r0'
        self.canonical = self.root / 'usr/share/sonic/device' / self.platform
        self.alias = self.root / 'usr/share/sonic/platform'
        self.data = {'PLATFORM': {'num_psus': 2}}
        self.plugin = {'FAN': {'present': {}}}
        self.write_data(self.canonical)
        self.api_class = definitions(
            PDDF + 'pddfapi.py', os=os, glob=glob, re=re, json=json,
            __file__=str(SOURCE / (PDDF + 'pddfapi.py')),
            device_info=SimpleNamespace(get_platform_and_hwsku=lambda: (self.platform, 'Accton-AS5835-54X')),
            LED_CTRL_LOCK_PATH=str(self.root / 'var/lock/pddf-locks/pddf-api-led.lock'),
        )['PddfApi']
        self.api_class.__init__.__globals__['LED_CTRL_LOCK_PATH'] = str(
            self.root / 'var/lock/pddf-locks/pddf-api-led.lock')

    def write_data(self, base):
        (base / 'pddf').mkdir(parents=True)
        (base / 'pddf/pddf-device.json').write_text(json.dumps(self.data))
        (base / 'pddf/pd-plugin.json').write_text(json.dumps(self.plugin))

    def load_context(self):
        return load_file('as5835_context_test', DEVICE + 'as5835_54x-r0/sonic_platform/pddf_context.py')

    def mapped(self, value):
        path = os.fspath(value)
        if path.startswith('/usr/share/sonic/'):
            return str(self.root / path.lstrip('/'))
        return path

    def initialize(self):
        # Redirect only the fixed SONiC paths; the production PDDF constructor
        # still creates a real symlink and reads both real JSON files.
        real_exists, real_symlink, real_unlink = os.path.exists, os.symlink, os.unlink
        real_open = builtins.open
        with patch.dict(sys.modules, {
            'sonic_platform_pddf_base.pddfapi': SimpleNamespace(PddfApi=self.api_class),
        }):
            context = self.load_context()
        with patch.object(os.path, 'exists', side_effect=lambda p: real_exists(self.mapped(p))), \
             patch.object(os, 'symlink', side_effect=lambda src, dst: real_symlink(self.mapped(src), self.mapped(dst))), \
             patch.object(os, 'unlink', side_effect=lambda p: real_unlink(self.mapped(p))), \
             patch.object(builtins, 'open', side_effect=lambda p, *a, **kw: real_open(self.mapped(p), *a, **kw)):
            return context.load_environment()

    def test_host_startup_creates_missing_platform_link_before_plugin_read(self):
        self.assertFalse(self.alias.exists())
        api, plugin = self.initialize()
        self.assertTrue(self.alias.is_symlink())
        self.assertEqual(self.alias.resolve(), self.canonical)
        self.assertEqual(api.data, self.data)
        self.assertEqual(plugin, self.plugin)

    def test_existing_host_link_is_preserved(self):
        self.alias.symlink_to(self.canonical)
        inode = self.alias.lstat().st_ino
        self.initialize()
        self.assertEqual(self.alias.lstat().st_ino, inode)

    def test_container_platform_directory_is_preserved(self):
        self.write_data(self.alias)
        inode = self.alias.stat().st_ino
        api, plugin = self.initialize()
        self.assertFalse(self.alias.is_symlink())
        self.assertEqual(self.alias.stat().st_ino, inode)
        self.assertEqual(api.data, self.data)
        self.assertEqual(plugin, self.plugin)

    def test_pddf_initialization_errors_are_not_masked_by_plugin_lookup(self):
        factory = Mock(side_effect=RuntimeError('PDDF initialization failed'))
        with patch.dict(sys.modules, {
            'sonic_platform_pddf_base.pddfapi': SimpleNamespace(PddfApi=factory),
        }):
            context = self.load_context()
        with self.assertRaisesRegex(RuntimeError, 'PDDF initialization failed'):
            context.load_environment()
