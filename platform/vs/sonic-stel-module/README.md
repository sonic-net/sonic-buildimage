# sonic-stel-module

Minimal generic netlink kernel module for SONiC Stream Telemetry (STEL).

## Overview

Registers the `sonic_stel` generic netlink family with an `ipfix` multicast group.
This module acts as a relay: vslib (virtual SAI) sends IPFIX data records via
`SONIC_STEL_CMD_SEND_IPFIX`, and the module multicasts them to all listeners
(countersyncd) subscribed to the `ipfix` group.

## Data flow

```
vslib → genetlink (SONIC_STEL_CMD_SEND_IPFIX) → sonic_stel.ko → genlmsg_multicast → countersyncd
```

## Building

### With sonic-buildimage

The module is built automatically as part of the SONiC build system.

### Standalone (for development)

```bash
make -C /lib/modules/$(uname -r)/build M=$(pwd)/modules modules
```

Requires kernel headers for your running kernel.

## DKMS

A `dkms.conf` is not included in this package; use the debian packaging
for automated builds within sonic-buildimage.

## References

- [HFT HLD](https://github.com/sonic-net/SONiC/blob/master/doc/high-frequency-telemetry/high-frequency-telemetry-hld.md)
- [SAI TAM Stream Telemetry Proposal](https://github.com/opencomputeproject/SAI/blob/master/doc/TAM/SAI-Proposal-TAM-stream-telemetry.md)
- [sonic-genl-packet](https://github.com/sonic-net/sonic-genl-packet) — similar genetlink module pattern
