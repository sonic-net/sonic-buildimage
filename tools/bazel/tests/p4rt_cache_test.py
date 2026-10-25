#!/usr/bin/env python3
"""Check P4RT cache inputs without building packages or running Bazel."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class P4rtCacheTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        for name in ('Makefile.cache', 'rules/functions', 'rules/p4rt.mk', 'rules/p4rt.dep'):
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, target)
        (self.root / '.platform').write_text('fixture\n')
        (self.root / 'Makefile').write_text('# Unrelated root Makefile\n')
        source = self.root / 'src/sonic-p4rt'
        source.mkdir(parents=True)
        (source / 'Makefile').write_text('# P4RT source Makefile\n')
        self.readme = source / 'README.bazel.md'
        self.readme.write_text('P4RT source instructions\n')
        pins = source / 'sonic-pins'
        pins.mkdir()
        (pins / 'runtime.cc').write_text('// P4RT source\n')
        for directory, files in ((self.root, ['src/sonic-p4rt/Makefile',
                                            'src/sonic-p4rt/README.bazel.md']),
                                 (pins, ['runtime.cc'])):
            subprocess.run(['git', 'init', '--quiet'], cwd=directory, check=True)
            subprocess.run(['git', 'add', '--', *files], cwd=directory, check=True)

    def cache_inputs(self, extension, category):
        name = 'sonic-p4rt_0.0.1_amd64.' + extension
        directory = 'target/' + category + '/trixie'
        (self.root / directory).mkdir(parents=True, exist_ok=True)
        makefile = f'''.SECONDEXPANSION:
SRC_PATH = src
RULES_PATH = rules
TARGET_PATH = target
DEBS_PATH = {directory}
FILES_PATH = {directory}
CONFIGURED_PLATFORM = undefined
SONIC_DPKG_CACHE_METHOD = none
SONIC_BUILD_QUIETER = 1
SONIC_COMMON_BASE_FILES_LIST =
SONIC_P4RT = {name}
$(SONIC_P4RT)_SRC_PATH = src/sonic-p4rt
SONIC_MAKE_{'DEBS' if category == 'debs' else 'FILES'} = $(SONIC_P4RT)
include Makefile.cache
'''
        command = ['make', '--no-print-directory', '-s', '-f', '-',
                   'SONIC_COMMON_BASE_FILES_LIST=',
                   'SONIC_DPKG_CACHE_DIR=' + str(self.root),
                   directory + '/' + name + '.dep']
        result = subprocess.run(command, input=makefile, text=True, cwd=self.root,
                                env=dict(os.environ, PWD=str(self.root)),
                                capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        sidecar = self.root / directory / (name + '.dep')
        files = sidecar.read_text().split()
        self.assertIn('src/sonic-p4rt/README.bazel.md', files)
        self.assertIn('src/sonic-p4rt/Makefile', files)
        self.assertNotIn('README.bazel.md', files)
        self.assertNotIn('Makefile', files)
        self.assertNotIn('src/sonic-p4rt/sonic-pins', files)
        self.assertEqual(sidecar.with_suffix('.smdep').read_text().split(),
                         ['sonic-pins/runtime.cc'])
        return sidecar.with_suffix('.dep.sha').read_text()

    def test_deb_cache_tracks_the_source_directory(self):
        before = self.cache_inputs('deb', 'debs')
        self.readme.write_text('Updated P4RT source instructions\n')
        self.assertNotEqual(before, self.cache_inputs('deb', 'debs'))

    def test_tar_cache_tracks_the_source_directory(self):
        self.cache_inputs('tar.bz2', 'files')


if __name__ == '__main__':
    unittest.main()
