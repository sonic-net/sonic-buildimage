from swsscommon import swsscommon
import os
import re
import time
import json
import pytest

def test_InvalidNexthop(dvs, testlog):

    dvs.copy_file("/etc/frr/", "bgp/files/invalid_nexthop/bgpd.conf")
    dvs.runcmd("supervisorctl start bgpd")
    dvs.runcmd("ip addr add fc00::1/126 dev Ethernet0")
    dvs.runcmd("config interface startup Ethernet0")

    dvs.servers[0].runcmd("ip addr add fc00::2/126 dev eth0")
    dvs.servers[0].runcmd("ifconfig eth0 up")

    time.sleep(5)

    print(dvs.runcmd("supervisorctl status"))

    p = dvs.servers[0].runcmd_async("exabgp -d bgp/files/invalid_nexthop/invalid_nexthop.conf")

    time.sleep(10)

    (exit_code, output) = dvs.runcmd(["vtysh", "-c", "show bgp ipv6"])

    p.terminate()
    p = p.wait()

    print(exit_code, output)

    assert "3333::/64" in output
