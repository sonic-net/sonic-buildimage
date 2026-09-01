#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
reason=TEST
export reason
. "$SCRIPT_DIR/../snmpcommunity"

assert_valid()
{
    sonic_is_valid_snmp_community "$1" || {
        printf 'expected valid community: %s\n' "$1" >&2
        exit 1
    }
}

assert_invalid()
{
    if sonic_is_valid_snmp_community "$1"; then
        printf 'expected invalid community: %s\n' "$1" >&2
        exit 1
    fi
}

assert_valid 'public'
assert_valid 'site-1_ro'
assert_valid 'safe;id>/tmp/example'
assert_valid 'hash#colon:value'
assert_valid '01234567890123456789012345678901'

assert_invalid 'abc'
assert_invalid '012345678901234567890123456789012'
assert_invalid 'has space'
assert_invalid "has'quote"
assert_invalid 'has@sign'
assert_invalid 'has,comma'
assert_invalid 'has\backslash'
assert_invalid "line
break"

test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' 0 1 2 15

config=$test_dir/snmp.yml
printf '%s\n' \
    'snmp_location: lab' \
    'snmp_rocommunity: old-value' \
    'other_setting: true' > "$config"
chmod 0640 "$config"

sonic_write_snmp_community 'safe;id>/tmp/example' "$config"
[ "$(stat -c '%a' "$config")" = 640 ]
expected=$test_dir/expected.yml
printf '%s\n' \
    'snmp_location: lab' \
    "snmp_rocommunity: 'safe;id>/tmp/example'" \
    'other_setting: true' > "$expected"
cmp "$expected" "$config"

printf '%s\n' 'snmp_location: lab' > "$config"
sonic_write_snmp_community 'hash#colon:value' "$config"
printf '%s\n' \
    'snmp_location: lab' \
    "snmp_rocommunity: 'hash#colon:value'" > "$expected"
cmp "$expected" "$config"

rm -f "$config"
sonic_write_snmp_community 'site-1_ro' "$config"
printf '%s\n' "snmp_rocommunity: 'site-1_ro'" > "$expected"
cmp "$expected" "$config"
