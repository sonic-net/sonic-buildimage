"""Source rendering after the real managed-snapshot validation gate."""
import copy
from pathlib import Path
import runpy
from unittest.mock import patch

from jinja2 import Environment, FileSystemLoader, StrictUndefined, nodes
import pytest

from frrcfgd.isis_config import validate_srv6_config


FRAMEWORK = Path(__file__).resolve().parents[1]
TEMPLATE = FRAMEWORK / 'templates' / 'srv6'


def profile(behavior=None, sid_format=None):
    locator = {'prefix': '2001:db8:100::'}
    if behavior:
        locator['behavior'] = behavior
    if sid_format:
        locator['format'] = sid_format
    return {
        'PORT': {'Ethernet0': {}},
        'LOOPBACK_INTERFACE': {'Loopback0': {}},
        'ISIS_GLOBALS': {'default': {'enabled': 'true',
                                     'srv6_locator': 'LOC1',
                                     'srv6_interface': 'Loopback0'}},
        'SRV6_MY_LOCATORS': {'LOC1': locator},
        'SRV6_MY_SOURCE': {'tunnel-source': {'source-address': '2001:db8::1'}},
        'SRV6_MY_SIDS': {
            ('LOC1', '2001:db8:100::/48'): {'action': 'uN'},
            ('LOC1', '2001:db8:100:1::/64'): {'action': 'uDT46'},
            ('LOC1', '2001:db8:100:a::/128'): {
                'action': 'uA', 'interface': 'Ethernet0', 'adj': 'fe80::2'},
            ('LOC1', '2001:db8:100:b::/128'): {'decap_dscp_mode': 'pipe'},
        },
        'BGP_GLOBALS': {'default': {'local_asn': '65000'}},
    }


def render(config, frr_version=None):
    """Exercise the startup adapter contract; no FRR process is invoked."""
    validate_srv6_config(config, frr_version=frr_version)
    environment = Environment(loader=FileSystemLoader(str(TEMPLATE)),
                              trim_blocks=True, undefined=StrictUndefined)
    return environment.get_template('srv6.conf.j2').render(**config)


@pytest.mark.parametrize('behavior,sid_format', [
    (None, None), ('usid', None), ('usid', 'usid-f3216'),
])
def test_srv6_template_preserves_used_profiles(behavior, sid_format):
    config = profile(behavior, sid_format)
    before = copy.deepcopy(config)
    output = render(config)
    assert 'prefix 2001:db8:100::/48 block-len 32 node-len 16 func-bits 16' in output
    assert ('behavior usid' in output) == (behavior == 'usid')
    assert ('format usid-f3216' in output) == bool(sid_format)
    assert 'source-address 2001:db8::1' in output
    assert 'sid 2001:db8:100::/48 locator LOC1 behavior uN' in output
    assert 'sid 2001:db8:100:1::/64 locator LOC1 behavior uDT46 vrf default' in output
    assert 'sid 2001:db8:100:a::/128 locator LOC1 behavior uA interface Ethernet0 nexthop fe80::2' in output
    assert 'sid 2001:db8:100:b::/128' not in output
    assert output.index('prefix 2001:db8:100::/48') < output.index('static-sids')
    assert output.index('static-sids') < output.index('router isis default')
    assert '  locator LOC1\n  interface Loopback0\n' in output
    assert 'uncompressed-f4024' not in output
    assert config == before


def test_srv6_template_accepts_explicit_prefix_and_legacy_source_key():
    config = profile()
    config['SRV6_MY_LOCATORS']['LOC1']['prefix'] += '/48'
    config['SRV6_MY_SOURCE']['legacy-source'] = config['SRV6_MY_SOURCE'].pop('tunnel-source')
    assert 'prefix 2001:db8:100::/48 block-len 32' in render(config)


def test_srv6_template_preserves_optional_ua_adjacency_and_string_keys():
    config = profile()
    config['SRV6_MY_SIDS'] = {'|'.join(key): row for key, row in config['SRV6_MY_SIDS'].items()}
    config['SRV6_MY_SIDS']['LOC1|2001:db8:100:a::/128'].pop('adj')
    output = render(config)
    assert 'behavior uA interface Ethernet0\n' in output
    assert 'nexthop' not in output


