# sonic-config-engine

## Deploy-time port configuration with sonic-cfggen

`sonic-cfggen` generates `PORT` using the supported breakout modes in
`platform.json` and an HWSKU selection file passed with `-S` / `--hwsku-config`.
The selection file can be stored anywhere and uses the existing `hwsku.json` format:

```json
{
    "interfaces": {
        "Ethernet0": {"default_brkout_mode": "1x100G[40G]"}
    }
}
```

For each selected physical port (cage), cfggen creates logical ports with aliases,
lanes, speeds, indices, subports, and FEC where needed. Optional `fec`, `autoneg`,
and `role` settings are also supported. Cages left out of the selection are not
configured. Unreadable files and unsupported modes cause an error, not fallback.

Generate ports directly or through a matching minigraph:

```shell
sonic-cfggen -k Generic \
    -p /usr/share/sonic/platform/platform.json \
    -S /tmp/deployment-hwsku.json --var-json PORT

sonic-cfggen -m /etc/sonic/minigraph.xml \
    -p /usr/share/sonic/platform/platform.json \
    -S /tmp/deployment-hwsku.json --print-data
```

Use the HWSKU and file paths for your device. These commands print data without
applying configuration. Generating ports without a minigraph requires `-k`,
not just `-p` and `-S`.

### Selection and fallback

When using JSON, `-S` takes priority over installed `hwsku.json`; without either,
cfggen generates the minimum layout. Without `-p`, cfggen chooses `platform.json`
with a nonempty `interfaces` section if installed HWSKU JSON exists or no matching
`port_config.ini` exists. Otherwise it uses the INI file. `-S` does not affect
INI files; pass `-p` to choose JSON instead.

With `-k`, cfggen looks for a port configuration file before checking CONFIG_DB.
Passing `-S` uses that selection instead of saved `PORT` rows. Other callers of
the shared reader can still reuse CONFIG_DB data when no file paths are supplied
and the HWSKU matches, or no HWSKU was requested.

**Minimum means one logical port per cage at the highest supported default
speed**, not the lowest speed. Equal-speed ties use the mode name that comes
first alphabetically. A cage without a supported one-port mode causes an error,
even if multi-port modes exist. This requires a `platform.json` with breakout modes.

Fallback generates `PORT` but does not create `BREAKOUT_CFG`. External and
installed selections still provide their `BREAKOUT_CFG` entries.

Minigraph speed and lane handling is unchanged; references between configuration
tables are not fully checked. Missing HWSKU directories and Generic symlinks
are not created automatically.
