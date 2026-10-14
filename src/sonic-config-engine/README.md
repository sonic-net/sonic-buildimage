# Platform-JSON PORT generation

Direct generation requires an HWSKU name:

```bash
sonic-cfggen -k <hwsku> -p /path/to/platform.json \
    -S /path/to/hwsku.json --print-data
```

`-S` is the optional HWSKU selection file; lowercase `-s` selects a Redis socket.
Without `-k`, `-p/-S` alone does not trigger direct generation. Minigraph remains
a separate supported input path.

## Selection and fallback

On the platform-JSON file path, explicit `-S` selection wins over installed HWSKU
discovery. Both `PORT` and `BREAKOUT_CFG` use the same selection. Unsupported or
malformed selections fail explicitly; omitted cages remain omitted.

Without either selection file, each cage uses its highest-speed single-port mode,
with a lexical tie-breaker at equal speed. This shared fallback also applies to
minigraph and library callers of `get_port_config()`. It generates `PORT` only,
not `BREAKOUT_CFG`. A cage without a single-port mode aborts generation with the
interface and valid modes in the error; use an explicit selection on such platforms.

Single-port aliases may be lists or bare strings such as `"Eth1"`; expansion
preserves the complete alias. Capability matching retains semantic equality of
port count, supported-speed set and assigned lanes, rather than requiring equal
default speeds. Legacy INI parsing is unchanged.

Existing ConfigDB-first lookup remains for eligible callers without an explicit
port-config file. Printing generated data does not write ConfigDB. A fallback
layout is not a reconstruction of an absent deployment-specific HWSKU selection.
Dynamic breakout requires `BREAKOUT_CFG` in the applied configuration.

## Consumer follow-up

`sonic-platform-common`'s `SfpUtilHelper.read_porttab_mappings()` retries its
explicit `porttabfile` only after an empty initial lookup. A nonempty shared
fallback can suppress that retry. A separate consumer change must establish
source precedence deliberately while preserving ConfigDB-first behavior.
This config-engine change does not implement that cross-repository adjustment.

## Regression tests

In the normal SONiC build/test environment:

```bash
CFGGEN_UNIT_TESTING=2 python3 -m pytest -q tests/test_cfggen_platformJson.py
```

The focused cases isolate HWSKU discovery and cover explicit/installed selection,
fallback without `BREAKOUT_CFG`, string aliases, and multi-port-only failures.
