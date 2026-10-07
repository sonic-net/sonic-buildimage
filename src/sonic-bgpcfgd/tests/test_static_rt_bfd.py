import time
from unittest.mock import patch
#from unittest.mock import MagicMock, patch

from staticroutebfd.main import *
from swsscommon import swsscommon

@patch('swsscommon.swsscommon.DBConnector.__init__')
@patch('swsscommon.swsscommon.ProducerStateTable.__init__')
@patch('swsscommon.swsscommon.Table.__init__')
def constructor(mock_db, mock_producer, mock_tbl):
    mock_db.return_value = None
    mock_producer.return_value = None
    mock_tbl.return_value = None

    srt_bfd = StaticRouteBfd()
    return srt_bfd

def set_del_test(dut, hdlr, op, args, e_bfd_dict, e_srt_dict):
    set_del_test.bfd_dict = {}
    set_del_test.srt_dict = {}

    def bfd_app_set(key, data):
        set_del_test.bfd_dict["set_"+key] = data.copy()
    def bfd_app_del(key):
        set_del_test.bfd_dict["del_"+key] = {}
    def srt_app_set(key, data):
        set_del_test.srt_dict["set_"+key] = data.copy()
    def srt_app_del(key):
        set_del_test.srt_dict["del_"+key] = {}

    def compare_dict(r, e):
        if len(r) == 0 and len(e) == 0:
            return True
        if len(r) != len(e):
            return False
        for k in e:
            if k not in r:
                return False
            if type(e[k]) is str:
                r_sort = "".join(sorted([x.strip() for x in r[k].split(',')]))
                e_sort = "".join(sorted([x.strip() for x in e[k].split(',')]))
                if r_sort != e_sort:
                    return False
            if type(e[k]) is dict:
                ret = compare_dict(r[k], e[k])
                if not ret:
                    return False
        return True

    dut.set_bfd_session_into_appl_db = bfd_app_set
    dut.del_bfd_session_from_appl_db = bfd_app_del
    dut.set_static_route_into_appl_db = srt_app_set
    dut.del_static_route_from_appl_db = srt_app_del

    if op == "SET":
        if hdlr == "bfd":
            dut.bfd_state_set_handler(*args)
        if hdlr == "srt":
            dut.static_route_set_handler(*args)
        if hdlr == "intf":
            dut.interface_set_handler(*args)
    elif op == "DEL":
        if hdlr == "bfd":
            dut.bfd_state_del_handler(*args)
        if hdlr == "srt":
            dut.static_route_del_handler(*args)
        if hdlr == "intf":
            dut.interface_del_handler(*args)
    else:
        assert False, "Wrong operation"

    assert compare_dict(set_del_test.bfd_dict, e_bfd_dict)
    assert compare_dict(set_del_test.srt_dict, e_srt_dict)

def intf_setup(dut):
    set_del_test(dut, "intf",
        "SET",
        ("if1|192.168.1.1/24", {}
        ),
        {},
        {}
    )
    set_del_test(dut, "intf",
        "SET",
        ("if2|192.168.2.1/24", {}
        ),
        {},
        {}
    )
    set_del_test(dut, "intf",
        "SET",
        ("if3|192.168.3.1/24", {}
        ),
        {},
        {}
    )
    set_del_test(dut, "intf",
        "SET",
        ("if1|2603:10E2:400:1::1/64",{}
        ),
        {},
        {}
    )
    set_del_test(dut, "intf",
        "SET",
        ("if2|2603:10E2:400:2::1/64",{}
        ),
        {},
        {}
    )
    set_del_test(dut, "intf",
        "SET",
        ("if3|2603:10E2:400:3::1/64",{}
        ),
        {},
        {}
    )

