import pytest
import mock
import vnet_monitor_base
from vnet_monitor_base import *
from mock_modules import *
import time

"""
Test parameters
"""
T1_MAC = "00:AA:BB:CC:DD:EE"
T1_LOOPBACK_V4 = "20.0.0.1"
T1_LOOPBACK_V6 = "fe80::f0b8:a4ff:fe9b:c964"
T0_LOOPBACK = "10.0.0.1"
FILTER_MAC = "00:00:00:00:00:00"
VIP_V4 = "30.0.0.1/32"
VIP_V6 = "fe80::f0b8:af:fb:c4"
VNI = 8000
INTERVAL = 5000
MULTIPLIER = 3
VXLAN_PORT = 65330
UDP_PORT = 10000
OVERLAY_MAC = "00:AA:BB:CC:DD:EE"

def test_ip_addr():
    assert(ip_addr('192.168.0.2/32') == '192.168.0.2')
    assert(ip_addr('fc00:1::32/128') == 'fc00:1::32')

@pytest.fixture(scope='module', autouse=True)
def global_teardown():
    yield
    # Ignore any exception during quit
    try:
        if g_ping_stats:
            g_ping_stats.teardown()
    except:
        pass

class TestPingStatusCache(object):
    """
    Test cases for class PingStatusCache
    """
    @pytest.fixture(autouse=True)
    def setup(self):
        self.cache = PingStatusCache()
        self.state_db_table = MockStateDbTable()
        self.cache.set_state_db_table(self.state_db_table)
        yield
        self.cache.teardown()
    
    def clear_cached_stats(self):
        self.cache.stats = {}
    
    def clear_state_db(self):
        self.state_db_table.data = {}

    def state_db_status(self, t0_loopback, vip):
        table_key = t0_loopback + "|" + vip
        return self.state_db_table.hget(table_key, "state")[1]
    
    def test_make_key(self):
        assert(self.cache.make_key('192.168.0.2/32', '192.168.0.3') == ('192.168.0.2', '192.168.0.3'))
        assert(self.cache.make_key('fc00:1::32/128', 'fc00:1::33') == ('fc00:1::32', 'fc00:1::33'))

    def test_add_entry(self):
        try:
            t0_loopback = '10.1.0.32'
            vip = '1.1.1.1/32'
            self.cache.add_entry(t0_loopback, vip, INTERVAL, MULTIPLIER, OVERLAY_MAC)
            # Verify the entry is in local cache
            assert(self.cache.has_entry(t0_loopback, vip))
            assert(self.cache.compare_entry(t0_loopback, vip, INTERVAL, MULTIPLIER, OVERLAY_MAC))
            # Verify the entry is writen into state_db
            assert(self.state_db_status(t0_loopback, vip) == 'down')
            # Remove entry
            self.cache.remove_entry(t0_loopback, vip)
            # Verify the entry is removed from state_db
            table_key = t0_loopback + "|" + vip
            assert self.state_db_table.hget(table_key, "state") == (False, None)
        finally:
            self.clear_cached_stats()
            self.clear_state_db()
    
    def test_add_entry_existing_state_db(self):
        """
        This test case is to verift existing entry in state_db is not overwriten
        """
        try:
            t0_loopback = '10.1.0.32'
            vip = '1.1.1.1/32'
            table_key = t0_loopback + "|" + vip
            # Write up status into state_db
            fvs = [('state', 'up')]
            self.state_db_table.set(table_key, fvs)
            self.cache.add_entry(t0_loopback, vip, INTERVAL, MULTIPLIER, OVERLAY_MAC)
            # Verify the entry is in local cache
            assert(self.cache.has_entry(t0_loopback, vip))
            # Verify the entry in state_db remains up
            assert(self.state_db_status(t0_loopback, vip) == 'up')
        finally:
            self.clear_cached_stats()
            self.clear_state_db()

    def test_state_up(self):
        try:
            t0_loopback = '10.1.0.32'
            vip = '1.1.1.1/32'
            self.cache.add_entry(t0_loopback, vip, INTERVAL, MULTIPLIER, OVERLAY_MAC)
            seq_num = 0
            self.cache.set_up_state(t0_loopback, vip, seq_num)
            table_key = t0_loopback + "|" + vip
            # Verify the entry in state_db is up
            assert(self.state_db_status(t0_loopback, vip) == 'up')
            cache_key = self.cache.make_key(t0_loopback, vip)
            expected_values = {
                "t0_lo": t0_loopback,
                "card_vip": vip,
                "seq_num": 1,
                "timeout_count": 0,
                "state": "up",
                "ping_state": PING_REPLIED,
                "interval": INTERVAL,
                "multiplier": MULTIPLIER,
                "overlay_mac": OVERLAY_MAC,
                "last_response": 0
            }
            # Fix the last_response and last_timeout_check time to actual value
            expected_values['last_response'] = self.cache.stats[cache_key]['last_response']
            expected_values['last_timeout_check'] = self.cache.stats[cache_key]['last_timeout_check']
            # Verify the cached status is as expected
            assert(self.cache.stats[cache_key] == expected_values)
        finally:
            self.clear_cached_stats()
            self.clear_state_db()

    def test_state_transit(self):
        try:
            t0_loopback = '10.1.0.32'
            vip = '1.1.1.1/32'
            self.cache.add_entry(t0_loopback, vip, INTERVAL, MULTIPLIER, OVERLAY_MAC)
            seq_num = 0
            # Set the state to up
            self.cache.set_up_state(t0_loopback, vip, seq_num)
            table_key = t0_loopback + "|" + vip
            # Verify the entry in state_db is up
            assert(self.state_db_status(t0_loopback, vip) == 'up')
            cache_key = self.cache.make_key(t0_loopback, vip)
            expected_values = {
                "t0_lo": t0_loopback,
                "card_vip": vip,
                "seq_num": seq_num+1,
                "timeout_count": 0,
                "state": "up",
                "ping_state": PING_REPLIED,
                "interval": INTERVAL,
                "multiplier": MULTIPLIER,
                "overlay_mac": OVERLAY_MAC
            }
            # Fix the last_response and last_timeout_check time to actual value
            expected_values['last_response'] = self.cache.stats[cache_key]['last_response']
            expected_values['last_timeout_check'] = self.cache.stats[cache_key]['last_timeout_check']
            assert(self.cache.stats[cache_key] == expected_values)
            # The 1st timeout -> state should be up
            time.sleep(INTERVAL / 1000)
            # Verify the cached status is as expected
            assert(self.cache.stats[cache_key]['state'] == 'up')
            # Verify the entry in state_db is up
            assert(self.state_db_status(t0_loopback, vip) == 'up')

            # The 2nd timeout -> state should remain up
            time.sleep(INTERVAL / 1000)
            # Verify the cached status is as expected
            assert(self.cache.stats[cache_key]['state'] == 'up')
            # Verify the entry in state_db is up
            assert(self.state_db_status(t0_loopback, vip) == 'up')
            # The 3rd timeout -> state should be down
            time.sleep(INTERVAL / 1000 + 1.5)
            # Verify the cached status is as expected
            assert(self.cache.stats[cache_key]['state'] == 'down')
            # Verify the entry in state_db is down
            assert(self.state_db_status(t0_loopback, vip) == 'down')
            
            # Now get a up event
            seq_num += 1
            self.cache.set_up_state(t0_loopback, vip, seq_num)
            expected_values['seq_num'] = seq_num + 1
            expected_values['timeout_count'] = 0
            expected_values['state'] = 'up'
            # Fix the last_response and last_timeout_check time to actual value
            expected_values['last_response'] = self.cache.stats[cache_key]['last_response']
            expected_values['last_timeout_check'] = self.cache.stats[cache_key]['last_timeout_check']
            # Verify the cached status is as expected
            assert(self.cache.stats[cache_key] == expected_values)
            # Verify the entry in state_db is up
            assert(self.state_db_status(t0_loopback, vip) == 'up')
        finally:
            self.clear_cached_stats()
            self.clear_state_db()

