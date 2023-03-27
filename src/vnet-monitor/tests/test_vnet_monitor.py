import pytest
import mock
import vnet_monitor_base
from vnet_monitor_base import *
from mock_modules import *

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
INTERVAL = 1000
MULTIPLIER = 3
VXLAN_PORT = 4789
UDP_PORT = 10000

def test_ip_addr():
    assert(ip_addr('192.168.0.2/32') == '192.168.0.2')
    assert(ip_addr('fc00:1::32/128') == 'fc00:1::32')

class TestPingStatusCache(object):
    """
    Test cases for class PingStatusCache
    """
    @pytest.fixture(autouse=True)
    def setup(self):
        self.cache = PingStatusCache()
        self.state_db_table = MockStateDbTable()
        self.cache.set_state_db_table(self.state_db_table)
    
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
            self.cache.add_entry(t0_loopback, vip)
            # Verify the entry is in local cache
            assert(self.cache.has_entry(t0_loopback, vip))
            # Verify the entry is writen into state_db
            assert(self.state_db_status(t0_loopback, vip) == 'down')
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
            self.cache.add_entry(t0_loopback, vip)
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
            self.cache.add_entry(t0_loopback, vip)
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
                "ping_state": PING_INITIAL # The ping_state is reset to initial after updating sn
            }
            # Verify the cached status is as expected
            assert(self.cache.stats[cache_key] == expected_values)
        finally:
            self.clear_cached_stats()
            self.clear_state_db()

    def test_state_transit(self):
        try:
            t0_loopback = '10.1.0.32'
            vip = '1.1.1.1/32'
            self.cache.add_entry(t0_loopback, vip)
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
                "ping_state": PING_INITIAL
            }
            assert(self.cache.stats[cache_key] == expected_values)
            # The 1st timeout
            seq_num += 1
            self.cache.set_timeout_state(t0_loopback, vip, seq_num, 3)
            expected_values['seq_num'] = seq_num + 1
            expected_values['timeout_count'] += 1
            expected_values['state'] = 'up'
            # Verify the cached status is as expected
            assert(self.cache.stats[cache_key] == expected_values)
            # Verify the entry in state_db is up
            assert(self.state_db_status(t0_loopback, vip) == 'up')
            # The 2nd timeout
            seq_num += 1
            self.cache.set_timeout_state(t0_loopback, vip, seq_num, 3)
            expected_values['seq_num'] = seq_num + 1
            expected_values['timeout_count'] += 1
            # Verify the cached status is as expected
            assert(self.cache.stats[cache_key] == expected_values)
            # Verify the entry in state_db is up
            assert(self.state_db_status(t0_loopback, vip) == 'up')
            # The 3rd timeout
            seq_num += 1
            self.cache.set_timeout_state(t0_loopback, vip, seq_num, 3)
            expected_values['seq_num'] = seq_num + 1
            expected_values['timeout_count'] = 0
            expected_values['state'] = 'down'
            # Verify the cached status is as expected
            assert(self.cache.stats[cache_key] == expected_values)
            # Verify the entry in state_db is down
            assert(self.state_db_status(t0_loopback, vip) == 'down')
            # Now get a up event
            seq_num += 1
            self.cache.set_up_state(t0_loopback, vip, seq_num)
            expected_values['seq_num'] = seq_num + 1
            expected_values['timeout_count'] = 0
            expected_values['state'] = 'up'
            # Verify the cached status is as expected
            assert(self.cache.stats[cache_key] == expected_values)
            # Verify the entry in state_db is up
            assert(self.state_db_status(t0_loopback, vip) == 'up')
        finally:
            self.clear_cached_stats()
            self.clear_state_db()

