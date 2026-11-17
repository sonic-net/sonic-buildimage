# sonic-config-engine

## Deploy-time port configuration with sonic-cfggen

`sonic-cfggen` generates `CONFIG_DB.PORT` from `platform.json` and a deploy-time
`hwsku.json` supplied with `-S` / `--hwsku-config`. The selection file may live anywhere:

```json
{
    "interfaces": {
        "Ethernet0": {"default_brkout_mode": "1x100G[40G]"}
    }
}
```

Only selected platform cages or generated child ports are configured. Breakout
modes supply port attributes; `fec`, `autoneg`, and `role` may be overridden.
Invalid or unreadable selections fail rather than fall back.

Generate ports directly or through a matching minigraph:

```shell
sonic-cfggen -k Generic \
    -p /usr/share/sonic/platform/platform.json \
    -S /tmp/deployment-hwsku.json --var-json PORT

sonic-cfggen -m /etc/sonic/minigraph.xml \
    -p /usr/share/sonic/platform/platform.json \
    -S /tmp/deployment-hwsku.json --print-data
```

These commands print configuration without applying it. Direct port generation requires `-k`.

### Selection and fallback

JSON precedence is `-S`, installed `hwsku.json`, then minimum generation.
Without `-p`, discovery prefers usable JSON when installed HWSKU JSON exists,
or when no matching INI exists on single-ASIC devices. Eligible CONFIG_DB data
can still be reused.
`-S` bypasses saved `PORT` but does not override INI discovery; use `-p` to force JSON.

**Minimum means one logical port per cage at the highest supported default speed.**
Cages without a one-port mode require an explicit selection. This fallback also
applies to other `get_port_config()` callers.

Only direct `-k` generation from installed `hwsku.json` emits `BREAKOUT_CFG`;
other output does not support `config interface breakout`.

### Minigraph

Minimum and explicit JSON layouts preserve speeds, lanes, and supplied
FEC/autonegotiation. On single-ASIC devices, missing channel/VLAN members,
interface parents, route/FG-ECMP references, or unresolved dataplane ACL attachments
cause errors, not topology repair. Legacy sources and multi-ASIC topology handling
are unchanged.

A missing HWSKU directory links to an existing base SKU directory (name without
the final hyphen-separated suffix). Link failures warn and parsing continues.