def create_task_ping(interval=INTERVAL):
    g_ping_task_list.append(
                {
                    KEY_T0_LO: T0_LOOPBACK,
                    KEY_CARD_VIP: VIP_V4,
                    KEY_OVERLAY_MAC: OVERLAY_MAC,
                    KEY_VNI: VNI,
                    KEY_INTERVAL: interval,
                    KEY_MULTIPLIER: MULTIPLIER,
                    KEY_SEQ_NUM: 0,
                    KEY_LAST_PING: 0
                }
    )


class TestTaskPing(object):
    """
    Test cases to verify Ping task
    """
    @pytest.fixture(autouse=True)
    def setup(self):
        self.task_ping = TaskPing(g_ping_task_list, T1_MAC, T1_LOOPBACK_V4, T1_LOOPBACK_V6)
        yield
        self.task_ping.teardown()
    
    def test_init_method(self):
        with mock.patch('socket.socket') as mock_socket:
            mock_socket.return_value.setsockopt.return_value = 0
            mock_socket.return_value.bind.return_value = 0
            self.task_ping.init()
            mock_socket.return_value.setsockopt.assert_called_with(socket.SOL_IP, socket.IP_TOS, (DEFAULT_DSCP << 2))
            mock_socket.return_value.bind.assert_called_with((T1_LOOPBACK_V4, 0))
            assert(self.task_ping.socket_ping != None)
            assert(self.task_ping.worker_thread != None)
            
    def test_do_task(self):
        with mock.patch('socket.socket') as mock_socket:
            mock_socket.return_value.sendto.return_value = 0
            self.task_ping.init()
            create_task_ping()
            time.sleep(INTERVAL/1000 + 0.5)
            self.task_ping.socket_ping.sendto.assert_called_with(mock.ANY, (T0_LOOPBACK, VXLAN_PORT))

    def test_teardown(self):
        with mock.patch('socket.socket') as mock_socket:
            mock_socket.return_value.close.return_value = 0
            self.task_ping.init()
            self.task_ping.teardown()
            assert(self.task_ping.socket_ping == None)


