import copy
import re
from unittest.mock import MagicMock, NonCallableMagicMock, patch

swsscommon_module_mock = MagicMock(ConfigDBConnector = NonCallableMagicMock)
# because can’t use dotted names directly in a call, have to create a dictionary and unpack it using **:
mockmapping = {'swsscommon.swsscommon': swsscommon_module_mock}

@patch.dict('sys.modules', **mockmapping)
def test_contructor():
    from vrrpcfgd.vrrpcfgd import VRRPConfigDaemon
    daemon = VRRPConfigDaemon()
    daemon.start()
    for table, hdlr in daemon.table_handler_list:
        daemon.config_db.subscribe.assert_any_call(table, hdlr)
    daemon.config_db.pubsub.psubscribe.assert_called_once()
    assert(daemon.config_db.sub_thread.is_alive() == True)
    daemon.stop()
    daemon.config_db.pubsub.punsubscribe.assert_called_once()
    assert(daemon.config_db.sub_thread.is_alive() == False)

class CmdMapTestInfo:
    data_buf = {}
    def __init__(self, table, key, data, exp_cmd, no_del = False, neg_cmd = None,
                 chk_data = None, daemons = None, ignore_tail = False):
        self.table_name = table
        self.key = key
        self.data = data
        self.vtysh_cmd = exp_cmd
        self.no_del = no_del
        self.vtysh_neg_cmd = neg_cmd
        self.chk_data = chk_data
        self.daemons = daemons
        self.ignore_tail = ignore_tail
    @classmethod
    def add_test_data(cls, test):
        assert(isinstance(test.data, dict))
        cls.data_buf.setdefault(
                test.table_name, {}).setdefault(test.key, {}).update(test.data)
    @classmethod
    def set_default_test_data(cls, test):
        assert(test.table_name in cls.data_buf and
               test.key in cls.data_buf[test.table_name])
        cache_data = cls.data_buf[test.table_name][test.key]
        assert(isinstance(test.data, dict))
        for k, v in test.data.items():
            assert(k in cache_data and cache_data[k] == v)
            if k == 'preempt':
                test.data[k] = 'disabled'
            if k == 'admin_status':
                test.data[k] = 'up'
        cls.data_buf.setdefault(
                test.table_name, {}).setdefault(test.key, {}).update(test.data)

    @classmethod
    def get_test_data(cls, test):
        assert(test.table_name in cls.data_buf and
               test.key in cls.data_buf[test.table_name])
        return copy.deepcopy(cls.data_buf[test.table_name][test.key])
    @staticmethod
    def compose_vtysh_cmd(cmd_list, negtive = False):
        cmdline = 'vtysh'
        for cmd in cmd_list:
            cmd = cmd.format('no ' if negtive else '')
            cmdline += " -c '%s'" % cmd
        return cmdline
    def check_running_cmd(self, mock, is_del):
        if is_del:
            vtysh_cmd = self.vtysh_cmd if self.vtysh_neg_cmd is None else self.vtysh_neg_cmd
        else:
            vtysh_cmd = self.vtysh_cmd
        if callable(vtysh_cmd):
            cmds = []
            for call in mock.call_args_list:
                assert(call[0][0] == self.table_name)
                cmds.append(call[0][1])
            vtysh_cmd(is_del, cmds, self.chk_data)
        else:
            if self.ignore_tail is False:
                mock.assert_called_with(self.table_name, self.compose_vtysh_cmd(vtysh_cmd, is_del),
                                        self.daemons)
            else:
                mock.assert_called_with(self.table_name, self.compose_vtysh_cmd(vtysh_cmd, is_del),
                                        self.daemons, self.ignore_tail)

conf_cmd = 'configure terminal'
conf_intf_cmd = lambda intf_name: [conf_cmd, 'interface %s' % intf_name]

vrrp_config_data = [
    CmdMapTestInfo('VRRP', 'Ethernet8|8', {'vid': '8'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp 8'], True),
    CmdMapTestInfo('VRRP6', 'Ethernet8|8', {'vid': '8'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp6 8'], True),
    CmdMapTestInfo('VRRP', 'Ethernet1|8', {'vip': ['7.7.7.7/24']},
                       conf_intf_cmd('Ethernet1') + ['{}vrrp 8 ip 7.7.7.7'], True),
    CmdMapTestInfo('VRRP', 'Ethernet8|8', {'admin_status': 'down'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp 8 shutdown']),
    CmdMapTestInfo('VRRP6', 'Ethernet8|8', {'admin_status': 'down'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp6 8 shutdown']),
    CmdMapTestInfo('VRRP6', 'Ethernet8|8', {'vip': ['2000::1/64']},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp6 8 ipv6 2000::1'], True),
    CmdMapTestInfo('VRRP_TRACK', 'Ethernet8|8|Ethernet5', {'priority_increment': '20'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp 8 track-interface Ethernet5 priority-dec 20'], True),
    CmdMapTestInfo('VRRP6_TRACK', 'Ethernet8|8|Ethernet5', {'priority_increment': '20'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp6 8 track-interface Ethernet5 priority-dec 20'], True),
    CmdMapTestInfo('VRRP', 'Ethernet8|8', {'preempt': 'enabled'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp 8 preempt']),
    CmdMapTestInfo('VRRP6', 'Ethernet8|8', {'preempt': 'enabled'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp6 8 preempt']),
    CmdMapTestInfo('VRRP', 'Ethernet8|8', {'version': '2'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp 8 version 2'], True),
    CmdMapTestInfo('VRRP', 'Ethernet8|8', {'adv_interval': '2000'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp 8 advertisement-interval 2000'], True),
    CmdMapTestInfo('VRRP6', 'Ethernet8|8', {'adv_interval': '2000'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp6 8 advertisement-interval 2000'], True),
    CmdMapTestInfo('VRRP', 'Ethernet8|8', {'priority': '150'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp 8 priority 150'], True),
    CmdMapTestInfo('VRRP6', 'Ethernet8|8', {'priority': '150'},
                       conf_intf_cmd('Ethernet8') + ['{}vrrp6 8 priority 150'], True),
]

@patch.dict('sys.modules', **mockmapping)
@patch('vrrpcfgd.vrrpcfgd.g_run_command')
def data_set_del_test(test_data, run_cmd):
    from vrrpcfgd.vrrpcfgd import VRRPConfigDaemon
    daemon = VRRPConfigDaemon()
    data_buf = {}
    # add data in list
    for test in test_data:
        run_cmd.reset_mock()
        hdlr = [h for t, h in daemon.table_handler_list if t == test.table_name]
        assert(len(hdlr) == 1)
        CmdMapTestInfo.add_test_data(test)
        hdlr[0](test.table_name, test.key, CmdMapTestInfo.get_test_data(test))
        test.check_running_cmd(run_cmd, False)
    # delete data in reverse direction
    for test in reversed(test_data):
        if test.no_del:
            continue
        run_cmd.reset_mock()
        hdlr = [h for t, h in daemon.table_handler_list if t == test.table_name]
        assert(len(hdlr) == 1)
        CmdMapTestInfo.set_default_test_data(test)
        hdlr[0](test.table_name, test.key, CmdMapTestInfo.get_test_data(test))
        test.check_running_cmd(run_cmd, True)

def test_vrrp_config():
    data_set_del_test(vrrp_config_data)
