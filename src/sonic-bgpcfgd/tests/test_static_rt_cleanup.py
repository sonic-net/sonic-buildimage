import unittest
from unittest.mock import MagicMock, patch, call

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_static_rt import StaticRouteMgr, IpNextHopSet, IpNextHop
from bgpcfgd.main import cleanup_static_routes_on_exit
from swsscommon import swsscommon
import socket


class TestStaticRouteCleanup(unittest.TestCase):
    
    def setUp(self):
        """Set up test fixtures before each test method."""
        self.cfg_mgr = MagicMock()
        self.directory = Directory()
        
        self.common_objs = {
            "directory": self.directory,
            "cfg_mgr": self.cfg_mgr,
            "tf": TemplateFabric(),
            "constants": {},
        }
        
        # Create StaticRouteMgr instances for testing
        self.config_mgr = StaticRouteMgr(self.common_objs, "CONFIG_DB", "STATIC_ROUTE")
        
        # Setup BGP ASN for redistribution tests
        self.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost", {"bgp_asn": "65100"})
    
    def test_cleanup_on_exit_no_routes(self):
        """Test cleanup_on_exit when no routes are cached."""
        # Clear any routes
        self.config_mgr.static_routes = {}
        
        # Call cleanup_on_exit
        self.config_mgr.cleanup_on_exit()
        
        self.cfg_mgr.push_list.assert_not_called()
        self.cfg_mgr.commit.assert_not_called()


if __name__ == "__main__":
    unittest.main()

    def test_cleanup_on_exit_with_ipv4_routes(self):
        """Test cleanup_on_exit with IPv4 static routes."""
        # Setup mock static routes
        nh1 = IpNextHop(socket.AF_INET, "false", "192.168.1.1", "", 0, "")
        nh2 = IpNextHop(socket.AF_INET, "false", "192.168.1.2", "", 0, "")
        nh_set = IpNextHopSet(False)
        nh_set.add(nh1)
        nh_set.add(nh2)
        
        self.config_mgr.static_routes = {
            "default": {
                "10.0.0.0/24": (nh_set, "1"),
                "10.0.1.0/24": (nh_set, "1")
            }
        }
        
        # Mock cfg_mgr.commit to return True (success)
        self.cfg_mgr.commit.return_value = True
        
        # Call cleanup_on_exit
        self.config_mgr.cleanup_on_exit()
        
        self.cfg_mgr.push_list.assert_called_once()
        self.cfg_mgr.commit.assert_called_once()
        
        # Verify cleanup commands were generated
        called_commands = self.cfg_mgr.push_list.call_args[0][0]
        self.assertTrue(len(called_commands) > 0)
        self.assertTrue(any("no ip route" in cmd for cmd in called_commands))

    def test_cleanup_on_exit_commit_failure(self):
        """Test cleanup_on_exit when cfg_mgr.commit fails."""
        # Setup mock static routes
        nh1 = IpNextHop(socket.AF_INET, "false", "192.168.1.1", "", 0, "")
        nh_set = IpNextHopSet(False)
        nh_set.add(nh1)
        
        self.config_mgr.static_routes = {
            "default": {
                "10.0.0.0/24": (nh_set, "1")
            }
        }
        
        # Mock cfg_mgr.commit to return False (failure)
        self.cfg_mgr.commit.return_value = False
        
        with patch("bgpcfgd.managers_static_rt.log_err") as mock_log_err:
            # Call cleanup_on_exit
            self.config_mgr.cleanup_on_exit()
            
            self.cfg_mgr.push_list.assert_called_once()
            self.cfg_mgr.commit.assert_called_once()
            
            # Should log error about commit failure
            mock_log_err.assert_called()
            self.assertTrue(any("Failed to commit" in str(call_args) for call_args in mock_log_err.call_args_list))

    def test_cleanup_on_exit_exception_handling(self):
        """Test cleanup_on_exit exception handling."""
        # Setup invalid data to cause exception
        self.config_mgr.static_routes = {
            "default": {
                "10.0.0.0/24": ("invalid_data", "1")  # Invalid data to cause exception
            }
        }
        
        with patch("bgpcfgd.managers_static_rt.log_err") as mock_log_err:
            # Call cleanup_on_exit
            self.config_mgr.cleanup_on_exit()
            
            mock_log_err.assert_called()
            self.assertTrue(any("Error during cleanup" in str(call_args) for call_args in mock_log_err.call_args_list))


class TestMainCleanupFunction(unittest.TestCase):
    
    def setUp(self):
        """Set up test fixtures for main cleanup function tests."""
        self.cfg_mgr = MagicMock()
        self.manager1 = MagicMock()
        self.manager2 = MagicMock()
        self.static_route_managers = [self.manager1, self.manager2]
    
    def test_cleanup_static_routes_on_exit_success(self):
        """Test cleanup_static_routes_on_exit with successful cleanup."""
        # Mock managers to return successfully (void methods return None)
        self.manager1.cleanup_on_exit.return_value = None
        self.manager2.cleanup_on_exit.return_value = None
        
        with patch("bgpcfgd.main.log_notice") as mock_log_notice:
            # Call the main cleanup function
            cleanup_static_routes_on_exit(self.cfg_mgr, self.static_route_managers)
            
            # Verify all managers were called
            self.manager1.cleanup_on_exit.assert_called_once()
            self.manager2.cleanup_on_exit.assert_called_once()
            
            # Verify logging
            mock_log_notice.assert_called()
            log_calls = [str(call_args) for call_args in mock_log_notice.call_args_list]
            self.assertTrue(any("Cleaning up static routes" in call for call in log_calls))
            self.assertTrue(any("delegated to individual managers" in call for call in log_calls))

    def test_cleanup_static_routes_on_exit_exception(self):
        """Test cleanup_static_routes_on_exit with exception handling."""
        # Mock one manager to raise an exception
        self.manager1.cleanup_on_exit.side_effect = Exception("Test exception")
        self.manager2.cleanup_on_exit.return_value = None
        
        with patch("bgpcfgd.main.log_err") as mock_log_err, \
             patch("bgpcfgd.main.log_notice") as mock_log_notice:
            
            # Call the main cleanup function
            cleanup_static_routes_on_exit(self.cfg_mgr, self.static_route_managers)
            
            # Verify error was logged
            mock_log_err.assert_called()
            self.assertTrue(any("Error during static route cleanup" in str(call_args) for call_args in mock_log_err.call_args_list))