class TestDBMonitor(object):
    """
    Test cases to verify class DBMonitor
    """
    def create_db_monitor(self):
        state_cache = mock.MagicMock()
        # Return False to simulate the entry is not in the cache
        state_cache.has_entry.return_value = False
        vnet_monitor_base.swsscommon = mock.MagicMock()
        vnet_monitor_base.daemon_base = mock.MagicMock()
        db_monitor = DBMonitor(
                                table_name="VNET_MONITOR_TABLE",
                                t1_mac=T1_MAC,
                                t1_loopback={4: T1_LOOPBACK_V4, 6: T1_LOOPBACK_V6},
                                vni=DEFAULT_VNI,
                                cached_stats=state_cache,
                                task_list=g_ping_task_list)
        return db_monitor
    
    def test_process_new_entry(self):
        # Clear global ping task list
        g_ping_task_list.clear()
        db_monitor = self.create_db_monitor()
        # IPV4
        key = T0_LOOPBACK + ":" + VIP_V4
        fvp = {
            "interval": INTERVAL,
            "multiplier": MULTIPLIER,
            "overlay_dmac": FILTER_MAC
            }
        db_monitor.process_new_entry(key, "SET", fvp)
        db_monitor.cached_stats.add_entry.assert_called_with(T0_LOOPBACK, VIP_V4, INTERVAL, MULTIPLIER, FILTER_MAC)
        assert(len(g_ping_task_list) == 1)

        # Update existing entry
        TMP_MAC = "AA:BB:CC:DD:EE:FF"
        fvp["overlay_dmac"] = TMP_MAC
        db_monitor.process_new_entry(key, "SET", fvp)
        db_monitor.cached_stats.add_entry.assert_called_with(T0_LOOPBACK, VIP_V4, INTERVAL, MULTIPLIER, TMP_MAC)
        assert(len(g_ping_task_list) == 1)

        # Remove entry test
        db_monitor.process_new_entry(key, "DEL", fvp)
        db_monitor.cached_stats.remove_entry.assert_called_with(T0_LOOPBACK, VIP_V4)
        assert(len(g_ping_task_list) == 0)

        # IPV6
        key = T0_LOOPBACK + ":" + VIP_V6
        fvp["overlay_dmac"] = FILTER_MAC
        db_monitor.process_new_entry(key, "SET", fvp)
        db_monitor.cached_stats.add_entry.assert_called_with(T0_LOOPBACK, VIP_V6, INTERVAL, MULTIPLIER, FILTER_MAC)
        assert(len(g_ping_task_list) == 1)

        # Remove entry test
        db_monitor.process_new_entry(key, "DEL", fvp)
        db_monitor.cached_stats.remove_entry.assert_called_with(T0_LOOPBACK, VIP_V6)
        assert(len(g_ping_task_list) == 0)

