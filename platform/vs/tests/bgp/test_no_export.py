from swsscommon import swsscommon
import os
import re
import time
import json

def test_bounce(dvs, testlog):
    dvs.copy_file("/etc/frr/", "bgp/files/no_export/bgpd.conf")
    dvs.runcmd("supervisorctl start bgpd")
    dvs.runcmd("ip addr add 10.0.0.0/31 dev Ethernet0")
    dvs.runcmd("config interface startup Ethernet0")

    dvs.runcmd("ip addr add 10.0.0.2/31 dev Ethernet4")
    dvs.runcmd("config interface startup Ethernet4")

    dvs.servers[0].runcmd("ip addr add 10.0.0.1/31 dev eth0")
    dvs.servers[0].runcmd("ifconfig eth0 up")

    dvs.servers[1].runcmd("ip addr add 10.0.0.3/31 dev eth0")
    dvs.servers[1].runcmd("ifconfig eth0 up")

    time.sleep(5)

    p1 = dvs.servers[0].runcmd_async("exabgp -d bgp/files/no_export/exabgp1.conf")
    p2 = dvs.servers[1].runcmd_async("exabgp -d bgp/files/no_export/exabgp2.conf")

    time.sleep(60)

    (exit_code, sum_res) =  dvs.runcmd(["vtysh", "-c", "show ip bgp sum"])
    (exit_code, all_route) = dvs.runcmd(["vtysh", "-c", "show ip bgp"])
    (exit_code, announce_route) = dvs.runcmd(["vtysh", "-c", "show ip bgp neighbors 10.0.0.3 advertised-routes"])

    p1.terminate()
    p1 = p1.wait()

    p2.terminate()
    p2 = p2.wait()

    print(sum_res)
    print(announce_route)
    assert "1.1.1.1/32" in all_route
    assert "1.1.1.1/32" not in announce_route
    assert "2.2.2.2/32" in all_route
    assert "2.2.2.2/32" in announce_route
