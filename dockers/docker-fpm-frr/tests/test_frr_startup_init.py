import os
import subprocess
from pathlib import Path

import pytest


STARTUP_INIT = Path(__file__).parents[1] / 'frr_startup_init.sh'


@pytest.mark.parametrize(
    'graceful_restart,enabled_key,db_failure,startup_mode,expected',
    [
        (
            True,
            'WARM_RESTART_ENABLE_TABLE|system',
            False,
            '',
            '-K',
        ),
        (
            True,
            'WARM_RESTART_ENABLE_TABLE|bgp',
            False,
            '',
            '-K',
        ),
        (
            True,
            'FAST_RESTART_ENABLE_TABLE|system',
            False,
            '',
            '-K',
        ),
        (True, '', False, '', ''),
        (False, 'WARM_RESTART_ENABLE_TABLE|system', False, '', ''),
        (True, 'WARM_RESTART_ENABLE_TABLE|system', True, '', ''),
        (
            True,
            'WARM_RESTART_ENABLE_TABLE|system',
            False,
            '--normal-start',
            '',
        ),
    ],
)
def test_initializes_bgpd_graceful_restart_argument(
    graceful_restart, enabled_key, db_failure, startup_mode, expected
):
    env = os.environ.copy()
    env['GRACEFUL_RESTART'] = 'true' if graceful_restart else 'false'
    env['ENABLED_KEY'] = enabled_key
    env['DB_FAILURE'] = 'true' if db_failure else 'false'
    script = r'''
sonic-db-cli() {
    if [[ "$DB_FAILURE" == "true" ]]; then
        return 1
    fi
    if [[ "$3" == "$ENABLED_KEY" && -n "$ENABLED_KEY" ]]; then
        printf 'true\n'
    else
        printf 'false\n'
    fi
}
logger() {
    :
}
grep() {
    [[ "$GRACEFUL_RESTART" == "true" ]]
}
source "$STARTUP_INIT"
initialize_frr_startup_environment $STARTUP_MODE
printf '%s' "$BGPD_GRACEFUL_RESTART_ARG"
'''

    result = subprocess.run(
        ['bash', '-c', script],
        env={
            **env,
            'STARTUP_INIT': str(STARTUP_INIT),
            'STARTUP_MODE': startup_mode,
        },
        check=True,
        capture_output=True,
        text=True,
    )

    assert result.stdout == expected
