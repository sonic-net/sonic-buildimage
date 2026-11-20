from unittest.mock import MagicMock, patch

import bgpcfgd.frr
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
import bgpcfgd
from copy import deepcopy


swsscommon_module_mock = MagicMock()

global_constants = {
    "bgp": {
        "allow_list": {
            "enabled": True,
            "default_pl_rules": {
                "v4": [ "deny 0.0.0.0/0 le 17" ],
                "v6": [
                    "deny 0::/0 le 59",
                    "deny 0::/0 ge 65"
                ]
            },
            "default_action": "permit",
            "drop_community": "123:123"
        }
    }
}

global_constants_with_prefix_match_tag = {
    "bgp": {
        "allow_list": {
            "enabled": True,
            "default_pl_rules": {
                "v4": [ "deny 0.0.0.0/0 le 17" ],
                "v6": [
                    "deny 0::/0 le 59",
                    "deny 0::/0 ge 65"
                ]
            },
            "default_action": "permit",
            "drop_community": "123:123",
            "prefix_match_tag": "1001"
        }
    }
}

@patch.dict("sys.modules", swsscommon=swsscommon_module_mock)
def set_del_test(op, args, currect_config, expected_config, update_global_default_action=None, update_constant_prefix_match_tag=False):
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    set_del_test.push_list_called = False
    def push_list(args):
        set_del_test.push_list_called = True
        assert args == expected_config
        return True
    #
    bgpcfgd.frr.run_command = lambda cmd: (0, "", "")
    #
    cfg_mgr = MagicMock()
    cfg_mgr.update.return_value = None
    cfg_mgr.push_list = push_list
    cfg_mgr.get_text.return_value = currect_config
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': deepcopy(global_constants) if not update_constant_prefix_match_tag else deepcopy(global_constants_with_prefix_match_tag)
    }

    mgr = BGPAllowListMgr(common_objs, "CONFIG_DB", "BGP_ALLOWED_PREFIXES")
    if update_global_default_action:
        mgr.constants["bgp"]["allow_list"]["default_action"] = update_global_default_action
    if op == "SET":
        mgr.set_handler(*args)
    elif op == "DEL":
        mgr.del_handler(*args)
    else:
        assert False, "Wrong operation"
    if expected_config:
        assert set_del_test.push_list_called, "cfg_mgr.push_list wasn't called"
    else:
        assert not set_del_test.push_list_called, "cfg_mgr.push_list was called"

def test_set_handler_with_community():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5|1010:2020", {
                "prefixes_v4": "10.20.30.0/24,30.50.0.0/16",
                "prefixes_v6": "fc00:20::/64,fc00:30::/64",
        }),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive'
        ],
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 le 128',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
        ]
    )

def test_set_handler_with_community_and_prefix_match_tag():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5|1010:2020", {
                "prefixes_v4": "10.20.30.0/24,30.50.0.0/16",
                "prefixes_v6": "fc00:20::/64,fc00:30::/64",
        }),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive'
        ],
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 le 128',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
        ],
        None, True
    )

def test_set_handler_with_community_and_permit_action():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5|1010:2020", {
                "prefixes_v4": "10.20.30.0/24,30.50.0.0/16",
                "prefixes_v6": "fc00:20::/64,fc00:30::/64",
                "default_action":"permit"
        }),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive'
        ],
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 le 128',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
        ]
    )

def test_set_handler_with_community_and_deny_action():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5|1010:2020", {
                "prefixes_v4": "10.20.30.0/24,30.50.0.0/16",
                "prefixes_v6": "fc00:20::/64,fc00:30::/64",
                "default_action":"deny"
        }),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive'
        ],
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 le 128',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community no-export additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community no-export additive'
        ]
    )


def test_set_handler_no_community():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5", {
                "prefixes_v4": "20.20.30.0/24,40.50.0.0/16",
                "prefixes_v6": "fc01:20::/64,fc01:30::/64",
        }),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
        ],
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 le 128',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
        ]
    )

def test_set_handler_no_community_and_prefix_match_tag():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5", {
                "prefixes_v4": "20.20.30.0/24,40.50.0.0/16",
                "prefixes_v6": "fc01:20::/64,fc01:30::/64",
        }),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
        ],
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 le 128',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            ' set tag 1001',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            ' set tag 1001',
        ],
        None,True
    )

