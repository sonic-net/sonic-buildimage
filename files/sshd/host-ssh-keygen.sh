#!/bin/bash

set -e

[ -r /etc/ssh/ssh_host_rsa_key ] || {
    rm -f /etc/ssh/ssh_host_*_key*
    /usr/bin/ssh-keygen -t rsa -N '' -f /etc/ssh/ssh_host_rsa_key
}
