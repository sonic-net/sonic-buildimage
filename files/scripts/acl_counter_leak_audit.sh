#!/bin/bash
# Audit (and optionally clean) orphaned ACL counter hashes in COUNTERS_DB.
# See sonic-net/sonic-sairedis#1982
#
# Usage:
#   acl_counter_leak_audit.sh                          # dry run, report only (default, safe)
#   acl_counter_leak_audit.sh --delete                  # confirm + UNLINK stale keys
#   acl_counter_leak_audit.sh --delete --stability-interval 300
#   acl_counter_leak_audit.sh --namespace asic0 ...     # multi-asic platforms
#
# WHAT THIS SCRIPT DOES
#   1. Reads the live ACL rule -> counter OID map from COUNTERS_DB
#      (ACL_COUNTER_RULE_MAP / COUNTERS_ACL_COUNTER_RULE_MAP).
#   2. Cross-checks against SAI_OBJECT_TYPE_ACL_COUNTER objects in ASIC_DB.
#   3. Enumerates COUNTERS:oid:* hashes in COUNTERS_DB and keeps only those
#      that are Redis hashes AND contain at least one SAI_ACL_COUNTER_ATTR_*
#      field (i.e. are corroborated as ACL counter hashes -- a bare OID
#      prefix match is never treated as proof).
#   4. Stale = corroborated ACL counter hash whose OID is absent from both
#      the name map and ASIC_DB.
#   5. In --delete mode, the whole pipeline above is executed a second time
#      after a configurable stability interval, and only OIDs that are
#      stale in BOTH passes are considered. Immediately before deleting,
#      a third, fresh live snapshot is taken and any key found alive is
#      dropped from the batch; if the live set shrinks unexpectedly the
#      run aborts rather than risk deleting live counters.
#
# SAFETY NOTES / ASSUMPTIONS
#   - This script talks to Redis exclusively through `sonic-db-cli`, using
#     the logical database names (ASIC_DB, COUNTERS_DB) rather than
#     hardcoded numeric indices, so it automatically respects the running
#     device's database_config.json (single/multi Redis instance,
#     unix-socket vs TCP, multi-ASIC namespaces via --namespace).
#   - Requires a `sonic-db-cli` build that supports `-j/--json` output
#     (used only for HGETALL/SCAN parsing); the script verifies this at
#     startup and aborts with a clear message otherwise.
#   - Uses SCAN/UNLINK (never KEYS/DEL) so it does not block the Redis
#     event loop shared with syncd/orchagent, and UNLINK is always invoked
#     with a bounded, non-empty batch of validated keys.
#   - Do NOT run --delete while ACL configuration is being changed, during
#     a warm/fast/cold reboot, or while orchagent/syncd is restarting.
#     Live counter creation/removal during that window can race with this
#     script's snapshots; the two-pass stability check and pre-delete
#     revalidation reduce but cannot fully eliminate this window.
#   - This targets a single Redis/namespace context per run. On multi-ASIC
#     platforms, run once per --namespace.
#
# RECOMMENDED PRODUCTION PROCEDURE
#   1. Run in default (dry-run) mode and review the report file.
#   2. Confirm independently (e.g. `show acl counters`) that the reported
#      stale OIDs do not correspond to configured ACL rules.
#   3. Confirm no ACL config changes / reboots / orchagent-syncd restarts
#      are in progress or planned during the run.
#   4. Re-run with --delete during a maintenance window; the script will
#      re-validate staleness after the stability interval before touching
#      anything, and will refuse to run if it cannot re-confirm liveness
#      data immediately before deleting.
#   5. After deletion, verify ACL counters and control-plane health.
#
# KNOWN LIMITATIONS
#   - Candidate enumeration issues one TYPE and one HKEYS call per
#     COUNTERS:oid:* key; on very large COUNTERS tables this can take
#     noticeable wall-clock time. This is a deliberate correctness/latency
#     trade-off for an offline audit tool, not a hot-path/control-plane
#     script.
#   - The stability check only compares two point-in-time snapshots; it
#     cannot detect state that flaps faster than the configured interval.