def test_set_handler_no_community_with_permit_action():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5", {
                "prefixes_v4": "20.20.30.0/24,40.50.0.0/16",
                "prefixes_v6": "fc01:20::/64,fc01:30::/64",
                "default_action":"permit"
        }),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
        ],
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 le 128',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
        ]
    )
def test_set_handler_no_community_with_deny_action():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5", {
                "prefixes_v4": "20.20.30.0/24,40.50.0.0/16",
                "prefixes_v6": "fc01:20::/64,fc01:30::/64",
                "default_action":"deny"
        }),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
        ],
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 le 128',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community no-export additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community no-export additive'
        ]
    )

def test_del_handler_with_community():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5|1010:2020",),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 ge 25',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 ge 17',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 ge 65',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',

            ""
        ],
        [
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            'no bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
        ]
    )

def test_del_handler_with_exiting_community_deny_action():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5|1010:2020",),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 ge 25',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 ge 17',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 ge 65',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community no-export additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community no-export additive',
            ""
        ],
        [
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            'no bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
        ]
    )


def test_del_handler_with_exiting_community_permit_action():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5|1010:2020",),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 ge 25',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 ge 17',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 ge 65',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
            ""
        ],
        [
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            'no bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
        ]
    )

def test_del_handler_with_exiting_community_deny_action_global_deny():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5|1010:2020",),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 ge 25',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 ge 17',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 ge 65',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community no-export additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community no-export additive',
            ""
        ],
        [
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            'no bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
        ],
        "deny"
    )


def test_del_handler_with_exiting_community_permit_action_global_deny():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5|1010:2020",),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 ge 25',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 ge 17',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 ge 65',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
            ""
        ],
        [
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            'no bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community no-export additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community no-export additive',
        ],
        "deny"
    )


def test_del_handler_no_community():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5",),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 ge 25',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 ge 17',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 ge 65',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
            " "
        ],
        [
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
        ]
    )
def test_del_handler_with_no_community_deny_action():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5",),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 ge 25',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 ge 17',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 ge 65',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community no-export additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community no-export additive',
            ""
        ],
        [
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
        ]
    )
def test_del_handler_with_no_community_permit_action_global_deny():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5",),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 ge 25',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 ge 17',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 ge 65',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
            ""
        ],
        [
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            'no route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community no-export additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community no-export additive',
        ],
        "deny"
    )



def test_set_handler_with_community_data_is_already_presented():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5|1010:2020", {
            "prefixes_v4": "10.20.30.0/24,30.50.0.0/16",
            "prefixes_v6": "fc00:20::/64,fc00:30::/64",
        }),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 le 128',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
            ""
        ],
        []
    )

@patch.dict("sys.modules", swsscommon=swsscommon_module_mock)
def test_set_handler_no_community_data_is_already_presented():
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    cfg_mgr = MagicMock()
    cfg_mgr.update.return_value = None
    cfg_mgr.get_text.return_value = [
        'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
        'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 le 32',
        'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 le 32',
        'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
        'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
        'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
        'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 le 128',
        'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
        ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
        'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
        ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
        'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
        ' set community 123:123 additive',
        'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
        ' set community 123:123 additive',
        ""
    ]
    common_objs = {
            'directory': Directory(),
            'cfg_mgr': cfg_mgr,
            'tf': TemplateFabric(),
            'constants': global_constants,
    }
    mgr = BGPAllowListMgr(common_objs, "CONFIG_DB", "BGP_ALLOWED_PREFIXES")
    mgr.set_handler("DEPLOYMENT_ID|5", {
        "prefixes_v4": "20.20.30.0/24,40.50.0.0/16",
        "prefixes_v6": "fc01:20::/64,fc01:30::/64",
    })
    assert not cfg_mgr.push_list.called, "cfg_mgr.push_list was called, but it shouldn't have been"

def test_del_handler_with_community_no_data():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5|1010:2020",),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive'
        ],
        []
    )

def test_del_handler_no_community_no_data():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5",),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive'
        ],
        []
    )

