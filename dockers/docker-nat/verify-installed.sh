#!/bin/sh
# Run inside the produced archive, with a read-only filesystem and no network.
set -eu

for table in iptables ip6tables arptables ebtables; do
    update-alternatives --query "$table" | grep -Fx "Value: /usr/sbin/$table-nft"
    for suffix in '' -save -restore; do
        test "$(readlink "/etc/alternatives/$table$suffix")" = "/usr/sbin/$table-nft$suffix"
        test -x "/usr/sbin/$table$suffix"
    done
done
iptables --version | grep -F nf_tables
brctl --version
conntrack --version
for package in bridge-utils conntrack iptables; do
    test "$(dpkg-query -W -f='${db:Status-Status}' "$package")" = installed
done
for daemon in natmgrd natsyncd; do
    test -x "/usr/bin/$daemon"
    grep -Fx "program:$daemon" /etc/supervisor/critical_processes
    grep -Fx "command=/usr/bin/$daemon" /etc/supervisor/conf.d/supervisord.conf
    libs=$(ldd "/usr/bin/$daemon")
    case "$libs" in *'not found'*) echo "$libs" >&2; exit 1;; esac
done
test -x /usr/bin/start.sh
test -x /usr/bin/restore_nat_entries.py
python3 -B -c 'from sonic_py_common.logger import Logger; from swsscommon import swsscommon'
if [ "$1" = docker-nat-dbg ]; then
    for daemon in natmgrd natsyncd; do
        gdb -nx -batch -ex 'info line main' "/usr/bin/$daemon" | grep -E '^Line [0-9]+ of'
    done
fi
