"""Bind owner results, static image checks and same-run installed probes."""
import hashlib
import re

IMAGES = ('docker-sonic-p4rt', 'docker-sonic-p4rt-dbg')
PROGRAMS = ('p4rt', 'p4rt_program_table', 'p4rt_read',
            'p4rt_set_forwarding_pipeline', 'p4rt_write')


def digest(value):
    assert isinstance(value, str) and re.fullmatch(r'[0-9a-f]{64}', value)
    return value


def verify(read, raw, *, source, owner, outputs, test_targets, owner_test_helper_sha256):
    """Return checked observations; failures raise before an acceptance is written."""
    public, validation = read('public-receipt.json'), read('validation.json')
    assert public['status'] == 'built' and validation['status'] == 'passed'
    for record in (public, validation):
        assert record['revision'] == source and record['owner_revision'] == owner
        assert record['architecture'] == 'amd64'
    artifacts = public['artifacts']
    assert artifacts == validation['artifacts'] and len(artifacts) == len(outputs) == 27
    assert {n: r['target'] for n, r in artifacts.items()} == outputs
    assert not any(n.endswith('.deb') for n in artifacts)
    for record in artifacts.values():
        digest(record['sha256'])
        assert record['bytes'] > 0

    owner_source = read('owner-source.json')
    assert owner_source['commit'] == owner and owner_source['bazel'] == '6.4.0'
    assert owner_source['architecture'] == 'amd64'
    assert owner_source['test_targets'] == test_targets and len(set(test_targets)) == len(test_targets) > 0
    handoff = public['owner_handoff']
    assert handoff == validation['owner_handoff']
    assert handoff['source_revision'] == owner and handoff['kind'] == 'sonic-p4rt-owner-payload'
    assert handoff['producer_bazel_version'] == '6.4.0'
    assert handoff['sdk_producer_bazel_version'] == '8.5.1'
    assert handoff['sdk_inventory'] == owner_source['sdk_inventory']
    common = handoff['parent_common']
    assert common['path'] == 'usr/lib/x86_64-linux-gnu/libswsscommon.so.0.0.0'
    assert digest(common['sha256']) == handoff['sdk_inventory']['lib/libswsscommon.so.0']['sha256']
    assert common['parent_source_tar_sha256'] == artifacts['config-engine.source.tar']['sha256']
    assert artifacts['p4rt.source.tar']['sha256'] == owner_source['tar_sha256']['p4rt_runtime.tar']
    assert set(handoff['files']) == {'p4rt_runtime.tar', 'p4rt_runtime_debug.tar',
                                  'p4rt_runtime_contract.json', 'p4rt_runtime_debug_contract.json'}
    for n, h in owner_source['tar_sha256'].items():
        assert digest(h) == handoff['files'][n]['sha256'] and handoff['files'][n]['bytes'] > 0
    for suffix, package in (('', 'sonic-p4rt'), ('_debug', 'sonic-p4rt-dbgsym')):
        name = 'p4rt_runtime' + suffix + '_contract.json'
        payload = 'p4rt_runtime' + suffix + '.tar'
        content = raw(name)
        assert hashlib.sha256(content).hexdigest() == owner_source['contract_sha256'][name] == handoff['files'][name]['sha256']
        assert len(content) == handoff['files'][name]['bytes']
        contract = read(name)
        assert contract['owner'] == 'sonic-pins' and contract['schema'] == 1
        assert contract['source'] == {'kind': 'workspace', 'name': 'com_github_sonic_net_sonic_pins', 'revision': owner}
        assert set(contract['packages']) == {package}
        p = contract['packages'][package]
        assert p['source_revision'] == owner and p['target'] == '//p4rt_app:p4rt_runtime' + suffix
        assert p['payload_sha256'] == owner_source['tar_sha256'][payload]
        assert p['control_fields']['Architecture'] == 'amd64' and p['control_fields']['Package'] == package
        assert set(p['required_paths']) == {'usr/local/bin/' + n + ('.dwp' if suffix else '') for n in PROGRAMS}
    for n in ('owner-action-audit.json', 'owner-test-action-audit.json'):
        a = read(n)
        assert a['deb_outputs'] == a['common_compilation_actions'] == 0
        assert a['sdk_header_count'] > 0 and a['sdk_libraries'] == ['external/sonic_swss_common/lib/libswsscommon.so.0']
        assert a['action_counts']['CppCompile'] > 0 and a['action_counts']['PackageTar'] == 2
    assert read('owner-test-action-audit.json')['action_counts']['TestRunner'] == len(test_targets)
    assert read('owner-tests.json') == {'status': 'passed', 'owner_revision': owner}
    results, summary = read('owner-test-results.json'), read('owner-test-summary.json')
    assert set(results) == set(test_targets) and set(results.values()) == {'PASSED'}
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
    assert native[common['path']]['sha256'] == common['sha256']
    for entry in validation['elfs']:
        assert entry['sha256'] == native[entry['path']]['sha256']
    pairs = validation['source_debug_pairs']
    assert len(pairs) == len(native) - len(PROGRAMS)
    assert {p['path'] for p in pairs} == set(native) - owner_paths
    for p in pairs:
        assert p['build_id'] and p['debug_path'] == 'usr/lib/debug/.build-id/' + p['build_id'][:2] + '/' + p['build_id'][2:] + '.debug'
    assert validation['p4rt_owner_test_sha256'] == owner_test_helper_sha256
    assert set(validation['p4rt_debug_pairs']) == set(PROGRAMS)
    for n, p in validation['p4rt_debug_pairs'].items():
        assert p['runtime_path'] == 'usr/local/bin/' + n and p['dwp_path'] == p['runtime_path'] + '.dwp'
        assert p['runtime_sha256'] == native[p['runtime_path']]['sha256']
        assert p['with_dwp']['found'] is True and p['without_dwp']['found'] is False
        assert p['with_dwp']['file'] == p['file'] and p['line'] > 0
        assert p['file'] == ('p4rt_app/p4rt.cc' if n == 'p4rt' else 'p4rt_app/scripts/' + n + '.cc')
        digest(p['dwp_sha256'])
    stages = validation['stages']
    assert set(stages) == {'runtime', 'symbols', 'debug'}
    assert all(s['status'] == 'passed' for s in stages.values())
    assert stages['symbols']['base_manifest_digest'] == stages['runtime']['manifest_digest']
    assert stages['debug']['base_manifest_digest'] == stages['symbols']['manifest_digest']
    for child, parent in (('symbols', 'runtime'), ('debug', 'symbols')):
        a, b = stages[child]['rootfs']['diff_ids'], stages[parent]['rootfs']['diff_ids']
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
        assert r['supervisor_states'] == {'dependent-startup': 'EXITED', 'p4rt': 'RUNNING', 'rsyslogd': 'RUNNING', 'start': 'EXITED', 'supervisor-proc-exit-listener': 'RUNNING'}
        if stage == 'runtime':
            assert r['debug_checks'] == r['debug_tool_checks'] == 0 and r['split_debug_checks'] == {}
        else:
            assert r['debug_checks'] == 4 and r['debug_tool_checks'] == 1
            assert set(r['split_debug_checks']) == set(PROGRAMS)
            for n, row in r['split_debug_checks'].items():
                assert row['with_dwp'] == validation['p4rt_debug_pairs'][n]['with_dwp']
                assert row['without_dwp']['found'] is False
    inc = read('incremental.json')
    assert inc['status'] == 'passed' and inc['revision'] == source
    assert inc['input'] == 'dockers/docker-sonic-p4rt/p4rt_vars.j2'
    keys = ('images', 'outputs', 'base', 'owner_handoff')
    project = lambda s: {k: inc[s][k] for k in keys}
    assert project('baseline') == project('no_op') == project('restored')
    for state in ('baseline', 'no_op', 'changed', 'restored'):
        assert inc[state]['seconds'] >= 0 and set(inc[state]['images']) == set(IMAGES) | {'p4rt_with_symbols'}
        assert set(inc[state]['outputs']) == {n + '.gz' for n in IMAGES} | {'p4rt.source.tar', 'p4rt.debug.tar'}
    assert inc['changed']['base'] == inc['baseline']['base']
    assert inc['changed']['owner_handoff'] == inc['baseline']['owner_handoff']
    for n in IMAGES:
        assert inc['changed']['images'][n] != inc['baseline']['images'][n]
        assert inc['changed']['outputs'][n + '.gz'] != inc['baseline']['outputs'][n + '.gz']
        assert inc['baseline']['outputs'][n + '.gz'] == artifacts[n + '.gz']['sha256']
    for n in ('p4rt.source.tar', 'p4rt.debug.tar'):
        assert inc['changed']['outputs'][n] == inc['baseline']['outputs'][n] == artifacts[n]['sha256']
