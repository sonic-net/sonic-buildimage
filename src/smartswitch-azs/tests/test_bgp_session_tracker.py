import pytest
import mock
import bgp_session_tracker_base
from bgp_session_tracker_base import *
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
        config_db.set_entry("BGP_NEIGHBOR", "10.0.0.1", {"name": "T1_IPV4_Neighbor_1"})
        config_db.set_entry("BGP_NEIGHBOR", "10.0.0.2", {"name": "T1_IPV4_Neighbor_2"})
        route_tbl.set(DEFAULT_ROUTE_KEY, {"nexthop": "10.0.0.1,10.0.0.2"})
        config_db.set_entry("PORTCHANNEL", "Po1031", {"admin_status": "up"})
        config_db.set_entry("PORTCHANNEL", "Po1032", {"admin_status": "up"})
        return config_db, route_tbl
    
    def test_process_default_route_update(self):
        bgp_session_tracker_base.swsscommon = mock.MagicMock()
        bgp_session_tracker_base.daemon_base = mock.MagicMock()
        db_monitor = BgpSessionTracker()
        config_db, route_tbl = self.setup_tables()
        
        #TESTCASE 1: Delete 1 nexthop from the default route. Interface should not be brought down
        route_tbl.set(DEFAULT_ROUTE_KEY, {"nexthop": "10.0.0.1"})
        db_monitor.cache_t1_bgp_sessions(config_db)
        db_monitor.process_default_route_update(DEFAULT_ROUTE_KEY, route_tbl, config_db)
        assert db_monitor.interfaces_down == False
        assert config_db.get_table("PORTCHANNEL")["Po1031"]["admin_status"] == "up"
        assert config_db.get_table("PORTCHANNEL")["Po1032"]["admin_status"] == "up"
        
        #TESTCASE 2: Delete default route. Interface should be brought down
        route_tbl.delete(DEFAULT_ROUTE_KEY)
        db_monitor.process_default_route_update(DEFAULT_ROUTE_KEY, route_tbl, config_db)
        assert db_monitor.interfaces_down == True
        assert config_db.get_table("PORTCHANNEL")["Po1031"]["admin_status"] == "down"
        assert config_db.get_table("PORTCHANNEL")["Po1032"]["admin_status"] == "down"

        #TESTCASE 3: Add 1 nexthop back to the default route. Interface should be brought up
        route_tbl.set(DEFAULT_ROUTE_KEY, {"nexthop": "10.0.0.1"})
        db_monitor.process_default_route_update(DEFAULT_ROUTE_KEY, route_tbl, config_db)
        assert db_monitor.interfaces_down == False
        assert config_db.get_table("PORTCHANNEL")["Po1031"]["admin_status"] == "up"
        assert config_db.get_table("PORTCHANNEL")["Po1032"]["admin_status"] == "up"


        #TESTCASE 4: Nexthop does not point to T1 interfaces. Interface should be brought down
        route_tbl.set(DEFAULT_ROUTE_KEY, {"nexthop": "10.0.0.3"})
        db_monitor.process_default_route_update(DEFAULT_ROUTE_KEY, route_tbl, config_db)
        assert db_monitor.interfaces_down == True
        assert config_db.get_table("PORTCHANNEL")["Po1031"]["admin_status"] == "down"
        assert config_db.get_table("PORTCHANNEL")["Po1032"]["admin_status"] == "down"

        #TESTCASE 5: Nexthop points to T1 interfaces. Interface should be brought up
        route_tbl.set(DEFAULT_ROUTE_KEY, {"nexthop": "10.0.0.1"})
        db_monitor.process_default_route_update(DEFAULT_ROUTE_KEY, route_tbl, config_db)
        assert db_monitor.interfaces_down == False
        assert config_db.get_table("PORTCHANNEL")["Po1031"]["admin_status"] == "up"
        assert config_db.get_table("PORTCHANNEL")["Po1032"]["admin_status"] == "up"