def test_set_handler_with_community_update_prefixes_add():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5|1010:2020", {
            "prefixes_v4": "10.20.30.0/24,30.50.0.0/16,80.90.0.0/16",
            "prefixes_v6": "fc00:20::/64,fc00:30::/64,fc02::/64",
        }),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 le 128',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
            ""
        ],
        [
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 40 permit 80.90.0.0/16 le 32',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 50 permit fc02::/64 le 128',
        ]
    )

def test_set_handler_no_community_update_prefixes_add():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5", {
            "prefixes_v4": "20.20.30.0/24,40.50.0.0/16,80.90.0.0/16",
            "prefixes_v6": "fc01:20::/64,fc01:30::/64,fc02::/64",
        }),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 le 128',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
            ""
        ],
        [
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 40 permit 80.90.0.0/16 le 32',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 50 permit fc02::/64 le 128',
        ]
    )

def test_set_handler_with_community_update_prefixes_remove():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5|1010:2020", {
            "prefixes_v4": "10.20.30.0/24",
            "prefixes_v6": "fc00:20::/64",
        }),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 30 permit 30.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 40 permit fc00:30::/64 le 128',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
            ""
        ],
        [
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V4 seq 20 permit 10.20.30.0/24 le 32',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_1010:2020_V6 seq 30 permit fc00:20::/64 le 128',
        ]
    )

def test_set_handler_no_community_update_prefixes_remove():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5", {
            "prefixes_v4": "20.20.30.0/24",
            "prefixes_v6": "fc01:20::/64",
        }),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 le 128',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 30000',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 30000',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_V6 permit 65535',
            ' set community 123:123 additive',
            ""
        ],
        [
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 le 32',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
        ]
    )

def test_set_handler_with_neighbor_type():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5|NEIGHBOR_TYPE|OpticalLonghaulTerminal", {
            "prefixes_v4": "10.62.64.0/22 ge 30,"\
                           "10.1.44.0/23 ge 30,"\
                           "10.17.92.0/23 ge 30,"\
                           "10.73.92.0/23 ge 30,"\
                           "10.26.170.0/24 ge 30,"\
                           "10.26.171.0/24 ge 30,"\
                           "10.26.255.0/24 ge 30",
            "prefixes_v6": "fc01:20::/64",
        }),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 20 permit 20.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 30 permit 40.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V6 seq 40 permit fc01:30::/64 le 128',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V4 permit 30000',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V6 permit 30000',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V6',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V6 permit 65535',
            ' set community 123:123 additive',
            ""
        ],
        [
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 20 permit 10.62.64.0/22 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 30 permit 10.1.44.0/23 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 40 permit 10.17.92.0/23 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 50 permit 10.73.92.0/23 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 60 permit 10.26.170.0/24 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 70 permit 10.26.171.0/24 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V4 seq 80 permit 10.26.255.0/24 ge 30',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V6',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_empty_V6 seq 30 permit fc01:20::/64 le 128',
        ]
    )

def test_set_handler_with_neighbor_type_and_community():
    set_del_test(
        "SET",
        ("DEPLOYMENT_ID|5|NEIGHBOR_TYPE|OpticalLonghaulTerminal|1010:2020", {
            "prefixes_v4": "10.62.64.0/22 ge 30,"\
                           "10.1.44.0/23 ge 30,"\
                           "10.17.92.0/23 ge 30,"\
                           "10.73.92.0/23 ge 30,"\
                           "10.26.170.0/24 ge 30,"\
                           "10.26.171.0/24 ge 30,"\
                           "10.26.255.0/24 ge 30",
            "prefixes_v6": "fc01:20::/64",
        }),
        [
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 20 permit 20.20.30.0/24 le 32',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 30 permit 40.50.0.0/16 le 32',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V6 seq 30 permit fc01:20::/64 le 128',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V6 seq 40 permit fc01:30::/64 le 128',
            'bgp community-list standard COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020 permit 1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V4 permit 10',
            ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V6 permit 10',
            ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V6',
            ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V6 permit 65535',
            ' set community 123:123 additive',
            ""
        ],
        [
            'no ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 10 deny 0.0.0.0/0 le 17',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 20 permit 10.62.64.0/22 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 30 permit 10.1.44.0/23 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 40 permit 10.17.92.0/23 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 50 permit 10.73.92.0/23 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 60 permit 10.26.170.0/24 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 70 permit 10.26.171.0/24 ge 30',
            'ip prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V4 seq 80 permit 10.26.255.0/24 ge 30',
            'no ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V6',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V6 seq 10 deny ::/0 le 59',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V6 seq 20 deny ::/0 ge 65',
            'ipv6 prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_COMMUNITY_1010:2020_V6 seq 30 permit fc01:20::/64 le 128',
        ]
    )

