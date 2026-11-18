# Traffic shift in frrcfgd

`BGP_DEVICE_GLOBAL/STATE/tsa_enabled` controls traffic-shift-away (TSA) for
the IPv4/IPv6 unicast address families configured in `BGP_NEIGHBOR_AF` and
`BGP_PEER_GROUP_AF`.

With TSA enabled, frrcfgd advertises only prefixes from `LOOPBACK_INTERFACE`
in the corresponding VRF. These prefixes must also pass the configured
outbound policy. Other advertisements are withdrawn without shutting down
BGP sessions. This is different from `BGP_GLOBALS/graceful_shutdown`, which
deprioritizes routes rather than withdrawing them.

Private `FRRCFGD_TSA_` prefix-lists and route-map wrappers implement the
filter. The wrappers call the configured outbound route-map, so its entries
and attributes are preserved. That prefix is reserved for frrcfgd-generated
objects. Disabling TSA or removing the field restores the current outbound
policies and removes the private objects. Startup rendering and daemon
replay also retain TSA.

To enable TSA with `config apply-patch`, use:

```json
[
  {"op": "add", "path": "/BGP_DEVICE_GLOBAL/STATE/tsa_enabled", "value": "true"}
]
```

The parent table and `STATE` object must exist. Set the value to `"false"`
to restore advertisements. The existing `sonic-bgp-device-global.yang`
boolean leaf is used; no schema change is required.

This implementation handles the local CONFIG_DB flag. Chassis-wide
coordination through CHASSIS_APP_DB and bgpcfgd's topology-specific internal
route exemptions are separate features.
