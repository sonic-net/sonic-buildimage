#!/bin/bash
# Unit/integration tests for acl_counter_leak_audit.sh
# Uses a mock sonic-db-cli that simulates ASIC_DB / COUNTERS_DB state so the
# script can be exercised end-to-end without a real Redis instance.
#
# Usage: bash files/scripts/tests/test_acl_counter_leak_audit.sh

set -euo pipefail

PASS=0
FAIL=0
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="${SCRIPT_DIR}/acl_counter_leak_audit.sh"

red()   { printf '\033[31m%s\033[0m' "$*"; }
green() { printf '\033[32m%s\033[0m' "$*"; }

assert_eq() {
  local test_name="$1" expected="$2" actual="$3"
  if [[ "$expected" == "$actual" ]]; then
    echo "  $(green PASS): ${test_name}"
    (( ++PASS ))
  else
    echo "  $(red FAIL): ${test_name}"
    echo "    expected: $(printf '%q' "$expected")"
    echo "    actual:   $(printf '%q' "$actual")"
    (( ++FAIL ))
  fi
}

assert_contains() {
  local test_name="$1" haystack="$2" needle="$3"
  if [[ "$haystack" == *"$needle"* ]]; then
    echo "  $(green PASS): ${test_name}"
    (( ++PASS ))
  else
    echo "  $(red FAIL): ${test_name}"
    echo "    expected to contain: $(printf '%q' "$needle")"
    (( ++FAIL ))
  fi
}

MOCK_DIR="$(mktemp -d)"
STATE_DIR="$(mktemp -d)"
cleanup() { rm -rf "${MOCK_DIR}" "${STATE_DIR}"; }
trap cleanup EXIT

# ── Mock sonic-db-cli ────────────────────────────────────────────────
# State is a JSON file: { "<DB>": { "<key>": {"type": "hash", "fields": {...}} } }
cat > "${MOCK_DIR}/sonic-db-cli" <<'MOCK_DBCLI'
#!/usr/bin/env python3
import fnmatch
import json
import os
import sys

STATE_FILE = os.environ["MOCK_STATE_FILE"]


def load():
    with open(STATE_FILE) as fh:
        return json.load(fh)


def save(state):
    with open(STATE_FILE, "w") as fh:
        json.dump(state, fh)


def main():
    args = sys.argv[1:]
    json_mode = False
    while args and args[0].startswith("-"):
        if args[0] in ("-j", "--json"):
            json_mode = True
            args = args[1:]
        elif args[0] in ("-n", "--namespace"):
            args = args[2:]
        elif args[0] in ("-s", "--unixsocket"):
            args = args[1:]
        elif args[0] in ("-h", "--help"):
            print("usage: sonic-db-cli [-h] [-s] [-j] [-n NAMESPACE] db_or_op [cmd ...]")
            print("  -j, --json  Print command result as JSON")
            return 0
        else:
            args = args[1:]

    if not args:
        return 1

    db = args[0]
    if len(args) == 1:
        if db == "PING":
            print("True")
        return 0

    cmd = args[1].upper()
    rest = args[2:]
    state = load()
    db_state = state.setdefault(db, {})

    if cmd == "PING":
        print("True")
    elif cmd == "EXISTS":
        print("1" if rest[0] in db_state else "0")
    elif cmd == "TYPE":
        obj = db_state.get(rest[0])
        print(obj["type"] if obj else "none")
    elif cmd == "HGETALL":
        obj = db_state.get(rest[0], {})
        fields = obj.get("fields", {})
        if json_mode:
            print(json.dumps(fields))
        else:
            for k, v in fields.items():
                print(k)
                print(v)
    elif cmd == "HKEYS":
        obj = db_state.get(rest[0], {})
        for k in obj.get("fields", {}):
            print(k)
    elif cmd == "SCAN":
        pattern = None
        if "MATCH" in rest:
            pattern = rest[rest.index("MATCH") + 1]
        keys = sorted(
            k for k in db_state
            if pattern is None or fnmatch.fnmatchcase(k, pattern)
        )
        if json_mode:
            print(json.dumps(["0", keys]))
        else:
            print("0")
            for k in keys:
                print(k)
    elif cmd == "UNLINK":
        count = 0
        for k in rest:
            if k in db_state:
                del db_state[k]
                count += 1
        save(state)
        print(count)
    else:
        sys.stderr.write("mock sonic-db-cli: unsupported command %s\n" % cmd)
        return 1
    return 0


