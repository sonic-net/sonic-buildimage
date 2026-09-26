#!/bin/bash

set -euo pipefail

script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
hook="${script_dir}/../config-migration-post-hooks.d/10-remove-legacy-public-snmp-community"
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/snmp-community-migration.XXXXXX")
trap 'rm -rf "${test_dir}"' EXIT

fail()
{
    echo "FAIL: $*" >&2
    exit 1
}

run_hook()
{
    SNMP_CONFIG="${test_dir}/snmp.yml" \
    CONFIG_DB_PATH="${test_dir}" \
    CONFIG_DB_FILES="config_db.json" \
    WARM_BOOT="${1:-false}" \
    SONIC_DB_CLI="${test_dir}/sonic-db-cli" \
        bash "${hook}"
}

printf '#!/bin/bash\ncase "$2" in\n HLEN) echo 1 ;;\n HGET) echo RO ;;\n DEL) echo "$*" >>"${SONIC_DB_LOG}" ;;\nesac\n' >"${test_dir}/sonic-db-cli"
chmod +x "${test_dir}/sonic-db-cli"
export SONIC_DB_LOG="${test_dir}/sonic-db.log"

printf 'snmp_rocommunity: public\nsnmp_location: rack-1\n' >"${test_dir}/snmp.yml"
printf '{"SNMP_COMMUNITY":{"public":{"TYPE":"RO"},"private":{"TYPE":"RW"}}}\n' >"${test_dir}/config_db.json"
run_hook
grep -Fxq 'snmp_location: rack-1' "${test_dir}/snmp.yml" || fail 'location was not preserved'
! grep -q '^snmp_rocommunity:' "${test_dir}/snmp.yml" || fail 'old YAML entry remains'
jq -e '.SNMP_COMMUNITY.public == null and .SNMP_COMMUNITY.private.TYPE == "RW"' "${test_dir}/config_db.json" >/dev/null || fail 'JSON migration was incorrect'

printf 'snmp_rocommunity: operator-value\n' >"${test_dir}/snmp.yml"
printf '{"SNMP_COMMUNITY":{"public":{"TYPE":"RO"}}}\n' >"${test_dir}/config_db.json"
run_hook
jq -e '.SNMP_COMMUNITY.public.TYPE == "RO"' "${test_dir}/config_db.json" >/dev/null || fail 'row changed without old marker'

printf 'snmp_rocommunity: public\n' >"${test_dir}/snmp.yml"
printf '{"SNMP_COMMUNITY":{"public":{"TYPE":"RO","SOURCE":"operator"}}}\n' >"${test_dir}/config_db.json"
run_hook
jq -e '.SNMP_COMMUNITY.public.SOURCE == "operator"' "${test_dir}/config_db.json" >/dev/null || fail 'extended row was removed'

printf 'snmp_rocommunity: public\n' >"${test_dir}/snmp.yml"
printf '{"SNMP_COMMUNITY":{"public":{"TYPE":"RO"}}}\n' >"${test_dir}/config_db.json"
: >"${SONIC_DB_LOG}"
run_hook true
grep -Fxq 'CONFIG_DB DEL SNMP_COMMUNITY|public' "${SONIC_DB_LOG}" || fail 'live row was not removed'
run_hook true
[ "$(wc -l <"${SONIC_DB_LOG}")" -eq 1 ] || fail 'migration was not idempotent'

# A failed live database update must keep the YAML marker for a later retry.
printf 'snmp_rocommunity: public\n' >"${test_dir}/snmp.yml"
printf '{"SNMP_COMMUNITY":{"public":{"TYPE":"RO"}}}\n' >"${test_dir}/config_db.json"
printf '#!/bin/bash\nexit 1\n' >"${test_dir}/sonic-db-cli"
if run_hook true; then
    fail 'database failure was ignored'
fi
grep -Fxq 'snmp_rocommunity: public' "${test_dir}/snmp.yml" || fail 'retry marker was lost'

printf 'snmp_rocommunity: public\n' >"${test_dir}/snmp.yml"
printf '{invalid json\n' >"${test_dir}/config_db.json"
if run_hook false 2>/dev/null; then
    fail 'invalid JSON was ignored'
fi
grep -Fxq 'snmp_rocommunity: public' "${test_dir}/snmp.yml" || fail 'marker was lost after invalid JSON'

echo 'PASS: legacy SNMP community migration'