set -euo pipefail

PROG="$(basename "$0")"

DO_DELETE=0
NAMESPACE=""
STABILITY_INTERVAL=60
BATCH_SIZE=500
REPORT_FILE=""
MAX_BATCH_SIZE=1000

usage() {
    cat <<EOF
Usage: $PROG [options]

  --delete                    Actually UNLINK confirmed-stale keys (default: dry run).
  --namespace NS              Pass -n NS to sonic-db-cli (multi-ASIC platforms).
  --stability-interval SEC    Seconds between the two staleness confirmation
                              passes before deletion (default: ${STABILITY_INTERVAL}).
                              Ignored in dry-run mode.
  --batch-size N              Max keys per UNLINK batch (default: ${BATCH_SIZE},
                              max: ${MAX_BATCH_SIZE}).
  --report-file PATH          Where to write the persistent audit report
                              (default: a timestamped file under /tmp).
  -h, --help                  Show this help.

Safe to run in default (dry-run) mode on a live device: it only performs
read-only Redis operations. --delete is destructive; see the script header
comment for the recommended production procedure.
EOF
}

while (($# > 0)); do
    case "$1" in
        --delete) DO_DELETE=1; shift ;;
        --namespace) NAMESPACE="${2:?--namespace requires an argument}"; shift 2 ;;
        --stability-interval) STABILITY_INTERVAL="${2:?--stability-interval requires an argument}"; shift 2 ;;
        --batch-size) BATCH_SIZE="${2:?--batch-size requires an argument}"; shift 2 ;;
        --report-file) REPORT_FILE="${2:?--report-file requires an argument}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "ERROR: unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
done

if ! [[ "$STABILITY_INTERVAL" =~ ^[0-9]+$ ]]; then
    echo "ERROR: --stability-interval must be a non-negative integer" >&2
    exit 2
fi
if ! [[ "$BATCH_SIZE" =~ ^[0-9]+$ ]] || (( BATCH_SIZE < 1 )); then
    echo "ERROR: --batch-size must be a positive integer" >&2
    exit 2
fi
if (( BATCH_SIZE > MAX_BATCH_SIZE )); then
    echo "ERROR: --batch-size $BATCH_SIZE exceeds the safety cap of $MAX_BATCH_SIZE" >&2
    exit 2
fi

for c in sonic-db-cli python3 mktemp sort comm awk; do
    command -v "$c" >/dev/null 2>&1 || { echo "ERROR: required command '$c' not found in PATH" >&2; exit 1; }
done

if ! sonic-db-cli --help 2>&1 | grep -qE -- '-j, --json|--json'; then
    echo "ERROR: sonic-db-cli on this device does not appear to support -j/--json." >&2
    echo "       This script relies on JSON output to parse HGETALL/SCAN replies" >&2
    echo "       safely; refusing to fall back to fragile text parsing." >&2
    exit 1
fi

if [[ -z "$REPORT_FILE" ]]; then
    REPORT_FILE="/tmp/acl_counter_leak_audit-$(date +%Y%m%dT%H%M%S).report"
fi
: > "$REPORT_FILE"
chmod 600 "$REPORT_FILE" 2>/dev/null || true

WORKDIR=$(mktemp -d /tmp/aclcnt.XXXXXX)
chmod 700 "$WORKDIR"
cleanup() { rm -rf -- "$WORKDIR"; }
trap cleanup EXIT

report() { echo "$@" | tee -a "$REPORT_FILE"; }
die() {
    echo "ERROR: $*" >&2
    echo "ERROR: $*" >> "$REPORT_FILE"
    exit 1
}

NS_ARGS=()
[[ -n "$NAMESPACE" ]] && NS_ARGS=(-n "$NAMESPACE")

