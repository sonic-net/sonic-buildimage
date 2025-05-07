import pytest
import mock
import bgp_session_tracker
from bgp_session_tracker import *
from mock_modules import *

"""
Test parameters
"""
DEFAULT_ROUTE_KEY = "0.0.0.0/0"

class TestBgpSessionTracker(object):
    """
    Test cases to verify BGP session tracker
    """
    def setup_tables(self):
        """
        Set up the tables for the test cases
        """
        config_db = MockConfigDb()
        route_tbl = MockRouteTable()
        lag_table = MockStateLagTable()
        state_db_tbl = MockStateDbTable()
        config_db.set_entry("BGP_NEIGHBOR", "10.0.0.1", {"name": "T1_IPV4_Neighbor_1"})
        config_db.set_entry("BGP_NEIGHBOR", "10.0.0.2", {"name": "T1_IPV4_Neighbor_2"})
        route_tbl.set(DEFAULT_ROUTE_KEY, {"nexthop": "10.0.0.1,10.0.0.2"})
        config_db.set_entry("PORTCHANNEL", "PortChannel1031", {"admin_status": "up"})
        config_db.set_entry("PORTCHANNEL", "PortChannel1032", {"admin_status": "up"})
        return config_db, route_tbl, lag_table, state_db_tbl
    
    def test_process_default_route_update(self):
        bgp_session_tracker.swsscommon = mock.MagicMock()
        bgp_session_tracker.daemon_base = mock.MagicMock()
        db_monitor = BgpSessionTracker()
        config_db, route_tbl, lag_table, state_db_tbl = self.setup_tables()
        db_monitor.state_db_tbl = state_db_tbl
        db_monitor.tbl = route_tbl
        db_monitor.config_db = config_db
        db_monitor.lag_tbl = lag_table
        db_monitor.route_table = route_tbl
        state_db_tbl.set("T1toWL", {})

        #TESTCASE 1: Delete 1 nexthop from the default route. Interface should not be brought down
        lag_table.set("PortChannel1031", {"admin_status": "up"})
        lag_table.set("PortChannel1032", {"admin_status": "up"})
        route_tbl.set(DEFAULT_ROUTE_KEY, {"nexthop": "10.0.0.1"})
        db_monitor.cache_t1_bgp_sessions()
        db_monitor.process_default_route_update()
        assert db_monitor.interfaces_down == False
        assert state_db_tbl.get("T1toWL") == {"interfaces_are_shutdown": "no"}
        assert config_db.get_table("PORTCHANNEL")["PortChannel1031"]["admin_status"] == "up"
        assert config_db.get_table("PORTCHANNEL")["PortChannel1032"]["admin_status"] == "up"
        
        #TESTCASE 2: Delete default route. Interface should be brought down
        lag_table.set("PortChannel1031", {"admin_status": "down"})
        lag_table.set("PortChannel1032", {"admin_status": "down"})
        route_tbl.delete(DEFAULT_ROUTE_KEY)
        db_monitor.process_default_route_update()
        assert db_monitor.interfaces_down == True
        assert state_db_tbl.get("T1toWL") == {"bgp_sessions_up": "no", "interfaces_are_shutdown": "yes"}
        assert config_db.get_table("PORTCHANNEL")["PortChannel1031"]["admin_status"] == "down"
        assert config_db.get_table("PORTCHANNEL")["PortChannel1032"]["admin_status"] == "down"

        #TESTCASE 3: Add 1 nexthop back to the default route. Interface should be brought up
        lag_table.set("PortChannel1031", {"admin_status": "up"})
        lag_table.set("PortChannel1032", {"admin_status": "up"})
        route_tbl.set(DEFAULT_ROUTE_KEY, {"nexthop": "10.0.0.1"})
        db_monitor.process_default_route_update()
        assert db_monitor.interfaces_down == False
        assert state_db_tbl.get("T1toWL") == {"bgp_sessions_up": "yes", "interfaces_are_shutdown": "no"}
        assert config_db.get_table("PORTCHANNEL")["PortChannel1031"]["admin_status"] == "up"
        assert config_db.get_table("PORTCHANNEL")["PortChannel1032"]["admin_status"] == "up"

        #TESTCASE 4: Nexthop does not point to T1 interfaces. Interface should be brought down
        lag_table.set("PortChannel1031", {"admin_status": "down"})
        lag_table.set("PortChannel1032", {"admin_status": "down"})
        route_tbl.set(DEFAULT_ROUTE_KEY, {"nexthop": "10.0.0.3"})
        db_monitor.process_default_route_update()
        assert db_monitor.interfaces_down == True
        assert state_db_tbl.get("T1toWL") == {"bgp_sessions_up": "no", "interfaces_are_shutdown": "yes"}
        assert config_db.get_table("PORTCHANNEL")["PortChannel1031"]["admin_status"] == "down"
        assert config_db.get_table("PORTCHANNEL")["PortChannel1032"]["admin_status"] == "down"

        #TESTCASE 5: Nexthop points to T1 interfaces. Interface should be brought up
        lag_table.set("PortChannel1031", {"admin_status": "up"})
        lag_table.set("PortChannel1032", {"admin_status": "up"})
        route_tbl.set(DEFAULT_ROUTE_KEY, {"nexthop": "10.0.0.1"})
        db_monitor.process_default_route_update()
        assert db_monitor.interfaces_down == False
        assert state_db_tbl.get("T1toWL") == {"bgp_sessions_up": "yes", "interfaces_are_shutdown": "no"}
        assert config_db.get_table("PORTCHANNEL")["PortChannel1031"]["admin_status"] == "up"
        assert config_db.get_table("PORTCHANNEL")["PortChannel1032"]["admin_status"] == "up"