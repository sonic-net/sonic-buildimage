from unittest.mock import MagicMock

import os

import bgpcfgd.managers_bgp
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from swsscommon import swsscommon

from . import swsscommon_test
from .util import load_constants


TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')


def constructor(require_loopback=True, include_mgmt_interface=False):
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': MagicMock(),
        'tf': TemplateFabric(TEMPLATE_PATH),
        'constants': load_constants()['constants'],
    }
    return_value_map = {
        "['vtysh', '-H', '/dev/null', '-c', 'show bgp vrfs json']": (
            0,
            '{"vrfs": {"default": {}}}',
            '',
        ),
        "['vtysh', '-c', 'show bgp vrf default neighbors json']": (
            0,
            '{}',
            '',
        ),
    }
    bgpcfgd.managers_bgp.run_command = lambda cmd: return_value_map[str(cmd)]

    manager = bgpcfgd.managers_bgp.BGPPeerMgrBase(
        common_objs,
        'CONFIG_DB',
        swsscommon.CFG_BGP_NEIGHBOR_TABLE_NAME,
        'general',
        True,
        require_loopback=require_loopback,
        include_mgmt_interface=include_mgmt_interface,
    )
    manager.directory.put(
        'CONFIG_DB',
        swsscommon.CFG_DEVICE_METADATA_TABLE_NAME,
        'localhost',
        {'bgp_asn': '65100'},
    )
    manager.directory.put(
        'CONFIG_DB',
        swsscommon.CFG_DEVICE_NEIGHBOR_METADATA_TABLE_NAME,
        'TOR',
        {},
    )
    manager.directory.put(
        'LOCAL',
        'local_addresses',
        'Ethernet4|30.30.30.30',
        {'interface': 'Ethernet4', 'prefixlen': '24'},
    )
    manager.directory.put(
        'LOCAL',
        'interfaces',
        'Ethernet4',
        {'admin_status': 'up'},
    )
    return manager


def test_default_peer_manager_options():
    manager = constructor()

    assert manager.require_loopback is True
    assert manager.include_mgmt_interface is False
    assert (
        'CONFIG_DB',
        swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME,
        'Loopback0',
    ) in manager.deps
    assert (
        'CONFIG_DB',
        swsscommon.CFG_MGMT_INTERFACE_TABLE_NAME,
        '',
    ) not in manager.deps


def test_optional_mgmt_interface_without_loopback():
    manager = constructor(
        require_loopback=False,
        include_mgmt_interface=True,
    )
    manager.directory.put(
        'CONFIG_DB',
        swsscommon.CFG_MGMT_INTERFACE_TABLE_NAME,
        'eth0|10.0.0.1/24',
        {},
    )
    manager.peer_group_mgr.update = MagicMock(return_value=True)
    manager.templates['add'].render = MagicMock(return_value='')

    result = manager.set_handler(
        '30.30.30.1',
        {
            'asn': '65200',
            'holdtime': '180',
            'keepalive': '60',
            'local_addr': '30.30.30.30',
            'name': 'TOR',
            'nhopself': '0',
            'rrclient': '0',
        },
    )

    assert result
    assert (
        'CONFIG_DB',
        swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME,
        'Loopback0',
    ) not in manager.deps
    assert (
        'CONFIG_DB',
        swsscommon.CFG_MGMT_INTERFACE_TABLE_NAME,
        '',
    ) in manager.deps
    render_args = manager.templates['add'].render.call_args.kwargs
    assert render_args['CONFIG_DB__MGMT_INTERFACE'] == {
        ('eth0', '10.0.0.1/24'): {},
    }
    assert 'loopback0_ipv4' not in render_args
