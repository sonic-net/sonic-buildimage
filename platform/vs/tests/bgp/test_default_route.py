from swsscommon import swsscommon
import os
import re
import time
import json
import pytest

def test_DefaultRoute(dvs, testlog):

    dvs.copy_file("/etc/frr/", "bgp/files/default_route/bgpd.conf")
    dvs.runcmd("supervisorctl start bgpd")
    dvs.runcmd("ip addr add 10.10.10.0/31 dev Ethernet0")
    dvs.runcmd("config interface startup Ethernet0")
    dvs.runcmd("ip route del 0.0.0.0/0")
    dvs.runcmd("vtysh -c \"confgure terminal\" -c \"ip route 0.0.0.0/0 via 172.17.0.1 200\"")

    dvs.servers[0].runcmd("ip addr add 10.10.10.1/31 dev eth0")
    dvs.servers[0].runcmd("ifconfig eth0 up")

    time.sleep(5)

    print(dvs.runcmd("supervisorctl status"))

    p = dvs.servers[0].runcmd_async("exabgp -d bgp/files/default_route/default_route.conf")

    time.sleep(10)

    (exit_code, output) = dvs.runcmd(["redis-cli", "hgetall", "ROUTE_TABLE:0.0.0.0/0"])
    print(exit_code, output)

    # make sure 10.10.10.1 is the correct next hop for default route
    assert "10.10.10.1" in output

    # insert default route for table default
    dvs.runcmd("ip route add default via 172.17.0.1 table default")

    (exit_code, output) = dvs.runcmd(["redis-cli", "hgetall", "ROUTE_TABLE:0.0.0.0/0"])
    print(exit_code, output)

    time.sleep(10)
    # make sure 10.10.10.1 is still the correct next hop for default route
    assert "10.10.10.1" in output

    p.terminate()
    p = p.wait()