def test_del_handler_with_neighbor_type_community_no_data():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5|NEIGHBOR_TYPE|OpticalLonghaulTerminal|1010:2020",),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V6 permit 65535',
            ' set community 123:123 additive'
        ],
        []
    )

def test_del_handler_with_neighbor_type_no_data():
    set_del_test(
        "DEL",
        ("DEPLOYMENT_ID|5|NEIGHBOR_TYPE|OpticalLonghaulTerminal",),
        [
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V4 permit 65535',
            ' set community 123:123 additive',
            'route-map ALLOW_LIST_DEPLOYMENT_ID_5_NEIGHBOR_OpticalLonghaulTerminal_V6 permit 65535',
            ' set community 123:123 additive'
        ],
        []
    )

@patch.dict("sys.modules", swsscommon=swsscommon_module_mock)
def test___set_handler_validate():
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': global_constants,
    }
    mgr = BGPAllowListMgr(common_objs, "CONFIG_DB", "BGP_ALLOWED_PREFIXES")
    data = {
        "prefixes_v4": "20.20.30.0/24,40.50.0.0/16",
        "prefixes_v6": "fc01:20::/64,fc01:30::/64",
    }
    assert mgr._BGPAllowListMgr__set_handler_validate("DEPLOYMENT_ID|5|1010:2020", data)
    assert mgr._BGPAllowListMgr__set_handler_validate(
        "DEPLOYMENT_ID|5|NEIGHBOR_TYPE|OpticalLonghaulTerminal|1010:2020", data
    )
    assert mgr._BGPAllowListMgr__parse_key(
        "DEPLOYMENT_ID|5|COMMUNITY_NEIGHBOR_TYPE_VALUE"
    ) == (5, "COMMUNITY_NEIGHBOR_TYPE_VALUE", "")
    assert not mgr._BGPAllowListMgr__set_handler_validate("DEPLOYMENT_ID|5|1010:2020", None)
    assert not mgr._BGPAllowListMgr__set_handler_validate("DEPLOYMENT_ID1|5|1010:2020", data)
    assert not mgr._BGPAllowListMgr__set_handler_validate("DEPLOYMENT_ID|z|1010:2020", data)
    assert not mgr._BGPAllowListMgr__set_handler_validate("DEPLOYMENT_ID|4294967296", data)
    assert not mgr._BGPAllowListMgr__set_handler_validate("DEPLOYMENT_ID|99999999999", data)
    assert not mgr._BGPAllowListMgr__set_handler_validate("DEPLOYMENT_ID|5|NEIGHBOR_TYPE", data)
    assert not mgr._BGPAllowListMgr__set_handler_validate(
        "DEPLOYMENT_ID|5|NEIGHBOR_TYPE|leaf\nroute-map injected", data
    )
    assert not mgr._BGPAllowListMgr__set_handler_validate(
        "DEPLOYMENT_ID|5|NEIGHBOR_TYPE|leaf|1010:2020\n", data
    )
    assert not mgr._BGPAllowListMgr__set_handler_validate(
        "DEPLOYMENT_ID|5|NEIGHBOR_TYPE|leaf;exit", data
    )
    assert not mgr._BGPAllowListMgr__set_handler_validate("DEPLOYMENT_ID|5|1010:2020", {
        "prefixes_v4": "20.20.30.0/24,40.50.0.0/16",
        "prefixes_v6": "20.20.30.0/24,40.50.0.0/16",
    })
    assert not mgr._BGPAllowListMgr__set_handler_validate("DEPLOYMENT_ID|5|1010:2020", {
        "prefixes_v4": "fc01:20::/64,fc01:30::/64",
        "prefixes_v6": "fc01:20::/64,fc01:30::/64",
    })
    for prefixes_v4 in (
        "1.2.3.4/x",
        "1.2.3.4",
        "10.0.0.0/255.255.255.0",
        "10.0.0.0/0.0.0.255",
        "10.0.0.0/24 ge 33",
        "10.0.0.0/24 le 20",
        "10.0.0.0/24 le " + "9" * 5000,
        "10.0.0.0/24\nroute-map injected",
    ):
        assert not mgr._BGPAllowListMgr__set_handler_validate(
            "DEPLOYMENT_ID|5|1010:2020", {"prefixes_v4": prefixes_v4}
        )
    assert not mgr._BGPAllowListMgr__set_handler_validate(
        "DEPLOYMENT_ID|5|1010:2020", {"prefixes_v6": "fe80::1%eth0/64"}
    )


