import pytest
import dhcp_server.dhcp_server_utils as dhcp_server_utils
from unittest.mock import patch, PropertyMock
from dhcp_server.dhcp_cfggen import DhcpServCfgGenerator


@pytest.fixture(scope="function")
def mock_swsscommon_dbconnector_init():
    with patch.object(dhcp_server_utils.swsscommon.DBConnector, "__init__", return_value=None) as mock_dbconnector_init:
        yield mock_dbconnector_init


@pytest.fixture(scope="function")
def mock_swsscommon_table_init():
    with patch.object(dhcp_server_utils.swsscommon.Table, "__init__", return_value=None) as mock_table_init:
        yield mock_table_init


@pytest.fixture(scope="function")
def mock_get_render_template():
    with patch("dhcp_server.dhcp_cfggen.DhcpServCfgGenerator._get_render_template", return_value=None) as mock_template:
        yield mock_template


@pytest.fixture
def mock_parse_port_map_alias(scope="function"):
    with patch("dhcp_server.dhcp_cfggen.DhcpServCfgGenerator._parse_port_map_alias", return_value=None) as mock_map, \
         patch.object(DhcpServCfgGenerator, "port_alias_map", return_value={"Ethernet24": "etp7", "Ethernet28": "etp8"},
                      new_callable=PropertyMock), \
         patch.object(DhcpServCfgGenerator, "lease_update_script_path", return_value="/etc/kea/lease_update.sh",
                      new_callable=PropertyMock), \
         patch.object(DhcpServCfgGenerator, "lease_path", return_value="/tmp/kea-lease.csv", new_callable=PropertyMock):
        yield mock_map


