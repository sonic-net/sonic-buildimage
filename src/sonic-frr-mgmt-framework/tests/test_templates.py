"""Render tests for the startup/reload jinja2 templates.

frrcfgd has two independent render paths for the same CONFIG_DB fields: the runtime
vtysh path (tested in test_config.py) and these templates, which build /etc/frr/frr.conf
at boot and on `config reload`. These tests pin the template side so the two cannot
silently diverge.

Includes that live outside this package (docker-fpm-frr's common/ and zebra/ templates)
are stubbed out, as are the non-BGP daemon includes, so a render exercises only the
templates shipped here.
"""
import os

from jinja2 import ChoiceLoader, DictLoader, Environment, FileSystemLoader

TEMPLATE_ROOT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'templates')

# Includes pulled in by frr.conf.j2 that are either shipped by docker-fpm-frr or belong to
# another daemon; neither is under test here.
STUBBED_INCLUDES = {
    'common/daemons.common.conf.j2': '',
    'zebra/zebra.interfaces.conf.j2': '',
    'staticd.db.default_route.conf.j2': '',
    'staticd.db.conf.j2': '',
    'ospfd.conf.j2': '',
    'bfdd.conf.j2': '',
}


def render(template, **params):
    loader = ChoiceLoader([
        DictLoader(STUBBED_INCLUDES),
        FileSystemLoader([os.path.join(TEMPLATE_ROOT, sub)
                          for sub in ('bgpd', 'frr', 'staticd', 'ospfd', 'bfdd')]),
    ])
    env = Environment(loader=loader, trim_blocks=True)
    return env.get_template(template).render(**params)


def lines(rendered):
    return [line.strip() for line in rendered.splitlines() if line.strip() not in ('', '!')]


# ---------------------------------------------------------------- ebgp-requires-policy

def bgp_globals(**attrs):
    attrs.setdefault('local_asn', '65100')
    return {'BGP_GLOBALS': {'default': attrs}}


def test_ebgp_requires_policy_false():
    out = lines(render('bgpd.conf.db.j2', **bgp_globals(ebgp_requires_policy='false')))
    assert 'no bgp ebgp-requires-policy' in out
    assert 'bgp ebgp-requires-policy' not in out


def test_ebgp_requires_policy_true():
    out = lines(render('bgpd.conf.db.j2', **bgp_globals(ebgp_requires_policy='true')))
    assert 'bgp ebgp-requires-policy' in out
    assert 'no bgp ebgp-requires-policy' not in out


def test_ebgp_requires_policy_absent():
    # No field: leave FRR's built-in default alone, emit nothing either way.
    out = lines(render('bgpd.conf.db.j2', **bgp_globals()))
    assert not any('ebgp-requires-policy' in line for line in out)


# --------------------------------------------------------------------- route-map on-match

def route_map(stmt, **attrs):
    attrs.setdefault('route_operation', 'permit')
    return {'ROUTE_MAP': {('TEST_RM', stmt): attrs}}


def test_route_map_on_match_next():
    out = lines(render('bgpd.conf.db.route_map.j2',
                       **route_map('10', set_on_match_action='ON_MATCH_NEXT')))
    assert 'route-map TEST_RM permit 10' in out
    assert 'on-match next' in out


def test_route_map_on_match_goto():
    out = lines(render('bgpd.conf.db.route_map.j2',
                       **route_map('10', set_on_match_action='ON_MATCH_GOTO',
                                   set_on_match_goto='20')))
    assert 'on-match goto 20' in out


def test_route_map_on_match_goto_without_target():
    # Rejected by the YANG 'must', so it cannot reach the renderer from a validated config;
    # if it ever does, fail safe rather than emit an incomplete clause. The runtime handler
    # (handle_rmap_on_match) drops the same input.
    out = lines(render('bgpd.conf.db.route_map.j2',
                       **route_map('10', set_on_match_action='ON_MATCH_GOTO')))
    assert 'route-map TEST_RM permit 10' in out
    assert not any(line.startswith('on-match') for line in out)


def test_route_map_on_match_absent():
    out = lines(render('bgpd.conf.db.route_map.j2', **route_map('10')))
    assert not any(line.startswith('on-match') for line in out)


# --------------------------------------------------------------------------- set src

def test_route_map_set_src():
    out = lines(render('bgpd.conf.db.route_map.j2',
                       **route_map('10', set_src='10.1.0.32')))
    assert 'set src 10.1.0.32' in out