def test_set_del_ipv6():
    dut = constructor()
    intf_setup(dut)

    set_del_test(dut, "srt",
        "SET",
        ("2603:10e2:400::4/128", {
            "bfd": "true",
            "ifname": "if1, if2, if3",
            "nexthop": "2603:10E2:400:1::2,2603:10E2:400:2::2,2603:10e2:400:3::2"
        }),
        {
            "set_default:default:2603:10e2:400:1::2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '2603:10E2:400:1::1'},
            "set_default:default:2603:10e2:400:2::2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '2603:10E2:400:2::1'},
            "set_default:default:2603:10e2:400:3::2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '2603:10E2:400:3::1'}
        },
        {}
    )

    set_del_test(dut, "bfd",
        "SET",
        ("2603:10e2:400:1::2", {
            "state": "Up"
        }),
        {},
        {'set_default:2603:10e2:400::4/128': {'nexthop': '2603:10e2:400:1::2', 'ifname': 'if1', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("2603:10e2:400:2::2", {
            "state": "Up"
        }),
        {},
        {'set_default:2603:10e2:400::4/128': {'nexthop': '2603:10e2:400:1::2,2603:10e2:400:2::2', 'ifname': 'if1,if2', 'nexthop-vrf': 'default,default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("2603:10e2:400:3::2", {
            "state": "Up"
        }),
        {},
        {'set_default:2603:10e2:400::4/128': {'nexthop': '2603:10e2:400:1::2,2603:10e2:400:2::2,2603:10e2:400:3::2', 'ifname': 'if1,if2,if3', 'nexthop-vrf': 'default,default,default', 'expiry': 'false'}}
    )

    set_del_test(dut, "srt",
        "DEL",
        ("2603:10e2:400::4/128", { }),
        {
            "del_default:default:2603:10e2:400:1::2" : {},
            "del_default:default:2603:10e2:400:2::2" : {},
            "del_default:default:2603:10e2:400:3::2" : {}
        },
        {'del_default:2603:10e2:400::4/128': { }}
    )

@patch('staticroutebfd.main.log_err')
def test_invalid_key(mocked_log_err):
    dut = constructor()
    intf_setup(dut)

    set_del_test(dut, "srt",
        "SET",
        ("2.2.2/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
        }),
        {},
        {}
    )
    mocked_log_err.assert_called_with("invalid ip prefix for static route: '2.2.2/24'")

def test_set_del():
    dut = constructor()
    intf_setup(dut)

    #test #1
    set_del_test(dut, "srt",
        "SET",
        ("2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
        }),
        {
            "set_default:default:192.168.1.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.1.1'},
            "set_default:default:192.168.2.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.2.1'},
            "set_default:default:192.168.3.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.3.1'}
        },
        {}
    )

    set_del_test(dut, "bfd",
        "SET",
        ("192.168.1.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.1.2', 'ifname': 'if1', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.2.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2 ', 'ifname': 'if2,if1', 'nexthop-vrf': 'default,default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.3.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2,192.168.3.2 ', 'ifname': 'if2,if1,if3', 'nexthop-vrf': 'default,default,default', 'expiry': 'false'}}
    )

    #test #2
    set_del_test(dut, "srt",
        "SET",
        ("2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2",
            "ifname": "if1, if2",
        }),
        {
            "del_default:default:192.168.3.2" : {}
        },
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2 ', 'ifname': 'if2,if1', 'nexthop-vrf': 'default,default', 'expiry': 'false'}}
    )

    #test #3
    set_del_test(dut, "srt",
        "DEL",
        ("2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2",
            "ifname": "if1, if2",
        }),
        {
            "del_default:default:192.168.1.2" : {},
            "del_default:default:192.168.2.2" : {}
        },
        {'del_default:2.2.2.0/24': {}}
    )

    # test add a non-bfd static route
    set_del_test(dut, "srt",
        "SET",
        ("3.3.3.0/24", {
            "nexthop": "192.168.1.2 , 192.168.2.2",
            "ifname": "if1, if2",
        }),
        {},
        {}
    )

    # test delete a non-bfd static route
    set_del_test(dut, "srt",
        "DEL",
        ("3.3.3.0/24", {}),
        {},
        {}
    )

