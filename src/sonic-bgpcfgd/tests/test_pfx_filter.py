from bgpcfgd.template import TemplateFabric
from collections import OrderedDict
import pytest


def test_pfx_filter_none():
    res = TemplateFabric.pfx_filter(None)
    assert isinstance(res, OrderedDict) and len(res) == 0

def test_pfx_filter_empty_tuple():
    res = TemplateFabric.pfx_filter(())
    assert isinstance(res, OrderedDict) and len(res) == 0

def test_pfx_filter_empty_list():
    res = TemplateFabric.pfx_filter([])
    assert isinstance(res, OrderedDict) and len(res) == 0

def test_pfx_filter_empty_dict():
    res = TemplateFabric.pfx_filter({})
    assert isinstance(res, OrderedDict) and len(res) == 0

def test_pfx_filter_strings():
    src = {
        'Loopback0': {},
        'Loopback1': {},
    }
    expected = OrderedDict([])
    res = TemplateFabric.pfx_filter(src)
    assert res == expected

def test_pfx_filter_mixed_keys():
    src = {
        'Loopback0': {},
        ('Loopback0', '11.11.11.11/32'): {},
        'Loopback1': {},
        ('Loopback1', '55.55.55.55/32'): {},
    }
    expected = OrderedDict(
        [
            (('Loopback1', '55.55.55.55/32'), {}),
            (('Loopback0', '11.11.11.11/32'), {}),
        ]
    )
    res = TemplateFabric.pfx_filter(src)
    assert dict(res) == dict(expected)


def test_pfx_filter_pfx_v4_w_mask():
    src = {
        ('Loopback0', '11.11.11.11/32'): {},
        ('Loopback1', '55.55.55.55/32'): {},
    }
    expected = OrderedDict(
        [
            (('Loopback1', '55.55.55.55/32'), {}),
            (('Loopback0', '11.11.11.11/32'), {}),
        ]
    )
    res = TemplateFabric.pfx_filter(src)
    assert dict(res) == dict(expected)

def test_pfx_filter_pfx_v6_w_mask():
    src = {
        ('Loopback0', 'fc00::/128'): {},
        ('Loopback1', 'fc00::1/128'): {},
    }
    expected = OrderedDict(
        [
            (('Loopback0', 'fc00::/128'), {}),
            (('Loopback1', 'fc00::1/128'), {}),
        ]
    )
    res = TemplateFabric.pfx_filter(src)
    assert res == expected

def test_pfx_filter_pfx_v4_no_mask():
    src = {
        ('Loopback0', '11.11.11.11'): {},
        ('Loopback1', '55.55.55.55'): {},
    }
    expected = OrderedDict(
        [
            (('Loopback1', '55.55.55.55/32'), {}),
            (('Loopback0', '11.11.11.11/32'), {}),
        ]
    )
    res = TemplateFabric.pfx_filter(src)
    assert dict(res) == dict(expected)

def test_pfx_filter_pfx_v6_no_mask():
    src = {
        ('Loopback0', 'fc00::'): {},
        ('Loopback1', 'fc00::1'): {},
    }
    expected = OrderedDict(
        [
            (('Loopback0', 'fc00::/128'), {}),
            (('Loopback1', 'fc00::1/128'), {}),
        ]
    )
    res = TemplateFabric.pfx_filter(src)
    assert res == expected


def test_pfx_filter_pfx_comprehensive():
    src = {
        'Loopback0': {},
        ('Loopback0', 'fc00::'): {},
        'Loopback1': {},
        ('Loopback1', 'fc00::1/128'): {},
        ('Loopback2', '11.11.11.11/32'): {},
        ('Loopback3', '55.55.55.55'): {},
        'Loopback2': {},
        'Loopback3': {},
        ('Loopback5', '22.22.22.1/24'): {},
        ('Loopback6', 'fc00::55/64'): {},
    }
    expected = OrderedDict(
        [
            (('Loopback1', 'fc00::1/128'), {}),
            (('Loopback3', '55.55.55.55/32'), {}),
            (('Loopback6', 'fc00::55/64'), {}),
            (('Loopback2', '11.11.11.11/32'), {}),
            (('Loopback0', 'fc00::/128'), {}),
            (('Loopback5', '22.22.22.1/24'), {}),
        ]
    )
    res = TemplateFabric.pfx_filter(src)
    assert dict(res) == dict(expected)

@pytest.fixture
def test_pfx_filter_wrong_ip(caplog):
    src = {
        ('Loopback0', 'wrong_ip'): {},
    }
    res = TemplateFabric.pfx_filter(src)
    assert "'wrong_ip' is invalid ip address" in caplog.text
    assert isinstance(res, OrderedDict) and len(res) == 0

