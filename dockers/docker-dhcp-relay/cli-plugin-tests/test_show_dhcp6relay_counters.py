import sys
import os
from unittest import mock
sys.path.append('../cli/show/plugins/')
import show_dhcp_relay as show

from click.testing import CliRunner

try:
    modules_path = os.path.join(os.path.dirname(__file__), "../../../src/sonic-utilities")
    test_path = os.path.join(modules_path, "tests")
    mock_table_path = os.path.join(test_path, "mock_tables")
    sys.path.insert(0, modules_path)
    sys.path.insert(0, test_path)
    sys.path.insert(0, mock_table_path)
    import dbconnector
except KeyError:
    pass

expected_counts_v6 = """\
  Message Type     Vlan1000(RX)
--------------  ---------------

  Message Type     Vlan1000(TX)
--------------  ---------------

"""

expected_counts_v4 = """\
  Message Type     Vlan1000(RX)
--------------  ---------------

  Message Type     Vlan1000(TX)
--------------  ---------------

"""

class TestDhcp6RelayCounters(object):

    def test_show_counts(self):           
        runner = CliRunner()
        result = runner.invoke(show.dhcp6relay_counters.commands["counts"], ["-i Vlan1000"])
        print(result.output)
        assert result.output == expected_counts_v6

class TestDhcpRelayCounters(object):

    def test_show_counts(self):           
        runner = CliRunner()
        result = runner.invoke(show.dhcp4relay_counters.commands["counts"], ["-i Vlan1000"])
        print(result.output)
        assert result.output == expected_counts_v4

