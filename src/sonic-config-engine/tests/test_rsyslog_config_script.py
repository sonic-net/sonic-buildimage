import os
import shutil
import subprocess
import tempfile
import textwrap

from unittest import TestCase

import tests.common_utils as utils

LOOPBACK_IP = '127.0.0.1'
DOCKER0_IP = '240.127.1.1'
INCLUDE_LINE = 'include(file="/run/sonic-rsyslog/docker0-inputs.conf" mode="optional")'

STUBS = {
    'sonic-db-cli': '''\
        #!/bin/bash
        case "$2 $3 $4" in
            "HGET DEVICE_METADATA|localhost platform") echo test-platform ;;
            "keys FEATURE|"*)
                for feature in $RSYSLOG_TEST_FEATURES; do
                    [ "FEATURE|$feature" = "$3" ] && echo "$3"
                done ;;
        esac
        exit 0
        ''',
    'ip': '''\
        #!/bin/bash
        case "$*" in
            "-j -4 addr list lo scope host") echo '[{"addr_info":[{"local":"%s"}]}]' ;;
            "-o -4 addr list docker0")
                [ -n "$RSYSLOG_TEST_DOCKER0_IP" ] && \\
                    echo "3: docker0    inet $RSYSLOG_TEST_DOCKER0_IP/24 brd 240.127.1.255 scope global docker0" ;;
        esac
        exit 0
        ''' % LOOPBACK_IP,
    # Records each call and the docker0 inputs file as rsyslog would load it.
    'systemctl': '''\
        #!/bin/bash
        echo "$*" >> "$RSYSLOG_TEST_SYSTEMCTL_LOG"
        rm -f "$RSYSLOG_TEST_SNAPSHOT"
        [ -e "$RSYSLOG_TEST_FRAGMENT" ] && cp "$RSYSLOG_TEST_FRAGMENT" "$RSYSLOG_TEST_SNAPSHOT"
        exit "$RSYSLOG_TEST_SYSTEMCTL_STATUS"
        ''',
    'sonic-cfggen': '''\
        #!/bin/bash
        args=()
        for arg in "$@"; do
            if [ "$arg" = "-d" ]; then args+=(-j "$RSYSLOG_TEST_CONFIG_DB"); else args+=("$arg"); fi
        done
        exec %s "$RSYSLOG_TEST_CFGGEN" "${args[@]}"
        ''' % utils.PYTHON_INTERPRETTER,
}


def fragment_for(ip):
    return ('input(type="imudp" address="%s" port="514")\n'
            'input(type="imrelp" address="%s" port="2514")\n') % (ip, ip)