sys.exit(main())
MOCK_DBCLI
chmod +x "${MOCK_DIR}/sonic-db-cli"

export PATH="${MOCK_DIR}:${PATH}"

write_state() {
  cat > "${STATE_DIR}/state.json"
}

run_script() {
  MOCK_STATE_FILE="${STATE_DIR}/state.json" "$SCRIPT" "$@"
}

# ── Fixture: 2 live ACL counters (in map + ASIC_DB), 1 stale ACL counter
# hash (corroborated by SAI_ACL_COUNTER_ATTR_* fields but absent from both),
# and 1 unrelated hash (port counter shape) that must never be treated as
# an ACL-counter candidate despite sharing the same "oid:0x..." key shape.
write_state <<'JSON'
{
  "COUNTERS_DB": {
    "ACL_COUNTER_RULE_MAP": {
      "type": "hash",
      "fields": {
        "ACL_TABLE|RULE1": "oid:0x1500000000001",
        "ACL_TABLE|RULE2": "oid:0x1500000000002"
      }
    },
    "COUNTERS:oid:0x1500000000001": {
      "type": "hash",
      "fields": {
        "SAI_ACL_COUNTER_ATTR_PACKETS": "10",
        "SAI_ACL_COUNTER_ATTR_BYTES": "1000"
      }
    },
    "COUNTERS:oid:0x1500000000002": {
      "type": "hash",
      "fields": {
        "SAI_ACL_COUNTER_ATTR_PACKETS": "20",
        "SAI_ACL_COUNTER_ATTR_BYTES": "2000"
      }
    },
    "COUNTERS:oid:0x1500000000099": {
      "type": "hash",
      "fields": {
        "SAI_ACL_COUNTER_ATTR_PACKETS": "0",
        "SAI_ACL_COUNTER_ATTR_BYTES": "0"
      }
    },
    "COUNTERS:oid:0x1000000000005": {
      "type": "hash",
      "fields": {
        "SAI_PORT_STAT_IF_IN_UCAST_PKTS": "5"
      }
    }
  },
  "ASIC_DB": {
    "ASIC_STATE:SAI_OBJECT_TYPE_ACL_COUNTER:oid:0x1500000000001": {"type": "string", "fields": {}},
    "ASIC_STATE:SAI_OBJECT_TYPE_ACL_COUNTER:oid:0x1500000000002": {"type": "string", "fields": {}}
  }
}
JSON

echo "=== dry run: correct classification, nothing deleted ==="
report_file="${STATE_DIR}/dryrun.report"
out="$(run_script --report-file "$report_file")"
rc=$?
assert_eq "dry run exits 0" "0" "$rc"
assert_contains "reports 2 live OIDs" "$out" "live OIDs (map + ASIC_DB union) : 2"
assert_contains "reports 3 corroborated ACL hashes (excludes port counter)" "$out" "corroborated ACL counter hashes : 3"
assert_contains "reports 1 stale candidate" "$out" "stale candidates (pass 1)       : 1"
assert_contains "identifies the correct stale key" "$out" "COUNTERS:oid:0x1500000000099"
assert_contains "dry run banner present" "$out" "DRY RUN -- nothing deleted."

still_present="$(python3 -c "
import json
state = json.load(open('${STATE_DIR}/state.json'))
print('yes' if 'COUNTERS:oid:0x1500000000099' in state['COUNTERS_DB'] else 'no')
")"
assert_eq "dry run does not delete the stale key" "yes" "$still_present"

echo ""
echo "=== delete mode: stale key removed, live keys preserved ==="
report_file2="${STATE_DIR}/delete.report"
out2="$(run_script --delete --stability-interval 1 --report-file "$report_file2")"
rc2=$?
assert_eq "delete run exits 0" "0" "$rc2"
assert_contains "reports stable stale count 1" "$out2" "stable stale (both passes) : 1"
assert_contains "reports final confirmed-stale count 1" "$out2" "final confirmed-stale key count : 1"
assert_contains "reports 1 deleted" "$out2" "deleted 1/1"