class TestReplyMonitor(object):
    """
    Test cases for class ReplyMonitor
    """
    def create_reply_monitor(self):
        reply_monitor = ReplyMonitor(
                                    {4: T1_LOOPBACK_V4, 6: T1_LOOPBACK_V6},
                                    UDP_PORT)
        return reply_monitor
    
    def test_sniff_parameter(self):
        reply_monitor = self.create_reply_monitor()
        assert(reply_monitor.sniffstring == "udp and (dst host " + T1_LOOPBACK_V4 + " or dst host " + T1_LOOPBACK_V6 +  ") and dst port " + str(UDP_PORT))
    
    def test_restart_sniff(self):
        try:
            vnet_monitor_base.AsyncSniffer = mock.MagicMock()
            vnet_monitor_base.AsyncSniffer.return_value = mock.MagicMock()
            # Mock DBConnector
            vnet_monitor_base.swsscommon.DBConnector = mock.MagicMock()
            vnet_monitor_base.swsscommon.DBConnector.return_value = mock.MagicMock()
            # Mock Table
            vnet_monitor_base.swsscommon.SubscriberStateTable = mock.MagicMock()
            vnet_monitor_base.swsscommon.SubscriberStateTable.return_value = mock.MagicMock()
            vnet_monitor_base.swsscommon.SubscriberStateTable.return_value.pop = mock.MagicMock()
            vnet_monitor_base.swsscommon.SubscriberStateTable.return_value.pop.return_value = ("Ethernet8","", {"oper_status": "up"})
            # Mock Select
            vnet_monitor_base.swsscommon.Select = mock.MagicMock()
            vnet_monitor_base.swsscommon.Select.return_value = mock.MagicMock()
            # Mock Select.select
            vnet_monitor_base.swsscommon.Select.return_value.select = mock.MagicMock()
            vnet_monitor_base.swsscommon.Select.return_value.select.return_value = (swsscommon.Select.TIMEOUT, 0)

            reply_monitor = self.create_reply_monitor()
            reply_monitor._wait_intf_up_msg = mock.MagicMock()
            reply_monitor._wait_intf_up_msg.side_effect = lambda : time.sleep(1)
            reply_monitor._get_oper_up_iface_list = mock.MagicMock()
            reply_monitor._get_oper_up_iface_list.return_value = ["Ethernet0", "Ethernet4"]
            # Start sniffing
            reply_monitor.start()
            assert(reply_monitor.sniffing_iface == ["Ethernet0", "Ethernet4"])
            reply_monitor.sniffer.start.assert_called_with()
            # One more port is up
            reply_monitor._get_oper_up_iface_list.return_value = ["Ethernet0", "Ethernet4", "Ethernet8"]
            time.sleep(3)
            # Verify sniffer is not restarted (pending on ports being stable)
            assert(reply_monitor.sniffing_iface == ["Ethernet0", "Ethernet4"])
            time.sleep(10)
            # Verify sniffer is restarted as ports have been stable for a while
            assert(reply_monitor.sniffing_iface == ["Ethernet0", "Ethernet4", "Ethernet8"])
            reply_monitor.sniffer.start.assert_called_with()
        finally:
            reply_monitor.stop()