db_cli() { sonic-db-cli "${NS_ARGS[@]}" "$@"; }
db_cli_json() { sonic-db-cli -j "${NS_ARGS[@]}" "$@"; }

# --- 0. Validate the active Redis/database assumptions -------------------
for db in ASIC_DB COUNTERS_DB; do
    local_ping=""
    if ! local_ping=$(db_cli "$db" PING 2>&1); then
        die "could not reach $db via sonic-db-cli (namespace='${NAMESPACE:-<default>}'): $local_ping"
    fi
    [[ "$local_ping" == "True" ]] || die "unexpected PING reply from $db: '$local_ping' -- refusing to proceed against an unexpected Redis instance"
done
report "=== 0. Redis connectivity OK for ASIC_DB and COUNTERS_DB (namespace='${NAMESPACE:-<default>}') ==="

OID_RE='^oid:0x[0-9a-fA-F]+$'
ASIC_KEY_RE='^ASIC_STATE:SAI_OBJECT_TYPE_ACL_COUNTER:(oid:0x[0-9a-fA-F]+)$'
COUNTERS_KEY_RE='^COUNTERS:(oid:0x[0-9a-fA-F]+)$'

# --- helper: non-blocking SCAN, one matched key per line to $3 -----------
scan_keys() {
    local db="$1" pattern="$2" outfile="$3"
    local cursor="0" raw cursor_file
    cursor_file="$WORKDIR/.cursor.$$"
    : > "$outfile"
    while :; do
        raw=$(db_cli_json "$db" SCAN "$cursor" MATCH "$pattern" COUNT 1000) || die "SCAN against $db ('$pattern') failed"
        python3 - "$cursor_file" "$raw" >> "$outfile" <<'PYEOF'
import json
import sys

cursor_file, raw = sys.argv[1], sys.argv[2]
try:
    parsed = json.loads(raw)
except Exception as exc:  # noqa: BLE001
    sys.stderr.write("malformed SCAN reply: %s\n" % exc)
    sys.exit(1)
if not isinstance(parsed, list) or len(parsed) != 2 or not isinstance(parsed[1], list):
    sys.stderr.write("unexpected SCAN reply shape\n")
    sys.exit(1)
with open(cursor_file, "w") as fh:
    fh.write(str(parsed[0]))
for key in parsed[1]:
    print(key)
PYEOF
        cursor="$(cat "$cursor_file")"
        [[ "$cursor" =~ ^[0-9]+$ ]] || die "SCAN against $db returned a non-numeric cursor: '$cursor'"
        [[ "$cursor" == "0" ]] && break
    done
    rm -f "$cursor_file"
}

# --- helper: HGETALL a hash and print its *values*, one per line ---------
hgetall_values() {
    local db="$1" key="$2" raw
    raw=$(db_cli_json "$db" HGETALL "$key") || die "HGETALL $key against $db failed"
    python3 - "$raw" <<'PYEOF'
import json
import sys

raw = sys.argv[1]
try:
    parsed = json.loads(raw)
except Exception as exc:  # noqa: BLE001
    sys.stderr.write("malformed HGETALL reply: %s\n" % exc)
    sys.exit(1)
if not isinstance(parsed, dict):
    sys.stderr.write("unexpected HGETALL reply shape (expected an object)\n")
    sys.exit(1)
for value in parsed.values():
    print(value)
PYEOF
}