def test_set_del_handlers_reject_unsafe_keys():
    data = {"prefixes_v4": "20.20.30.0/24"}
    for key in (
        "DEPLOYMENT_ID|5|NEIGHBOR_TYPE",
        "DEPLOYMENT_ID|5|NEIGHBOR_TYPE|leaf\nroute-map injected",
    ):
        set_del_test("SET", (key, data), [], [])
        set_del_test("DEL", (key,), [], [])


@patch.dict("sys.modules", swsscommon=swsscommon_module_mock)
def test___find_peer_group():
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    cfg_mgr = MagicMock()
    cfg_mgr.update.return_value = None
    cfg_mgr.get_text.return_value = [
        'router bgp 64601',
        ' neighbor BGPSLBPassive peer-group',
        ' neighbor BGPSLBPassive remote-as 65432',
        ' neighbor BGPSLBPassive passive',
        ' neighbor BGPSLBPassive ebgp-multihop 255',
        ' neighbor BGPSLBPassive update-source 10.1.0.32',
        ' neighbor PEER_V4 peer-group',
        ' neighbor PEER_V4_INT peer-group',
        ' neighbor PEER_V6 peer-group',
        ' neighbor PEER_V6_INT peer-group',
        ' neighbor 10.0.0.1 remote-as 64802',
        ' neighbor 10.0.0.1 peer-group PEER_V4',
        ' neighbor 10.0.0.1 description ARISTA01T1',
        ' neighbor 10.0.0.1 timers 3 10',
        ' neighbor fc00::2 remote-as 64802',
        ' neighbor fc00::2 peer-group PEER_V6',
        ' neighbor fc00::2 description ARISTA01T1',
        ' neighbor fc00::2 timers 3 10',
        ' address-family ipv4 unicast',
        '  neighbor BGPSLBPassive activate',
        '  neighbor BGPSLBPassive soft-reconfiguration inbound',
        '  neighbor BGPSLBPassive route-map FROM_BGP_SPEAKER in',
        '  neighbor BGPSLBPassive route-map TO_BGP_SPEAKER out',
        '  neighbor PEER_V4 soft-reconfiguration inbound',
        '  neighbor PEER_V4 allowas-in 1',
        '  neighbor PEER_V4 route-map FROM_BGP_PEER_V4 in',
        '  neighbor PEER_V4 route-map TO_BGP_PEER_V4 out',
        '  neighbor PEER_V4_INT soft-reconfiguration inbound',
        '  neighbor PEER_V4_INT allowas-in 1',
        '  neighbor PEER_V4_INT route-map FROM_BGP_PEER_V4 in',
        '  neighbor PEER_V4_INT route-map TO_BGP_PEER_V4 out',
        '  neighbor 10.0.0.1 activate',
        ' exit-address-family',
        ' address-family ipv6 unicast',
        '  neighbor BGPSLBPassive activate',
        '  neighbor PEER_V6 soft-reconfiguration inbound',
        '  neighbor PEER_V6 allowas-in 1',
        '  neighbor PEER_V6 route-map FROM_BGP_PEER_V6 in',
        '  neighbor PEER_V6 route-map TO_BGP_PEER_V6 out',
        '  neighbor PEER_V6_INT soft-reconfiguration inbound',
        '  neighbor PEER_V6_INT allowas-in 1',
        '  neighbor PEER_V6_INT route-map FROM_BGP_PEER_V6 in',
        '  neighbor PEER_V6_INT route-map TO_BGP_PEER_V6 out',
        '  neighbor fc00::2 activate',
        ' exit-address-family',
        'route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 10',
        ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_0_COMMUNITY_1010:1010',
        ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_0_COMMUNITY_1010:1010_V4',
        'route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 30000',
        ' match ip address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_0_COMMUNITY_empty_V4',
        'route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 65535',
        ' set community 5060:12345 additive',
        'route-map ALLOW_LIST_DEPLOYMENT_ID_0_V6 permit 10',
        ' match community COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_0_COMMUNITY_1010:1010',
        ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_0_COMMUNITY_1010:1010_V6',
        'route-map ALLOW_LIST_DEPLOYMENT_ID_0_V6 permit 30000',
        ' match ipv6 address prefix-list PL_ALLOW_LIST_DEPLOYMENT_ID_0_COMMUNITY_empty_V6',
        'route-map ALLOW_LIST_DEPLOYMENT_ID_0_V6 permit 65535',
        ' set community 5060:12345 additive',
        'route-map FROM_BGP_PEER_V4 permit 100',
        'route-map FROM_BGP_PEER_V4 permit 2',
        ' call ALLOW_LIST_DEPLOYMENT_ID_0_V4',
        ' on-match next',
        'route-map FROM_BGP_PEER_V6 permit 1',
        ' set ipv6 next-hop prefer-global ',
        'route-map FROM_BGP_PEER_V6 permit 100',
        'route-map FROM_BGP_PEER_V6 permit 2',
        ' call ALLOW_LIST_DEPLOYMENT_ID_0_V6',
        ' on-match next',
        'route-map FROM_BGP_SPEAKER permit 10',
        'route-map RM_SET_SRC permit 10',
        ' set src 10.1.0.32',
        'route-map RM_SET_SRC6 permit 10',
        ' set src FC00:1::32',
        'route-map TO_BGP_PEER_V4 permit 100',
        'route-map TO_BGP_PEER_V6 permit 100',
        'route-map TO_BGP_SPEAKER deny 1',
    ]
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': global_constants,
    }
    mgr = BGPAllowListMgr(common_objs, "CONFIG_DB", "BGP_ALLOWED_PREFIXES")
    values = mgr._BGPAllowListMgr__find_peer_group(0, '')
    assert set(values) == {'PEER_V4_INT', 'PEER_V6_INT', 'PEER_V6', 'PEER_V4'}

