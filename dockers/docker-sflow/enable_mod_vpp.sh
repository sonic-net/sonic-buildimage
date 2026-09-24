#!/bin/bash
# On VPP platforms, load hsflowd mod_vpp so VPP sw_if_index in packet samples
# maps into the Linux ifIndex namespace. No-op on every other platform.

HSFLOWD_CONF=/etc/hsflowd.conf

[ "$(sonic-cfggen -y /etc/sonic/sonic_version.yml -v asic_type)" = "vpp" ] || exit 0
grep -qE '^\s*vpp\s*\{' "$HSFLOWD_CONF" && exit 0

sed -i '/^sflow {/a\  vpp { osIndex=on }' "$HSFLOWD_CONF"
