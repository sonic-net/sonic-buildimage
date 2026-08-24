import os
import sys
import threading
import pytest
import threading
import time
from unittest.mock import MagicMock, patch, call
from sonic_py_common.general import load_module_from_source

# Mock the docker module before importing
sys.modules['docker'] = MagicMock()
sys.modules['docker.APIClient'] = MagicMock()
sys.modules['sonic_py_common.device_info'] = MagicMock()
sys.modules['sonic_py_common.logger'] = MagicMock()

# Load the docker-wait-any script as a module
docker_wait_any = load_module_from_source("docker_wait_any",
        os.path.join(os.path.dirname(os.path.realpath(__file__)), "../ctrmgr/docker-wait-any"))


class TestDockerWaitAny:
    """Test suite for docker-wait-any script."""

    def setup_method(self):
        """Reset global variables before each test."""
        docker_wait_any.g_thread_exit_event.clear()
        docker_wait_any.g_service = []
        docker_wait_any.g_dep_services = []

    @patch('docker_wait_any.APIClient')
    @patch('sys.exit')
    def test_all_containers_running_timeout(self, mock_exit, mock_api_client):
        """Test with args -s swss -d syncd teamd, all containers running, test will not exit."""
        # Setup mock Docker client that blocks on wait (containers keep running)
        mock_docker_client = MagicMock()
        mock_api_client.return_value = mock_docker_client
        
        # Mock docker.wait to block indefinitely (simulate containers running)
        wait_event = threading.Event()
        mock_docker_client.wait.side_effect = lambda _: wait_event.wait()  # Wait indefinitely
        
        with patch('sys.argv', ['docker-wait-any', '-s', 'swss', '-d', 'syncd', 'teamd']):
            # Run main() in a separate thread and abort after 2 seconds
            main_thread = threading.Thread(target=docker_wait_any.main)
            main_thread.daemon = True
            main_thread.start()
            
            # Let it run for 2 seconds then terminate
            time.sleep(2)
        
        # Verify all docker_client.wait() are called for each container
        expected_calls = [call('swss'), call('syncd'), call('teamd')]
        mock_docker_client.wait.assert_has_calls(expected_calls, any_order=True)
        
        # Verify main does not exit normally after timeout
        mock_exit.assert_not_called()
        
        # Forcefully terminate the main thread by setting exit event
        docker_wait_any.g_thread_exit_event.set()
        main_thread.join(timeout=1)

    @patch('docker_wait_any.APIClient')
    @patch('sys.exit')
    def test_swss_exits_main_will_exit(self, mock_exit, mock_api_client):
        """Test with args -s swss -d syncd teamd, all containers running except swss exit after 1 second, main will exit."""
        # Setup mock Docker client
        mock_docker_client = MagicMock()
        mock_api_client.return_value = mock_docker_client
        
        # Mock docker.wait behavior: swss exits after 1 second, others wait indefinitely
        wait_event = threading.Event()
        
        def mock_wait(container):
            if container == 'swss':
                time.sleep(1)  # swss exits after 1 second
                return None
            else:
                wait_event.wait()  # syncd and teamd wait indefinitely
        
        mock_docker_client.wait.side_effect = mock_wait
        
        with patch('sys.argv', ['docker-wait-any', '-s', 'swss', '-d', 'syncd', 'teamd']):
            # Run main() in a separate thread
            main_thread = threading.Thread(target=docker_wait_any.main)
            main_thread.daemon = True
            main_thread.start()
            
            # Wait for main to complete (should exit when swss exits)
            main_thread.join(timeout=3)
        
        # Verify all docker_client.wait() are called for each container
        expected_calls = [call('swss'), call('syncd'), call('teamd')]
        mock_docker_client.wait.assert_has_calls(expected_calls, any_order=True)
        
        # Verify main exits normally when swss exits
        mock_exit.assert_called_once_with(0)

    @patch('docker_wait_any.APIClient')
    @patch('sys.exit')
    def test_empty_args_main_will_exit(self, mock_exit, mock_api_client):
        """Test with args empty, main will exit with return 0."""
        # Setup mock Docker client
        mock_docker_client = MagicMock()
        mock_api_client.return_value = mock_docker_client
        
        with patch('sys.argv', ['docker-wait-any']):
            # Run main() in a separate thread with timeout to prevent hanging
            main_thread = threading.Thread(target=docker_wait_any.main)
            main_thread.daemon = True
            main_thread.start()
            
            # Wait for main to complete (should exit immediately)
            main_thread.join(timeout=2)
        
        # Verify main exits with return code 0 when no containers specified
        mock_exit.assert_called_once_with(0)

    @patch('docker_wait_any.APIClient')
    @patch('docker_wait_any.device_info.is_warm_restart_enabled')
    @patch('docker_wait_any.device_info.is_fast_reboot_enabled')
    @patch('sys.exit')
    def test_teamd_exits_warm_restart_main_will_not_exit(self, mock_exit, mock_fast_reboot, mock_warm_restart, mock_api_client):
        """Test with args -s swss -d syncd teamd, all containers running except teamd exit after 1 second, but warm_restart_enabled, main will not exit."""
        # Setup warm restart enabled
        mock_warm_restart.return_value = True
        mock_fast_reboot.return_value = False
        
        # Setup mock Docker client
        mock_docker_client = MagicMock()
        mock_api_client.return_value = mock_docker_client
        
        # Mock docker.wait behavior: teamd exits after 1 second, others wait indefinitely
        wait_event = threading.Event()
        
        def mock_wait(container):
            if container == 'teamd':
                time.sleep(1)  # teamd exits after 1 second
                return None
            else:
                wait_event.wait()  # swss and syncd wait indefinitely
        
        mock_docker_client.wait.side_effect = mock_wait
        
        with patch('sys.argv', ['docker-wait-any', '-s', 'swss', '-d', 'syncd', 'teamd']):
            # Run main() in a separate thread
            main_thread = threading.Thread(target=docker_wait_any.main)
            main_thread.daemon = True
            main_thread.start()
            
            # Let it run for 3 seconds (teamd should exit but main continues due to warm restart)
            time.sleep(3)
        
        # Verify all docker_client.wait() are called for each container
        expected_calls = [call('swss'), call('syncd'), call('teamd')]
        mock_docker_client.wait.assert_has_calls(expected_calls, any_order=True)
        
        # Verify main does not exit when warm restart is enabled and teamd exits
        mock_exit.assert_not_called()
        
        # Forcefully terminate the main thread by setting exit event
        docker_wait_any.g_thread_exit_event.set()
        main_thread.join(timeout=1)