@patch.dict("sys.modules", swsscommon=swsscommon_module_mock)
def test___to_prefix_list():
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': global_constants,
    }
    mgr = BGPAllowListMgr(common_objs, "CONFIG_DB", "BGP_ALLOWED_PREFIXES")

    res_v4 = mgr._BGPAllowListMgr__to_prefix_list(mgr.V4, ["1.2.3.4/32", "10.20.20.10/24"])
    assert res_v4 == ["permit 1.2.3.4/32", "permit 10.20.20.10/24 le 32"]
    res_v6 = mgr._BGPAllowListMgr__to_prefix_list(mgr.V6, ["fc00::1/128", "fc00::/64"])
    assert res_v6 == ["permit fc00::1/128", "permit fc00::/64 le 128"]

@patch.dict("sys.modules", swsscommon=swsscommon_module_mock)
def construct_BGPAllowListMgr(constants):
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': constants,
    }
    mgr = BGPAllowListMgr(common_objs, "CONFIG_DB", "BGP_ALLOWED_PREFIXES")
    return mgr

def test___get_enabled_enabled():
    constants = {
        "bgp": {
            "allow_list": {
                "enabled": True,
            }
        }
    }
    mgr = construct_BGPAllowListMgr(constants)
    assert mgr._BGPAllowListMgr__get_enabled()

def test___get_enabled_disabled_1():
    constants = {
        "bgp": {
            "allow_list": {
                "enabled": False,
            }
        }
    }
    mgr = construct_BGPAllowListMgr(constants)
    assert not mgr._BGPAllowListMgr__get_enabled()

def test___get_enabled_disabled_2():
    constants = {
        "bgp": {
            "allow_list": {}
        }
    }
    mgr = construct_BGPAllowListMgr(constants)
    assert not mgr._BGPAllowListMgr__get_enabled()