def test_route_map_set_src_v6():
    out = lines(render('bgpd.conf.db.route_map.j2',
                       **route_map('10', set_src='fc00:1::32')))
    assert 'set src fc00:1::32' in out


# ----------------------------------------------------------------- PROTOCOL_ROUTE_MAP bind

def frr_conf(**params):
    params.setdefault('DEVICE_METADATA', {})
    return lines(render('frr.conf.j2', **params))


def test_protocol_route_map_binds():
    out = frr_conf(PROTOCOL_ROUTE_MAP={
        ('ipv4', 'bgp'): {'route_map': 'RM_SET_SRC'},
        ('ipv6', 'bgp'): {'route_map': 'RM_SET_SRC6'},
        ('ipv4', 'static'): {'route_map': 'RM_SET_SRC'},
    })
    assert 'ip protocol bgp route-map RM_SET_SRC' in out
    assert 'ipv6 protocol bgp route-map RM_SET_SRC6' in out
    assert 'ip protocol static route-map RM_SET_SRC' in out


def test_protocol_route_map_absent():
    out = frr_conf()
    assert not any('protocol' in line and 'route-map' in line for line in out)


def test_protocol_route_map_row_without_route_map():
    out = frr_conf(PROTOCOL_ROUTE_MAP={('ipv4', 'bgp'): {}})
    assert not any('protocol' in line and 'route-map' in line for line in out)


def test_set_src_route_map_renders_with_its_bind():
    # The RM_SET_SRC loopback-source setup end to end: a ROUTE_MAP carrying 'set src' plus
    # the PROTOCOL_ROUTE_MAP row that binds it, both out of a single frr.conf render.
    out = frr_conf(ROUTE_MAP={('RM_SET_SRC', '10'): {'route_operation': 'permit',
                                                     'set_src': '10.1.0.32'}},
                   PROTOCOL_ROUTE_MAP={('ipv4', 'bgp'): {'route_map': 'RM_SET_SRC'}})
    assert 'route-map RM_SET_SRC permit 10' in out
    assert 'set src 10.1.0.32' in out
    assert 'ip protocol bgp route-map RM_SET_SRC' in out


# --------------------------------------------------------- device-wide routing knobs
#
# bgpcfgd renders these from its own zebra/bgpd templates. frrcfgd rendered none of them,
# so a frrcfgd DUT silently diverged -- most sharply on nexthop-group retention, where
# losing the line reverts zebra to FRR's 180s default. Each knob is opt-in: absent means
# nothing is emitted and FRR's own default stands, so upgrading changes no installation.

def device_metadata(**attrs):
    return {'DEVICE_METADATA': {'localhost': attrs}}


def test_nexthop_group_keep_time_rendered():
    out = frr_conf(**device_metadata(nexthop_group_keep_time='1'))
    assert 'zebra nexthop-group keep 1' in out


def test_nexthop_group_keep_time_absent_emits_nothing():
    # Absent => FRR's default (180s) applies; emitting anything here would change behaviour
    # for every existing installation on upgrade.
    out = frr_conf(**device_metadata())
    assert not any('nexthop-group keep' in line for line in out)


def test_zebra_nexthop_kernel_enable_rendered():
    out = frr_conf(**device_metadata(zebra_nexthop='enabled'))
    assert 'zebra nexthop kernel enable' in out


def test_zebra_nexthop_kernel_disabled_emits_nothing():
    out = frr_conf(**device_metadata(zebra_nexthop='disabled'))
    assert not any('nexthop kernel' in line for line in out)


def test_suppress_fib_pending_enabled():
    out = lines(render('bgpd.conf.db.j2',
                       BGP_GLOBALS={'default': {'local_asn': '65100'}},
                       **device_metadata(**{'suppress-fib-pending': 'enabled'})))
    assert 'bgp suppress-fib-pending' in out
    assert 'no bgp suppress-fib-pending' not in out


def test_suppress_fib_pending_disabled():
    out = lines(render('bgpd.conf.db.j2',
                       BGP_GLOBALS={'default': {'local_asn': '65100'}},
                       **device_metadata(**{'suppress-fib-pending': 'disabled'})))
    assert 'no bgp suppress-fib-pending' in out


def test_suppress_fib_pending_absent_emits_nothing():
    out = lines(render('bgpd.conf.db.j2', BGP_GLOBALS={'default': {'local_asn': '65100'}},
                       **device_metadata()))
    assert not any('suppress-fib-pending' in line for line in out)
