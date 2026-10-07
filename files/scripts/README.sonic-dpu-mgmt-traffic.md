# DPU management traffic forwarding

Run `sonic-dpu-mgmt-traffic.sh` as root on a SmartSwitch with `eth0` and an
IPv4-configured `bridge-midplane`.

```bash
sonic-dpu-mgmt-traffic.sh outbound -e
sonic-dpu-mgmt-traffic.sh inbound -e --dpus dpu0,dpu1 --ports 9090,9091
sonic-dpu-mgmt-traffic.sh inbound -d --dpus dpu0,dpu1 --ports 9090,9091
```

Inbound forwarding maps each switch port to the corresponding DPU's SSH port
22. `--dpus all` uses sorted DPU names; the port list follows that order.
The bridge address remains the SNAT source so DPU replies can use their
connected midplane route without a default route. Outbound forwarding retains
management-interface masquerading.

Ports must contain decimal digits and be in the range 1024-65535. Leading zeros
are normalized to decimal, not interpreted as octal. Occupied local ports are
rejected on both enable and disable, preserving the existing occupancy check.
Each selected DPU must have exactly one canonical IPv4 address in the
`DHCP_SERVER_IPV4_PORT` CONFIG_DB `ips@` field. Multi-address entries, IPv6,
prefixes and malformed values are rejected rather than silently selecting a
destination. The DHCP table's general multi-address schema is unchanged.

`--nofwctrl` leaves the global and management-interface IPv4 forwarding
settings unchanged. Use it when changing one direction without disabling the
other. Without it, enable writes `1` and disable writes `0`, as before.
Existing rules are not duplicated, and deleting absent rules is a no-op.

All configuration is validated before forwarding or firewall mutations.
Database, port-query, rule-query, mutation and forwarding-write failures return
a nonzero status. A failure after earlier successful changes can leave partial
state; the script does not roll back unrelated firewall rules. Replacing the
script does not itself change existing rules; they change on invocation.
Redis responses are validated even when the client exits successfully, since
older `redis-cli` versions return zero for server errors. No newer `-e` option
is required.

## Regression tests

Tests use only Python 3.9+ and its standard library. Root tests must run inside a private
network namespace; the suite refuses to use the host namespace.

```bash
sudo unshare --net python3 files/scripts/tests/test_sonic_dpu_mgmt_traffic.py
sudo unshare --net env F068_INTEGRATION=1 \
    python3 files/scripts/tests/test_sonic_dpu_mgmt_traffic.py
sudo unshare --net env F068_INTEGRATION=1 F068_REDIS_BIN=/usr/bin \
    python3 files/scripts/tests/test_sonic_dpu_mgmt_traffic.py
```

The first command mocks external commands and checks argument boundaries,
validation, error propagation and idempotence. The second also uses real
iptables, veth links and TCP endpoints in private namespaces to check inbound
DNAT/bridge SNAT, outbound masquerading, reply routing and `--nofwctrl`.
It requires `ip`, `iptables`, `unshare`, `nsenter` and Python 3.
The third command additionally starts temporary Redis servers with only private
Unix sockets. `F068_REDIS_BIN` must identify a directory containing
`redis-server` and `redis-cli`. It tests real scalar reads, multi-address rejection,
Redis command/connection errors, and uses real Redis in the forwarding test.
Redis data and processes are removed when the tests finish.

For the legacy backend, set `F068_IPTABLES=iptables-legacy` and
`F068_IPTABLES_SAVE=iptables-legacy-save`; both executables must be available.

Set `F068_BASELINE` to a saved original script to compare valid firewall
argument sequences for inbound/outbound enable and disable. Optional
`F068_SCRIPT` selects a script under test; it never installs that script.
No test writes production Redis or the host firewall. Namespace tests model
the Linux forwarding path; they do not replace SmartSwitch hardware validation.
