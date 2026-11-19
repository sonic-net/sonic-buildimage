import os
import subprocess
from pathlib import Path


ENTRYPOINT = Path(__file__).parents[1] / 'supervisord_entrypoint.sh'


def run_entrypoint(tmp_path, *arguments):
    helper_mode = tmp_path / 'helper-mode'
    helper = tmp_path / 'frr_startup_init.sh'
    helper.write_text(
        'initialize_frr_startup_environment() {\n'
        '    printf "%s" "$1" > "$HELPER_MODE"\n'
        '    export BGPD_GRACEFUL_RESTART_ARG="from-helper"\n'
        '}\n'
    )

    supervisor = tmp_path / 'supervisord'
    supervisor.write_text(
        '#!/usr/bin/env bash\n'
        'printf "%s\\n" "$BGPD_GRACEFUL_RESTART_ARG"\n'
        'for argument in "$@"; do\n'
        '    printf "%s\\n" "$argument"\n'
        'done\n'
    )
    supervisor.chmod(0o755)

    entrypoint = tmp_path / 'supervisord_entrypoint.sh'
    entrypoint.write_text(
        ENTRYPOINT.read_text()
        .replace('/usr/bin/frr_startup_init.sh', str(helper))
        .replace('/usr/local/bin/supervisord', str(supervisor))
    )
    entrypoint.chmod(0o755)

    env = os.environ.copy()
    env.pop('BGPD_GRACEFUL_RESTART_ARG', None)
    env['HELPER_MODE'] = str(helper_mode)
    result = subprocess.run(
        [str(entrypoint), *arguments],
        env=env,
        check=True,
        capture_output=True,
        text=True,
    )
    return helper_mode.read_text(), result.stdout


def test_initializes_shared_frr_environment_for_normal_start(tmp_path):
    mode, output = run_entrypoint(tmp_path)

    assert mode == '--normal-start'
    assert output == 'from-helper\n'


def test_forwards_arguments_to_supervisord(tmp_path):
    _, output = run_entrypoint(
        tmp_path,
        '-c',
        '/tmp/supervisor config.conf',
        '',
    )

    assert output == 'from-helper\n-c\n/tmp/supervisor config.conf\n\n'
