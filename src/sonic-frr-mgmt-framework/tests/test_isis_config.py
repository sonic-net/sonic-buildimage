"""Desired-snapshot validation; no native application or readiness is implied."""

import copy

import pytest

from frrcfgd.isis_config import (
    ConfigError, normalize_managed_config, validate_isis_config, validate_srv6_config,
)


SID_KEY = 'LOC1|2001:db8:100:a::/128'


@pytest.fixture
def config():
    return {
        'PORT': {'Ethernet0': {}},
        'INTERFACE': {'Ethernet0': {}},
        'LOOPBACK_INTERFACE': {'Loopback0': {}, ('Loopback0', '2001:db8::1/128'): {}},
        'VRF': {'Blue': {}},
        'ISIS_GLOBALS': {'default': {
            'enabled': 'true', 'net_title': '49.0001.0000.0000.0001.00', 'level': 'level-2',
            'srv6_locator': 'LOC1', 'srv6_interface': 'Loopback0'}},
        'ISIS_GLOBALS_TIMERS': {'default': {'lsp_mtu': '1400', 'lsp_refresh_interval': '100',
                                            'lsp_lifetime': '500'}},
        'ISIS_INTERFACE': {'Ethernet0': {'enabled': 'true', 'ipv4_enabled': 'false',
                                         'ipv6_enabled': 'true', 'metric': '1500', 'circuit_type': 'p2p'}},
        'SRV6_MY_SOURCE': {'tunnel-source': {'source-address': '2001:db8::1'}},
        'SRV6_MY_LOCATORS': {'LOC1': {'prefix': '2001:db8:100::/48', 'block_len': '32',
                                      'node_len': '16', 'func_len': '16', 'arg_len': '0',
                                      'behavior': 'usid', 'format': 'usid-f3216'}},
        'SRV6_MY_SIDS': {SID_KEY: {'action': 'uN'}},
        'BGP_GLOBALS': {'default': {'local_asn': '65000'}},
        'BGP_NEIGHBOR': {('Blue', '192.0.2.2'): {'admin_status': 'down', 'asn': '65001'}},
    }


def validate_both(config):
    validate_isis_config(config)
    validate_srv6_config(config, frr_version=(10, 5, 4))


def test_default_single_instance_and_full_srv6_profile_validate_without_mutating_input(config):
    before = copy.deepcopy(config)
    validate_both(config)
    assert config == before


def test_tuple_keys_normalize_once_while_foreign_bgp_keys_and_values_are_retained(config):
    for table in ('ISIS_GLOBALS', 'ISIS_GLOBALS_TIMERS', 'ISIS_INTERFACE',
                  'SRV6_MY_SOURCE', 'SRV6_MY_LOCATORS', 'SRV6_MY_SIDS'):
        config[table] = {tuple(key.split('|')): row for key, row in config[table].items()}
    before = copy.deepcopy(config)
    normalized = normalize_managed_config(config)
    assert normalized['ISIS_GLOBALS'] == {'default': config['ISIS_GLOBALS'][('default',)]}
    assert SID_KEY in normalized['SRV6_MY_SIDS'] and 'LOC1' in normalized['SRV6_MY_LOCATORS']
    assert normalized['BGP_NEIGHBOR'] == before['BGP_NEIGHBOR']
    validate_both(config)
    assert config == before
    normalized['BGP_NEIGHBOR'][('Blue', '192.0.2.2')]['admin_status'] = 'up'
    assert config['BGP_NEIGHBOR'][('Blue', '192.0.2.2')]['admin_status'] == 'down'


def test_duplicate_serialized_managed_key_is_not_silently_overwritten(config):
    config['ISIS_GLOBALS'][('default',)] = copy.deepcopy(config['ISIS_GLOBALS']['default'])
    with pytest.raises(ConfigError, match='Duplicate serialized'):
        normalize_managed_config(config)


@pytest.mark.parametrize('table,key,validator', [
    ('ISIS_GLOBALS', 'default', validate_isis_config),
    ('ISIS_GLOBALS_TIMERS', 'default', validate_isis_config),
    ('ISIS_INTERFACE', 'Ethernet0', validate_isis_config),
    ('SRV6_MY_SOURCE', 'tunnel-source', validate_srv6_config),
    ('SRV6_MY_LOCATORS', 'LOC1', validate_srv6_config),
    ('SRV6_MY_SIDS', SID_KEY, validate_srv6_config),
])
def test_unknown_managed_field_is_rejected_before_render(config, table, key, validator):
    config[table][key]['unknown-setting'] = 'unexpected'
    with pytest.raises(ConfigError, match='Unsupported managed fields'):
        validator(config)


