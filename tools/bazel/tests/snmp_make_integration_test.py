"""Check the SNMP image selector without executing a package or image producer."""
import os
from pathlib import Path
import subprocess
import unittest

ROOT=Path(__file__).resolve().parents[3]
MAKE='''
TARGET_PATH=target
DOCKERS_PATH=dockers
DBG_IMAGE_MARK=dbg
DOCKER_CONFIG_ENGINE_TRIXIE=docker-config-engine-trixie.gz
SONIC_PY_COMMON_PY3=py-common.whl
SONIC_PLATFORM_COMMON_PY3=platform-common.whl
ASYNCSNMP_PY3=asyncsnmp.whl
SNMP=legacy-snmp.deb
SNMPD=legacy-snmpd.deb
SONIC_IMAGE_VERSION=snmp-test-version
include rules/functions rules/docker-snmp.mk rules/docker-snmp.dep tools/bazel/docker.mk
.PHONY: probe
probe:
\t@echo runtime=$(SONIC_BAZEL_DOCKER_IMAGES)
\t@echo debug=$(SONIC_BAZEL_DBG_DOCKER_IMAGES)
\t@echo runtime_label=$(docker-snmp.gz_BAZEL_TARGET)
\t@echo debug_label=$(docker-snmp-dbg.gz_BAZEL_TARGET)
\t@echo deps=$(docker-snmp.gz_BAZEL_DEPENDS)
\t@echo debug_deps=$(docker-snmp-dbg.gz_BAZEL_DEPENDS)
\t@echo args=$(docker-snmp.gz_BAZEL_ARGS)
\t@echo debug_args=$(docker-snmp-dbg.gz_BAZEL_ARGS)
\t@echo legacy_debs=$(docker-snmp.gz_DEPENDS)
\t@echo legacy_wheels=$(docker-snmp.gz_PYTHON_WHEELS)
\t@echo legacy_base=$(docker-snmp.gz_LOAD_DOCKERS)
\t@echo source_path=$(docker-snmp.gz_PATH)
\t@echo debug_source_path=$(docker-snmp-dbg.gz_PATH)
'''

class SnmpMakeTests(unittest.TestCase):
    def run_make(self, **overrides):
        settings=dict(BUILD_WITH_BAZEL_WHEN_AVAILABLE='y',BLDENV='trixie',CONFIGURED_PLATFORM='vs',
                      CONFIGURED_ARCH='amd64',INCLUDE_FIPS='y',ENABLE_ASAN='n')
        settings.update(overrides)
        result=subprocess.run(['make','-s','--no-print-directory','-f','-','probe',*[key+'='+value for key,value in settings.items()]],
            input=MAKE,cwd=ROOT,text=True,capture_output=True,check=True,env={k:v for k,v in os.environ.items() if k!='BASH_ENV'})
        return dict(line.split('=',1) for line in result.stdout.splitlines())

    def test_native_profile_uses_only_source_image_and_manifest_inputs(self):
        row=self.run_make()
        self.assertEqual(row['runtime'],'docker-snmp.gz')
        self.assertEqual(row['debug'],'docker-snmp-dbg.gz')
        self.assertEqual(row['runtime_label'],'//dockers/docker-snmp:docker-snmp.gz')
        self.assertEqual(row['debug_label'],'//dockers/docker-snmp:docker-snmp-dbg.gz')
        self.assertEqual(set(row['deps'].split()),{'target/bazel-manifests/'+name+'/manifest.json' for name in
                         ('docker-base-trixie','docker-config-engine-trixie','docker-snmp')})
        self.assertEqual(set(row['debug_deps'].split()),set(row['deps'].split())|{'target/bazel-manifests/docker-snmp-dbg/manifest.json'})
        self.assertEqual(row['args'],'--//dockers/docker-snmp:image_version=snmp-test-version')
        self.assertEqual(row['debug_args'],row['args'])
        self.assertEqual(row['source_path'], 'dockers/docker-snmp')
        self.assertEqual(row['debug_source_path'], row['source_path'])

    def test_legacy_contract_and_unsupported_profiles_are_preserved(self):
        baseline=self.run_make()
        for settings in [{'BUILD_WITH_BAZEL_WHEN_AVAILABLE':'n'},{'BLDENV':'bookworm'},
                         {'CONFIGURED_ARCH':'arm64'},{'CONFIGURED_PLATFORM':'broadcom'},
                         {'INCLUDE_FIPS':'n'},{'ENABLE_ASAN':'y'},{'CROSS_BUILD_ENVIRON':'y'},
                         {'MULTIARCH_QEMU_ENVIRON':'y'},{'DBG_IMAGE_MARK':'custom'}]:
            with self.subTest(settings=settings):
                row=self.run_make(**settings)
                self.assertEqual(row['runtime'],'')
                self.assertEqual(row['debug'],'')
                for key in ['legacy_debs','legacy_wheels','legacy_base']:
                    self.assertEqual(row[key],baseline[key])
        self.assertEqual(baseline['legacy_debs'],'legacy-snmp.deb legacy-snmpd.deb')
        self.assertEqual(baseline['legacy_wheels'],'py-common.whl platform-common.whl asyncsnmp.whl')
        self.assertEqual(baseline['legacy_base'],'docker-config-engine-trixie.gz')

if __name__=='__main__':unittest.main()