def test_srv6_template_disabled_parent_retains_binding_intent():
    config = profile()
    config['ISIS_GLOBALS']['default']['enabled'] = 'false'
    before = copy.deepcopy(config)
    output = render(config)
    assert 'router isis default' not in output
    assert 'locator LOC1' in output and 'source-address 2001:db8::1' in output
    assert config == before


def test_srv6_template_does_not_add_an_implicit_binding_or_empty_domain():
    config = profile()
    config['ISIS_GLOBALS']['default'].pop('srv6_locator')
    config['ISIS_GLOBALS']['default'].pop('srv6_interface')
    assert 'router isis default' not in render(config)
    assert 'segment-routing' not in render({'BGP_GLOBALS': config['BGP_GLOBALS']})


@pytest.mark.parametrize('mixed', [False, True])
def test_srv6_template_preserves_existing_named_isis_startup_scope(mixed):
    config = profile()
    config['ISIS_GLOBALS'] = {'Blue': {'enabled': 'true'}}
    if mixed:
        config['ISIS_GLOBALS']['default'] = {'enabled': 'true'}
    output = render(config)
    assert 'segment-routing' not in output and 'static-sids' not in output
    assert 'router isis' not in output


def test_srv6_template_allows_shared_policy_prefix_without_static_sid():
    config = profile()
    config['SRV6_MY_LOCATORS']['OTHER'] = {'prefix': '2001:db8:200::'}
    config['SRV6_MY_SIDS'][('OTHER', '2001:db8:100::/48')] = {'decap_dscp_mode': 'pipe'}
    output = render(config)
    assert output.count('sid 2001:db8:100::/48') == 1


@pytest.mark.parametrize('invalid', [
    {'arg_len': '16'}, {'vrf': 'Vrf2'}, {'prefix': '2001:db8:100::/64'},
    {'format': 'usid-f3216'},
])
def test_srv6_validation_rejects_unrepresentable_locator_before_render(invalid):
    config = profile()
    config['SRV6_MY_LOCATORS']['LOC1'].update(invalid)
    output = []
    with pytest.raises(ValueError):
        output.append(render(config))
    assert output == []


@pytest.mark.parametrize('invalid', ['source', 'duplicate', 'binding', 'ua-adj'])
def test_srv6_validation_rejects_invalid_graph_before_render(invalid):
    config = profile()
    if invalid == 'source':
        config['SRV6_MY_SOURCE']['other'] = {'source-address': '2001:db8::2'}
    elif invalid == 'duplicate':
        config['SRV6_MY_LOCATORS']['OTHER'] = {'prefix': '2001:db8:200::'}
        config['SRV6_MY_SIDS'][('OTHER', '2001:0DB8:0100::/48')] = {'action': 'uN'}
    elif invalid == 'binding':
        config['ISIS_GLOBALS']['default']['srv6_interface'] = 'Loopback999'
    else:
        config['SRV6_MY_SIDS'][('LOC1', '2001:db8:100:a::/128')]['adj'] = '192.0.2.1'
    with pytest.raises(ValueError):
        render(config)


def test_srv6_target_function_limit_is_not_a_master_schema_default():
    config = profile()
    config['SRV6_MY_LOCATORS']['LOC1']['func_len'] = '21'
    assert 'func-bits 21' in render(config)
    with pytest.raises(ValueError, match='20'):
        render(config, frr_version=(10, 5, 4))


def test_srv6_canonical_template_is_packaged_and_included_once():
    with patch('setuptools.setup') as setup:
        runpy.run_path(str(FRAMEWORK / 'setup.py'), run_name='srv6_setup_metadata')
    files = [name for _, names in setup.call_args.kwargs['data_files'] for name in names]
    assert files.count('templates/srv6/srv6.conf.j2') == 1
    environment = Environment()
    parsed = environment.parse((FRAMEWORK / 'templates/frr/frr.conf.j2').read_text())
    includes = [node.template.value for node in parsed.find_all(nodes.Include)]
    assert includes.count('srv6.conf.j2') == 1
    assert includes.index('isisd.conf.j2') < includes.index('srv6.conf.j2')
