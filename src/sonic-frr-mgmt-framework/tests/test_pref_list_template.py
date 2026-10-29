import os

import jinja2
import pytest

TEMPLATE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        '..', 'templates', 'bgpd', 'bgpd.conf.db.pref_list.j2')


def render(prefix_set, prefix):
    with open(TEMPLATE) as f:
        template = jinja2.Template(f.read())
    out = template.render(PREFIX_SET=prefix_set, PREFIX=prefix)
    return [line.strip() for line in out.splitlines() if line.strip() and line.strip() != '!']


# A masklength range is stored as 'MIN..MAX'. The template emits 'ge MIN' only when MIN is
# longer than the prefix itself; that comparison must be numeric. As text, '32' < '8', so a
# /8 prefix with 32..32 lost its lower bound and matched every length from /8 to /32.
@pytest.mark.parametrize('mode, prefix, length_range, expected', [
    ('IPv4', '10.0.0.0/8', '32..32', 'ip prefix-list PL seq 10 permit 10.0.0.0/8 ge 32 le 32'),
    ('IPv4', '10.0.0.0/8', '24..28', 'ip prefix-list PL seq 10 permit 10.0.0.0/8 ge 24 le 28'),
    ('IPv4', '10.0.0.0/9', '10..32', 'ip prefix-list PL seq 10 permit 10.0.0.0/9 ge 10 le 32'),
    ('IPv4', '10.0.0.0/24', '30..32', 'ip prefix-list PL seq 10 permit 10.0.0.0/24 ge 30 le 32'),
    # the lower bound equals the prefix length: no 'ge' needed
    ('IPv4', '10.0.0.0/24', '24..32', 'ip prefix-list PL seq 10 permit 10.0.0.0/24 le 32'),
    ('IPv4', '10.0.0.0/16', '16..24', 'ip prefix-list PL seq 10 permit 10.0.0.0/16 le 24'),
    ('IPv6', '2001::/16', '100..128', 'ipv6 prefix-list PL seq 10 permit 2001::/16 ge 100 le 128'),
    ('IPv6', '2001:db8::/32', '48..64', 'ipv6 prefix-list PL seq 10 permit 2001:db8::/32 ge 48 le 64'),
])
def test_prefix_range(mode, prefix, length_range, expected):
    lines = render({'PL': {'mode': mode}},
                   {('PL', '10', prefix, length_range): {'action': 'permit'}})
    assert lines == [expected]


def test_prefix_exact():
    lines = render({'PL': {'mode': 'IPv4'}},
                   {('PL', '10', '10.0.0.0/8', 'exact'): {'action': 'deny'}})
    assert lines == ['ip prefix-list PL seq 10 deny 10.0.0.0/8']


def test_prefix_without_sequence_number():
    # PREFIX_NOSEQ_LIST keys carry no sequence number
    lines = render({'PL': {'mode': 'IPv4'}},
                   {('PL', '10.0.0.0/8', '32..32'): {'action': 'permit'}})
    assert lines == ['ip prefix-list PL  permit 10.0.0.0/8 ge 32 le 32']