def test___get_enabled_disabled_3():
    constants = {
        "bgp": {}
    }
    mgr = construct_BGPAllowListMgr(constants)
    assert not mgr._BGPAllowListMgr__get_enabled()

def test___get_enabled_disabled_4():
    constants = {}
    mgr = construct_BGPAllowListMgr(constants)
    assert not mgr._BGPAllowListMgr__get_enabled()

def test___get_default_action_deny():
    constants = {
        "bgp": {
            "allow_list": {
                "enabled": True,
                "default_action": "deny",
                "drop_community": "123:123"
            }
        }
    }
    data = {}
    mgr = construct_BGPAllowListMgr(constants)
    assert mgr._BGPAllowListMgr__get_default_action_community(data) == "no-export"

def test___get_default_action_permit_1():
    constants = {
        "bgp": {
            "allow_list": {
                "enabled": True,
                "default_action": "permit",
                "drop_community": "123:123"
            }
        }
    }
    data = {}
    mgr = construct_BGPAllowListMgr(constants)
    assert mgr._BGPAllowListMgr__get_default_action_community(data) == "123:123"

def test___get_default_action_permit_2():
    constants = {
        "bgp": {
            "allow_list": {
                "enabled": True,
                "drop_community": "123:123"
            }
        }
    }
    data = {}
    mgr = construct_BGPAllowListMgr(constants)
    assert mgr._BGPAllowListMgr__get_default_action_community(data) == "123:123"

def test___get_default_action_permit_3():
    constants = {
        "bgp": {
            "allow_list": {
                "enabled": False,
                "drop_community": "123:123"
            }
        }
    }
    data = {}
    mgr = construct_BGPAllowListMgr(constants)
    assert mgr._BGPAllowListMgr__get_default_action_community(data) == "123:123"

# FIXME: more testcases for coverage


def allow_list_with_directory(running_default=None):
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    cfg_mgr = MagicMock()
    cfg_mgr.update.return_value = None
    cfg_mgr.changes = ""
    cfg_mgr.get_text.return_value = [] if running_default is None else [
        "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 65535", " set community %s additive" % running_default,
        "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V6 permit 65535", " set community %s additive" % running_default]
    common_objs = {'directory': Directory(), 'cfg_mgr': cfg_mgr, 'tf': TemplateFabric(),
                   'constants': deepcopy(global_constants)}
    return BGPAllowListMgr(common_objs, "CONFIG_DB", "BGP_ALLOWED_PREFIXES"), common_objs['directory'], cfg_mgr


def test_default_action_is_recorded_for_the_peer_render():
    mgr, directory, cfg_mgr = allow_list_with_directory()

    def recorded(route_map_name):
        return directory.get("LOCAL", "allow_list_default_action", route_map_name)

    # FRR holds no rule: nothing is recorded before a row is applied
    assert not directory.path_exist("LOCAL", "allow_list_default_action", "ALLOW_LIST_DEPLOYMENT_ID_0_V4")
    mgr.set_handler("DEPLOYMENT_ID|0", {"prefixes_v4": "10.20.30.0/24", "default_action": "deny"})
    assert recorded("ALLOW_LIST_DEPLOYMENT_ID_0_V4") == "deny"
    assert recorded("ALLOW_LIST_DEPLOYMENT_ID_0_V6") == "deny"
    # other deployments and neighbor-type policies have their own route-maps and records
    mgr.set_handler("DEPLOYMENT_ID|5", {"prefixes_v4": "10.20.30.0/24", "default_action": "permit"})
    mgr.set_handler("DEPLOYMENT_ID|0|NEIGHBOR_TYPE|TOR", {"prefixes_v4": "10.20.30.0/24", "default_action": "permit"})
    assert recorded("ALLOW_LIST_DEPLOYMENT_ID_5_V4") == "permit"
    assert recorded("ALLOW_LIST_DEPLOYMENT_ID_0_NEIGHBOR_TOR_V4") == "permit"
    assert recorded("ALLOW_LIST_DEPLOYMENT_ID_0_V4") == "deny"
    # the policies of one deployment share its route-maps, whose default rule is the last applied
    # policy's (a policy without default_action applies the constants' default; a removed policy puts
    # the constants' default back): existing behaviour of the manager, which the record follows
    mgr.set_handler("DEPLOYMENT_ID|0|1010:2020", {"prefixes_v4": "10.20.30.0/24"})
    assert recorded("ALLOW_LIST_DEPLOYMENT_ID_0_V4") == "permit"
    mgr.set_handler("DEPLOYMENT_ID|0", {"prefixes_v4": "10.20.30.0/24", "default_action": "deny"})
    assert recorded("ALLOW_LIST_DEPLOYMENT_ID_0_V6") == "deny"
    mgr.del_handler("DEPLOYMENT_ID|0")
    assert recorded("ALLOW_LIST_DEPLOYMENT_ID_0_V4") == "permit"
    assert recorded("ALLOW_LIST_DEPLOYMENT_ID_0_V6") == "permit"