# --- 1 & 2 & 3. One audit pass: build map_oids/asic_oids/keep/candidates/stale
# Writes: <dir>/keep_oids, <dir>/candidate_oids, <dir>/stale_oids
run_audit_pass() {
    local dir="$1"
    mkdir -p "$dir"

    # --- name map -----------------------------------------------------
    local map_name=""
    for candidate in ACL_COUNTER_RULE_MAP COUNTERS_ACL_COUNTER_RULE_MAP; do
        local exists
        exists=$(db_cli COUNTERS_DB EXISTS "$candidate") || die "EXISTS $candidate failed"
        if [[ "$exists" == "1" ]]; then
            map_name="$candidate"
            break
        fi
    done
    [[ -n "$map_name" ]] || die "no ACL counter name map found in COUNTERS_DB (checked ACL_COUNTER_RULE_MAP, COUNTERS_ACL_COUNTER_RULE_MAP). Inspect manually with 'sonic-db-cli COUNTERS_DB KEYS *ACL_COUNTER*MAP*' before proceeding."

    local map_type
    map_type=$(db_cli COUNTERS_DB TYPE "$map_name") || die "TYPE $map_name failed"
    [[ "$map_type" == "hash" ]] || die "$map_name exists but is type '$map_type', not hash -- unexpected schema, aborting instead of silently ignoring it"

    hgetall_values COUNTERS_DB "$map_name" | sort -u > "$dir/map_values"
    if [[ -s "$dir/map_values" ]] && grep -vE "$OID_RE" "$dir/map_values" > "$dir/map_bad"; then
        [[ -s "$dir/map_bad" ]] && die "$map_name contains value(s) that are not well-formed OIDs: $(paste -sd, "$dir/map_bad")"
    fi
    cp "$dir/map_values" "$dir/live_map_oids"

    # --- ASIC_DB cross-check ------------------------------------------
    scan_keys ASIC_DB 'ASIC_STATE:SAI_OBJECT_TYPE_ACL_COUNTER:*' "$dir/asic_keys_raw"
    : > "$dir/live_asic_oids"
    if [[ -s "$dir/asic_keys_raw" ]]; then
        while IFS= read -r k; do
            [[ "$k" =~ $ASIC_KEY_RE ]] || die "unexpected key shape from ASIC_DB ACL counter scan: '$k'"
            echo "${BASH_REMATCH[1]}" >> "$dir/live_asic_oids"
        done < "$dir/asic_keys_raw"
    fi
    sort -u -o "$dir/live_asic_oids" "$dir/live_asic_oids"

    sort -u "$dir/live_map_oids" "$dir/live_asic_oids" > "$dir/keep_oids"
    [[ -s "$dir/keep_oids" ]] || die "keep-list (live ACL counter OIDs) is empty -- refusing to continue, this would classify every counter as stale"

    # --- candidate discovery: hash + ACL-specific field corroboration -
    scan_keys COUNTERS_DB 'COUNTERS:oid:*' "$dir/all_counter_keys"
    : > "$dir/candidate_oids"
    local skipped_non_hash=0 skipped_no_field=0
    if [[ -s "$dir/all_counter_keys" ]]; then
        while IFS= read -r key; do
            [[ "$key" =~ $COUNTERS_KEY_RE ]] || die "unexpected key shape from COUNTERS_DB oid scan: '$key'"
            local oid="${BASH_REMATCH[1]}"
            local ktype
            ktype=$(db_cli COUNTERS_DB TYPE "$key") || die "TYPE $key failed"
            if [[ "$ktype" != "hash" ]]; then
                skipped_non_hash=$((skipped_non_hash + 1))
                continue
            fi
            local fields
            fields=$(db_cli COUNTERS_DB HKEYS "$key") || die "HKEYS $key failed"
            if ! grep -q '^SAI_ACL_COUNTER_ATTR_' <<<"$fields"; then
                skipped_no_field=$((skipped_no_field + 1))
                continue
            fi
            echo "$oid" >> "$dir/candidate_oids"
        done < "$dir/all_counter_keys"
    fi
    sort -u -o "$dir/candidate_oids" "$dir/candidate_oids"
    echo "$skipped_non_hash" > "$dir/skipped_non_hash"
    echo "$skipped_no_field" > "$dir/skipped_no_field"

    comm -23 "$dir/candidate_oids" "$dir/keep_oids" > "$dir/stale_oids"
}