def test_set_del_vrf():
    dut = constructor()
    intf_setup(dut)

    set_del_test(dut, "srt",
        "SET",
        ("vrfred|2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
            "nexthop-vrf": "testvrf1, , default",
        }),
        {
            "set_testvrf1:default:192.168.1.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.1.1'},
            "set_vrfred:default:192.168.2.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.2.1'},
            "set_default:default:192.168.3.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.3.1'}
        },
        {}
    )

    set_del_test(dut, "bfd",
        "SET",
        ("testvrf1|default|192.168.1.2", {
            "state": "Up"
        }),
        {},
        {'set_vrfred:2.2.2.0/24': {'nexthop': '192.168.1.2', 'ifname': 'if1', 'nexthop-vrf': 'testvrf1', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("vrfred|default|192.168.2.2", {
            "state": "Up"
        }),
        {},
        {'set_vrfred:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2 ', 'ifname': 'if2,if1', 'nexthop-vrf': 'testvrf1,vrfred', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("default|default|192.168.3.2", {
            "state": "Up"
        }),
        {},
        {'set_vrfred:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2,192.168.3.2 ', 'ifname': 'if2,if1,if3', 'nexthop-vrf': 'testvrf1,vrfred,default', 'expiry': 'false'}}
    )

    set_del_test(dut, "srt",
        "SET",
        ("vrfred|2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2",
            "ifname": "if1, if2",
            "nexthop-vrf": "testvrf1,",
        }),
        {
            "del_default:default:192.168.3.2" : {}
        },
        {'set_vrfred:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2 ', 'ifname': 'if2,if1', 'nexthop-vrf': 'vrfred, testvrf1', 'expiry': 'false'}}
    )

    set_del_test(dut, "srt",
        "DEL",
        ("vrfred|2.2.2.0/24", { }),
        {
            "del_testvrf1:default:192.168.1.2" : {},
            "del_vrfred:default:192.168.2.2" : {}
        },
        {'del_vrfred:2.2.2.0/24': {}}
    )

