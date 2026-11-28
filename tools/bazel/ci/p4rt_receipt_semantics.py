"""Bind owner results, static image checks and same-run installed probes."""
import re

IMAGES = ('docker-sonic-p4rt', 'docker-sonic-p4rt-dbg')
PROGRAMS = ('p4rt', 'p4rt_program_table', 'p4rt_read',
            'p4rt_set_forwarding_pipeline', 'p4rt_write')


def digest(value):
    assert isinstance(value, str) and re.fullmatch(r'[0-9a-f]{64}', value)
    return value


def verify(read, *, source, owner, outputs, test_targets):
    """Return checked observations; failures raise before an acceptance is written."""
    public, validation = read('public-receipt.json'), read('validation.json')
    assert public['status'] == 'built' and validation['status'] == 'passed'
    for record in (public, validation):
        assert record['revision'] == source and record['owner_revision'] == owner
        assert record['architecture'] == 'amd64'
    artifacts = public['artifacts']
    assert artifacts == validation['artifacts'] and len(artifacts) == len(outputs) == 23
    assert {n: r['target'] for n, r in artifacts.items()} == outputs
    assert not any(n.endswith('.deb') for n in artifacts)
    for record in artifacts.values():
        digest(record['sha256'])
        assert record['bytes'] > 0

    runtime_meta = read('p4rt-runtime.packages.json')
    assert runtime_meta['source_modules']['sonic-pins']['version'] == '0.0.1-' + owner
    digest(runtime_meta['source_contract_sha256'])
    p4rt_pkg = next(pkg for pkg in runtime_meta['source_packages'] if pkg['package'] == 'sonic-p4rt')
    assert p4rt_pkg['target'] == '@@sonic-pins+//p4rt_app:p4rt_runtime'
    assert {entry['path'] for entry in p4rt_pkg['native_files']} == {'usr/local/bin/' + n for n in PROGRAMS}

    owner_tests, summary = read('owner-tests.json'), read('owner-test-summary.json')
    assert owner_tests['status'] == 'passed' and owner_tests['owner_revision'] == owner
    assert owner_tests['targets'] == test_targets and len(set(test_targets)) == len(test_targets) == 1
    assert owner_tests['results'] == {target: 'passed' for target in test_targets}
    assert summary['status'] == 'passed' and summary['owner_revision'] == owner
    assert summary['target_count'] == len(summary['tests']) == len(test_targets)
    assert set(summary['tests']) == set(test_targets)
    for row in summary['tests'].values():
        assert row['status'] == 'PASSED' and row['tests'] > 0
        assert row['failures'] == row['errors'] == row['skipped'] == 0
        digest(row['xml_sha256'])
    assert summary['testcase_count'] == sum(r['tests'] for r in summary['tests'].values()) > 0

    native, runtime = validation['native'], validation['runtime_native']
    owner_paths = {'usr/local/bin/' + n for n in PROGRAMS}
    assert owner_paths <= set(runtime) <= set(native)
    assert all(v['elf_machine'] == 62 for v in native.values())
    assert {v['path'] for v in validation['elfs']} == set(native)
    assert len(validation['elfs']) == len(native)
    for entry in validation['elfs']:
        assert entry['sha256'] == native[entry['path']]['sha256']
    pairs = validation['source_debug_pairs']
    assert len(pairs) == len(native) and {p['path'] for p in pairs} == set(native)
    for p in pairs:
        assert p['build_id'] and p['debug_path'] == 'usr/lib/debug/.build-id/' + p['build_id'][:2] + '/' + p['build_id'][2:] + '.debug'

    stages = validation['stages']
    assert set(stages) == {'runtime', 'debug'}
    assert all(s['status'] == 'passed' for s in stages.values())
    assert stages['debug']['base_manifest_digest'] == stages['runtime']['manifest_digest']
    a, b = stages['debug']['rootfs']['diff_ids'], stages['runtime']['rootfs']['diff_ids']
    assert a[:len(b)] == b and len(a) > len(b)
    for image, stage in zip(IMAGES, ('runtime', 'debug')):
        assert read(image + '.validation.json') == stages[stage]
        r = read(image + '.runtime.json')
        assert r['status'] == 'passed' and r['architecture'] == 'amd64'
        assert r['revision'] == source and r['owner_revision'] == owner
        assert r['archive_sha256'] == artifacts[image + '.gz']['sha256']
        assert r['config_and_layers_verified'] is True and r['configdb_rendered_startup'] is True
        assert r['p4runtime_verify'] == 'passed' and r['p4runtime_unsupported_action'] == 'rejected'
        assert r['loader_checks'] == r['source_elf_count'] == len(runtime if stage == 'runtime' else native)
        assert r['supervisor_states'] == {
            'dependent-startup': 'EXITED',
            'p4rt': 'RUNNING',
            'rsyslogd': 'RUNNING',
            'start': 'EXITED',
            'supervisor-proc-exit-listener': 'RUNNING',
        }
        if stage == 'runtime':
            assert r['debug_checks'] == r['debug_tool_checks'] == r['build_id_debug_checks'] == 0
        else:
            assert r['debug_checks'] == 9 and r['debug_tool_checks'] == 1
            assert r['build_id_debug_checks'] == len(PROGRAMS)

    inc = read('incremental.json')
    assert inc['status'] == 'passed' and inc['revision'] == source
    assert inc['input'] == 'dockers/docker-sonic-p4rt/p4rt_vars.j2'
    keys = ('images', 'outputs', 'base')
    project = lambda s: {k: inc[s][k] for k in keys}
    assert project('baseline') == project('no_op') == project('restored')
    for state in ('baseline', 'no_op', 'changed', 'restored'):
        assert inc[state]['seconds'] >= 0 and set(inc[state]['images']) == set(IMAGES)
        assert set(inc[state]['outputs']) == {n + '.gz' for n in IMAGES} | {'p4rt.source.tar', 'p4rt.debug.tar'}
    assert inc['changed']['base'] == inc['baseline']['base']
    for n in IMAGES:
        assert inc['changed']['images'][n] != inc['baseline']['images'][n]
        assert inc['changed']['outputs'][n + '.gz'] != inc['baseline']['outputs'][n + '.gz']
        assert inc['baseline']['outputs'][n + '.gz'] == artifacts[n + '.gz']['sha256']
    for n in ('p4rt.source.tar', 'p4rt.debug.tar'):
        assert inc['changed']['outputs'][n] == inc['baseline']['outputs'][n] == artifacts[n]['sha256']