report "=== 1-4. Building ACL counter OID inventory (pass 1) ==="
run_audit_pass "$WORKDIR/pass1"
LIVE1=$(wc -l < "$WORKDIR/pass1/keep_oids")
CAND1=$(wc -l < "$WORKDIR/pass1/candidate_oids")
STALE1=$(wc -l < "$WORKDIR/pass1/stale_oids")
report "live OIDs (map + ASIC_DB union) : $LIVE1"
report "corroborated ACL counter hashes : $CAND1 (skipped non-hash: $(cat "$WORKDIR/pass1/skipped_non_hash"), skipped no ACL field: $(cat "$WORKDIR/pass1/skipped_no_field"))"
report "stale candidates (pass 1)       : $STALE1"

report
report "--- sample of stale candidates (first 5) ---"
head -5 "$WORKDIR/pass1/stale_oids" | while read -r o; do
    report "COUNTERS:$o"
done

report
report "--- sample of PRESERVED live counters (first 5) ---"
head -5 "$WORKDIR/pass1/keep_oids" | while read -r o; do
    report "COUNTERS:$o"
done

report
report "NOTE: --delete must never be run while ACL configuration is changing, or"
report "      during orchagent/syncd restarts, warm/fast/cold reboots. Live churn"
report "      can race with this script's snapshots even with the safeguards below."

if (( DO_DELETE == 0 )); then
    report
    report "DRY RUN -- nothing deleted."
    report "Full stale OID list: $WORKDIR/pass1/stale_oids (workdir removed on exit)"
    report "Persistent report  : $REPORT_FILE"
    report "Re-run with --delete once you have independently reviewed the above."
    exit 0
fi

if (( STALE1 == 0 )); then
    report
    report "No stale keys found on the first pass; nothing to delete. Exiting."
    exit 0
fi

report
report "=== 5. Stability check: waiting ${STABILITY_INTERVAL}s and re-auditing before any deletion ==="
sleep "$STABILITY_INTERVAL"

run_audit_pass "$WORKDIR/pass2"
LIVE2=$(wc -l < "$WORKDIR/pass2/keep_oids")
STALE2=$(wc -l < "$WORKDIR/pass2/stale_oids")
report "live OIDs (pass 2)  : $LIVE2"
report "stale candidates (pass 2) : $STALE2"

comm -12 "$WORKDIR/pass1/stale_oids" "$WORKDIR/pass2/stale_oids" > "$WORKDIR/stable_stale_oids"
HEALED=$(comm -23 "$WORKDIR/pass1/stale_oids" "$WORKDIR/pass2/stale_oids" | wc -l)
NEWLY_STALE=$(comm -13 "$WORKDIR/pass1/stale_oids" "$WORKDIR/pass2/stale_oids" | wc -l)
STABLE=$(wc -l < "$WORKDIR/stable_stale_oids")
report "stable stale (both passes) : $STABLE"
report "healed between passes (no longer stale, NOT deleted) : $HEALED"
report "newly stale in pass 2 (NOT deleted this run, needs its own stability window) : $NEWLY_STALE"

if (( STABLE == 0 )); then
    report
    report "0 OIDs remained stale across the stability window; nothing to delete. Exiting."
    exit 0
fi

report
report "=== 6. Final pre-delete revalidation (fresh live snapshot) ==="
mkdir -p "$WORKDIR/final"
FINAL_MAP_TMP="$WORKDIR/final_map_oids"
FINAL_ASIC_TMP="$WORKDIR/final_asic_oids"