@pytest.mark.parametrize('instance', ['1', 'Blue'])
def test_nondefault_or_second_isis_instance_is_outside_recovery_scope(config, instance):
    config['ISIS_GLOBALS'][instance] = {'enabled': 'false'}
    with pytest.raises(ConfigError, match='one default'):
        validate_isis_config(config)


def test_disabled_intent_retains_dependent_rows_without_activating_or_discarding_them(config):
    config['ISIS_GLOBALS']['default']['enabled'] = 'false'
    config['ISIS_INTERFACE']['Ethernet0']['enabled'] = 'false'
    before = copy.deepcopy(config)
    validate_both(config)
    assert config == before
    assert config['ISIS_GLOBALS_TIMERS']['default']['lsp_mtu'] == '1400'
    assert config['ISIS_INTERFACE']['Ethernet0']['metric'] == '1500'


@pytest.mark.parametrize('families', [
    {}, {'ipv4_enabled': True, 'ipv6_enabled': True},
    {'ipv4_enabled': True, 'ipv6_enabled': False},
    {'ipv4_enabled': False, 'ipv6_enabled': True},
    {'ipv4_enabled': False, 'ipv6_enabled': False},
])
def test_omitted_and_explicit_address_families_including_both_disabled_are_valid(config, families):
    interface = config['ISIS_INTERFACE']['Ethernet0']
    interface.pop('ipv4_enabled')
    interface.pop('ipv6_enabled')
    interface.update(families)
    before = copy.deepcopy(interface)
    validate_isis_config(config)
    assert interface == before


@pytest.mark.parametrize('field', ['ipv4_enabled', 'ipv6_enabled'])
def test_invalid_af_boolean_is_rejected(config, field):
    config['ISIS_INTERFACE']['Ethernet0'][field] = 'enabled'
    with pytest.raises(ConfigError, match='boolean'):
        validate_isis_config(config)


@pytest.mark.parametrize('net', [
    '49.0001.0000.0000.00', '49.0001.0000.0000.0001.00',
    'AA.' + '.'.join(['aBcD'] * 9) + '.00',
])
def test_net_schema_grammar_minimum_normal_and_maximum_lengths(config, net):
    config['ISIS_GLOBALS']['default']['net_title'] = net
    validate_isis_config(config)


@pytest.mark.parametrize('net', [
    '49..0001.0000.0000.0001.00', '49.0001.0000.0000.0001.00.',
    '49000100000000000100', '49.001.0000.0000.0001.00',
    '4900.0001.0000.0000.0001.00', '49.0001.0000.0000.0001.01',
    '49.0001.0000.00', '49.' + '.'.join(['0001'] * 10) + '.00',
    '49.000G.0000.0000.0001.00', None,
])
def test_net_noncanonical_dots_groups_nsel_and_lengths_are_rejected(config, net):
    config['ISIS_GLOBALS']['default']['net_title'] = net
    with pytest.raises(ConfigError, match='NET'):
        validate_isis_config(config)


def test_orphan_timer_and_foreign_vrf_binding_are_rejected(config):
    config['ISIS_GLOBALS_TIMERS']['Blue'] = {'lsp_mtu': '1497'}
    with pytest.raises(ConfigError, match='Orphan'):
        validate_isis_config(config)
    del config['ISIS_GLOBALS_TIMERS']['Blue']
    config['INTERFACE']['Ethernet0']['vrf_name'] = 'Blue'
    with pytest.raises(ConfigError, match='default-VRF'):
        validate_isis_config(config)


def test_unrelated_foreign_vrf_interface_and_bgp_are_preserved(config):
    config['INTERFACE'][('Ethernet4',)] = {'vrf_name': 'Blue'}
    before = copy.deepcopy(config)
    validate_both(config)
    assert config == before


@pytest.mark.parametrize('changes', [
    {'lsp_mtu': '127'}, {'lsp_mtu': '4353'},
    {'lsp_refresh_interval': '0'}, {'lsp_lifetime': '349'},
    {'lsp_refresh_interval': '201', 'lsp_lifetime': '500'},
])
def test_timer_bounds_and_refresh_lifetime_relation_are_validated(config, changes):
    config['ISIS_GLOBALS_TIMERS']['default'].update(changes)
    with pytest.raises(ConfigError, match='timer|lifetime'):
        validate_isis_config(config)


