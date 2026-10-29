from .base_image_script_util import (
    json_lines,
    load_state,
    log_lines,
    run_script,
    update_state,
    write_platform_env,
)


def run_tsa(script_env, *args):
    return run_script(script_env, 'TSA', *args)


def test_requires_root(script_env):
    script_env['env']['TEST_EUID'] = '1000'

    result = run_tsa(script_env)

    assert result.returncode == 1
    assert result.stdout == 'Root priveleges are needed for this operation\n'
    assert log_lines(script_env['ts_path']) == []


def test_supervisor_chassis_tsa_already_in_maintenance(script_env):
    write_platform_env(script_env, 'supervisor=1\n')
    update_state(script_env, 'default', chassis_tsa_supported='true', tsa_enabled='true')

    result = run_tsa(script_env)

    assert result.returncode == 0
    assert 'Chassis is already in Maintenance' in result.stdout
    assert 'sudo config save' in result.stdout
    assert json_lines(script_env['writes_path']) == []
    assert log_lines(script_env['logger_path']) == [
        '-t TSA -p user.info Chassis is already in Maintenance'
    ]
    assert log_lines(script_env['ts_path']) == []


def test_supervisor_chassis_tsa_normal_to_maintenance(script_env):
    write_platform_env(script_env, 'supervisor=1\n')
    update_state(script_env, 'default', chassis_tsa_supported='true', tsa_enabled='false')

    result = run_tsa(script_env)

    assert result.returncode == 0
    assert 'Chassis Mode: Normal -> Maintenance' in result.stdout
    assert json_lines(script_env['writes_path']) == [
        {'ns': None, 'state': {'tsa_enabled': 'true'}}
    ]
    assert any('CHASSIS_APP_DB' in line and 'tsa_enabled' in line
               for line in log_lines(script_env['dbcli_path']))
    assert load_state(script_env)['default']['tsa_enabled'] == 'true'
    assert log_lines(script_env['ts_path']) == []


def test_supervisor_rexec_to_linecards(script_env):
    write_platform_env(script_env, 'supervisor=1\n')
    update_state(script_env, 'default', chassis_tsa_supported='false')

    result = run_tsa(script_env)

    assert result.returncode == 0
    assert 'Chassis Mode: Normal -> Maintenance' in result.stdout
    assert 'rexec all -c' in result.stdout
    assert json_lines(script_env['writes_path']) == [
        {'ns': None, 'state': {'tsa_enabled': 'true'}}
    ]
    assert log_lines(script_env['rexec_path']) == ['all -c sudo TSA chassis']
    assert log_lines(script_env['ts_path']) == []


def test_supervisor_rexec_failure_is_logged(script_env):
    write_platform_env(script_env, 'supervisor=1\n')
    script_env['env']['REXEC_EXIT'] = '1'

    result = run_tsa(script_env)

    assert result.returncode == 0
    assert any('rexec TSA to linecards failed' in line
               for line in log_lines(script_env['logger_path']))


def test_linecard_invokes_ts_and_prints_save_hint(script_env):
    result = run_tsa(script_env)

    assert result.returncode == 0
    assert log_lines(script_env['ts_path']) == ['TSA']
    assert 'Please execute \'sudo config save\' to preserve System mode in Maintenance' in result.stdout


def test_linecard_chassis_argument_skips_save_hint(script_env):
    result = run_tsa(script_env, 'chassis')

    assert result.returncode == 0
    assert log_lines(script_env['ts_path']) == ['TSA']
    assert 'sudo config save' not in result.stdout
    assert 'WARNING' not in result.stdout


def test_linecard_warns_when_subtype_is_upstream_lc(script_env):
    update_state(script_env, 'default', subtype='UpstreamLC')

    result = run_tsa(script_env)

    assert result.returncode == 0
    assert 'WARNING: Please execute \'TSA\' on Supervisor' in result.stdout


def test_dualtor_toggles_mux_to_standby(script_env):
    update_state(script_env, 'default', subtype='DualToR')
    script_env['env']['MUX_STATUS'] = 'Ethernet0 active'

    result = run_tsa(script_env)

    assert result.returncode == 0
    assert log_lines(script_env['show_path']) == ['mux status']
    assert log_lines(script_env['sudo_path']) == ['config mux mode standby all']
    assert any('Toggle all mux mode to standby' in line
               for line in log_lines(script_env['logger_path']))


def test_stops_startup_tsa_tsb_service(script_env):
    script_env['env']['SYSTEMCTL_ACTIVE'] = 'active'
    script_env['env']['SYSTEMCTL_SUB'] = 'running'

    result = run_tsa(script_env)

    assert result.returncode == 0
    assert 'Stopping startup_tsa_tsb.service before configuring TSA' in result.stdout
    assert 'Successfully removed TSA-TSB service flag.' in result.stdout
    assert any(line.startswith('stop startup_tsa_tsb.service')
               for line in log_lines(script_env['systemctl_path']))
    assert any('STATE_DB HDEL' in line for line in log_lines(script_env['dbcli_path']))


def test_startup_service_flag_removal_failure(script_env):
    script_env['env']['SYSTEMCTL_ACTIVE'] = 'active'
    script_env['env']['SYSTEMCTL_SUB'] = 'running'
    script_env['env']['DBCLI_HDEL_EXIT'] = '1'

    result = run_tsa(script_env)

    assert result.returncode == 0
    assert 'Failed to remove TSA-TSB service flag!' in result.stderr


def test_skips_stopping_service_when_started_by_service(script_env):
    script_env['env']['STARTED_BY_TSA_TSB_SERVICE'] = '1'
    script_env['env']['SYSTEMCTL_ACTIVE'] = 'active'
    script_env['env']['SYSTEMCTL_SUB'] = 'running'

    result = run_tsa(script_env)

    assert result.returncode == 0
    assert 'Stopping startup_tsa_tsb.service' not in result.stdout
    assert log_lines(script_env['ts_path']) == ['TSA']
