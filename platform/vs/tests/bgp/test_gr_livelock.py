from swsscommon import swsscommon
import os
import re
import time
import json
import random

def output(dvs, title):
    print("=========================== %s ===========================" % title)
    exit_cod,  sum_res =  dvs.runcmd(["vtysh", "-c", "show ip bgp sum"])
    exit_code, all_route = dvs.runcmd(["vtysh", "-c", "show ip bgp"])
    exit_code, neig_1           = dvs.runcmd(["vtysh", "-c", "show ip bgp neighbors 10.0.0.1"])
    exit_code, announce_route_1 = dvs.runcmd(["vtysh", "-c", "show ip bgp neighbors 10.0.0.1 advertised-routes"])
    exit_code, received_route_1 = dvs.runcmd(["vtysh", "-c", "show ip bgp neighbors 10.0.0.1 routes"])
    exit_code, announce_route_3 = dvs.runcmd(["vtysh", "-c", "show ip bgp neighbors 10.0.0.3 advertised-routes"])
    exit_code, received_route_3 = dvs.runcmd(["vtysh", "-c", "show ip bgp neighbors 10.0.0.3 routes"])
    print("Summary:")
    print(sum_res)
    print("Received routes:")
    print("10.0.0.1")
    print(received_route_1)
    print("10.0.0.3")
    print(received_route_3)
    print("Announces routes:")
    print("10.0.0.1")
    print(announce_route_1)
    print("10.0.0.3")
    print(announce_route_3)
    print("Neighbors")
    print("10.0.0.1")
    print(neig_1)
    print("======================================================")

def mkdir(path):
    if not os.path.exists(path):
        os.makedirs(path)

def remove(path):
    if os.path.exists(path):
        os.unlink(path)

def get_target_env(idx):
    return '/usr/local/etc/exabgp/exabgp_%d.env' % idx

def prepare_exa_env(dvs, idx):
    mkdir('/usr/local/etc')
    mkdir('/usr/local/etc/exabgp')
    tmp_name = '/tmp/env.%d.%d' % (random.randint(0, 10000000), os.getpid())
    dvs.servers[idx].runcmd("exabgp --fi > %s" % tmp_name)
    with open(tmp_name) as r_fp:
        with open(get_target_env(idx), 'w') as w_fp:
            for line in r_fp:
                if line.startswith('pipename'):
                    line = "pipename = 'exabgp.%d'\n" % idx
                w_fp.write(line)
    os.unlink(tmp_name)


def run_exa(dvs, idx, cfg):
    prepare_exa_env(dvs, idx)
    run_dir = "/var/run/exabgp"
    mkdir(run_dir)
    fifo_in_path  = "%s/exabgp.%d.in"  % (run_dir, idx)
    fifo_out_path = "%s/exabgp.%d.out" % (run_dir, idx)
    remove(fifo_in_path)
    remove(fifo_out_path)
    os.mkfifo(fifo_in_path)
    os.mkfifo(fifo_out_path)
    os.chmod(fifo_in_path,  0o666)
    os.chmod(fifo_out_path, 0o666)
    print("!!! Start exabgp instance %d" % idx)
    cmd = "exabgp -d --env %s %s" % (get_target_env(idx), cfg)
    print("Cmd is ___ %s ___" % cmd)
    return dvs.servers[idx].runcmd_async(cmd)

def run_exacli(dvs, idx, cmd):
    return dvs.servers[idx].runcmd('exabgpcli --env %s %s' % (get_target_env(idx), cmd))

def test_gr_livelock(dvs, testlog):
    # update exabgp to version 4.0.10
    dvs.servers[0].runcmd("pip install 'exabgp==4.0.10' --force-reinstall ")
    #
    dvs.copy_file("/etc/frr/", "bgp/files/gr_livelock/bgpd.conf")
    dvs.runcmd("supervisorctl stop bgpd")
    time.sleep(5)
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

    # Run two bgp neighbors
    p1 = run_exa(dvs, 0, "bgp/files/gr_livelock/exabgp1.conf")
    p2 = run_exa(dvs, 1, "bgp/files/gr_livelock/exabgp2.conf")

    time.sleep(30)

    output(dvs, "First neighbor doesn't have GR enabled")

    # Check that we announce routes from the 1st neigbbor to 2nd, if 1st neighbor doesn't support graceful restart
    _, announced_routes = dvs.runcmd(["vtysh", "-c", "show ip bgp neighbors 10.0.0.3 advertised-routes"])
    assert '2.2.2.2/32' in announced_routes

    # Stop 1st neighbor
    run_exacli(dvs, 0, 'shutdown')
    p1 = p1.wait()

    # Wait until frr thinks that 1st neighbor was shut down
    time.sleep(300)

    # Start the 1st neighbor again with graceful restart enabled
    p1 = run_exa(dvs, 0, "bgp/files/gr_livelock/exabgp1.graceful.conf")

    time.sleep(60)

    output(dvs, "First neighbor has GR enalbed")

    run_exacli(dvs, 0, 'announce route 1.10.0.0/24 next-hop self')

    time.sleep(60)

    output(dvs, "Announced a new route from the first neighbor")

    # Check that we announce all routes from the 1st neighbor
    _, announced_routes = dvs.runcmd(["vtysh", "-c", "show ip bgp neighbors 10.0.0.3 advertised-routes"])
    assert '2.2.2.2/32'  in announced_routes
    assert '1.10.0.0/24' in announced_routes

    run_exacli(dvs, 0, 'shutdown')
    p1 = p1.wait()

    run_exacli(dvs, 1, 'shutdown')
    p2 = p2.wait()