def create_task_ping(interval=INTERVAL):
    TaskPing.socket_ping = mock.MagicMock()
    task_ping = TaskPing(
                        expected_running_timestamp_ms=monotonic_ms() + random.randint(0, interval),
                        t1_mac=T1_MAC,
                        t1_loopback=T1_LOOPBACK_V4,
                        t0_loopback=T0_LOOPBACK,
                        special_mac=FILTER_MAC,
                        card_vip=VIP_V4,
                        vni=VNI,
                        interval_ms=interval,
                        multiplier=MULTIPLIER,
                        seq_num=0)
    return task_ping

def create_task_timeout():
    task_timeout = TaskTimeout(monotonic_ms(), T0_LOOPBACK, VIP_V4, 0, MULTIPLIER)
    return task_timeout

class TestTaskPing(object):
    """
    Test cases to verify Ping task
    """
    def test_init_method(self):
        with mock.patch('socket.socket') as mock_socket:
            mock_socket.return_value.setsockopt.return_value = 0
            mock_socket.return_value.bind.return_value = 0
            TaskPing.init(T1_LOOPBACK_V4)
            mock_socket.return_value.setsockopt.assert_called_with(socket.SOL_IP, socket.IP_TOS, (DEFAULT_DSCP << 2))
            mock_socket.return_value.bind.assert_called_with((T1_LOOPBACK_V4, 0))
    
    def test_teardown(self):
        with mock.patch('socket.socket') as mock_socket:
            mock_socket.return_value.close.return_value = 0
            TaskPing.init(T1_LOOPBACK_V4)
            TaskPing.teardown()
            assert(TaskPing.socket_ping == None)
            
    def test_do_task(self):
        with mock.patch('socket.socket') as mock_socket:
            task_ping = create_task_ping()
            task_ping.do_task()
            task_ping.socket_ping.sendto.assert_called_with(mock.ANY, (T0_LOOPBACK, VXLAN_PORT))

    def test_next_task(self):
        now = monotonic_ms()
        task_ping = create_task_ping()
        task_ping.next_ping_task()
        assert(task_ping.seq_num == 1)
        assert(task_ping.expected_running_timestamp_ms >= now + INTERVAL)

class TestTaskTimeout(object):
    """
    Test cases to verify TaskTimeout
    """
    def test_do_task(self):
        task_timeout = create_task_timeout()
        with mock.patch('vnet_monitor_base.logger_helper') as mock_logger:
            task_timeout.do_task()
            mock_logger.log_debug.assert_called_with(mock.ANY)

class TestTaskRunner(object):
    """
    Test cases to verify TaskRunner
    """
    def create_task_runner(self):
        mock_cache = mock.MagicMock()
        mock_cache.has_entry.return_value = True
        task_runner = TaskRunner(mock_cache)
        return task_runner
    
    def test_add_task(self):
        task_runner = self.create_task_runner()
        mock_task = mock.MagicMock()
        task_runner.add_task(mock_task)
        assert(task_runner.task_queue.get() == mock_task)
    
    def test_run_task(self):
        try:
            task_runner = self.create_task_runner()
            longer_interval = 2000
            task_ping = create_task_ping(interval=longer_interval)
            task_ping.expected_running_timestamp_ms = monotonic_ms() + longer_interval
            task_runner.add_task(task_ping)
            wait = longer_interval/1000 # From ms to s
            # Start running task in another thread
            task_runner.start()
            time.sleep(wait*0.1)
            # Task shouldn't be run after a short period
            task_ping.socket_ping.sendto.assert_not_called()
            time.sleep(wait)
            # Ping task should run after Interval
            task_ping.socket_ping.sendto.assert_called_with(mock.ANY, (T0_LOOPBACK, VXLAN_PORT))
            task_runner.stop()
            # The next ping should be scheduled
            assert(isinstance(task_runner.task_queue.get(), TaskPing))
            # There should be a pending Timeout Task in the queue
            assert(isinstance(task_runner.task_queue.get(), TaskTimeout))
        finally:
            task_runner.stop()

