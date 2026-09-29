# sonic-gribi

Terminates gRIBI from an SDN controller and programs the resulting routes into
SONiC over orchagent's ZeroMQ route channel. No routing protocol runs on the
switch.

    gRIBI client ──grpc──▶ gribi.Service (wraps gribigo)
                              │ hook
                              ▼
                        adapter.Programmer ──▶ swsszmq.Producer ──PUSH──▶ orchagent :8100
                              ▲                                              │ SAI
                        fibtrack.Tracker ◀── applstate.Listener ◀── Redis pub/sub

The gRIBI server is [openconfig/gribigo](https://github.com/openconfig/gribigo),
with two patches (see Network instances and Add cost). orchagent is untouched. Everything here is the translation between
the two.

| Package | Role |
|---|---|
| `swsszmq` | swsscommon's BinarySerializer frame format and a PUSH producer |
| `adapter` | gribigo's post-change RIB hook → `ROUTE_TABLE` rows, groups flattened inline |
| `neigh` | next-hop address → egress interface, from APPL_DB `NEIGH_TABLE` |
| `applstate` | listens on `APPL_DB_ROUTE_TABLE_RESPONSE_CHANNEL` for orchagent's verdict |
| `fibtrack` | one slot per route key, joining sends to responses |
| `gribi` | holds each route's `FIB_PROGRAMMED` until orchagent has answered |
| `routekey` | `(network instance, prefix)` → `ROUTE_TABLE` key |
| `config` | CONFIG_DB's `GRIBI` and `VRF` tables |
| `cmd/gribid` | wiring and flags |
| `e2e` | on-box test (`-tags e2e`) |

## Why ZeroMQ

With `SYSTEM_DEFAULTS|swss_zmq status=enabled` in CONFIG_DB, RouteOrch reads
`ROUTE_TABLE` only from a ZeroMQ PULL socket (`ZmqRouteServer`, orchagent's
`-q tcp://127.0.0.1`, port 8100) and never from Redis. Routes sent this way do
not appear in APPL_DB at all (`dbPersistence=false`); their only trace is
orchagent's response and the `APPL_STATE_DB ROUTE_TABLE:<key>` entry it writes
on success.

Only `ROUTE_TABLE` and `LABEL_ROUTE_TABLE` travel over ZeroMQ. `NEXTHOP_GROUP_TABLE`
does not, so a gRIBI next-hop group is flattened into every route that uses it
(`nexthop`, `ifname`, `weight` as index-aligned comma lists), exactly as
fpmsyncd does for BGP ECMP. When a group or next hop changes, every dependent
route is re-sent in one frame.

The frame format is `sonic-swss-common/common/binaryserializer.h`: a `u64` pair
count, then `(u64 len, bytes, u64 len, bytes)` pairs; pair 0 is
`("APPL_DB", "ROUTE_TABLE")`, then per tuple `(key, fieldCount)` followed by
the fields. Zero fields is a DEL.

## Batching

orchagent drains and commits one ZeroMQ frame at a time, and each commit has a
fixed cost. So the RIB hook does not send a route when gribigo installs it; it
queues the write, and the queue leaves as one frame when any of these happens:

- gribigo has run every operation of a ModifyRequest (the wrapper flushes before
  it reads the next request), or a Flush RPC has finished;
- the queue holds `-zmq-batch-max` routes (default 1024, orchagent's `-b`);
- `-zmq-batch-linger` (default 50ms) has passed since the first queued write.
  gribigo takes about 50 µs per route, so a large request takes a while to run;
  the linger bounds how long a route waits for it. `0` disables the timer.

A frame is also cut before it reaches half of orchagent's 16 MiB receive buffer.
Writes keep their RIB order, and orchagent merges writes to one key within a
frame, last one wins. A failed send fails every route in the frame.

## Add cost

gribigo v0.1.3 merged every added entry into its RIB with
`ygot.MergeStructInto`, which rebuilds the whole table it merges into, so each
add cost O(routes installed): about 2300 routes/s into an empty RIB, under 100
by 75k. `patches/gribigo-rib-add-o1.patch` stores the entry directly, so an add
costs the same at any RIB size.

## What FIB_PROGRAMMED means here

gribigo alone echoes `FIB_PROGRAMMED` as soon as an entry is in its RIB. This
agent wraps the Modify stream: a route's `FIB_PROGRAMMED` is held back until
routeorch publishes `SWSS_RC_SUCCESS` for that key, and becomes `FIB_FAILED`
with orchagent's `err_str` on any other status, or with a timeout message
(`-fib-ack-timeout`, default 30s) when no response arrives. No response usually
means an unresolved next hop that routeorch keeps retrying. Next hops and
next-hop groups have no SONiC object of their own and are acknowledged at once.

Every route needs an egress interface. A next hop with an `interface-ref` uses
it; otherwise the address is looked up in `NEIGH_TABLE`. A route with a leg
that cannot be resolved is not programmed at all and fails with a message
naming the address, rather than being installed with fewer legs than asked.

## Running it

Prerequisites on the switch:

    sonic-db-cli CONFIG_DB hgetall "SYSTEM_DEFAULTS|swss_zmq"   # status: enabled
    show suppress-fib-pending                                   # Enabled
    ps -ef | grep orchagent                                     # ... -q tcp://127.0.0.1
    sonic-db-cli APPL_DB keys "NEIGH_TABLE:*"                    # next hops resolved

routeorch reports route results, which become gRIBI's FIB acknowledgements,
only when orchagent runs with `-F`, set from `suppress-fib-pending`
(`sudo config suppress-fib-pending enabled`, then `config save` and
`config reload`). Without it gribid refuses `RIB_AND_FIB_ACK` sessions with
`FAILED_PRECONDITION`; `RIB_ACK` sessions still work.

The image ships gribid in the `gribi` container, with the feature disabled:

    sudo config feature state gribi enabled

gribid reads CONFIG_DB's `GRIBI` table (`sonic-gribi.yang`) at startup:

| Field | Default | |
|---|---|---|
| `GRIBI\|config` `port` | 9340 | |
| `GRIBI\|config` `fib_ack_timeout` | 30 | seconds |
| `GRIBI\|config` `log_level` | `info` | `debug`, `info`, `warn`, `error` |
| `GRIBI\|config` `enable_reflection` | `false` | gRPC reflection, for grpcurl |
| `GRIBI\|certs` `server_crt`, `server_key` | | serve TLS |
| `GRIBI\|certs` `ca_crt` | | require client certificates signed by it |

Without certificates gRIBI is served in plaintext, with a warning in the log.
Configuration changes take effect on `sudo systemctl restart gribi`.

### Network instances

gribid serves the `DEFAULT` network instance plus one per VRF in CONFIG_DB's
`VRF` table, named as SONiC names it (`Vrfblue`). Routes in `DEFAULT` are
keyed by bare prefix, the rest as `<vrf>:<prefix>`, as routeorch expects.
Next hops are resolved only among neighbors on interfaces bound to the route's
VRF. gribigo only knows the instances that existed when it started, so after
adding a VRF, restart gribi.

gribigo's server does not attach its change hook to instances added through
`WithVRFs`; `patches/gribigo-vrf-hook.patch` fixes that in `vendor/` until an
upstream release does.

### By hand

`make` vendors the dependencies, applies `patches/`, and builds a static
`build/bin/gribid` that needs neither Go nor any shared library on the switch.
Build and test through `make` (or after `make`, with plain `go` commands,
which use `vendor/`); a build straight from the module cache lacks the gribigo
fixes. If a parent directory has a `go.work` that does not list this module,
prefix the commands with `GOWORK=off`.

    make
    scp build/bin/gribid admin@<switch>:/tmp/
    /tmp/gribid -zmq tcp://127.0.0.1:8100 -redis 127.0.0.1:6379

Flags given explicitly override the `GRIBI` table: `-listen`,
`-fib-ack-timeout`, `-debug`, `-reflection`, `-insecure` (ignore
`GRIBI|certs`), and `-vrf` (repeatable; replaces the `VRF` table as the
list of network instances). `-zmq` and `-redis` locate orchagent and Redis;
`-zmq-batch-max` and `-zmq-batch-linger` tune [batching](#batching). Routes
are logged one per line only at debug level.

## Verifying

The hermetic suite needs no switch:

    make check

On a box, with the agent running:

    CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go test -c -tags e2e -o e2e.test ./e2e
    ./e2e.test -test.v -target localhost:9340 -redis localhost:6379 \
      -nexthops 10.0.0.57,10.0.0.59 -prefix 10.20.0.0/16 \
      -nexthops6 fc00::72,fc00::7a -prefix6 2001:db8:20::/64

Add `-vrf Vrfblue -vrf-nexthop <neighbor in Vrfblue>` to cover a VRF.

It programs, replaces and deletes routes, asserts `FIB_PROGRAMMED` for each,
and checks `APPL_STATE_DB` (`protocol gribi`) and the ASIC_DB next hops. A
second test programs a route over an unresolved next hop and expects
`FIB_FAILED` with nothing in the ASIC.

By hand:

    sonic-db-cli APPL_STATE_DB hgetall "ROUTE_TABLE:10.20.0.0/16"   # protocol gribi
    sonic-db-cli ASIC_DB keys "*ROUTE_ENTRY*" | grep 10.20.0.0
    sonic-db-cli APPL_DB exists "ROUTE_TABLE:10.20.0.0/16"          # 0: no mirror, by design

## Known limits

- One slot per key, not per operation: two writes to one key that orchagent
  did not merge, with different outcomes, are both reported with the first.
- Redis pub/sub is at-most-once. A lost response becomes `FIB_FAILED` by
  timeout, never a false success.
- `ROUTE_TABLE` is shared with fpmsyncd; nothing arbitrates between them.
- MPLS label entries are accepted by gribigo and ignored here.
- A VRF created after gribid started needs a restart before it can take
  routes.

## Authors

Alton Lo and Randy Zhu.