def test_srv6_source_is_singleton_even_with_different_configdb_keys(config):
    config['SRV6_MY_SOURCE']['another-source'] = {'source-address': '2001:db8::2'}
    with pytest.raises(ConfigError, match='singleton'):
        validate_srv6_config(config)
    del config['SRV6_MY_SOURCE']['another-source']
    config['SRV6_MY_SOURCE']['tunnel-source']['source-address'] = '::'
    with pytest.raises(ConfigError, match='source address'):
        validate_srv6_config(config)


@pytest.mark.parametrize('changes', [
    {'behavior': None}, {'block_len': '16', 'node_len': '32'}, {'func_len': '20'},
    {'format': 'unknown-format'}, {'arg_len': '1'},
])
def test_f3216_requires_supported_behavior_format_and_geometry(config, changes):
    config['SRV6_MY_LOCATORS']['LOC1'].update(changes)
    with pytest.raises(ConfigError):
        validate_srv6_config(config, frr_version=(10, 5, 4))


def test_sid_prefix_collision_is_detected_after_ipv6_normalization(config):
    config['SRV6_MY_LOCATORS']['LOC2'] = {'prefix': '2001:db8:200::/48'}
    config['SRV6_MY_SIDS']['LOC2|2001:0db8:0100:000a::/128'] = {'action': 'uN'}
    with pytest.raises(ConfigError, match='Conflicting native SID prefix'):
        validate_srv6_config(config)


def test_policy_only_row_does_not_acquire_static_sid_ownership_or_collide(config):
    config['SRV6_MY_LOCATORS']['LOC2'] = {'prefix': '2001:db8:200::/48'}
    key = 'LOC2|2001:db8:100:a::/128'
    config['SRV6_MY_SIDS'][key] = {'decap_dscp_mode': 'pipe'}
    before = copy.deepcopy(config)
    validate_srv6_config(config)
    assert config == before and 'action' not in config['SRV6_MY_SIDS'][key]


@pytest.mark.parametrize('adj', [None, 'fe80::2', '2001:0db8:0000::2'])
def test_ua_interface_accepts_absent_or_valid_optional_adj(config, adj):
    row = {'action': 'uA', 'interface': 'Ethernet0'}
    if adj is not None:
        row['adj'] = adj
    config['SRV6_MY_SIDS'][SID_KEY] = row
    validate_srv6_config(config)


def test_ua_requires_configured_interface_and_rejects_unknown_nexthop_leaf(config):
    config['SRV6_MY_SIDS'][SID_KEY] = {'action': 'uA', 'interface': 'Ethernet999'}
    with pytest.raises(ConfigError, match='configured interface'):
        validate_srv6_config(config)
    config['SRV6_MY_SIDS'][SID_KEY] = {'action': 'uA', 'interface': 'Ethernet0',
                                       'nexthop': 'fe80::2'}
    with pytest.raises(ConfigError, match='Unsupported managed fields'):
        validate_srv6_config(config)


@pytest.mark.parametrize('adj', ['192.0.2.1', 'invalid', None])
def test_ua_explicit_invalid_adj_is_rejected(config, adj):
    config['SRV6_MY_SIDS'][SID_KEY] = {'action': 'uA', 'interface': 'Ethernet0', 'adj': adj}
    with pytest.raises(ConfigError, match='adj'):
        validate_srv6_config(config)


def test_udt_named_decap_vrf_is_retained_and_unknown_reference_rejected(config):
    config['SRV6_MY_SIDS'][SID_KEY] = {'action': 'uDT46', 'decap_vrf': 'Blue'}
    validate_srv6_config(config)
    config['SRV6_MY_SIDS'][SID_KEY]['decap_vrf'] = 'Unknown'
    with pytest.raises(ConfigError, match='Unknown decapsulation VRF'):
        validate_srv6_config(config)


@pytest.mark.parametrize('version,function,valid', [
    ((10, 5, 4), 20, True), ((10, 5, 4), 21, False),
    ((10, 6, 0), 21, True), ((11, 0, 0), 64, True),
])
def test_function_limit_is_selected_by_target_frr_version(config, version, function, valid):
    locator = config['SRV6_MY_LOCATORS']['LOC1']
    locator.pop('format')
    locator.pop('behavior')
    locator['func_len'] = str(function)
    if valid:
        validate_srv6_config(config, frr_version=version)
    else:
        with pytest.raises(ConfigError, match='at most 20'):
            validate_srv6_config(config, frr_version=version)