class TestDBMonitor(object):
    """
    Test cases to verify class DBMonitor
    """
    def create_db_monitor(self):
        task_runner = mock.MagicMock()
        state_cache = mock.MagicMock()
        vnet_monitor_base.swsscommon = mock.MagicMock()
        vnet_monitor_base.daemon_base = mock.MagicMock()
        db_monitor = DBMonitor(
                                table_name="VNET_MONITOR_TABLE",
                                t1_mac=T1_MAC,
                                t1_loopback={4: T1_LOOPBACK_V4, 6: T1_LOOPBACK_V6},
                                vni=DEFAULT_VNI,
                                task_runner=task_runner,
                                cached_stats=state_cache)
        return db_monitor
    
    def test_process_new_entry(self):
        db_monitor = self.create_db_monitor()
        # IPV4
        key = T0_LOOPBACK + ":" + VIP_V4
        fvp = {
            "interval": INTERVAL,
            "multiplier": MULTIPLIER,
            "overlay_dmac": FILTER_MAC
            }
        db_monitor.process_new_entry(key, "SET", fvp)
        db_monitor.cached_stats.add_entry.assert_called_with(T0_LOOPBACK, VIP_V4)
        called_args = db_monitor.task_runner.add_task.call_args[0][0]
        assert(called_args.t1_mac == T1_MAC)
        assert(called_args.t1_loopback == T1_LOOPBACK_V4)
        assert(called_args.t0_loopback == T0_LOOPBACK)
        assert(called_args.special_mac == FILTER_MAC)
        assert(called_args.card_vip == VIP_V4)
        assert(called_args.vni == VNI)
        assert(called_args.interval_ms == INTERVAL)
        assert(called_args.seq_num == 0)
        # Remove entry test
        db_monitor.process_new_entry(key, "DEL", fvp)
        db_monitor.cached_stats.remove_entry.assert_called_with(T0_LOOPBACK, VIP_V4)
        # IPV6
        key = T0_LOOPBACK + ":" + VIP_V6
        db_monitor.process_new_entry(key, "SET", fvp)
        db_monitor.cached_stats.add_entry.assert_called_with(T0_LOOPBACK, VIP_V6)
        called_args = db_monitor.task_runner.add_task.call_args[0][0]
        assert(called_args.t1_mac == T1_MAC)
        assert(called_args.t1_loopback == T1_LOOPBACK_V6)
        assert(called_args.t0_loopback == T0_LOOPBACK)
        assert(called_args.special_mac == FILTER_MAC)
        assert(called_args.card_vip == VIP_V6)
        assert(called_args.vni == VNI)
        assert(called_args.interval_ms == INTERVAL)
        assert(called_args.seq_num == 0)
        # Remove entry test
        db_monitor.process_new_entry(key, "DEL", fvp)
        db_monitor.cached_stats.remove_entry.assert_called_with(T0_LOOPBACK, VIP_V6)

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
            reply_monitor = self.create_reply_monitor()
            reply_monitor._wait_intf_up_msg = mock.MagicMock()
            reply_monitor._wait_intf_up_msg.side_effect = lambda : time.sleep(1)
            reply_monitor._get_oper_up_iface_list = mock.MagicMock()
            reply_monitor._get_oper_up_iface_list.return_value = ["Ethernet0", "Ethernet4"]
            # Start sniffing
            reply_monitor.start()
            assert(reply_monitor.sniffing_iface == ["Ethernet0", "Ethernet4"])
            assert(reply_monitor.sniffer.start.called_with())
            # One more port is up
            reply_monitor._get_oper_up_iface_list.return_value = ["Ethernet0", "Ethernet4", "Ethernet8"]
            time.sleep(3)
            # Verify sniffer is restarted
            assert(reply_monitor.sniffing_iface == ["Ethernet0", "Ethernet4", "Ethernet8"])
            assert(reply_monitor.sniffer.start.called_with())
        finally:
            reply_monitor.stop()

