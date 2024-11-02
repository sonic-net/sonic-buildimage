#!/usr/bin/env python3
"""
    Script to enable Receive Packet Steering (RPS).
    This script reads the number of available CPUs in the system
    and configures RPS on all front-panel ports using sysfs.
"""
import os
import sys
import syslog
import argparse

def write_syslog(message, *args):
    """
    Write a message to syslog.

    Args:
        message (str): Message string to be logged
        args: Optional args

    Returns:
        None
    """

    if args:
        message %= args
    syslog.syslog(syslog.LOG_NOTICE, message)


def get_num_cpus():
    """
    Get number of CPUs in the system.

    Returns:
        ncpus (int): Number of CPUs
    """
    ncpus = 0
    cpu_count = os.cpu_count()
    if cpu_count is not None:
        ncpus = cpu_count

    return ncpus


def get_cpumask(ncpus):
    """
    Get a hex cpumask string.

    Args:
        ncpus (int): Number of CPUs

    Returns:
        cpu_mask (str): CPU mask as hex string
    """
    cpu_mask = '0'
    if isinstance(ncpus, int) and ncpus >= 0:
        cpu_mask = hex(pow(2, ncpus) - 1)[2:]

    return cpu_mask


def configure_rps(intf):
    """
    Configure Receive Packet Steering (RPS)

    Returns:
        rv (int): zero for success and non-zero otherwise
    """
    rv = 0
    NET_DIR_PATH = "/sys/class/net"

    num_cpus = get_num_cpus()
    if not num_cpus:
        rv = -1
        return rv

    cpumask = get_cpumask(num_cpus)
    if cpumask == '0':
        # Nothing to do
        return rv

    queues_path = os.path.join(NET_DIR_PATH, intf, "queues")
    queues = os.listdir(queues_path)
    num_rx_queues = len([q for q in queues if q.startswith("rx")])
    for q in range(num_rx_queues):
        rps_cpus_path = os.path.join(queues_path, "rx-{}", "rps_cpus").format(q)
        with open(rps_cpus_path, 'w') as file:
            file.write(cpumask)

    return rv


def main():
    rv = -1
    try:
        parser = argparse.ArgumentParser(description='Configure RPS.')
        parser.add_argument('interface', type=str, help='Network interface')
        args = parser.parse_args()
        rv = configure_rps(args.interface)
        write_syslog("configure_rps {} for interface {}".format
                    ("failed" if rv else "successful", args.interface))
    except Exception as e:
        write_syslog("configure_rps exception: {}".format(str(e)))

    sys.exit(rv)


if __name__ == "__main__":
    main()

