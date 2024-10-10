#!/usr/bin/env python3

import os
import ast
import subprocess

'''
Script to enable Receive Packet Steering (RPS)
'''

def exec_cmd(cmd):
    p = subprocess.Popen(cmd, shell=False, executable='/bin/bash',
                         stdout = subprocess.PIPE)
    return (p.communicate()[0].strip())


def configure_rps():
    num_cpus = ast.literal_eval(exec_cmd("nproc"))
    cpumask = hex(pow(2, num_cpus) - 1)[2:]
    net_dir_path = "/sys/class/net"

    for intf in os.listdir(net_dir_path):
        if "Ethernet" in intf:
            queues_path = os.path.join(net_dir_path, intf, "queues")
            queues = os.listdir(queues_path)
            num_rx_queues = len([q for q in queues if q.startswith("rx")])
            for q in range(num_rx_queues):
                path = os.path.join(queues_path, "rx-{}", "rps_cpus").format(q)
                command = "echo {} | sudo tee  {}".format(cpumask, path)
                print ("Setting {} to cpu mask {}".format(path, cpumask))
                print (command)
                exec_cmd(command)


if __name__ == "__main__":
    configure_rps()