# Re-derive the keep-list one more time, as close to deletion time as possible.
{
    map_name=""
    for candidate in ACL_COUNTER_RULE_MAP COUNTERS_ACL_COUNTER_RULE_MAP; do
        exists=$(db_cli COUNTERS_DB EXISTS "$candidate") || die "EXISTS $candidate failed (final check)"
        if [[ "$exists" == "1" ]]; then
            map_name="$candidate"
            break
        fi
    done
    [[ -n "$map_name" ]] || die "ACL counter name map disappeared before deletion -- aborting"
    map_type=$(db_cli COUNTERS_DB TYPE "$map_name") || die "TYPE $map_name failed (final check)"
    [[ "$map_type" == "hash" ]] || die "$map_name changed type to '$map_type' before deletion -- aborting"
    hgetall_values COUNTERS_DB "$map_name" | sort -u > "$FINAL_MAP_TMP"
}
scan_keys ASIC_DB 'ASIC_STATE:SAI_OBJECT_TYPE_ACL_COUNTER:*' "$WORKDIR/final_asic_keys_raw"
: > "$FINAL_ASIC_TMP"
if [[ -s "$WORKDIR/final_asic_keys_raw" ]]; then
    while IFS= read -r k; do
        [[ "$k" =~ $ASIC_KEY_RE ]] || die "unexpected ASIC_DB key shape in final check: '$k'"
        echo "${BASH_REMATCH[1]}" >> "$FINAL_ASIC_TMP"
    done < "$WORKDIR/final_asic_keys_raw"
fi
sort -u -o "$FINAL_ASIC_TMP" "$FINAL_ASIC_TMP"
sort -u "$FINAL_MAP_TMP" "$FINAL_ASIC_TMP" > "$WORKDIR/final/keep_oids"

FINAL_LIVE=$(wc -l < "$WORKDIR/final/keep_oids")
[[ "$FINAL_LIVE" -gt 0 ]] || die "final live snapshot came back empty -- refusing to delete; this would indicate the map/ASIC_DB became unreachable or emptied unexpectedly"

# Abort if the live set shrank drastically since pass 2: that is a sign of
# control-plane churn (syncd/orchagent restart, ASIC_DB reprogramming) racing
# with this script, not evidence that more counters are stale.
HALF_LIVE2=$(( (LIVE2 + 1) / 2 ))
if (( FINAL_LIVE < HALF_LIVE2 )); then
    die "live OID set shrank from $LIVE2 to $FINAL_LIVE immediately before deletion -- aborting, this looks like control-plane churn rather than genuine leaks"
fi

comm -23 "$WORKDIR/stable_stale_oids" "$WORKDIR/final/keep_oids" > "$WORKDIR/final_stale_oids"
FILTERED_OUT=$(comm -12 "$WORKDIR/stable_stale_oids" "$WORKDIR/final/keep_oids" | wc -l)
FINAL_STALE=$(wc -l < "$WORKDIR/final_stale_oids")
report "keys re-confirmed live at the last moment (excluded from deletion) : $FILTERED_OUT"
report "final confirmed-stale key count : $FINAL_STALE"

if (( FINAL_STALE == 0 )); then
    report
    report "0 stale keys remained after the final pre-delete revalidation; nothing to delete. Exiting."
    exit 0
fi

sed 's/^/COUNTERS:/' "$WORKDIR/final_stale_oids" > "$WORKDIR/final_stale_keys"
cp "$WORKDIR/final_stale_keys" "$REPORT_FILE.deleted_keys"
chmod 600 "$REPORT_FILE.deleted_keys" 2>/dev/null || true

report
report "=== 7. Deleting $FINAL_STALE stale keys (UNLINK, batches of $BATCH_SIZE) ==="
n=0
batch=()
delete_batch() {
    local -n _batch="$1"
    (( ${#_batch[@]} > 0 )) || return 0
    db_cli COUNTERS_DB UNLINK "${_batch[@]}" > /dev/null
}
while IFS= read -r key; do
    batch+=("$key")
    if (( ${#batch[@]} >= BATCH_SIZE )); then
        delete_batch batch
        n=$((n + ${#batch[@]}))
        report "  deleted $n/$FINAL_STALE"
        batch=()
        sleep 0.05
    fi
done < "$WORKDIR/final_stale_keys"
delete_batch batch
n=$((n + ${#batch[@]}))
report "  deleted $n/$FINAL_STALE"

report
report "done. Deleted key list preserved at: ${REPORT_FILE}.deleted_keys"
report "Persistent report: $REPORT_FILE"
