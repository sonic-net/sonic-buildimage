import click
import copy
import pytest
import sys
sys.path.append("../cli/show/plugins/")
import show_dhcp_relay

from unittest.mock import patch, call, MagicMock, ANY
from click.testing import CliRunner

SUPPORTED_DHCPV4_TYPE = [
    "Unknown", "Discover", "Offer", "Request", "Decline", "Ack", "Nak", "Release", "Inform", "Bootp"
]
SUPPORTED_DIR = ["TX", "RX"]
expected_counts_v4 = """\
Packet type Abbr: Un - Unknown, Dis - Discover, Off - Offer, Req - Request,
                  Ack - Acknowledge, Nack - NegativeAcknowledge, Rel - Release,
                  Inf - Inform, Dec - Decline, Mal - Malformed, Drp - Dropped

+-------------+------+-------+-------+-------+-------+--------+-------+-------+-------+-------+-------+
| Vlan (TX)   | Un   | Dis   | Off   | Req   | Ack   | Nack   | Rel   | Inf   | Dec   | Mal   | Drp   |
+=============+======+=======+=======+=======+=======+========+=======+=======+=======+=======+=======+
+-------------+------+-------+-------+-------+-------+--------+-------+-------+-------+-------+-------+
"""
expected_counts_v4_rx = """\
Packet type Abbr: Un - Unknown, Dis - Discover, Off - Offer, Req - Request,
                  Ack - Acknowledge, Nack - NegativeAcknowledge, Rel - Release,
                  Inf - Inform, Dec - Decline, Mal - Malformed, Drp - Dropped

+-------------+------+-------+-------+-------+-------+--------+-------+-------+-------+-------+-------+
| Vlan (RX)   | Un   | Dis   | Off   | Req   | Ack   | Nack   | Rel   | Inf   | Dec   | Mal   | Drp   |
+=============+======+=======+=======+=======+=======+========+=======+=======+=======+=======+=======+
+-------------+------+-------+-------+-------+-------+--------+-------+-------+-------+-------+-------+
"""

expected_counts_v4_type = """\
+-------------------+------+------+
| Vlan (Discover)   | TX   | RX   |
+===================+======+======+
+-------------------+------+------+
"""



def test_plugin_registration():
    cli = MagicMock()
    show_dhcp_relay.register(cli)



class TestDhcpRelayCounters(object):

    def test_show_vlan_counts(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp4relay_counters.commands["vlan-counts"], ["Vlan1000"])
        print(result.output)
        assert result.output == expected_counts_v4

    def test_show_vlan_counts_dir(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp4relay_counters.commands["vlan-counts"], ["Vlan1000", "-d", "RX"])
        print(result.output)
        assert result.output == expected_counts_v4_rx

    def test_show_vlan_counts_type(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp4relay_counters.commands["vlan-counts"], ["Vlan1000", "-t", "Discover"])
        print(result.output)
        assert result.output == expected_counts_v4_type

    def test_show_ipv4_vlan_counters(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp_relay_ipv4.commands["vlan-counters"], ["Vlan1000"])
        print(result.output)
        assert result.output == expected_counts_v4

    def test_show_ipv4_vlan_counters_dir(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp_relay_ipv4.commands["vlan-counters"], ["Vlan1000", "-d", "RX"])
        print(result.output)
        assert result.output == expected_counts_v4_rx

    def test_show_ipv4_vlan_counters_type(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp_relay_ipv4.commands["vlan-counters"], ["Vlan1000", "-t", "Discover"])
        print(result.output)
        assert result.output == expected_counts_v4_type