def test_default_rule_is_written_again_when_the_pending_changes_already_write_it():
    mgr, directory, cfg_mgr = allow_list_with_directory(running_default="no-export")
    pushed = []
    cfg_mgr.push_list = lambda cmds: pushed.extend(cmds)
    # FRR already holds the policy's action and nothing is pending: the rule is not written
    mgr.set_handler("DEPLOYMENT_ID|0", {"prefixes_v4": "10.20.30.0/24", "default_action": "deny"})
    assert "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 65535" not in pushed
    # a peer render of this pass wrote both rules: the policy writes them again, after it
    cfg_mgr.changes = ("route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 65535\n set community 123:123 additive\n"
                       "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V6 permit 65535\n set community 123:123 additive\n")
    mgr.set_handler("DEPLOYMENT_ID|0", {"prefixes_v4": "10.20.30.0/24", "default_action": "deny"})
    assert pushed[-4:] == ["route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 65535", " set community no-export additive",
                           "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V6 permit 65535", " set community no-export additive"]
    # a pending write that already sets the policy's action is left as the last one
    del pushed[:]
    cfg_mgr.changes = ("route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 65535\n set community no-export additive\n"
                       "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V6 permit 65535\n set community no-export additive\n")
    mgr.set_handler("DEPLOYMENT_ID|0", {"prefixes_v4": "10.20.30.0/24", "default_action": "deny"})
    assert not [line for line in pushed if line.endswith("permit 65535") or line.startswith(" set community")]
    # the running rule differs from the policy's action although a pending write already sets it (a
    # write FRR refused earlier): the rule is written, and so the peer groups are refreshed, as before
    del pushed[:]
    cfg_mgr.get_text.return_value = [
        "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 65535", " set community 123:123 additive",
        "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V6 permit 65535", " set community 123:123 additive"]
    mgr.set_handler("DEPLOYMENT_ID|0", {"prefixes_v4": "10.20.30.0/24", "default_action": "deny"})
    assert [line for line in pushed if line.endswith("permit 65535")] == [
        "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 65535", "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V6 permit 65535"]


def test_record_starts_from_the_rule_frr_holds():
    mgr, directory, cfg_mgr = allow_list_with_directory(running_default="no-export")
    assert directory.get("LOCAL", "allow_list_default_action", "ALLOW_LIST_DEPLOYMENT_ID_0_V4") == "deny"
    assert directory.get("LOCAL", "allow_list_default_action", "ALLOW_LIST_DEPLOYMENT_ID_0_V6") == "deny"


def test_nothing_is_recorded_when_the_families_hold_different_rules():
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    cfg_mgr = MagicMock()
    cfg_mgr.update.return_value = None
    cfg_mgr.changes = ""
    cfg_mgr.get_text.return_value = [
        "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V4 permit 65535", " set community no-export additive",
        "route-map ALLOW_LIST_DEPLOYMENT_ID_0_V6 permit 65535", " set community 123:123 additive"]
    common_objs = {'directory': Directory(), 'cfg_mgr': cfg_mgr, 'tf': TemplateFabric(),
                   'constants': deepcopy(global_constants)}
    BGPAllowListMgr(common_objs, "CONFIG_DB", "BGP_ALLOWED_PREFIXES")
    # the render applies one action to both families, so a rule the families disagree on is not repeated
    assert not common_objs['directory'].path_exist("LOCAL", "allow_list_default_action", "ALLOW_LIST_DEPLOYMENT_ID_0_V4")
