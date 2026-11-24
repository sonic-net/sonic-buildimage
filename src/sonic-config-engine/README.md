# sonic-config-engine

### Minigraph

Generate configuration through a matching minigraph:

```shell
sonic-cfggen -m /etc/sonic/minigraph.xml \
    -p /usr/share/sonic/platform/platform.json \
    -S /tmp/deployment-hwsku.json --print-data
```

This command prints configuration without applying it.

The minigraph command above does not emit `BREAKOUT_CFG`.

Minimum and explicit JSON layouts preserve speeds, lanes, and supplied
FEC/autonegotiation. On single-ASIC devices, missing channel/VLAN members,
interface parents, route/FG-ECMP references, or unresolved dataplane ACL attachments
cause errors, not topology repair. Legacy sources and multi-ASIC topology handling
are unchanged.

A missing HWSKU directory links to an existing base SKU directory (name without
the final hyphen-separated suffix). Link failures warn and parsing continues.