def test_bfd_del():
    dut = constructor()
    intf_setup(dut)

    set_del_test(dut, "srt",
        "SET",
        ("2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
        }),
        {
            "set_default:default:192.168.1.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.1.1'},
            "set_default:default:192.168.2.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.2.1'},
            "set_default:default:192.168.3.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.3.1'}
        },
        {}
    )

    set_del_test(dut, "bfd",
        "SET",
        ("192.168.1.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.1.2', 'ifname': 'if1', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.2.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2 ', 'ifname': 'if2,if1', 'nexthop-vrf': 'default,default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.3.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2,192.168.3.2 ', 'ifname': 'if2,if1,if3', 'nexthop-vrf': 'default,default,default', 'expiry': 'false'}}
    )

    #test bfd state del
    set_del_test(dut, "bfd",
        "DEL",
        ({"192.168.2.2"}),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.1.2,192.168.3.2 ', 'ifname': 'if1,if3', 'nexthop-vrf': 'default,default', 'expiry': 'false'}}
    )

def test_set_2routes():
    dut = constructor()
    intf_setup(dut)

    #test #4
    set_del_test(dut, "srt",
        "SET",
        ("2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
        }),
        {
            "set_default:default:192.168.1.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.1.1'},
            "set_default:default:192.168.2.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.2.1'},
            "set_default:default:192.168.3.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.3.1'}
        },
        {}
    )

    set_del_test(dut, "bfd",
        "SET",
        ("192.168.1.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.1.2', 'ifname': 'if1', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.2.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2 ', 'ifname': 'if2,if1', 'nexthop-vrf': 'default,default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.3.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2,192.168.3.2 ', 'ifname': 'if2,if1,if3', 'nexthop-vrf': 'default,default,default', 'expiry': 'false'}}
    )

    set_del_test(dut, "srt",
        "SET",
        ("3.3.3.0/24", {
            "bfd": "true",
            "nexthop": "192.168.2.2",
            "ifname": "if2",
        }),
        {},
        {'set_default:3.3.3.0/24': {'nexthop': '192.168.2.2', 'ifname': 'if2', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )

    #test #5
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.2.2", {
            "state": "Down"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.3.2,192.168.1.2 ', 'ifname': 'if3,if1', 'nexthop-vrf': 'default,default', 'expiry': 'false'}, 'del_default:3.3.3.0/24': {}}
    )

    #test #6
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.2.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2,192.168.3.2 ', 'ifname': 'if2,if1,if3', 'nexthop-vrf': 'default,default,default', 'expiry': 'false'},
         'set_default:3.3.3.0/24': {'nexthop': '192.168.2.2', 'ifname': 'if2', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )

def test_set_bfd_change_hold():
    dut = constructor()
    intf_setup(dut)

    #test #9 bfd: true -> false
    set_del_test(dut, "srt",
        "SET",
        ("2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
        }),
        {
            "set_default:default:192.168.1.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.1.1'},
            "set_default:default:192.168.2.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.2.1'},
            "set_default:default:192.168.3.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.3.1'}
        },
        {}
    )

    set_del_test(dut, "bfd",
        "SET",
        ("192.168.1.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.1.2', 'ifname': 'if1', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.2.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2 ', 'ifname': 'if2,if1', 'nexthop-vrf': 'default,default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.3.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2,192.168.3.2 ', 'ifname': 'if2,if1,if3', 'nexthop-vrf': 'default,default,default', 'expiry': 'false'}}
    )

    set_del_test(dut, "srt",
        "SET",
        ("2.2.2.0/24", {
            "bfd": "false",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
        }),
        {
            "del_default:default:192.168.1.2" : {},
            "del_default:default:192.168.2.2" : {},
            "del_default:default:192.168.3.2" : {}
        },
        {
         'del_default:2.2.2.0/24': {}
        }
    )
    return

    #test #10 'bfd': false --> true, write original rout first
    set_del_test(dut, "srt",
        "SET",
        ("2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
        }),
        {
            "set_default:default:192.168.1.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.1.1'},
            "set_default:default:192.168.2.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.2.1'},
            "set_default:default:192.168.3.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.3.1'}
        },
        {'set_default:2.2.2.0/24': {'bfd':'false', 'nexthop': '192.168.2.2,192.168.1.2,192.168.3.2 ', 'ifname': 'if2,if1,if3', 'expiry': 'false'}}
    )

    set_del_test(dut, "bfd",
        "SET",
        ("192.168.1.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.1.2', 'ifname': 'if1', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.2.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2 ', 'ifname': 'if2,if1', 'nexthop-vrf': 'default,default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.3.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2,192.168.3.2 ', 'ifname': 'if2,if1,if3', 'nexthop-vrf': 'default,default,default', 'expiry': 'false'}}
    )


def test_set_bfd_change_no_hold():
    dut = constructor()
    intf_setup(dut)

    #setup runtime "bfd"="false" condition``
    set_del_test(dut, "srt",
        "SET",
        ("2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
        }),
        {
            "set_default:default:192.168.1.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.1.1'},
            "set_default:default:192.168.2.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.2.1'},
            "set_default:default:192.168.3.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.3.1'}
        },
        {}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.1.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.1.2', 'ifname': 'if1', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.2.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2 ', 'ifname': 'if2,if1', 'nexthop-vrf': 'default,default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.3.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2,192.168.3.2 ', 'ifname': 'if2,if1,if3', 'nexthop-vrf': 'default,default,default', 'expiry': 'false'}}
    )
    set_del_test(dut, "srt",
        "SET",
        ("3.3.3.0/24", {
            "bfd": "true",
            "nexthop": "192.168.2.2",
            "ifname": "if2",
        }),
        {},
        {'set_default:3.3.3.0/24': {'nexthop': '192.168.2.2', 'ifname': 'if2', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )

    set_del_test(dut, "srt",
        "SET",
        ("2.2.2.0/24", {
            "bfd": "false",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
        }),
        {
            "del_default:default:192.168.1.2" : {},
            "del_default:default:192.168.3.2" : {}
        },
        {
         'del_default:2.2.2.0/24': {}
        }
    )

    #test #10 change 'bfd': false to true, because the bfd session "default:default:192.168.2.2" is up, so add that nexthop right after "bfd" change to "true"
    set_del_test(dut, "srt",
        "SET",
        ("2.2.2.0/24", {
            "bfd": "true",
            "nexthop": "192.168.1.2 , 192.168.2.2, 192.168.3.2",
            "ifname": "if1, if2, if3",
        }),
        {
            "set_default:default:192.168.1.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.1.1'},
            "set_default:default:192.168.3.2" : {'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50', 'multiplier': '3', 'local_addr': '192.168.3.1'}
        },
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2', 'ifname': 'if2', 'nexthop-vrf': 'default', 'expiry': 'false'}}
    )

    set_del_test(dut, "bfd",
        "SET",
        ("192.168.1.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2 ', 'ifname': 'if2,if1', 'nexthop-vrf': 'default,default', 'expiry': 'false'}}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.2.2", {
            "state": "Up"
        }),
        {},
        {}
    )
    set_del_test(dut, "bfd",
        "SET",
        ("192.168.3.2", {
            "state": "Up"
        }),
        {},
        {'set_default:2.2.2.0/24': {'nexthop': '192.168.2.2,192.168.1.2,192.168.3.2 ', 'ifname': 'if2,if1,if3', 'nexthop-vrf': 'default,default,default', 'expiry': 'false'}}
    )


def test_set_del_blackhole_route():
    dut = constructor()
    intf_setup(dut)

    test_set_del_blackhole_route.logs = []

    with patch('staticroutebfd.main.log_info', new=lambda msg: test_set_del_blackhole_route.logs.append(msg)):
        set_del_test(dut, "srt",
            "SET",
            ("2.2.2.0/24", {
                "blackhole": "true",
            }),
            {},
            {}
        )

        set_del_test(dut, "srt",
            "DEL",
            ("2.2.2.0/24", {
                "blackhole": "true",
            }),
            {},
            {}
        )

    assert "Blackholing static route encountered, skipping it" in test_set_del_blackhole_route.logs


def test_set_del_ifname_only_route():
    dut = constructor()
    intf_setup(dut)

    test_set_del_ifname_only_route.logs = []

    with patch('staticroutebfd.main.log_warn', new=lambda msg: test_set_del_ifname_only_route.logs.append(msg)):
        set_del_test(dut, "srt",
            "SET",
            ("2.2.2.0/24", {
                "ifname": "if1,if2",
            }),
            {},
            {}
        )

        set_del_test(dut, "srt",
            "DEL",
            ("2.2.2.0/24", {
                "ifname": "if1,if2",
            }),
            {},
            {}
        )

    assert "Static route bfd set Failed, nexthop, interface and vrf lists do not match or some of them is empty."\
        in test_set_del_ifname_only_route.logs


# ---------------------------------------------------------------------------
# Tests for _resync_bfd_sessions() — swss/bgp start-order race recovery
#
# We mock _get_state_bfd_keys() directly on the dut instance. This is the
# cleanest approach because it avoids swsscommon.Table patching which behaves
# differently between environments where swsscommon is a real C extension vs.
# a MagicMock (swsscommon_test.py).
# ---------------------------------------------------------------------------

def _setup_resync_dut(state_db_keys,
                      nexthop="192.168.1.2,192.168.2.2", ifname="if1,if2",
                      prefix="2.2.2.0/24", is_ipv6=False):
    """
    Build a StaticRouteBfd dut with BFD sessions from a static route,
    mock _get_state_bfd_keys() to return state_db_keys, and return
    (dut, reprogrammed_dict).
    """
    dut = constructor()
    intf_setup(dut)
    expected_bfd = {}
    for i, (nh, _) in enumerate(zip(nexthop.split(","), ifname.split(","))):
        nh = nh.strip()
        local = ("2603:10E2:400:%d::1" % (i + 1)) if is_ipv6 else ("192.168.%d.1" % (i + 1))
        expected_bfd["set_default:default:" + nh] = {
            'multihop': 'false', 'rx_interval': '50', 'tx_interval': '50',
            'multiplier': '3', 'local_addr': local,
        }
    set_del_test(dut, "srt", "SET",
        (prefix, {"bfd": "true", "nexthop": nexthop, "ifname": ifname}),
        expected_bfd, {}
    )
    # Mock _get_state_bfd_keys directly on the instance — no swsscommon.Table patching needed
    dut._get_state_bfd_keys = lambda: list(state_db_keys)
    reprogrammed = {}
    dut.set_bfd_session_into_appl_db = lambda key, data: reprogrammed.update({key: data})
    return dut, reprogrammed


def test_resync_bfd_sessions_missing():
    """
    All static_route BFD sessions missing from STATE_DB (APPL_DB was flushed).
    Should re-write all sessions and return False.
    """
    dut, reprogrammed = _setup_resync_dut([])
    result = dut._resync_bfd_sessions()
    assert result == False
    assert "default:default:192.168.1.2" in reprogrammed
    assert "default:default:192.168.2.2" in reprogrammed


def test_resync_bfd_sessions_all_present():
    """
    All static_route BFD sessions present in STATE_DB.
    Should return True and not re-write anything.
    """
    dut, reprogrammed = _setup_resync_dut([
        "default|default|192.168.1.2",
        "default|default|192.168.2.2",
    ])
    result = dut._resync_bfd_sessions()
    assert result == True
    assert len(reprogrammed) == 0


def test_resync_bfd_sessions_partial():
    """
    One session present in STATE_DB, one missing.
    Should re-write only the missing one and return False.
    """
    dut, reprogrammed = _setup_resync_dut(["default|default|192.168.1.2"])
    result = dut._resync_bfd_sessions()
    assert result == False
    assert "default:default:192.168.1.2" not in reprogrammed
    assert "default:default:192.168.2.2" in reprogrammed


def test_resync_bfd_sessions_ipv6():
    """
    IPv6 peer addresses contain colons. Key conversion must only replace the
    first 2 colons (vrf/intf separators), preserving the IPv6 address intact.
    """
    dut, reprogrammed = _setup_resync_dut(
        [],
        nexthop="2603:10e2:400:1::2,2603:10e2:400:2::2",
        ifname="if1,if2",
        prefix="2603:10e2:400::4/128",
        is_ipv6=True,
    )
    result = dut._resync_bfd_sessions()
    assert result == False
    # IPv6 colons inside the address must be preserved (only first 2 replaced)
    assert "default:default:2603:10e2:400:1::2" in reprogrammed
    assert "default:default:2603:10e2:400:2::2" in reprogrammed


def test_resync_bfd_sessions_empty_local_db():
    """
    No static_route BFD sessions in local_db.
    Should return True immediately without touching STATE_DB.
    """
    dut = constructor()
    called = []
    dut._get_state_bfd_keys = lambda: called.append(1) or []
    result = dut._resync_bfd_sessions()
    assert result == True
    assert len(called) == 0  # STATE_DB not queried


def test_resync_timer_disables_after_full_recovery():
    """
    Once _resync_bfd_sessions() returns True (all sessions in STATE_DB),
    the run() loop sets _next_resync_time=None to disable further checks.
    """
    dut, _ = _setup_resync_dut(["default|default|192.168.1.2"],
                                nexthop="192.168.1.2", ifname="if1")
    dut._next_resync_time = 0.0

    if dut._next_resync_time is not None and time.monotonic() >= dut._next_resync_time:
        if dut._resync_bfd_sessions():
            dut._next_resync_time = None
        else:
            dut._next_resync_time = time.monotonic() + dut.RESYNC_STATE_DB_CHECK_INTERVAL

    assert dut._next_resync_time is None


def test_resync_timer_reschedules_when_sessions_missing():
    """
    When _resync_bfd_sessions() returns False (sessions still missing), the
    run() loop reschedules _next_resync_time for another interval.
    """
    dut, _ = _setup_resync_dut([], nexthop="192.168.1.2", ifname="if1")
    dut._next_resync_time = 0.0

    before = time.monotonic()
    if dut._next_resync_time is not None and time.monotonic() >= dut._next_resync_time:
        if dut._resync_bfd_sessions():
            dut._next_resync_time = None
        else:
            dut._next_resync_time = time.monotonic() + dut.RESYNC_STATE_DB_CHECK_INTERVAL

    assert dut._next_resync_time is not None
    assert dut._next_resync_time >= before + dut.RESYNC_STATE_DB_CHECK_INTERVAL
