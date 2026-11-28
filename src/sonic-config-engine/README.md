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
Modes accept whitespace between option groups, such as `4x25G (4) [10G,1G]`.
Invalid or unreadable selections fail rather than fall back.

Generate ports directly:

```shell
sonic-cfggen -k Generic \
    -p /usr/share/sonic/platform/platform.json \
    -S /tmp/deployment-hwsku.json --var-json PORT
```

This command prints configuration without applying it. Direct port generation requires `-k`.

### Selection and fallback

JSON precedence is `-S`, installed HWSKU-specific `hwsku.json`, platform-root
`hwsku.json`, then minimum generation.
Without `-p`, HWSKU-specific profiles take priority over platform-root profiles,
with usable JSON preferred to INI within each location. When neither profile
exists, minimum generation is used. Eligible CONFIG_DB data can still be
reused.
`-S` bypasses saved `PORT` but does not override INI discovery; use `-p` to force JSON.

**Minimum means one logical port per cage at the highest supported default speed.**
Cages without a one-port mode require an explicit selection. This fallback also
applies to other `get_port_config()` callers. Interfaces with overlapping lane
assignments are not filtered.

Only direct `-k` generation from HWSKU-specific `hwsku.json` emits `BREAKOUT_CFG`;
explicit selections, platform-root selections and minimum generation do not
support `config interface breakout`.