deleted_gone="$(python3 -c "
import json
state = json.load(open('${STATE_DIR}/state.json'))
print('gone' if 'COUNTERS:oid:0x1500000000099' not in state['COUNTERS_DB'] else 'present')
")"
assert_eq "stale key actually removed from COUNTERS_DB" "gone" "$deleted_gone"

live_kept="$(python3 -c "
import json
state = json.load(open('${STATE_DIR}/state.json'))
db = state['COUNTERS_DB']
present = all(k in db for k in ['COUNTERS:oid:0x1500000000001', 'COUNTERS:oid:0x1500000000002', 'COUNTERS:oid:0x1000000000005'])
print('yes' if present else 'no')
")"
assert_eq "live and non-ACL keys are preserved" "yes" "$live_kept"

assert_eq "deleted-keys report file is written" "COUNTERS:oid:0x1500000000099" "$(cat "${report_file2}.deleted_keys")"

echo ""
echo "=== zero-stale guard: delete mode exits cleanly with no stale keys ==="
write_state <<'JSON'
{
  "COUNTERS_DB": {
    "ACL_COUNTER_RULE_MAP": {
      "type": "hash",
      "fields": {
        "ACL_TABLE|RULE1": "oid:0x1500000000001"
      }
    },
    "COUNTERS:oid:0x1500000000001": {
      "type": "hash",
      "fields": {
        "SAI_ACL_COUNTER_ATTR_PACKETS": "10",
        "SAI_ACL_COUNTER_ATTR_BYTES": "1000"
      }
    }
  },
  "ASIC_DB": {
    "ASIC_STATE:SAI_OBJECT_TYPE_ACL_COUNTER:oid:0x1500000000001": {"type": "string", "fields": {}}
  }
}
JSON
report_file3="${STATE_DIR}/zero.report"
out3="$(run_script --delete --stability-interval 1 --report-file "$report_file3")"
rc3=$?
assert_eq "zero-stale delete run exits 0" "0" "$rc3"
assert_contains "reports nothing to delete" "$out3" "No stale keys found on the first pass; nothing to delete."

echo ""
echo "=== malformed map data is rejected, not silently filtered ==="
write_state <<'JSON'
{
  "COUNTERS_DB": {
    "ACL_COUNTER_RULE_MAP": {
      "type": "hash",
      "fields": {
        "ACL_TABLE|RULE1": "not-an-oid"
      }
    }
  },
  "ASIC_DB": {}
}
JSON
set +e
out4="$(run_script 2>&1)"
rc4=$?
set -e
assert_eq "malformed OID in map aborts (nonzero exit)" "1" "$rc4"
assert_contains "error mentions malformed OID" "$out4" "not well-formed OIDs"

echo ""
echo "=== wrong-type map is rejected ==="
write_state <<'JSON'
{
  "COUNTERS_DB": {
    "ACL_COUNTER_RULE_MAP": {
      "type": "string",
      "fields": {}
    }
  },
  "ASIC_DB": {}
}
JSON
set +e
out5="$(run_script 2>&1)"
rc5=$?
set -e
assert_eq "non-hash map aborts (nonzero exit)" "1" "$rc5"
assert_contains "error mentions unexpected schema" "$out5" "not hash"

echo ""
echo "=== missing map aborts with guidance ==="
write_state <<'JSON'
{
  "COUNTERS_DB": {},
  "ASIC_DB": {}
}
JSON
set +e
out6="$(run_script 2>&1)"
rc6=$?
set -e
assert_eq "missing map aborts (nonzero exit)" "1" "$rc6"
assert_contains "error mentions no map found" "$out6" "no ACL counter name map found"

echo ""
echo "=============================="
echo "Results: $(green "${PASS} passed"), $(red "${FAIL} failed")"
echo "=============================="

if (( FAIL > 0 )); then
  exit 1
fi
echo "PASS!!"
exit 0