class TestRsyslogConfigScript(TestCase):
    """Runs files/image_config/rsyslog/rsyslog-config.sh, which renders
    rsyslog.conf.j2 through sonic-cfggen, in the suite that already tests that
    template and runs in CI. The script's absolute paths are pointed into a
    temporary directory (a path added to the script must be added to
    redirected_script), sonic-db-cli, ip and systemctl are stubs, and
    sonic-cfggen is the one from this tree, reading
    tests/data/rsyslog/config_db.json instead of CONFIG_DB."""

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        rsyslog_dir = os.path.join(self.test_dir, '..', '..', '..', 'files', 'image_config', 'rsyslog')
        self.work = tempfile.mkdtemp(prefix='rsyslog-config-')
        self.addCleanup(shutil.rmtree, self.work)
        self.bin_dir = os.path.join(self.work, 'bin')
        self.run_dir = os.path.join(self.work, 'run')
        self.conf = os.path.join(self.work, 'etc', 'rsyslog.conf')
        self.fragment = os.path.join(self.run_dir, 'sonic-rsyslog', 'docker0-inputs.conf')
        self.snapshot = os.path.join(self.work, 'fragment-at-systemctl.conf')
        self.asic_conf = os.path.join(self.work, 'device', 'asic.conf')
        self.systemctl_log = os.path.join(self.work, 'systemctl.log')
        for path in [self.bin_dir, self.run_dir, os.path.dirname(self.conf), os.path.dirname(self.asic_conf)]:
            os.makedirs(path)
        for name, body in STUBS.items():
            stub = os.path.join(self.bin_dir, name)
            with open(stub, 'w') as f:
                f.write(textwrap.dedent(body))
            os.chmod(stub, 0o755)
        with open(os.path.join(self.work, 'sonic_version.yml'), 'w') as f:
            f.write("build_version: 'test-version'\n")
        # sonic-cfggen looks a template up by file name in the system template
        # directories first, so the copy under test gets a name of its own.
        self.template = os.path.join(self.work, 'rsyslog-config-test.conf.j2')
        shutil.copyfile(os.path.join(rsyslog_dir, 'rsyslog.conf.j2'), self.template)
        self.script = self.redirected_script(os.path.join(rsyslog_dir, 'rsyslog-config.sh'))

    def redirected_script(self, script):
        paths = {
            '/usr/share/sonic/device/$PLATFORM/asic.conf': self.asic_conf,
            '/usr/share/sonic/templates/rsyslog.conf.j2': self.template,
            '/etc/sonic/sonic_version.yml': os.path.join(self.work, 'sonic_version.yml'),
            '/etc/rsyslog.conf': self.conf,
            '/run/sonic-rsyslog': os.path.dirname(self.fragment),
            '/tmp/rsyslog.conf.XXXXXX': os.path.join(self.work, 'rsyslog.conf.XXXXXX'),
        }
        with open(script) as f:
            source = f.read()
        for path, redirected in paths.items():
            self.assertIn(path, source, "rsyslog-config.sh no longer uses %s; update the paths in this test" % path)
            source = source.replace(path, redirected)
        redirected = os.path.join(self.work, 'rsyslog-config.sh')
        with open(redirected, 'w') as f:
            f.write(source)
        return redirected

    def run_script(self, features='dhcp_server', docker0_ip=DOCKER0_IP, num_asic=1, systemctl_status=0,
                   expect_status=0):
        if num_asic > 1:
            with open(self.asic_conf, 'w') as f:
                f.write('NUM_ASIC=%d\n' % num_asic)
        elif os.path.exists(self.asic_conf):
            os.remove(self.asic_conf)
        open(self.systemctl_log, 'w').close()
        if os.path.exists(self.snapshot):
            os.remove(self.snapshot)
        env = dict(os.environ,
                   PATH=self.bin_dir + os.pathsep + os.environ.get('PATH', os.defpath),
                   CFGGEN_UNIT_TESTING='2',
                   RSYSLOG_TEST_FEATURES=features,
                   RSYSLOG_TEST_DOCKER0_IP=docker0_ip,
                   RSYSLOG_TEST_FRAGMENT=self.fragment,
                   RSYSLOG_TEST_SNAPSHOT=self.snapshot,
                   RSYSLOG_TEST_SYSTEMCTL_LOG=self.systemctl_log,
                   RSYSLOG_TEST_SYSTEMCTL_STATUS=str(systemctl_status),
                   RSYSLOG_TEST_CONFIG_DB=os.path.join(self.test_dir, 'data', 'rsyslog', 'config_db.json'),
                   RSYSLOG_TEST_CFGGEN=os.path.join(self.test_dir, '..', 'sonic-cfggen'))
        # The script would inherit these when asic.conf or the feature key is absent.
        for name in ['NUM_ASIC', 'docker0_ip']:
            env.pop(name, None)
        result = subprocess.run(['bash', self.script], env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, universal_newlines=True)
        self.assertEqual(result.returncode, expect_status,
                         'rsyslog-config.sh exit %d\n%s%s' % (result.returncode, result.stdout, result.stderr))
        return result

    def read(self, path):
        if not os.path.exists(path):
            return None
        with open(path) as f:
            return f.read()

    def systemctl_calls(self):
        return self.read(self.systemctl_log).splitlines()

    def assert_conf(self, listener_ip, include, docker0_ip=DOCKER0_IP):
        conf = self.read(self.conf)
        self.assertIsNotNone(conf, 'rsyslog.conf was not installed')
        for line in ['input(type="imudp" address="%s" port="514")' % listener_ip,
                     'input(type="imrelp" address="%s" port="2514")' % listener_ip]:
            self.assertEqual(conf.count(line), 1, conf)
        if listener_ip != docker0_ip:
            self.assertNotIn(docker0_ip, conf)
        self.assertEqual(conf.count(INCLUDE_LINE), 1 if include else 0, conf)

    def test_first_boot_writes_fragment_then_restarts(self):
        self.run_script()
        self.assert_conf(LOOPBACK_IP, include=True)
        self.assertEqual(self.read(self.fragment), fragment_for(DOCKER0_IP))
        self.assertEqual(self.systemctl_calls(), ['restart rsyslog'])
        self.assertEqual(self.read(self.snapshot), fragment_for(DOCKER0_IP))

    def test_new_docker0_address_rewrites_fragment_then_restarts(self):
        self.run_script()
        self.assertEqual(self.read(self.fragment), fragment_for(DOCKER0_IP))
        self.run_script(docker0_ip='240.127.2.1')
        self.assert_conf(LOOPBACK_IP, include=True, docker0_ip='240.127.2.1')
        self.assertEqual(self.read(self.fragment), fragment_for('240.127.2.1'))
        self.assertEqual(self.systemctl_calls(), ['restart rsyslog'])

    def test_feature_removed_drops_fragment_then_restarts(self):
        self.run_script()
        self.assertEqual(self.read(self.fragment), fragment_for(DOCKER0_IP))
        self.run_script(features='')
        self.assert_conf(LOOPBACK_IP, include=False)
        self.assertIsNone(self.read(self.fragment))
        self.assertEqual(self.systemctl_calls(), ['restart rsyslog'])
        self.assertIsNone(self.read(self.snapshot))

    def test_without_bridged_feature_writes_no_fragment_and_restarts(self):
        self.run_script(features='')
        self.assert_conf(LOOPBACK_IP, include=False)
        self.assertIsNone(self.read(self.fragment))
        self.assertEqual(self.systemctl_calls(), ['restart rsyslog'])

    def test_multi_asic_listens_on_docker0_without_fragment(self):
        self.run_script(num_asic=2)
        self.assert_conf(DOCKER0_IP, include=False)
        self.assertIsNone(self.read(self.fragment))
        self.run_script(num_asic=2)
        self.assertIsNone(self.read(self.fragment))
        self.assertEqual(self.systemctl_calls(), ['restart rsyslog'])

    def test_fragment_write_failure_still_restarts_and_fails(self):
        os.rmdir(self.run_dir)
        open(self.run_dir, 'w').close()
        result = self.run_script(expect_status=1)
        self.assertIn('docker0 inputs not configured', result.stderr)
        self.assert_conf(LOOPBACK_IP, include=True)
        self.assertEqual(self.systemctl_calls(), ['restart rsyslog'])

    def test_failed_restart_fails_the_script(self):
        self.run_script(systemctl_status=7, expect_status=7)
        self.assertEqual(self.systemctl_calls(), ['restart rsyslog'])
