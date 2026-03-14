from unittest.mock import MagicMock
from bgpcfgd.directory import Directory
from bgpcfgd.managers_rm import RouteMapMgr
from swsscommon import swsscommon


test_rm_constants = {
    "deployment_id_asn_map": {
        "1": 12345,
        "2": 12346,
        "3": 12347,
        "4": 12348,
    }
}

def constructor():
    cfg_mgr = MagicMock()

    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'constants': test_rm_constants,
    }

    mgr = RouteMapMgr(common_objs, "APPL_DB", "BGP_PROFILE_TABLE")
    return mgr

def set_del_test(mgr, op, cfg_args, expected_ret, expected_cmds):
    set_del_test.push_list_called = False
    def push_list(cmds):
        set_del_test.push_list_called = True
        assert cmds in expected_cmds
        return True
    mgr.cfg_mgr.push_list = push_list

    if op == "SET":
        for i in range(0, len(cfg_args)):
            ret = mgr.set_handler(cfg_args[i][0], cfg_args[i][1])
            assert ret == expected_ret
    elif op == "DEL":
        for i in range(0, len(cfg_args)):               
            mgr.del_handler(cfg_args[i])
    else:
        assert False, "Wrong operation"

    if expected_cmds:
        assert set_del_test.push_list_called, "cfg_mgr.push_list wasn't called"
    else:
        assert not set_del_test.push_list_called, "cfg_mgr.push_list was called"

def test_set_del():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        [("FROM_SDN_SLB_ROUTES", {
            "community_id": "1234:1234"
        })],
        True,
        [
            ["route-map FROM_SDN_SLB_ROUTES_RM permit 100",
             " set as-path prepend 12346 12346",
             " set community 1234:1234",
             " set origin incomplete"]
        ]
    )

    set_del_test(
        mgr,
        "DEL",
        ("FROM_SDN_SLB_ROUTES",),
        True,
        [
            ["no route-map FROM_SDN_SLB_ROUTES_RM permit 100"]
        ]
    )

def test_set_del_com_sdn_apl():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        [("FROM_SDN_APPLIANCE_ROUTES", {
            "community_id": "1235:1235"
        })],
        True,
        [
            ["route-map FROM_SDN_APPLIANCE_ROUTES_RM permit 100",
             " set as-path prepend 12346 12346",
             " set community 1235:1235",
             " set origin incomplete"]
        ]
    )

    set_del_test(
        mgr,
        "DEL",
        ("FROM_SDN_APPLIANCE_ROUTES",),
        True,
        [
            ["no route-map FROM_SDN_APPLIANCE_ROUTES_RM permit 100"]
        ]
    )

def test_set_del_com_sdn_apl_and_slb():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        [("FROM_SDN_APPLIANCE_ROUTES", {
            "community_id": "1235:1235"
        }),
        ("FROM_SDN_SLB_ROUTES", {
            "community_id": "1234:1234"
        }
        )],
        True,
        [
            ["route-map FROM_SDN_APPLIANCE_ROUTES_RM permit 100",
             " set as-path prepend 12346 12346",
             " set community 1235:1235",
             " set origin incomplete"],
           ["route-map FROM_SDN_SLB_ROUTES_RM permit 100",
             " set as-path prepend 12346 12346",
             " set community 1234:1234",
             " set origin incomplete"]
                   
        ]
    )

    set_del_test(
        mgr,
        "DEL",
        ("FROM_SDN_APPLIANCE_ROUTES", "FROM_SDN_SLB_ROUTES",),
        True,
        [
            ["no route-map FROM_SDN_APPLIANCE_ROUTES_RM permit 100"],
            ["no route-map FROM_SDN_SLB_ROUTES_RM permit 100"]
        ] 
    )

