# FIREBREAK: one Redis login and one exit list per container

A SONiC Hackathon 2026 project that gives `dhcp_relay`, `snmp` and `otel` their **own Redis
user** and an **IPv4 `connect()` allow-list**, both enforced from the host. If one of those
containers is compromised, it can no longer rewrite TACACS, wipe CONFIG_DB, or dial out to an
address nobody approved.

We changed nothing in Redis, swss, or the container binaries. The control lives in one host
package, plus two image-template changes that mount the private socket directory and install
the package.

```
sudo config firebreak enable
docker exec snmp redis-cli -s /var/run/redis/redis.sock HSET 'TACPLUS_SERVER|evil' auth_type pap
(error) NOPERM
```

| | |
|---|---|
| **Project** | FIREBREAK |
| **Company** | Upscale |
| **Category** | System & software security |
| **Team** | Uma Ramanathan, John Cheung, Keerthan Reddy, Priyanka Ravichandran, Ananya Krishna |
| **Pull request** | [sonic-net/sonic-buildimage#29901](https://github.com/sonic-net/sonic-buildimage/pull/29901) |
| **Referenced community work** | [SONiC Container Hardening HLD](https://github.com/sonic-net/SONiC/blob/master/doc/Container%20Hardening/SONiC_container_hardening_HLD.md) |
| **Status** | Partially completed (see [Status](#status)) |

---

## The problem

Several SONiC services face the network, including SNMP, the REST API, gNMI, BGP and LLDP, and
each runs in a container. A bug or weak setting in any of them, such as SNMP communities with no
source-address limit ([PR #29874](https://github.com/sonic-net/sonic-buildimage/pull/29874)),
can compromise it. We are not exploiting any of these. We assume one container falls and ask what
it inherits. Two things:

1. **Default Redis access.** The container mounts the real Redis socket and connects as user
   `default` (`nopass ~* +@all`). It can plant a rogue `TACPLUS_SERVER` entry to take over switch
   login, or `FLUSHALL` CONFIG_DB. **Lock 1** gives each container its own Redis user with only
   the permissions it needs.
2. **Unsafe outbound connections.** With host networking and no egress control, the container can
   connect to any address to exfiltrate data or take instructions. **Lock 2** allows only
   approved IPv4 destinations.

This is the shape of the July 2026 Hugging Face incident
([OpenAI](https://openai.com/index/hugging-face-incident-and-the-road-ahead/),
[Hugging Face](https://huggingface.co/blog/agent-intrusion-technical-timeline)): a permitted exit
and a credential trusted everywhere took one pod to cluster admin. We do not claim to have fixed
that incident.

FIREBREAK complements the community [Container Hardening HLD](https://github.com/sonic-net/SONiC/blob/master/doc/Container%20Hardening/SONiC_container_hardening_HLD.md),
which removes privileges and host networking but leaves the container as Redis `default` with open
egress. FIREBREAK limits what a compromise inherits. It does not stop the first break-in, and host
root is out of scope.

## The idea: two locks, both outside the container

```
  host: firebreakd owns everything in the upper box
┌────────────────────────────────────────────────────────────────────────┐
│  /etc/sonic/firebreak/snmp.json    host-owned profile, not CONFIG_DB   │
│           │                                                            │
│           ▼                                                            │
│  Redis ACL user "snmp"  ◄── compiled: -@all + listed commands,         │
│           ▲                 read / write key patterns, channels        │
│           │ AUTH snmp <secret>  (once per connection)                  │
│  ┌────────┴───────────┐                                                │
│  │ redis_proxy (snmp) │ ── /var/run/redis/redis.sock (real Redis)      │
│  └────────▲───────────┘                                                │
│           │ /var/run/firebreak/snmp/redis.sock                         │
└────────────────────────────────────────────────────────────────────────┘
             │ bind-mounted as the container's /var/run/redis
┌────────────────────────────────────────────────────────────────────────┐
│  snmp container                                                        │
│    Redis:     only the proxy socket is visible, not the real one       │
│    connect()  ──► cgroup/connect4 BPF (attached by firebreakd)         │
│                     ├─ address on the allow-list ──► allowed           │
│                     └─ anything else ──► EPERM (not permitted)         │
└────────────────────────────────────────────────────────────────────────┘
```

| | Lock 1: Redis identity | Lock 2: connect() allow-list |
|---|---|---|
| Question | Who is this container, and what may it do in Redis? | Where may this container open a connection? |
| Mechanism | Host `redis_proxy` sends `AUTH` as the container's own Redis 7 ACL user | `cgroup/connect4` eBPF program attached to the container's cgroup |
| Without FIREBREAK | `ACL WHOAMI` is `default`. TACACS write is `OK`. | `connect()` to any address is allowed to try. |
| With FIREBREAK | Forbidden write returns `NOPERM`. | Unapproved address returns `Operation not permitted`. |

### Lock 1: a Redis identity per container

`redis_proxy` is a small host process that `firebreakd` starts for each protected container. It
listens on a private Unix socket that is mounted into the container where the real Redis socket
normally sits, so the application does not change. On every connection it sends `AUTH` to the
real Redis as that container's named ACL user, using a secret that exists only on the host, and
then forwards traffic in both directions. The container cannot choose its identity, skip the
login, or read the password. Redis itself enforces the ACL.

Why a proxy and not a login by the container: Redis on `127.0.0.1:6379` answers as `default`
when a client skips `AUTH`. If the container logged in itself, any process in it could skip
that step. Password-protecting `default` would break `swss`, `syncd`, `bgp`, `teamd` and the
rest, which is a community migration, not a FIREBREAK change. We tried native login first and
removed it for this reason.

A profile is a root-owned JSON file per container in `/etc/sonic/firebreak/`, outside anything
the container mounts. It is deliberately not a CONFIG_DB table, because a compromised container
can write some CONFIG_DB keys. `firebreakd` validates it, rejects dangerous commands such as
`FLUSHALL`, `CONFIG SET` and scripting, and compiles four lists into the Redis ACL: allowed
commands, readable key patterns, writable key patterns, and pub/sub channels. Redis must accept
the rules on a temporary user before they are promoted.

```jsonc
// config/snmp.json (excerpt)
"redis_user": "snmp",
"commands": ["hget", "hgetall", "subscribe", "hset", "..."],
"read_key_patterns": ["*"],
"write_key_patterns": [],
"channel_patterns": ["__keyspace@4__:*", "..."]
```

A command and a key pattern must both match. `snmp` lists `hset`, but its write list is empty,
so every write is refused.

| Container | May read | May write |
|---|---|---|
| `snmp` | everything (SONiC startup needs shared config) | nothing |
| `otel` | everything | nothing |
| `dhcp_relay` | everything | `DHCP*` keys only |

### Lock 2: a connect() allow-list

A small cgroup-attached eBPF `connect4` program checks each IPv4 `connect()` against a per-container
map. Anything not on the map returns `EPERM` before a packet leaves, so the kernel refuses the
call and the container cannot talk its way around it. Entries get in two ways:

- **Static:** addresses in `egress_allow_ipv4` in the host profile are always allowed.
- **Declared collector:** a `TELEMETRY` or `OTEL` endpoint in CONFIG_DB is only a request. It is
  admitted when it falls inside the host-approved `egress_approved_ipv4` ceiling. Otherwise it is
  listed as `rejected_collectors`. The shipped profiles approve nothing, and a container cannot
  edit either list.

Collectors are read from CONFIG_DB before the first BPF attach, so a legitimate collector is not
locked out at boot. Declarations are polled about every 5 seconds, and protection is re-checked
about every 15 seconds.

To approve and use a collector:

```bash
# 1. approve the address in the host profile (/etc/sonic/firebreak/snmp.json)
#      "egress_approved_ipv4": ["192.168.255.1/32"]
# 2. reload
sudo config firebreak reload
# 3. the operator declares the collector
redis-cli -s /var/run/redis/redis.sock -n 4 \
  HSET 'TELEMETRY|firebreak_test' otlp_endpoint '192.168.255.1:18431'
# 4. check
show firebreak egress snmp
```

## What the demo shows

One compromised container (`snmp`), simulated with `docker exec`. No exploit traffic. A spine
switch runs stock SONiC and a leaf switch runs FIREBREAK.

| Step | Spine, no FIREBREAK | Leaf, FIREBREAK on |
|---|---|---|
| `ACL WHOAMI` | `default` | named user `snmp` |
| `HSET 'TACPLUS_SERVER\|evil' ...` | `OK` | `NOPERM` |
| `connect()` to `203.0.113.1:443` | hangs or is unreachable, never refused | `Operation not permitted` |
| `connect()` to an approved collector | n/a | not refused |
| `show lldp table`, SNMP polling | works | still works |

```bash
show firebreak status          # which containers are protected and how it was verified
show firebreak acl snmp        # the Redis ACL FIREBREAK installed
show firebreak egress snmp     # the connect() allow-list
show firebreak sockets         # the private proxy socket the container sees
config firebreak enable snmp   # also: disable, reload
```

`protected` is published only after the profile, the required container processes, the live
ACL, the login path and the BPF program on that container's cgroup all check out. Drift is
detected and repaired, and an invalid reload is rejected while the previous valid policy stays
active.

## Files in this PR

| Path | Role |
|---|---|
| [`firebreakd/firebreakd.py`](firebreakd/firebreakd.py) | Host daemon: install, verify, repair and roll back protection per container |
| [`firebreakd/policy.py`](firebreakd/policy.py) | Profile validation and Redis ACL compile |
| [`firebreakd/verify_client.py`](firebreakd/verify_client.py) | Named-login probe used by the `protected` check |
| [`redis_proxy/redis_proxy.py`](redis_proxy/redis_proxy.py) | The Lock 1 proxy |
| [`bpf/connect4_loader.py`](bpf/connect4_loader.py) | Lock 2: `connect4` program loader and TCP 6379 redirect |
| [`config/`](config/) | Shipped profiles: `dhcp_relay.json`, `snmp.json`, `otel.json` |
| [`cli/`](cli/) | `show firebreak` and `config firebreak` |
| [`debian/`](debian/), [`firebreak.service`](firebreak.service), [`Makefile`](Makefile) | Package `sonic-firebreak_1.4_all.deb` and the systemd unit |
| [`rules/firebreak.mk`](../../rules/firebreak.mk) | Build rule |
| [`files/build_templates/`](../../files/build_templates/) | `docker_image_ctl.j2` and `sonic_debian_extension.j2` changes |

## Build and try it

```bash
make configure PLATFORM=vs
make SONIC_BUILD_JOBS=4 target/sonic-vs.img.gz
```

On the switch (FIREBREAK installs disabled):

```bash
sudo config firebreak enable
show firebreak status

docker exec snmp redis-cli -s /var/run/redis/redis.sock \
  HSET 'TACPLUS_SERVER|evil' auth_type pap          # NOPERM
docker exec snmp timeout 2 bash -c 'echo >/dev/tcp/203.0.113.1/443'
                                                    # Operation not permitted

sudo config firebreak disable                       # restores stock mounts
```

## What we completed during the hackathon

**Before the hackathon:** a Redis proxy proof of concept and a `connect()` allow-list proof of
concept.

**Built this week:**

- **Host proxy as the Lock 1 path**, with explicit per-container Redis 7 permissions (commands, read and write key patterns, channels) instead of broad ACL categories.
- **Host-approved collector addresses** for Lock 2, with scrape-before-attach, an approval ceiling, and deny by default.
- **Observed protection:** `protected` requires live verification of the ACL, identity, mount, service processes and BPF attachment, with drift repair and rollback.
- **A TCP 6379 redirect** (`redis_tcp_proxy_port`): a BPF rewrite that sends loopback Redis TCP into the proxy, so supervisor helpers authenticate as the named user.
- **Per-container operations:** `config firebreak enable | disable | reload` and `show firebreak status | acl | egress | sockets`, with clean un-protect that restores the stock mount and revokes the Redis user.
- **Profiles for `dhcp_relay`, `snmp` and `otel`.**
- **Packaging for `sonic-buildimage`**: a build rule ([`rules/firebreak.mk`](../../rules/firebreak.mk)) and two template changes ([`docker_image_ctl.j2`](../../files/build_templates/docker_image_ctl.j2), [`sonic_debian_extension.j2`](../../files/build_templates/sonic_debian_extension.j2)). The package installs disabled, so a stock image is unchanged until an operator runs `config firebreak enable`.
- **A full virtual-switch image build and fresh-VM validation** on the September 29 source: all three profiles reached `protected`, an unapproved IPv4 destination was refused, a forbidden write returned `NOPERM`, declaring an unapproved collector in CONFIG_DB did not grant access, and SNMP, LLDP, OTLP and DHCP checks passed before protection, with protection, after reboot and after rollback.

## Status

| Item | Status |
|---|---|
| Per-container Redis user through the host proxy | **Completed** |
| `connect()` allow-list with host-approved collectors | **Completed** |
| CLI, watchers, verified `protected` status, rollback | **Completed** |
| Profiles for `dhcp_relay`, `snmp`, `otel` | **Completed** |
| Profiles for other containers (`lldp`, `sflow`, `gnmi`, ...) | **Partially completed.** Design supports them; not written or tested. |
| Full-image validation of the proxy-only build | **Partially completed.** The full-image run used the September 29 source, before native login was removed. Later fixes were tested on a live VM, not a rebuilt image. |
| Container Hardening HLD alignment | **Partially completed.** Complementary by design; not tested together. |
| Policy as a CONFIG_DB table or YANG model | **Concept only.** The container can write some CONFIG_DB keys, so rules stay in a host file. |
| IPv6, unconnected UDP, DNS-safe egress | **Concept only.** |

Overall: **Partially completed.**

## Future work

The two locks and the host-side design are in place. What comes next widens their coverage:

- **Wider egress coverage.** Lock 2 enforces IPv4 `connect()` today. The same cgroup hook approach extends to IPv6, UDP and DNS-safe egress, and attaching the program at container start would also cover sockets opened before attach.
- **Loopback closed by default.** The `redis_tcp_proxy_port` redirect already sends loopback Redis TCP into the proxy. The next step is shipping it in the default profiles, so the raw-TCP path is closed out of the box.
- **More containers.** The profile format is container-agnostic, so `lldp`, `sflow`, `gnmi` and other feature containers can each get their own Redis user and exit list. `swss`, `syncd` and `database` share Redis `default` and will need their own design.
- **Validated end to end.** Rebuild the full image from the proxy-only source and rerun the fresh-VM validation, including reboot and rollback.
- **Better together with Container Hardening.** The community HLD removes privileges and host networking. FIREBREAK adds identity and egress on top. Testing the two on the same image gives a complete answer to "what does one compromise inherit?".
- **Upstream.** Take the profile model to the SONiC community for review, and explore a policy path that does not depend on a host file, such as a protected CONFIG_DB table or YANG model.

FIREBREAK is a second layer. It does not stop the first compromise, and it trusts the host, so it is meant to sit alongside patching and image hardening.
