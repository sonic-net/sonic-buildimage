import os
from pathlib import Path
import subprocess

import pytest


DOCKER_INIT = Path(__file__).resolve().parents[1] / 'docker_init.sh'


@pytest.mark.parametrize(
    'bgp_warm,system_warm,delete_rc',
    [
        ('true', 'false', 0),
        ('false', 'true', 0),
        ('false', 'false', 0),
        ('true', 'false', 1),
    ],
)
def test_eoiu_cleanup_before_supervisord(
    tmp_path, bgp_warm, system_warm, delete_rc,
):
    script_path = tmp_path / 'docker_init.sh'
    script = DOCKER_INIT.read_text()
    for path in ('/etc', '/var/sonic', '/usr/sbin'):
        script = script.replace(path, str(tmp_path))
    script_path.write_text(script)

    # Stub container-only operations and skip unrelated FRR config rendering.
    stubs = r'''
mkdir() { :; }
chown() { :; }
chmod() { :; }
rm() { :; }
ip() { :; }
grep() { return 1; }
sonic-cfggen() { :; }
jq() { echo test; }
logger() { echo TRACE:error; }
exec() { echo TRACE:start; }
sonic-db-cli() {
    if [[ "$2" == hget ]]; then
        case "$3" in
            'WARM_RESTART_ENABLE_TABLE|bgp') echo "$TEST_BGP_WARM";;
            'WARM_RESTART_ENABLE_TABLE|system') echo "$TEST_SYSTEM_WARM";;
            *) echo false;;
        esac
    elif [[ "$2" == DEL ]]; then
        printf 'TRACE:%s\n' "$*" >&2
        return "$TEST_DELETE_RC"
    else
        return 1
    fi
}
source "$1"
'''
    result = subprocess.run(
        ['bash', '-c', stubs, 'test', str(script_path)],
        env=dict(
            os.environ, NAMESPACE_ID='', TEST_BGP_WARM=bgp_warm,
            TEST_SYSTEM_WARM=system_warm, TEST_DELETE_RC=str(delete_rc),
        ),
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, timeout=10,
    )
    events = [
        line for line in result.stdout.splitlines()
        if line.startswith('TRACE:')
    ]
    expected = []
    if 'true' in (bgp_warm, system_warm):
        expected.append(
            'TRACE:STATE_DB DEL '
            'BGP_STATE_TABLE|IPv4|eoiu BGP_STATE_TABLE|IPv6|eoiu'
        )
    expected.append('TRACE:error' if delete_rc else 'TRACE:start')
    assert result.returncode == delete_rc, result.stdout
    assert events == expected
