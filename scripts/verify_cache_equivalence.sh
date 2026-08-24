#!/bin/bash
#
# verify_cache_equivalence.sh — Deep Comparison of Cached vs Fresh Build Artifacts
#
# ═══════════════════════════════════════════════════════════════════════════════
# PURPOSE
# ═══════════════════════════════════════════════════════════════════════════════
#
# Takes two build output directories (typically Build B and Build C from
# run_poc_builds.sh) and produces a comprehensive equivalence report.
# Classifies every difference as cosmetic (known-benign) or semantic (real drift).
#
# If ALL differences are cosmetic → cache is safe to use.
# If ANY semantic differences exist → cache has bugs that need fixing.
#
# ═══════════════════════════════════════════════════════════════════════════════
# WHAT IT COMPARES (4 implemented levels)
# ═══════════════════════════════════════════════════════════════════════════════
#
# Level 1: .deb packages (target/debs/)
#   - SHA256 raw comparison (expected to differ due to timestamps)
#   - Extract and compare file-by-file (dpkg-deb -x/-e for data and control)
#   - ELF binaries: strip debug info then compare
#   - Online/copy debs: must match exactly (no compilation involved)
#
# Level 2: Python wheels (target/python-wheels/)
#   - SHA256 raw comparison (may differ due to ZIP metadata)
#   - Unzip and compare .py source files
#   - Compare METADATA and RECORD manifests
#   - Ignore .pyc timestamp differences
#
# Level 3: Docker images (target/*.gz)
#   - Primary: tar-based comparison (no Docker daemon required)
#     Extract .gz → compare manifest.json, config JSON (minus timestamps), layer.tar SHA256s
#   - Fallback: docker load + container-diff or docker inspect (if tar extraction fails)
#
# Level 5: Installer image (target/sonic-*.img.gz)
#   - Extract and compare partition contents
#
# ═══════════════════════════════════════════════════════════════════════════════
# USAGE
# ═══════════════════════════════════════════════════════════════════════════════
#
#   ./scripts/verify_cache_equivalence.sh --dir-a DIR_A --dir-b DIR_B [OPTIONS]
#
# Required:
#   --dir-a DIR    First build directory (e.g., poc-results/build-B)
#   --dir-b DIR    Second build directory (e.g., poc-results/build-C)
#
# Optional:
#   --level LEVELS       Comma-separated levels to compare: 1,2,3,5 (default: 1,2,3,5)
#   --output-dir DIR     Report output directory (default: ./poc-results/comparison/)
#   --diffoscope         Enable diffoscope deep analysis on semantic diffs (default: enabled)
#   --no-diffoscope      Disable diffoscope deep analysis on semantic diffs
#   --max-report-size N  diffoscope max report size in bytes (default: 50000000)
#   --timeout N          Timeout per file comparison in seconds (default: 300)
#   --json               Output results in JSON format
#   --verbose            Show detailed per-file comparison output
#   --quick              Skip deep analysis; only do SHA256 + file listing comparison
#   --baseline FILE      JSON baseline from a fresh-vs-fresh run; a SEMANTIC diff is
#                        downgraded to BASELINE_NONDETERMINISM only when its differing-member
#                        signature is a SUBSET of the baseline's for that artifact (inherent
#                        build non-determinism, not a cache bug) and so does not trigger FAIL.
#                        A NEW differing member (e.g. a stale executable) is never masked.
#   --generate-baseline  Run as a baseline generator (fresh-vs-fresh); records the
#                        inherent non-determinism set. Implies --json.
#
# Examples:
#   # Standard comparison of Build B vs Build C
#   ./scripts/verify_cache_equivalence.sh \
#       --dir-a ./poc-results/build-B \
#       --dir-b ./poc-results/build-C
#
#   # Debs only with diffoscope deep analysis
#   ./scripts/verify_cache_equivalence.sh \
#       --dir-a ./poc-results/build-B \
#       --dir-b ./poc-results/build-C \
#       --level 1 --diffoscope
#
#   # Quick hash-only check
#   ./scripts/verify_cache_equivalence.sh \
#       --dir-a ./poc-results/build-B \
#       --dir-b ./poc-results/build-C --quick
#
# ═══════════════════════════════════════════════════════════════════════════════
# INTERPRETING RESULTS
# ═══════════════════════════════════════════════════════════════════════════════
#
# Each artifact gets a classification:
#
#   IDENTICAL   = SHA256 matches exactly (no further analysis needed)
#   COSMETIC    = Differences exist but are all known-benign patterns:
#                 timestamps, gzip headers, ar metadata, .pyc magic, etc.
#   SEMANTIC    = Real content difference — build output actually differs
#   MISSING     = Artifact present in one build but not the other
#   ERROR       = Comparison failed (tool error, timeout, etc.)
#
# Summary exit codes:
#   0 = All artifacts IDENTICAL or COSMETIC (cache is safe)
#   1 = SEMANTIC differences found (cache has correctness issues)
#   2 = Script/environment error
#
# ═══════════════════════════════════════════════════════════════════════════════
# KNOWN COSMETIC PATTERNS (auto-whitelisted)
# ═══════════════════════════════════════════════════════════════════════════════
#
# These differences are expected and do NOT indicate cache bugs:
#   - ar archive header timestamps (every .deb has these)
#   - tar entry mtime in data.tar.* and control.tar.*
#   - gzip/pigz header timestamp bytes (decompressed content compared)
#   - Docker image "Created" timestamp in manifest.json
#   - Docker "Tag" and build-specific labels in config JSON
#   - Docker rootfs.diff_ids (content-addressed layer hashes change with timestamps)
#   - .pyc file header (4-byte timestamp + source size)
#   - Build path strings in ELF .comment section
#   - Go binary non-determinism (.note.go.buildid, .go.buildinfo, .text variance)
#   - File ordering in tar archives (non-deterministic in some tools)
#   - stdeb-generated debian/changelog date lines
#   - ZIP extra field timestamps in .whl files
#   - Embedded .whl in platform .debs (ZIP timestamps; .py source compared)
#   - Date-only differences in man pages and info files (.gz content)
#
# ═══════════════════════════════════════════════════════════════════════════════
# PREREQUISITES
# ═══════════════════════════════════════════════════════════════════════════════
#
# Required tools:
#   - dpkg-deb, sha256sum, tar, gzip (standard — always available)
#   - diff, find, sort (standard)
#
# Optional tools (enable deeper analysis):
#   - diffoscope: detailed recursive .deb comparison (apt install diffoscope)
#   - container-diff: Docker image comparison (go install github.com/GoogleContainerTools/container-diff)
#   - objcopy: ELF debug stripping (from binutils — usually available)
#   - unzip: .whl extraction (usually available)
#   - docker: Docker image loading and inspection
#   - unsquashfs: installer/root filesystem extraction (squashfs-tools)
#
# ═══════════════════════════════════════════════════════════════════════════════
# CONTEXT
# ═══════════════════════════════════════════════════════════════════════════════
#
# This script is part of the DPKG Cache Validation toolkit (Phase 3):
#   - Phase 1: audit_dep_completeness.sh, check_common_files.sh
#   - Phase 2: run_poc_builds.sh, run_negative_controls.sh
#   - Phase 3: verify_cache_equivalence.sh (this script), classify_diff.sh, dump_cache_keys.sh
#
# Exit codes:
#   0 = All differences cosmetic (cache is safe)
#   1 = Semantic differences found (cache has bugs)
#   2 = Script error or missing prerequisites
#

set -uo pipefail

# --- Configuration ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# Defaults
DIR_A=""
DIR_B=""
LEVELS="1,2,3,5"
OUTPUT_DIR="./poc-results/comparison"
USE_DIFFOSCOPE=true
MAX_REPORT_SIZE=50000000
TIMEOUT=300
JSON_OUTPUT=false
VERBOSE=false
QUICK_MODE=false
BASELINE_FILE=""
GENERATE_BASELINE=false

# Counters
TOTAL_ARTIFACTS=0
COUNT_IDENTICAL=0
COUNT_COSMETIC=0
COUNT_SEMANTIC=0
COUNT_MISSING=0
COUNT_ERROR=0
COUNT_BASELINE_NONDETERMINISM=0
COUNT_EXPECTED_MISSING=0

# Results array for JSON/report output
declare -a RESULTS=()

# --- Argument Parsing ---
usage() {
    echo "Usage: $0 --dir-a DIR --dir-b DIR [OPTIONS]"
    echo ""
    echo "Required:"
    echo "  --dir-a DIR          First build directory (e.g., poc-results/build-B)"
    echo "  --dir-b DIR          Second build directory (e.g., poc-results/build-C)"
    echo ""
    echo "Optional:"
    echo "  --level LEVELS       Levels to compare: 1,2,3,5 (default: 1,2,3,5)"
    echo "  --output-dir DIR     Report output (default: ./poc-results/comparison/)"
    echo "  --diffoscope         Enable diffoscope deep analysis on semantic diffs (default: enabled)"
    echo "  --max-report-size N  diffoscope report limit in bytes (default: 50MB)"
    echo "  --timeout N          Per-file timeout in seconds (default: 300)"
    echo "  --json               JSON output format"
    echo "  --verbose            Detailed per-file output"
    echo "  --quick              SHA256 + file-list only (skip deep analysis)"
    echo "  --baseline FILE      JSON baseline (fresh-vs-fresh); a SEMANTIC diff is downgraded to"
    echo "                       BASELINE_NONDETERMINISM only if its differing-member set is a subset"
    echo "                       of the baseline's for that artifact (a new member is never masked)"
    echo "  --generate-baseline  Generate a baseline file from a fresh-vs-fresh run (implies --json)"
    echo "  --help               Show this help"
    exit "${1:-1}"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --dir-a) DIR_A="$2"; shift 2 ;;
        --dir-b) DIR_B="$2"; shift 2 ;;
        --level) LEVELS="$2"; shift 2 ;;
        --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
        --diffoscope) USE_DIFFOSCOPE=true; shift ;;
        --no-diffoscope) USE_DIFFOSCOPE=false; shift ;;
        --max-report-size) MAX_REPORT_SIZE="$2"; shift 2 ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        --json) JSON_OUTPUT=true; shift ;;
        --verbose) VERBOSE=true; shift ;;
        --quick) QUICK_MODE=true; shift ;;
        --baseline) BASELINE_FILE="$2"; shift 2 ;;
        --generate-baseline) GENERATE_BASELINE=true; JSON_OUTPUT=true; shift ;;
        --help|-h) usage 0 ;;
        *) echo -e "${RED}Unknown option: $1${NC}"; exit 1 ;;
    esac
done

if [[ -z "$DIR_A" || -z "$DIR_B" ]]; then
    echo -e "${RED}ERROR: --dir-a and --dir-b are required${NC}"
    usage
fi

if [[ ! -d "$DIR_A" ]]; then
    echo -e "${RED}ERROR: Directory not found: $DIR_A${NC}"; exit 2
fi
if [[ ! -d "$DIR_B" ]]; then
    echo -e "${RED}ERROR: Directory not found: $DIR_B${NC}"; exit 2
fi

if [[ -n "$BASELINE_FILE" && ! -f "$BASELINE_FILE" ]]; then
    echo -e "${RED}ERROR: Baseline file not found: $BASELINE_FILE${NC}"; exit 2
fi

# Load baseline SEMANTIC artifacts into associative arrays for O(1) lookup.
# A baseline is produced from a fresh-vs-fresh (non-cached) run: any SEMANTIC
# diff there reflects inherent build non-determinism, not a cache bug. When a
# baseline is supplied, a SEMANTIC diff is downgraded only if its differing-member
# signature is a subset of the baseline's for that artifact (see record_result).
#   BASELINE_SEMANTICS[artifact]          -> artifact was SEMANTIC in baseline
#   BASELINE_SIG_TOKENS[artifact<TAB>tok] -> member token was non-deterministic in baseline
declare -A BASELINE_SEMANTICS=()
declare -A BASELINE_SIG_TOKENS=()
if [[ -n "$BASELINE_FILE" ]]; then
    while IFS=$'\t' read -r artifact sig; do
        [[ -z "$artifact" ]] && continue
        BASELINE_SEMANTICS["$artifact"]=1
        if [[ -n "$sig" ]]; then
            local_oldIFS="$IFS"; IFS=','
            for tok in $sig; do
                [[ -n "$tok" ]] && BASELINE_SIG_TOKENS["$artifact"$'\t'"$tok"]=1
            done
            IFS="$local_oldIFS"
        fi
    done < <(python3 -c "
import json, sys
with open(sys.argv[1]) as f:
    data = json.load(f)
for r in data.get('results', []):
    if r.get('classification') == 'SEMANTIC':
        print(r.get('artifact', '') + '\t' + r.get('signature', ''))
" "$BASELINE_FILE" 2>/dev/null)
    echo -e "${CYAN}[INFO]${NC} Loaded baseline with ${#BASELINE_SEMANTICS[@]} known non-deterministic artifact(s)"
fi

# --- Helper Functions ---
log_info() { echo -e "${CYAN}[INFO]${NC} $*"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }
log_success() { echo -e "${GREEN}[OK]${NC} $*"; }

# Scratch directory for generated helper scripts; cleaned up on exit.
HELPER_DIR=$(mktemp -d)
trap 'rm -rf "$HELPER_DIR"' EXIT

# Python helper: read a tar archive from stdin and print, one per line,
# "<path>\t<xattr-name>=<hex-value>" for every security.* extended attribute
# (e.g. security.capability set via setcap). Used to detect capability drift in
# .deb payloads without requiring privileged extraction.
CAPS_FROM_TAR_PY="$HELPER_DIR/caps_from_tar.py"
cat > "$CAPS_FROM_TAR_PY" <<'PY'
import sys, tarfile, binascii

out = []
try:
    tf = tarfile.open(fileobj=sys.stdin.buffer, mode='r|*')
    for m in tf:
        xattrs = {}
        for k, v in (m.pax_headers or {}).items():
            if k.startswith('SCHILY.xattr.security.'):
                name = k[len('SCHILY.xattr.'):]
                raw = v.encode('latin-1') if isinstance(v, str) else v
                xattrs[name] = binascii.hexlify(raw).decode()
        for name in sorted(xattrs):
            out.append(f"{m.name}\t{name}={xattrs[name]}")
except Exception:
    pass
print("\n".join(sorted(out)))
PY

# Python helper: flatten a docker-archive image into its FINAL filesystem.
#   argv: <extracted_image_dir> <dest_rootfs_dir> <metadata_manifest_out>
# Layers are applied in manifest order (honouring whiteouts and opaque dirs), so
# the merged tree reflects true "latest-layer-wins" precedence. The merged files
# are materialised under dest for content comparison, and a sidecar manifest
# records type/mode/uid/gid/linkname per path (ownership is read from tar headers
# so it survives unprivileged extraction).
FLATTEN_DOCKER_PY="$HELPER_DIR/flatten_docker.py"
cat > "$FLATTEN_DOCKER_PY" <<'PY'
import json, os, posixpath, shutil, stat, sys, tarfile

img_dir, dest, manifest_out = sys.argv[1], sys.argv[2], sys.argv[3]

with open(os.path.join(img_dir, 'manifest.json')) as fh:
    layers = json.load(fh)[0]['Layers']

meta = {}  # rel-path -> (type, mode, uid, gid, linkname)

def safe(rel):
    rel = rel.lstrip('/')
    parts = []
    for p in rel.split('/'):
        if p in ('', '.'):
            continue
        if p == '..':
            return None
        parts.append(p)
    return '/'.join(parts)

def remove(rel):
    full = os.path.join(dest, rel)
    meta.pop(rel, None)
    for k in list(meta):
        if k.startswith(rel + '/'):
            meta.pop(k, None)
    if os.path.islink(full) or os.path.isfile(full):
        try: os.remove(full)
        except OSError: pass
    elif os.path.isdir(full):
        shutil.rmtree(full, ignore_errors=True)

for layer in layers:
    with tarfile.open(os.path.join(img_dir, layer)) as tf:
        for m in tf:
            rel = safe(m.name)
            if rel is None:
                continue
            base = posixpath.basename(rel)
            parent = posixpath.dirname(rel)
            if base == '.wh..wh..opq':              # opaque dir: drop lower contents
                prefix = (parent + '/') if parent else ''
                for k in list(meta):
                    if prefix == '' or k.startswith(prefix):
                        if k != parent:
                            remove(k)
                continue
            if base.startswith('.wh.'):             # whiteout: delete target
                target = posixpath.join(parent, base[4:]) if parent else base[4:]
                remove(target)
                continue
            full = os.path.join(dest, rel)
            if (os.path.islink(full) or os.path.isfile(full)) and not m.isdir():
                try: os.remove(full)
                except OSError: pass
            os.makedirs(os.path.dirname(full) or dest, exist_ok=True)
            if m.isdir():
                os.makedirs(full, exist_ok=True)
                meta[rel] = ('d', stat.S_IMODE(m.mode), m.uid, m.gid, '')
            elif m.issym():
                if os.path.lexists(full):
                    try: os.remove(full)
                    except OSError: pass
                try: os.symlink(m.linkname, full)
                except OSError: pass
                meta[rel] = ('l', 0, m.uid, m.gid, m.linkname)
            elif m.islnk():                          # hardlink -> copy resolved content
                src = os.path.join(dest, safe(m.linkname) or '')
                try: shutil.copyfile(src, full)
                except OSError: pass
                meta[rel] = ('f', stat.S_IMODE(m.mode), m.uid, m.gid, '')
            elif m.isfile():
                try:
                    with tf.extractfile(m) as srcf, open(full, 'wb') as dst:
                        shutil.copyfileobj(srcf, dst)
                except Exception:
                    pass
                meta[rel] = ('f', stat.S_IMODE(m.mode), m.uid, m.gid, '')
            else:
                meta[rel] = ('o', stat.S_IMODE(m.mode), m.uid, m.gid, '')

with open(manifest_out, 'w') as out:
    for rel in sorted(meta):
        t, mode, uid, gid, link = meta[rel]
        out.write(f"{rel}\t{t}\t{mode:04o}\t{uid}\t{gid}\t{link}\n")
PY



# Record a comparison result
# Build a normalized signature of *what* differs, so baseline matching can key on the
# actual difference rather than only the artifact name. For member-level comparisons
# (e.g. .deb payloads) the caller passes the accumulated differing-member list; we reduce
# it to a sorted, de-duplicated, comma-joined token set with volatile annotations/counts
# stripped. When no member set is available (quick-mode SHA mismatch, docker/whl summaries)
# we fall back to the normalized detail string as a single stable token.
_diff_signature() {
    local diffset="$1" detail="$2"
    if [[ -n "$diffset" ]]; then
        printf '%s' "$diffset" \
            | sed 's/\\n/\n/g' \
            | sed -E 's/\([^)]*\)//g; s/^[[:space:]]+//; s/[[:space:]]+$//' \
            | grep -v '^$' \
            | sort -u | paste -sd, -
    else
        printf '%s' "$detail" \
            | sed -E 's/ \(\+[0-9]+ more\)//; s/[0-9]+ files/N files/; s/e\.g\.[^);]*//' \
            | sed -E 's/^[[:space:]]+//; s/[[:space:]]+$//'
    fi
}

# True when every differing member in the current signature was ALSO known-nondeterministic
# for this artifact in the fresh-vs-fresh baseline (i.e. the current diff is a subset of the
# baseline diff). A NEW differing member (e.g. a stale executable restored from cache) makes
# this false, so it is never downgraded/masked. An empty signature is treated as NOT a subset
# (fail-safe: keep it SEMANTIC).
_sig_subset_of_baseline() {
    local artifact="$1" sig="$2"
    [[ -z "$sig" ]] && return 1
    local tok oldIFS="$IFS"
    IFS=','
    for tok in $sig; do
        [[ -z "$tok" ]] && continue
        if [[ -z "${BASELINE_SIG_TOKENS["$artifact"$'\t'"$tok"]:-}" ]]; then
            IFS="$oldIFS"; return 1
        fi
    done
    IFS="$oldIFS"
    return 0
}

record_result() {
    local artifact="$1"
    local classification="$2"  # IDENTICAL, COSMETIC, SEMANTIC, MISSING, ERROR
    local detail="$3"
    local diffset="${4:-}"     # optional: raw accumulated differing-member list

    # Normalized signature of the actual difference (see _diff_signature).
    local sig
    sig=$(_diff_signature "$diffset" "$detail")

    # Downgrade SEMANTIC to BASELINE_NONDETERMINISM only when this artifact was SEMANTIC in
    # a fresh-vs-fresh baseline AND the current differing-member set is a SUBSET of the
    # baseline's known-nondeterministic members (keyed by artifact + normalized difference).
    # This prevents an unrelated baseline non-determinism (e.g. a doc timestamp) from masking
    # a real cache defect (e.g. a stale executable or config) in the same artifact.
    if [[ "$classification" == "SEMANTIC" && -n "$BASELINE_FILE" && -n "${BASELINE_SEMANTICS[$artifact]:-}" ]]; then
        if _sig_subset_of_baseline "$artifact" "$sig"; then
            classification="BASELINE_NONDETERMINISM"
            detail="[baseline-matched] $detail"
        fi
    fi

    # Downgrade MISSING to EXPECTED_MISSING for auto-generated debug-symbol
    # companion packages. read-cache restores only cache-registered derived .deb
    # targets; it does NOT re-emit the auto-generated *-dbgsym companions, so they
    # are legitimately absent and never part of the runtime image (non-fatal).
    # Deliberately narrow — vendor/runtime packages still FAIL on MISSING.
    if [[ "$classification" == "MISSING" && "$artifact" == *-dbgsym_*.deb ]]; then
        classification="EXPECTED_MISSING"
        detail="[dbgsym companion not restored by cache — non-fatal] $detail"
    fi

    RESULTS+=("$classification|$artifact|$detail@@SIG@@$sig")
    ((TOTAL_ARTIFACTS++))

    case "$classification" in
        IDENTICAL) ((COUNT_IDENTICAL++)) ;;
        COSMETIC)  ((COUNT_COSMETIC++)) ;;
        SEMANTIC)  ((COUNT_SEMANTIC++)) ;;
        MISSING)   ((COUNT_MISSING++)) ;;
        ERROR)     ((COUNT_ERROR++)) ;;
        BASELINE_NONDETERMINISM) ((COUNT_BASELINE_NONDETERMINISM++)) ;;
        EXPECTED_MISSING) ((COUNT_EXPECTED_MISSING++)) ;;
    esac

    if $VERBOSE; then
        local color="$NC"
        case "$classification" in
            IDENTICAL) color="$GREEN" ;;
            COSMETIC)  color="$YELLOW" ;;
            SEMANTIC)  color="$RED" ;;
            MISSING)   color="$RED" ;;
            ERROR)     color="$RED" ;;
            BASELINE_NONDETERMINISM) color="$YELLOW" ;;
            EXPECTED_MISSING) color="$YELLOW" ;;
        esac
        printf "  ${color}%-10s${NC} %s\n" "$classification" "$artifact"
        if [[ -n "$detail" && "$classification" != "IDENTICAL" ]]; then
            echo "             $detail"
        fi
    fi
}

# Check if a tool is available
has_tool() {
    command -v "$1" &>/dev/null
}

# Normalize away date/time strings and check whether two text files differ
# only because of build timestamps embedded in generated docs/changelogs.
content_diff_is_date_only() {
    local file_a="$1" file_b="$2"

    has_tool python3 || return 1

    python3 - "$file_a" "$file_b" <<'PY' 2>/dev/null
import pathlib
import re
import sys

text_a = pathlib.Path(sys.argv[1]).read_text(errors='replace')
text_b = pathlib.Path(sys.argv[2]).read_text(errors='replace')

month_names = (
    r'Jan(?:uary)?|Feb(?:ruary)?|Mar(?:ch)?|Apr(?:il)?|May|Jun(?:e)?|'
    r'Jul(?:y)?|Aug(?:ust)?|Sep(?:t(?:ember)?)?|Oct(?:ober)?|'
    r'Nov(?:ember)?|Dec(?:ember)?'
)
weekday_names = r'Mon|Tue|Wed|Thu|Fri|Sat|Sun'

patterns = [
    (re.compile(rf'\b(?:{weekday_names}),\s+\d{{1,2}}\s+(?:{month_names})\s+\d{{4}}\s+\d{{2}}:\d{{2}}:\d{{2}}\s+[+-]\d{{4}}\b', re.I), '<RFC2822_DATE>'),
    (re.compile(rf'\b\d{{1,2}}\s+(?:{month_names})\s+\d{{4}}\b', re.I), '<DATE>'),
    (re.compile(rf'\b(?:{month_names})\s+\d{{1,2}},\s+\d{{4}}\b', re.I), '<DATE>'),
    (re.compile(r'\b\d{4}-\d{2}-\d{2}\b'), '<DATE>'),
    (re.compile(r'\b\d{2}:\d{2}:\d{2}\b'), '<TIME>'),
]

def normalize(text: str) -> str:
    for pattern, replacement in patterns:
        text = pattern.sub(replacement, text)
    return text

sys.exit(0 if normalize(text_a) == normalize(text_b) else 1)
PY
}

# Return 0 only when two text files contain the SAME set of lines (i.e. the
# difference is purely line/block reordering, as produced by some code
# generators with non-deterministic import/field ordering). Added/removed or
# modified lines make the files genuinely different and return non-zero.
content_diff_is_reorder_only() {
    local file_a="$1" file_b="$2"
    # Both must be text
    grep -Iq . "$file_a" 2>/dev/null || return 1
    grep -Iq . "$file_b" 2>/dev/null || return 1
    diff <(sort "$file_a") <(sort "$file_b") >/dev/null 2>&1
}

# Documentation/changelog payloads from some SONiC package generators differ
# only by (a) an embedded build-date trailer and/or (b) non-deterministic
# ordering of otherwise identical entries (e.g. frr's auto-generated
# debian/changelog regroups the same patch set under [ Author ] blocks in a
# different order on each build). Normalize dates, then require both files to
# contain the EXACT SAME multiset of non-blank lines (reordering only). Any
# added, removed or modified content line still makes them genuinely different
# and returns non-zero, so a stale/changed changelog is still caught.
content_diff_is_date_or_reorder() {
    local file_a="$1" file_b="$2"

    has_tool python3 || return 1

    python3 - "$file_a" "$file_b" <<'PY' 2>/dev/null
import pathlib
import re
import sys

month_names = (
    r'Jan(?:uary)?|Feb(?:ruary)?|Mar(?:ch)?|Apr(?:il)?|May|Jun(?:e)?|'
    r'Jul(?:y)?|Aug(?:ust)?|Sep(?:t(?:ember)?)?|Oct(?:ober)?|'
    r'Nov(?:ember)?|Dec(?:ember)?'
)
weekday_names = r'Mon|Tue|Wed|Thu|Fri|Sat|Sun'

patterns = [
    (re.compile(rf'\b(?:{weekday_names}),\s+\d{{1,2}}\s+(?:{month_names})\s+\d{{4}}\s+\d{{2}}:\d{{2}}:\d{{2}}\s+[+-]\d{{4}}\b', re.I), '<RFC2822_DATE>'),
    (re.compile(rf'\b\d{{1,2}}\s+(?:{month_names})\s+\d{{4}}\b', re.I), '<DATE>'),
    (re.compile(rf'\b(?:{month_names})\s+\d{{1,2}},\s+\d{{4}}\b', re.I), '<DATE>'),
    (re.compile(r'\b\d{4}-\d{2}-\d{2}\b'), '<DATE>'),
    (re.compile(r'\b\d{2}:\d{2}:\d{2}\b'), '<TIME>'),
]

def normalize(path: str):
    text = pathlib.Path(path).read_text(errors='replace')
    for pattern, replacement in patterns:
        text = pattern.sub(replacement, text)
    # Compare the multiset of non-blank, right-stripped lines so pure reordering
    # (and blank-line churn around moved blocks) is ignored, but added/removed
    # or modified content lines are not. Debian changelog author-attribution
    # headers ("  [ Some Name ]") are dropped because the same patch set is
    # non-deterministically regrouped under a varying number of these headers.
    author_header = re.compile(r'^\s*\[ .+ \]\s*$')
    return sorted(
        l.rstrip() for l in text.splitlines()
        if l.strip() and not author_header.match(l)
    )

sys.exit(0 if normalize(sys.argv[1]) == normalize(sys.argv[2]) else 1)
PY
}

# /etc/shadow & /etc/gshadow (and their .bak counterparts) embed a per-account
# "last password change" value (field 3, sp_lstchg: days since epoch) that build
# tooling sets to the current build date when an account is created. Two builds
# on different calendar days therefore differ ONLY in that field while the hash,
# uid/gid and aging policy are identical. Treat a difference confined to that
# single field as cosmetic; any other field change (hash, policy, added/removed
# account) is genuine and returns non-zero.
shadow_diff_is_lastchange_only() {
    local file_a="$1" file_b="$2"

    has_tool python3 || return 1

    python3 - "$file_a" "$file_b" <<'PY' 2>/dev/null
import pathlib
import sys

def normalize(path: str):
    out = []
    for line in pathlib.Path(path).read_text(errors='replace').splitlines():
        fields = line.split(':')
        if len(fields) >= 3:
            fields[2] = '<LASTCHG>'   # blank only sp_lstchg (last change date)
        out.append(':'.join(fields))
    return out

sys.exit(0 if normalize(sys.argv[1]) == normalize(sys.argv[2]) else 1)
PY
}

# Return 0 when two binaries differ in at most $max_bytes individual byte
# positions (used for kernel/EFI images that embed a build timestamp but are
# otherwise identical). A larger byte delta indicates a real payload change.
binaries_differ_by_at_most() {
    local file_a="$1" file_b="$2" max_bytes="$3"
    # Different size => more than a localized timestamp changed.
    local sa sb
    sa=$(stat -c%s "$file_a" 2>/dev/null || echo 0)
    sb=$(stat -c%s "$file_b" 2>/dev/null || echo 0)
    [[ "$sa" == "$sb" ]] || return 1
    local nbytes
    nbytes=$(cmp -l "$file_a" "$file_b" 2>/dev/null | wc -l)
    [[ "${nbytes:-0}" -le "$max_bytes" ]]
}

# Extract an ELF section in a way that still works for foreign-architecture
# artifacts (e.g. ARM .ko / ELF files on an x86_64 host).
extract_elf_section_for_compare() {
    local file="$1" section="$2" out="$3"

    : > "$out"

    if has_tool objcopy; then
        if objcopy -O binary -j "$section" "$file" "$out" >/dev/null 2>&1 && [[ -s "$out" ]]; then
            return 0
        fi
        : > "$out"
        if objcopy --only-section="$section" -O binary "$file" "$out" >/dev/null 2>&1 && [[ -s "$out" ]]; then
            return 0
        fi
    fi

    if has_tool objdump; then
        objdump -s -j "$section" "$file" 2>/dev/null | tail -n +4 > "$out"
        [[ -s "$out" ]] && return 0
    fi

    return 1
}

# Compare executable code sections after metadata/debug normalization.
elf_code_sections_identical() {
    local file_a="$1" file_b="$2"
    local tmp_a tmp_b
    local found_section=false

    tmp_a=$(mktemp)
    tmp_b=$(mktemp)

    # Include initialized-data sections (.data/.data.rel.ro) in addition to code
    # and read-only data, so that a data-only payload change is NOT masked as
    # cosmetic. .bss is NOBITS (no file content) and is covered by the overall
    # strip-debug cmp performed by callers.
    for section in .text .init.text .rodata .data .data.rel.ro; do
        local have_a=false have_b=false
        if extract_elf_section_for_compare "$file_a" "$section" "$tmp_a"; then
            have_a=true
        fi
        if extract_elf_section_for_compare "$file_b" "$section" "$tmp_b"; then
            have_b=true
        fi

        if $have_a || $have_b; then
            if ! $have_a || ! $have_b; then
                rm -f "$tmp_a" "$tmp_b"
                return 1
            fi
            found_section=true
            if ! cmp -s "$tmp_a" "$tmp_b"; then
                rm -f "$tmp_a" "$tmp_b"
                return 1
            fi
        fi
    done

    rm -f "$tmp_a" "$tmp_b"
    $found_section
}

# Recursively compare an embedded .deb/.udeb payload.
embedded_deb_is_cosmetic() {
    local deb_a="$1" deb_b="$2"
    local tmp_a tmp_b
    local data_dir_a data_dir_b ctrl_dir_a ctrl_dir_b

    has_tool dpkg-deb || return 1

    tmp_a=$(mktemp -d)
    tmp_b=$(mktemp -d)
    data_dir_a="$tmp_a/data"
    data_dir_b="$tmp_b/data"
    ctrl_dir_a="$tmp_a/control"
    ctrl_dir_b="$tmp_b/control"
    mkdir -p "$data_dir_a" "$data_dir_b" "$ctrl_dir_a" "$ctrl_dir_b"

    dpkg-deb -x "$deb_a" "$data_dir_a" 2>/dev/null || { rm -rf "$tmp_a" "$tmp_b"; return 1; }
    dpkg-deb -x "$deb_b" "$data_dir_b" 2>/dev/null || { rm -rf "$tmp_a" "$tmp_b"; return 1; }
    dpkg-deb -e "$deb_a" "$ctrl_dir_a" 2>/dev/null || true
    dpkg-deb -e "$deb_b" "$ctrl_dir_b" 2>/dev/null || true

    local files_a files_b
    files_a=$(cd "$data_dir_a" && find . -type f | sort)
    files_b=$(cd "$data_dir_b" && find . -type f | sort)
    [[ "$files_a" == "$files_b" ]] || { rm -rf "$tmp_a" "$tmp_b"; return 1; }

    while IFS= read -r rel; do
        [[ -z "$rel" ]] && continue
        local fa="$data_dir_a/$rel" fb="$data_dir_b/$rel"
        if ! cmp -s "$fa" "$fb"; then
            if ! is_cosmetic_diff "$fa" "$fb" "$rel"; then
                rm -rf "$tmp_a" "$tmp_b"
                return 1
            fi
        fi
    done <<< "$files_a"

    local ctrl_files_a ctrl_files_b
    ctrl_files_a=$(cd "$ctrl_dir_a" 2>/dev/null && find . -type f | sort || true)
    ctrl_files_b=$(cd "$ctrl_dir_b" 2>/dev/null && find . -type f | sort || true)
    [[ "$ctrl_files_a" == "$ctrl_files_b" ]] || { rm -rf "$tmp_a" "$tmp_b"; return 1; }

    while IFS= read -r rel; do
        [[ -z "$rel" ]] && continue
        local cfa="$ctrl_dir_a/$rel" cfb="$ctrl_dir_b/$rel"
        if ! cmp -s "$cfa" "$cfb"; then
            if [[ "$rel" == "./control" ]]; then
                local ctrl_non_cosmetic
                ctrl_non_cosmetic=$(diff "$cfa" "$cfb" 2>/dev/null | grep "^[<>]" | grep -cvE "Build-Ids:|Installed-Size:" 2>/dev/null || true)
                if [[ "${ctrl_non_cosmetic:-0}" != "0" ]]; then
                    rm -rf "$tmp_a" "$tmp_b"
                    return 1
                fi
            elif ! is_cosmetic_diff "$cfa" "$cfb" "$rel"; then
                rm -rf "$tmp_a" "$tmp_b"
                return 1
            fi
        fi
    done <<< "$ctrl_files_a"

    rm -rf "$tmp_a" "$tmp_b"
    return 0
}

# ═══════════════════════════════════════════════════════════════════════════════
# LEVEL 1: .deb Package Comparison
# ═══════════════════════════════════════════════════════════════════════════════
compare_debs() {
    echo ""
    echo -e "${BOLD}━━━ Level 1: .deb Package Comparison ━━━${NC}"
    echo ""

    # Find all .deb files in both directories
    local debs_a debs_b
    debs_a=$(find "$DIR_A" \( -name "*.deb" -o -name "*.udeb" \) -type f 2>/dev/null | sort)
    debs_b=$(find "$DIR_B" \( -name "*.deb" -o -name "*.udeb" \) -type f 2>/dev/null | sort)

    if [[ -z "$debs_a" && -z "$debs_b" ]]; then
        log_warn "No .deb files found in either directory"
        return
    fi

    # Build relative-path maps (use path relative to DIR_A/DIR_B as key to handle
    # same-named packages in different subdirs like bookworm/ vs trixie/)
    declare -A map_a=() map_b=()
    while IFS= read -r path; do
        [[ -z "$path" ]] && continue
        local rel_key="${path#$DIR_A/}"
        map_a["$rel_key"]="$path"
    done <<< "$debs_a"

    while IFS= read -r path; do
        [[ -z "$path" ]] && continue
        local rel_key="${path#$DIR_B/}"
        map_b["$rel_key"]="$path"
    done <<< "$debs_b"

    # Compare matching files (match by relative path)
    local total_debs=0 identical_debs=0
    local all_keys
    all_keys=$(printf '%s\n' "${!map_a[@]}" "${!map_b[@]}" | sort -u)
    while IFS= read -r name; do
        [[ -z "$name" ]] && continue
        ((total_debs++))

        if [[ -z "${map_a[$name]:-}" ]]; then
            record_result "$name" "MISSING" "Only in dir-b ($name)"
            continue
        fi
        if [[ -z "${map_b[$name]:-}" ]]; then
            record_result "$name" "MISSING" "Only in dir-a ($name)"
            continue
        fi

        # Use the full relative-path key (e.g. bookworm/foo.deb) as the artifact
        # identity so same-named packages in different distro dirs (bookworm/ vs
        # trixie/) stay distinct in results and --baseline keying.
        local display_name="$name"

        # Quick SHA256 check
        local hash_a hash_b
        hash_a=$(sha256sum "${map_a[$name]}" | awk '{print $1}')
        hash_b=$(sha256sum "${map_b[$name]}" | awk '{print $1}')

        if [[ "$hash_a" == "$hash_b" ]]; then
            record_result "$display_name" "IDENTICAL" ""
            ((identical_debs++))
            continue
        fi

        # Hashes differ — need deeper analysis
        if $QUICK_MODE; then
            record_result "$display_name" "SEMANTIC" "SHA256 mismatch (quick mode — no deep analysis)"
            continue
        fi

        # Deep comparison: extract and compare contents
        compare_deb_deep "${map_a[$name]}" "${map_b[$name]}" "$display_name"
    done <<< "$all_keys"

    log_info "Level 1 summary: $total_debs debs, $identical_debs identical by hash"
}

# Deep .deb comparison (extract and compare file-by-file)
# Thin wrapper: own the temp dirs and clean them up exactly once, after the
# worker returns. A `trap ... RETURN` inside the worker is unsafe — under
# `set -T`/functrace it fires on every nested helper return (e.g.
# is_cosmetic_diff / elf_code_sections_identical), deleting the dirs
# mid-comparison, and it also leaks the trap to the caller.
compare_deb_deep() {
    local tmp_a tmp_b rc
    tmp_a=$(mktemp -d)
    tmp_b=$(mktemp -d)
    _compare_deb_deep_impl "$1" "$2" "$3" "$tmp_a" "$tmp_b"
    rc=$?
    rm -rf "$tmp_a" "$tmp_b"
    return $rc
}

_compare_deb_deep_impl() {
    local deb_a="$1" deb_b="$2" name="$3"
    local tmp_a="$4" tmp_b="$5"

    # Extract .deb files using dpkg-deb (handles any compression format)
    local data_dir_a="$tmp_a/data" data_dir_b="$tmp_b/data"
    local ctrl_dir_a="$tmp_a/control" ctrl_dir_b="$tmp_b/control"
    mkdir -p "$data_dir_a" "$data_dir_b" "$ctrl_dir_a" "$ctrl_dir_b"

    dpkg-deb -x "$deb_a" "$data_dir_a" 2>/dev/null || {
        record_result "$name" "ERROR" "Failed to extract deb_a data"
        return
    }
    dpkg-deb -x "$deb_b" "$data_dir_b" 2>/dev/null || {
        record_result "$name" "ERROR" "Failed to extract deb_b data"
        return
    }
    dpkg-deb -e "$deb_a" "$ctrl_dir_a" 2>/dev/null || true
    dpkg-deb -e "$deb_b" "$ctrl_dir_b" 2>/dev/null || true

    # Compare all files by content (ignoring timestamps/permissions)
    local semantic_diff=false
    local control_semantic=false
    local diff_details=""
    local diff_count=0

    # Get file lists
    local files_a files_b
    files_a=$(cd "$data_dir_a" && find . -type f | sort)
    files_b=$(cd "$data_dir_b" && find . -type f | sort)

    # Check for file list differences
    local list_diff
    list_diff=$(diff <(echo "$files_a") <(echo "$files_b") 2>/dev/null || true)
    if [[ -n "$list_diff" ]]; then
        # .build-id/ SYMLINK path differences are cosmetic (the path encodes the
        # binary's build-id hash, which legitimately changes per build). But a
        # one-sided .build-id/ REGULAR file is real content drift, and any
        # non-.build-id path difference is always semantic.
        local non_buildid_diffs
        non_buildid_diffs=$(echo "$list_diff" | grep "^[<>]" | grep -cv "\.build-id/" 2>/dev/null || true)
        non_buildid_diffs=${non_buildid_diffs:-0}
        local buildid_regular=0
        while IFS= read -r dl; do
            [[ "$dl" =~ ^([<>])\ (.*\.build-id/.*)$ ]] || continue
            local side="${BASH_REMATCH[1]}" rel="${BASH_REMATCH[2]}"
            local base="$data_dir_a"; [[ "$side" == ">" ]] && base="$data_dir_b"
            [[ -f "$base/$rel" && ! -L "$base/$rel" ]] && ((buildid_regular++))
        done < <(echo "$list_diff" | grep "^[<>]")
        if [[ $non_buildid_diffs -gt 0 || $buildid_regular -gt 0 ]]; then
            semantic_diff=true
            diff_details="File list differs"
            diff_count=$((diff_count + 1))
        fi
    fi

    # Compare file metadata: full mode (setuid/setgid/sticky included),
    # ownership (user/group), and symlink targets for every path. dpkg-deb -c
    # reports these from the package's tar headers (so ownership is accurate even
    # though extraction runs unprivileged). The size/date/time columns are
    # content-derived and stripped. .build-id/ entries are excluded (their names
    # and link targets encode the per-build build-id hash).
    if ! $semantic_diff; then
        local meta_a meta_b meta_diff
        meta_a=$(dpkg-deb -c "$deb_a" 2>/dev/null | grep -v '\.build-id/' \
            | awk '{out=$1" "$2; for(i=6;i<=NF;i++) out=out" "$i; print out}' | sort)
        meta_b=$(dpkg-deb -c "$deb_b" 2>/dev/null | grep -v '\.build-id/' \
            | awk '{out=$1" "$2; for(i=6;i<=NF;i++) out=out" "$i; print out}' | sort)
        meta_diff=$(diff <(echo "$meta_a") <(echo "$meta_b") 2>/dev/null || true)
        if [[ -n "$meta_diff" ]]; then
            local meta_diff_count
            meta_diff_count=$(echo "$meta_diff" | grep -c "^[<>]" || true)
            semantic_diff=true
            diff_details="${diff_details:+$diff_details; }File mode/ownership/symlink metadata differs (${meta_diff_count:-0} entries)"
            diff_count=$((diff_count + 1))
        fi
    fi

    # Compare file capabilities (security.capability xattr, e.g. from setcap).
    # Read directly from the data tar headers via python so unprivileged
    # extraction does not hide capability drift (a security-relevant change).
    if ! $semantic_diff && has_tool python3; then
        local caps_a caps_b
        caps_a=$(dpkg-deb --fsys-tarfile "$deb_a" 2>/dev/null | python3 "$CAPS_FROM_TAR_PY" 2>/dev/null || true)
        caps_b=$(dpkg-deb --fsys-tarfile "$deb_b" 2>/dev/null | python3 "$CAPS_FROM_TAR_PY" 2>/dev/null || true)
        if [[ "$caps_a" != "$caps_b" ]]; then
            semantic_diff=true
            diff_details="${diff_details:+$diff_details; }File capabilities (setcap/xattr) differ"
            diff_count=$((diff_count + 1))
        fi
    fi

    # Compare file contents (collect multiple diffs, not just the first — Gap 2 fix)
    # Limit to 10 semantic diffs to avoid excessive runtime on large packages
    local all_diff_files=""
    local max_semantic_diffs=10
    while IFS= read -r file; do
        [[ -z "$file" ]] && continue
        [[ $diff_count -ge $max_semantic_diffs ]] && break
        local fa="$data_dir_a/$file" fb="$data_dir_b/$file"
        if [[ -f "$fa" && -f "$fb" ]]; then
            if ! cmp -s "$fa" "$fb"; then
                # Check if it's an ELF binary — strip debug and compare
                if file "$fa" | grep -q "ELF"; then
                    if has_tool objcopy; then
                        local stripped_a stripped_b
                        stripped_a=$(mktemp)
                        stripped_b=$(mktemp)
                        # Detect Go binaries and apply Go-specific normalization (Gap 3/8 fix)
                        if readelf -S "$fa" 2>/dev/null | grep -q "\.note\.go\.buildid\|\.go\.buildinfo"; then
                            # Go binary: remove Go-specific non-deterministic sections
                            objcopy --strip-debug \
                                --remove-section=.note.go.buildid \
                                --remove-section=.go.buildinfo \
                                --remove-section=.note.gnu.build-id \
                                "$fa" "$stripped_a" 2>/dev/null || cp "$fa" "$stripped_a"
                            objcopy --strip-debug \
                                --remove-section=.note.go.buildid \
                                --remove-section=.go.buildinfo \
                                --remove-section=.note.gnu.build-id \
                                "$fb" "$stripped_b" 2>/dev/null || cp "$fb" "$stripped_b"
                            if ! cmp -s "$stripped_a" "$stripped_b"; then
                                # Go binary differs after stripping metadata sections.
                                # Check if .text+.rodata are identical (compiler non-determinism)
                                # vs genuine code change.
                                if elf_code_sections_identical "$fa" "$fb"; then
                                    all_diff_files="$all_diff_files  [GO-NONDETERMINISTIC] $file\n"
                                else
                                    # Code sections differ — this is a real change, flag SEMANTIC
                                    semantic_diff=true
                                    diff_count=$((diff_count + 1))
                                    all_diff_files="$all_diff_files  [GO-SEMANTIC] $file\n"
                                    if [[ -z "$diff_details" || "$diff_details" == "File list differs" ]]; then
                                        diff_details="Go binary code sections differ: $file"
                                    fi
                                fi
                            fi
                        else
                            # C/C++ binary: standard strip-debug + remove build-id
                            local strip_ok_a=false strip_ok_b=false
                            objcopy --strip-debug \
                                --remove-section=.note.gnu.build-id \
                                "$fa" "$stripped_a" 2>/dev/null && strip_ok_a=true || cp "$fa" "$stripped_a"
                            objcopy --strip-debug \
                                --remove-section=.note.gnu.build-id \
                                "$fb" "$stripped_b" 2>/dev/null && strip_ok_b=true || cp "$fb" "$stripped_b"
                            if ! { $strip_ok_a && $strip_ok_b && cmp -s "$stripped_a" "$stripped_b"; }; then
                                if elf_code_sections_identical "$fa" "$fb"; then
                                    # Code sections (.text, .init.text, .rodata) are identical —
                                    # remaining drift is in symbol tables, comments, or debug metadata.
                                    all_diff_files="$all_diff_files  [CODE-SECTIONS-IDENTICAL] $file\n"
                                else
                                    semantic_diff=true
                                    diff_count=$((diff_count + 1))
                                    all_diff_files="$all_diff_files  $file\n"
                                    if [[ -z "$diff_details" || "$diff_details" == "File list differs" ]]; then
                                        diff_details="ELF binary differs after stripping debug: $file"
                                    fi
                                fi
                            fi
                        fi
                        rm -f "$stripped_a" "$stripped_b"
                    else
                        semantic_diff=true
                        diff_count=$((diff_count + 1))
                        all_diff_files="$all_diff_files  $file\n"
                        if [[ -z "$diff_details" ]]; then
                            diff_details="Binary differs (no objcopy for strip): $file"
                        fi
                    fi
                else
                    # Non-ELF file differs — check known cosmetic patterns
                    if is_cosmetic_diff "$fa" "$fb" "$file"; then
                        : # cosmetic, skip
                    elif [[ "$file" == *.deb || "$file" == *.udeb ]] && embedded_deb_is_cosmetic "$fa" "$fb"; then
                        : # embedded package differs only in archive/compression metadata
                    # Recursively compare embedded wheels: ALL files in BOTH
                    # directions, compiled extensions via ELF sections, and
                    # RECORD integrity (handled by wheel_dirs_cosmetic).
                    elif [[ "$file" == *.whl ]] && has_tool unzip; then
                        local emb_a emb_b
                        emb_a=$(mktemp -d)
                        emb_b=$(mktemp -d)
                        unzip -q "$fa" -d "$emb_a" 2>/dev/null || true
                        unzip -q "$fb" -d "$emb_b" 2>/dev/null || true
                        if ! wheel_dirs_cosmetic "$emb_a" "$emb_b"; then
                            semantic_diff=true
                            diff_count=$((diff_count + 1))
                            all_diff_files="$all_diff_files  $file (embedded wheel payload differs)\n"
                            if [[ -z "$diff_details" ]]; then
                                diff_details="Embedded wheel payload differs: $file"
                            fi
                        fi
                        rm -rf "$emb_a" "$emb_b"
                    # Gap 15 fix: strip debug from static .a libraries
                    elif [[ "$file" == *.a ]] && has_tool objcopy; then
                        local sa sb
                        sa=$(mktemp)
                        sb=$(mktemp)
                        objcopy --strip-debug "$fa" "$sa" 2>/dev/null || cp "$fa" "$sa"
                        objcopy --strip-debug "$fb" "$sb" 2>/dev/null || cp "$fb" "$sb"
                        if ! cmp -s "$sa" "$sb"; then
                            semantic_diff=true
                            diff_count=$((diff_count + 1))
                            all_diff_files="$all_diff_files  $file\n"
                            if [[ -z "$diff_details" ]]; then
                                diff_details="Static library differs after stripping debug: $file"
                            fi
                        fi
                        rm -f "$sa" "$sb"
                    else
                        semantic_diff=true
                        diff_count=$((diff_count + 1))
                        all_diff_files="$all_diff_files  $file\n"
                        if [[ -z "$diff_details" ]]; then
                            diff_details="File content differs: $file"
                        fi
                    fi
                fi
            fi
        fi
    done <<< "$files_a"

    # Append count if multiple diffs found
    if [[ $diff_count -gt 1 ]]; then
        diff_details="$diff_details (+$((diff_count - 1)) more)"
    fi

    if $semantic_diff; then
        record_result "$name" "SEMANTIC" "$diff_details" "$all_diff_files"
    else
        # Data contents are identical — check control for semantic differences
        # (dependency fields, maintainer scripts are NOT cosmetic)
        local control_details=""

        if [[ -d "$ctrl_dir_a" && -d "$ctrl_dir_b" ]]; then

            # Check ALL control files (not just a fixed list) for semantic differences
            # This catches shlibs, symbols, md5sums, and any other DEBIAN/ metadata
            local all_ctrl_files
            all_ctrl_files=$(cd "$ctrl_dir_a" && find . -type f | sort; cd "$ctrl_dir_b" && find . -type f | sort)
            all_ctrl_files=$(echo "$all_ctrl_files" | sort -u)

            while IFS= read -r cf_rel; do
                [[ -z "$cf_rel" ]] && continue
                local cfa="$ctrl_dir_a/$cf_rel" cfb="$ctrl_dir_b/$cf_rel"
                # File exists in one but not the other
                if [[ -f "$cfa" && ! -f "$cfb" ]] || [[ ! -f "$cfa" && -f "$cfb" ]]; then
                    control_semantic=true
                    control_details="Control file '$cf_rel' present in one build but not the other"
                    break
                fi
                # Both exist but differ
                if [[ -f "$cfa" && -f "$cfb" ]] && ! cmp -s "$cfa" "$cfb"; then
                    # For 'control' file: check if only cosmetic fields differ
                    if [[ "$cf_rel" == "./control" ]]; then
                        local ctrl_non_cosmetic
                        # Build-Ids (ELF hashes) and Installed-Size (computed from content) are cosmetic
                        ctrl_non_cosmetic=$(diff "$cfa" "$cfb" 2>/dev/null | grep "^[<>]" | grep -cvE "Build-Ids:|Installed-Size:" 2>/dev/null || true)
                        if [[ "${ctrl_non_cosmetic:-0}" == "0" ]]; then
                            continue  # Only cosmetic control fields differ
                        fi
                    fi
                    # md5sums changes are cosmetic if data content comparison already passed
                    if [[ "$cf_rel" == "./md5sums" ]]; then
                        continue  # md5sums reflect content hashes we already compared
                    fi
                    control_semantic=true
                    control_details="Control file '$cf_rel' differs between builds"
                    break
                fi
            done <<< "$all_ctrl_files"
        fi

        if $control_semantic; then
            record_result "$name" "SEMANTIC" "$control_details"
        else
            record_result "$name" "COSMETIC" "ar/tar timestamp differences only"
        fi
    fi

    # Auto-run diffoscope on semantic diffs (if available) for detailed root-cause analysis
    if { $semantic_diff || $control_semantic; } && $USE_DIFFOSCOPE && has_tool diffoscope; then
        local diffoscope_report="$OUTPUT_DIR/diffoscope/${name}.html"
        mkdir -p "$(dirname "$diffoscope_report")"
        timeout "$TIMEOUT" diffoscope \
            --max-report-size "$MAX_REPORT_SIZE" \
            --html "$diffoscope_report" \
            "$deb_a" "$deb_b" 2>/dev/null || true
        if [[ -f "$diffoscope_report" ]]; then
            log_info "  diffoscope report: $diffoscope_report"
        fi
    fi
}

# Check if a file difference is known-cosmetic
# Compare two NESTED .deb/.udeb archives (e.g. a build-hooks deb embedded inside
# a docker rootfs or another package) at the payload level. The .deb container
# wraps its data in an ar archive + gzip/xz stream whose headers carry build
# timestamps, so byte-identical payloads routinely produce different container
# bytes. This decides cosmetic-vs-semantic on the REAL payload: file set, full
# mode/ownership/symlink metadata, capability xattrs, and (ELF-aware) content.
# Returns 0 when only container/timestamp noise differs.
nested_deb_cosmetic() {
    local deb_a="$1" deb_b="$2"
    has_tool dpkg-deb || return 1
    local da db rc=0
    da=$(mktemp -d); db=$(mktemp -d)
    if ! dpkg-deb -x "$deb_a" "$da" 2>/dev/null || ! dpkg-deb -x "$deb_b" "$db" 2>/dev/null; then
        rm -rf "$da" "$db"; return 1
    fi

    # File set (excluding per-build .build-id/ paths) must match.
    local files_a files_b
    files_a=$(cd "$da" && find . -type f | grep -v '\.build-id/' | sort)
    files_b=$(cd "$db" && find . -type f | grep -v '\.build-id/' | sort)
    if [[ "$files_a" != "$files_b" ]]; then
        rm -rf "$da" "$db"; return 1
    fi

    # Full mode / ownership / symlink metadata (size+date columns stripped).
    local meta_a meta_b
    meta_a=$(dpkg-deb -c "$deb_a" 2>/dev/null | grep -v '\.build-id/' \
        | awk '{out=$1" "$2; for(i=6;i<=NF;i++) out=out" "$i; print out}' | sort)
    meta_b=$(dpkg-deb -c "$deb_b" 2>/dev/null | grep -v '\.build-id/' \
        | awk '{out=$1" "$2; for(i=6;i<=NF;i++) out=out" "$i; print out}' | sort)
    if [[ "$meta_a" != "$meta_b" ]]; then
        rm -rf "$da" "$db"; return 1
    fi

    # Capability xattrs (setcap) — security-relevant, read from tar headers.
    if has_tool python3; then
        local caps_a caps_b
        caps_a=$(dpkg-deb --fsys-tarfile "$deb_a" 2>/dev/null | python3 "$CAPS_FROM_TAR_PY" 2>/dev/null || true)
        caps_b=$(dpkg-deb --fsys-tarfile "$deb_b" 2>/dev/null | python3 "$CAPS_FROM_TAR_PY" 2>/dev/null || true)
        if [[ "$caps_a" != "$caps_b" ]]; then
            rm -rf "$da" "$db"; return 1
        fi
    fi

    # Per-file content (ELF-aware so build-id-only drift stays cosmetic).
    local f
    while IFS= read -r f; do
        [[ -z "$f" ]] && continue
        local fa="$da/$f" fb="$db/$f"
        cmp -s "$fa" "$fb" && continue
        if ! regular_files_cosmetic "$fa" "$fb" "${f#./}"; then
            rc=1; break
        fi
    done <<< "$files_a"

    rm -rf "$da" "$db"
    return $rc
}

is_cosmetic_diff() {
    local file_a="$1" file_b="$2" rel_path="$3"

    # Nested Debian packages (e.g. build-hooks debs embedded in docker rootfs):
    # decide on the payload, ignoring ar/gzip container timestamp noise.
    if [[ "$rel_path" == *.deb || "$rel_path" == *.udeb || "$rel_path" == *.ddeb ]]; then
        if nested_deb_cosmetic "$file_a" "$file_b"; then
            return 0
        fi
        return 1
    fi

    # .pyc files — timestamp in header
    if [[ "$rel_path" == *.pyc ]]; then
        return 0  # cosmetic
    fi

    # /etc/shadow & /etc/gshadow: only the last-password-change date field drifts
    # between builds on different days; everything else (hash/policy) must match.
    case "$rel_path" in
        */etc/shadow|*/etc/shadow-|etc/shadow|etc/shadow-|\
        */etc/gshadow|*/etc/gshadow-|etc/gshadow|etc/gshadow-)
            if shadow_diff_is_lastchange_only "$file_a" "$file_b"; then
                return 0  # only the last-change date field differs
            fi
            return 1
            ;;
    esac

    # changelog files with date stamps (and non-deterministic entry ordering)
    if [[ "$rel_path" == *changelog* || "$rel_path" == *CHANGELOG* ]]; then
        if content_diff_is_date_or_reorder "$file_a" "$file_b"; then
            return 0  # only date and/or entry ordering differ
        fi
    fi

    # Build-id or build-path in text files
    if [[ "$rel_path" == *.buildinfo || "$rel_path" == *.changes ]]; then
        return 0  # cosmetic
    fi

    # .gz files — decompress and compare content; gzip headers contain timestamps
    if [[ "$rel_path" == *.gz ]] && has_tool gzip; then
        local ungz_a ungz_b
        ungz_a=$(mktemp)
        ungz_b=$(mktemp)
        if gzip -dc "$file_a" > "$ungz_a" 2>/dev/null && gzip -dc "$file_b" > "$ungz_b" 2>/dev/null; then
            if cmp -s "$ungz_a" "$ungz_b"; then
                rm -f "$ungz_a" "$ungz_b"
                return 0  # gunzipped content identical — gzip header timestamp only
            fi
            # Content differs. Only treat date/time drift as cosmetic for KNOWN
            # documentation payloads (man pages, info docs, changelogs). For any
            # other compressed payload (e.g. configuration, rules, data files) a
            # changed timestamp may be semantically meaningful, so do NOT mask it.
            case "$rel_path" in
                *changelog*|*CHANGELOG*)
                    # Changelogs additionally tolerate non-deterministic entry
                    # ordering (same patch set, reshuffled author blocks).
                    if content_diff_is_date_or_reorder "$ungz_a" "$ungz_b"; then
                        rm -f "$ungz_a" "$ungz_b"
                        return 0
                    fi
                    ;;
                */man/*|*.1.gz|*.2.gz|*.3.gz|*.4.gz|*.5.gz|*.6.gz|*.7.gz|*.8.gz|*.9.gz|\
                *.info.gz|*.info-[0-9]*.gz|*/doc/*|*NEWS.gz)
                    if content_diff_is_date_only "$ungz_a" "$ungz_b"; then
                        rm -f "$ungz_a" "$ungz_b"
                        return 0
                    fi
                    ;;
            esac
        fi
        rm -f "$ungz_a" "$ungz_b"
    fi

    # Kernel-related files with embedded build timestamps
    if [[ "$rel_path" == *utsversion.h || "$rel_path" == *utsrelease.h ]]; then
        # These are single-line header files with build timestamp — verify only that differs
        if content_diff_is_date_only "$file_a" "$file_b"; then
            return 0
        fi
        return 1  # Non-date content change in kernel header
    fi
    if [[ "$rel_path" == */boot/vmlinuz-* ]]; then
        # Kernel binary: an embedded build timestamp changes only a handful of
        # bytes. Same size alone is NOT sufficient proof (a same-size payload
        # change would slip through), so require the byte-level delta to be tiny.
        if binaries_differ_by_at_most "$file_a" "$file_b" 64; then
            return 0  # localized timestamp/build-id bytes only
        fi
        return 1  # real kernel change
    fi

    # GRUB EFI binaries with embedded build timestamps
    if [[ "$rel_path" == */monolithic/gcd*.efi || "$rel_path" == */monolithic/grub*.efi ]]; then
        if binaries_differ_by_at_most "$file_a" "$file_b" 64; then
            return 0  # localized build timestamp bytes only
        fi
        return 1  # real content change
    fi

    # Doxygen/auto-generated documentation (timestamps in HTML/info)
    if [[ "$rel_path" == */html/*.html || "$rel_path" == *_tree.html || "$rel_path" == *grub-dev.info* ]]; then
        return 0  # generated docs contain generation timestamps or non-deterministic ordering
    fi

    # Non-deterministic file ordering in generated outputs (YANG trees).
    # Cosmetic ONLY if the two files contain the same set of lines reordered;
    # added/removed/changed lines are semantic.
    if [[ "$rel_path" == *allyangs.tree || "$rel_path" == *allyangs.txt ]]; then
        if content_diff_is_reorder_only "$file_a" "$file_b"; then
            return 0
        fi
        return 1
    fi

    # Generated Go code from protobuf/YANG. Generator output ordering is
    # non-deterministic, but real content changes must still be flagged: only
    # treat as cosmetic when the difference is pure line reordering.
    if [[ "$rel_path" == *ocbinds*.go || "$rel_path" == *_generated.go || "$rel_path" == *.pb.go ]]; then
        if content_diff_is_reorder_only "$file_a" "$file_b"; then
            return 0
        fi
        return 1
    fi

    # JSON files with non-deterministic key ordering
    if [[ "$rel_path" == *.json ]] && has_tool python3; then
        if python3 -c 'import json,sys;a=json.load(open(sys.argv[1]));b=json.load(open(sys.argv[2]));sys.exit(0 if json.dumps(a,sort_keys=True)==json.dumps(b,sort_keys=True) else 1)' "$file_a" "$file_b" 2>/dev/null; then
            return 0  # JSON semantically identical, only key ordering differs
        fi
    fi

    return 1  # NOT cosmetic — treat as semantic
}

# Validate the integrity of a wheel's RECORD file: every regular file listed in
# RECORD must exist with a matching sha256. A RECORD that disagrees with the
# on-disk payload (hash/size drift, missing/extra entries) indicates a
# corrupted or tampered wheel and is reported as non-cosmetic.
# Returns 0 when RECORD is consistent (or absent), 1 when inconsistent.
wheel_record_consistent() {
    local root="$1"
    has_tool python3 || return 0
    python3 - "$root" <<'PY' 2>/dev/null
import base64, csv, hashlib, pathlib, sys

root = pathlib.Path(sys.argv[1])
records = list(root.glob('*.dist-info/RECORD'))
if not records:
    sys.exit(0)  # nothing to validate
for record in records:
    base = record.parent.parent
    with record.open(newline='') as fh:
        for row in csv.reader(fh):
            if not row:
                continue
            rel = row[0]
            digest = row[1] if len(row) > 1 else ''
            target = (base / rel)
            # RECORD lists itself with empty hash/size — skip integrity check.
            if not digest:
                continue
            if not digest.startswith('sha256='):
                continue
            try:
                data = target.read_bytes()
            except OSError:
                sys.exit(1)  # listed file missing
            want = digest.split('=', 1)[1]
            got = base64.urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b'=').decode()
            if got != want:
                sys.exit(1)  # hash mismatch — payload diverges from RECORD
sys.exit(0)
PY
}

# Boolean comparison of two already-extracted wheel directories. Returns 0 when
# the difference is purely cosmetic (zip/pyc timestamps, RECORD self-hash) and 1
# when a semantic difference exists. Checks ALL files in BOTH directions, uses
# ELF section comparison for compiled extensions, and validates RECORD integrity
# on each side.
wheel_dirs_cosmetic() {
    local dir_a="$1" dir_b="$2"

    # RECORD must be internally consistent on each side.
    wheel_record_consistent "$dir_a" || return 1
    wheel_record_consistent "$dir_b" || return 1

    # Files present in A: compare content with B.
    local file_a rel_path fb
    while IFS= read -r file_a; do
        [[ -z "$file_a" ]] && continue
        rel_path="${file_a#$dir_a/}"
        [[ "$rel_path" == *.pyc ]] && continue
        [[ "$rel_path" == */RECORD ]] && continue
        fb="$dir_b/$rel_path"
        [[ -f "$fb" ]] || return 1  # file missing in B
        if ! cmp -s "$file_a" "$fb"; then
            if [[ "$rel_path" == *.so || "$rel_path" == *.so.* ]]; then
                elf_code_sections_identical "$file_a" "$fb" || return 1
            elif ! is_cosmetic_diff "$file_a" "$fb" "$rel_path"; then
                return 1
            fi
        fi
    done < <(find "$dir_a" -type f)

    # Files present only in B.
    local file_b fa
    while IFS= read -r file_b; do
        [[ -z "$file_b" ]] && continue
        rel_path="${file_b#$dir_b/}"
        [[ "$rel_path" == *.pyc ]] && continue
        [[ "$rel_path" == */RECORD ]] && continue
        fa="$dir_a/$rel_path"
        [[ -f "$fa" ]] || return 1  # file missing in A
    done < <(find "$dir_b" -type f)

    return 0
}

# ═══════════════════════════════════════════════════════════════════════════════
# LEVEL 2: Python Wheel Comparison
# ═══════════════════════════════════════════════════════════════════════════════
compare_wheels() {
    echo ""
    echo -e "${BOLD}━━━ Level 2: Python Wheel Comparison ━━━${NC}"
    echo ""

    local wheels_a wheels_b
    wheels_a=$(find "$DIR_A" -name "*.whl" -type f 2>/dev/null | sort)
    wheels_b=$(find "$DIR_B" -name "*.whl" -type f 2>/dev/null | sort)

    if [[ -z "$wheels_a" && -z "$wheels_b" ]]; then
        log_warn "No .whl files found in either directory"
        return
    fi

    # Build relative-path maps
    declare -A whl_map_a=() whl_map_b=()
    while IFS= read -r path; do
        [[ -z "$path" ]] && continue
        whl_map_a["${path#$DIR_A/}"]="$path"
    done <<< "$wheels_a"

    while IFS= read -r path; do
        [[ -z "$path" ]] && continue
        whl_map_b["${path#$DIR_B/}"]="$path"
    done <<< "$wheels_b"

    local total_whls=0 identical_whls=0
    local all_whl_keys
    all_whl_keys=$(printf '%s\n' "${!whl_map_a[@]}" "${!whl_map_b[@]}" | sort -u)
    while IFS= read -r rel_key; do
        [[ -z "$rel_key" ]] && continue
        ((total_whls++))
        # Use the full relative-path key as the artifact identity so same-named
        # wheels in different distro dirs stay distinct in results / --baseline.
        local name="$rel_key"

        if [[ -z "${whl_map_a[$rel_key]:-}" ]]; then
            record_result "$name" "MISSING" "Only in dir-b"
            continue
        fi
        if [[ -z "${whl_map_b[$rel_key]:-}" ]]; then
            record_result "$name" "MISSING" "Only in dir-a"
            continue
        fi

        # Quick SHA256
        local hash_a hash_b
        hash_a=$(sha256sum "${whl_map_a[$rel_key]}" | awk '{print $1}')
        hash_b=$(sha256sum "${whl_map_b[$rel_key]}" | awk '{print $1}')

        if [[ "$hash_a" == "$hash_b" ]]; then
            record_result "$name" "IDENTICAL" ""
            ((identical_whls++))
            continue
        fi

        if $QUICK_MODE; then
            record_result "$name" "SEMANTIC" "SHA256 mismatch (quick mode)"
            continue
        fi

        # Deep comparison: unzip and compare
        compare_wheel_deep "${whl_map_a[$rel_key]}" "${whl_map_b[$rel_key]}" "$name"
    done <<< "$all_whl_keys"

    log_info "Level 2 summary: $total_whls wheels, $identical_whls identical by hash"
}

# Thin wrapper (see compare_deb_deep): own the temp dirs and clean up once,
# after the worker returns — avoids an unsafe `trap ... RETURN` in the worker.
compare_wheel_deep() {
    local name="$3"

    if ! has_tool unzip; then
        record_result "$name" "ERROR" "unzip not available for .whl extraction"
        return
    fi

    local tmp_a tmp_b rc
    tmp_a=$(mktemp -d)
    tmp_b=$(mktemp -d)
    _compare_wheel_deep_impl "$1" "$2" "$3" "$tmp_a" "$tmp_b"
    rc=$?
    rm -rf "$tmp_a" "$tmp_b"
    return $rc
}

_compare_wheel_deep_impl() {
    local whl_a="$1" whl_b="$2" name="$3"
    local tmp_a="$4" tmp_b="$5"

    unzip -q "$whl_a" -d "$tmp_a" 2>/dev/null || {
        record_result "$name" "ERROR" "Failed to unzip whl_a"
        return
    }
    unzip -q "$whl_b" -d "$tmp_b" 2>/dev/null || {
        record_result "$name" "ERROR" "Failed to unzip whl_b"
        return
    }

    # Compare .py source files (ignoring .pyc)
    local semantic_diff=false
    local diff_details=""

    # Compare METADATA
    local meta_a meta_b
    meta_a=$(find "$tmp_a" -name "METADATA" | head -1)
    meta_b=$(find "$tmp_b" -name "METADATA" | head -1)

    if [[ -n "$meta_a" && -n "$meta_b" ]]; then
        if ! cmp -s "$meta_a" "$meta_b"; then
            # Normalize only date fields in METADATA (Version IS semantic)
            local meta_norm_a meta_norm_b
            meta_norm_a=$(grep -vE "^(Date|Build-Date|Created):" "$meta_a" 2>/dev/null || cat "$meta_a")
            meta_norm_b=$(grep -vE "^(Date|Build-Date|Created):" "$meta_b" 2>/dev/null || cat "$meta_b")
            if [[ "$meta_norm_a" != "$meta_norm_b" ]]; then
                semantic_diff=true
                diff_details="METADATA differs (non-date fields)"
            fi
        fi
    elif [[ -n "$meta_a" || -n "$meta_b" ]]; then
        # METADATA exists on one side only — broken/incomplete wheel
        semantic_diff=true
        diff_details="METADATA present in only one wheel"
    fi

    # Compare ALL files in the wheel (both directions)
    # This catches .py, .so, .yaml, .json, .cfg, .pth, templates, data files, etc.
    if ! $semantic_diff; then
        while IFS= read -r file_a; do
            [[ -z "$file_a" ]] && continue
            local rel_path="${file_a#$tmp_a/}"
            local fb="$tmp_b/$rel_path"

            # Skip .pyc files and RECORD (installer metadata with hashes)
            [[ "$rel_path" == *.pyc ]] && continue
            [[ "$rel_path" == */RECORD ]] && continue

            if [[ ! -f "$fb" ]]; then
                semantic_diff=true
                diff_details="File missing in B: $rel_path"
                break
            fi
            if ! cmp -s "$file_a" "$fb"; then
                # For .so files, use ELF section comparison
                if [[ "$rel_path" == *.so ]] || [[ "$rel_path" == *.so.* ]]; then
                    if ! elf_code_sections_identical "$file_a" "$fb"; then
                        semantic_diff=true
                        diff_details="Compiled extension differs: $rel_path"
                        break
                    fi
                else
                    semantic_diff=true
                    diff_details="File content differs: $rel_path"
                    break
                fi
            fi
        done < <(find "$tmp_a" -type f)
    fi

    # Validate RECORD integrity on each side (hash/size drift, missing entries).
    if ! $semantic_diff; then
        if ! wheel_record_consistent "$tmp_a"; then
            semantic_diff=true
            diff_details="RECORD inconsistent with payload in wheel A"
        elif ! wheel_record_consistent "$tmp_b"; then
            semantic_diff=true
            diff_details="RECORD inconsistent with payload in wheel B"
        fi
    fi

    # Check for files in B that don't exist in A
    if ! $semantic_diff; then
        while IFS= read -r file_b; do
            [[ -z "$file_b" ]] && continue
            local rel_path="${file_b#$tmp_b/}"
            [[ "$rel_path" == *.pyc ]] && continue
            [[ "$rel_path" == */RECORD ]] && continue
            local fa="$tmp_a/$rel_path"
            if [[ ! -f "$fa" ]]; then
                semantic_diff=true
                diff_details="File missing in A: $rel_path"
                break
            fi
        done < <(find "$tmp_b" -type f)
    fi

    if $semantic_diff; then
        record_result "$name" "SEMANTIC" "$diff_details"
    else
        record_result "$name" "COSMETIC" "ZIP metadata/pyc timestamp differences only"
    fi
}

# ═══════════════════════════════════════════════════════════════════════════════
# LEVEL 3: Docker Image Comparison
# ═══════════════════════════════════════════════════════════════════════════════

# Return 0 when two regular files differ only cosmetically. ELF objects are
# compared after stripping debug/build-id (and, for the remaining drift, by
# code+data section equality); other files fall back to is_cosmetic_diff.
regular_files_cosmetic() {
    local fa="$1" fb="$2" rel="$3"
    cmp -s "$fa" "$fb" && return 0
    if file "$fa" 2>/dev/null | grep -q "ELF"; then
        if has_tool objcopy; then
            local sa sb rc=1
            sa=$(mktemp); sb=$(mktemp)
            if readelf -S "$fa" 2>/dev/null | grep -q "\.note\.go\.buildid\|\.go\.buildinfo"; then
                objcopy --strip-debug --remove-section=.note.go.buildid \
                    --remove-section=.go.buildinfo --remove-section=.note.gnu.build-id \
                    "$fa" "$sa" 2>/dev/null || cp "$fa" "$sa"
                objcopy --strip-debug --remove-section=.note.go.buildid \
                    --remove-section=.go.buildinfo --remove-section=.note.gnu.build-id \
                    "$fb" "$sb" 2>/dev/null || cp "$fb" "$sb"
            else
                objcopy --strip-debug --remove-section=.note.gnu.build-id "$fa" "$sa" 2>/dev/null || cp "$fa" "$sa"
                objcopy --strip-debug --remove-section=.note.gnu.build-id "$fb" "$sb" 2>/dev/null || cp "$fb" "$sb"
            fi
            if cmp -s "$sa" "$sb"; then
                rc=0
            elif elf_code_sections_identical "$fa" "$fb"; then
                rc=0
            fi
            rm -f "$sa" "$sb"
            return $rc
        fi
        return 1  # ELF differs and cannot normalize
    fi
    is_cosmetic_diff "$fa" "$fb" "$rel"
}

# Return 0 when a docker rootfs path holds inherently non-deterministic build
# state (logs, caches, package-manager bookkeeping, byte-compiled python), whose
# drift is cosmetic. Everything else (binaries, configs, scripts) is significant.
docker_path_is_noise() {
    local rel="$1"
    case "/$rel" in
        *.pyc|*/__pycache__/*) return 0 ;;
        */.wh.*|*cache.tgz) return 0 ;;
        */var/log/*|*/var/cache/*|*/var/tmp/*|*/tmp/*) return 0 ;;
        */var/lib/apt/*|*/var/lib/dpkg/*) return 0 ;;
        */etc/machine-id|*/var/lib/dbus/machine-id) return 0 ;;
        */ld.so.cache|*aux-cache) return 0 ;;
        */var/lib/systemd/*|*/var/lib/sonic-*) return 0 ;;
    esac
    return 1
}

# Flatten a docker-archive (already extracted to $1) into a merged rootfs under
# $2, writing a metadata manifest to $3. Returns non-zero on failure.
flatten_docker_image() {
    local img_dir="$1" dest="$2" manifest_out="$3"
    has_tool python3 || return 1
    mkdir -p "$dest"
    python3 "$FLATTEN_DOCKER_PY" "$img_dir" "$dest" "$manifest_out" 2>/dev/null
}


compare_dockers() {
    echo ""
    echo -e "${BOLD}━━━ Level 3: Docker Image Comparison ━━━${NC}"
    echo ""

    local images_a images_b
    images_a=$(find "$DIR_A" -name "*.gz" -path "*/docker*" -type f 2>/dev/null | sort)
    images_b=$(find "$DIR_B" -name "*.gz" -path "*/docker*" -type f 2>/dev/null | sort)

    # Also check top-level .gz files that are docker images
    if [[ -z "$images_a" ]]; then
        images_a=$(find "$DIR_A" -maxdepth 2 -name "docker-*.gz" -type f 2>/dev/null | sort)
    fi
    if [[ -z "$images_b" ]]; then
        images_b=$(find "$DIR_B" -maxdepth 2 -name "docker-*.gz" -type f 2>/dev/null | sort)
    fi

    if [[ -z "$images_a" && -z "$images_b" ]]; then
        log_warn "No Docker images found in either directory"
        return
    fi

    # Build filename maps
    declare -A img_map_a=() img_map_b=()
    while IFS= read -r path; do
        [[ -z "$path" ]] && continue
        img_map_a["$(basename "$path")"]="$path"
    done <<< "$images_a"

    while IFS= read -r path; do
        [[ -z "$path" ]] && continue
        img_map_b["$(basename "$path")"]="$path"
    done <<< "$images_b"

    local total_imgs=0 identical_imgs=0
    local all_img_keys
    all_img_keys=$(printf '%s\n' "${!img_map_a[@]}" "${!img_map_b[@]}" | sort -u)
    while IFS= read -r name; do
        [[ -z "$name" ]] && continue
        ((total_imgs++))

        if [[ -z "${img_map_a[$name]:-}" ]]; then
            record_result "$name" "MISSING" "Only in dir-b"
            continue
        fi
        if [[ -z "${img_map_b[$name]:-}" ]]; then
            record_result "$name" "MISSING" "Only in dir-a"
            continue
        fi

        # Quick SHA256 (Docker images almost always differ due to layer timestamps)
        local hash_a hash_b
        hash_a=$(sha256sum "${img_map_a[$name]}" | awk '{print $1}')
        hash_b=$(sha256sum "${img_map_b[$name]}" | awk '{print $1}')

        if [[ "$hash_a" == "$hash_b" ]]; then
            record_result "$name" "IDENTICAL" ""
            ((identical_imgs++))
            continue
        fi

        if $QUICK_MODE; then
            # Quick mode cannot prove equivalence for Docker images — flag conservatively
            record_result "$name" "SEMANTIC" "SHA256 differs (quick mode — deep analysis required to verify)"
            continue
        fi

        # Deep Docker comparison
        compare_docker_deep "${img_map_a[$name]}" "${img_map_b[$name]}" "$name"
    done <<< "$all_img_keys"

    log_info "Level 3 summary: $total_imgs images, $identical_imgs identical by hash"
}

compare_docker_deep() {
    local img_a="$1" img_b="$2" name="$3"

    # Try tar-based comparison first (no Docker dependency)
    if compare_docker_tar "$img_a" "$img_b" "$name"; then
        return
    fi

    # Fallback: use Docker if available
    if ! has_tool docker; then
        record_result "$name" "ERROR" "Tar-based comparison failed and Docker not available"
        return
    fi

    compare_docker_with_daemon "$img_a" "$img_b" "$name"
}

# Tar-based Docker image comparison (no Docker daemon required).
# Docker images (.gz) are gzipped tar archives containing:
#   - manifest.json (layer references and config pointer)
#   - <sha256>.json (image config: env, cmd, labels, timestamps)
#   - <layer-id>/layer.tar (filesystem layers)
compare_docker_tar() {
    local img_a="$1" img_b="$2" name="$3"
    local tmp_a tmp_b

    tmp_a=$(mktemp -d -p "${TMPDIR:-${OUTPUT_DIR:-/tmp}}" 2>/dev/null || mktemp -d) || return 1
    tmp_b=$(mktemp -d -p "${TMPDIR:-${OUTPUT_DIR:-/tmp}}" 2>/dev/null || mktemp -d) || { rm -rf "$tmp_a"; return 1; }

    # Extract both images
    if ! tar xzf "$img_a" -C "$tmp_a" 2>/dev/null; then
        rm -rf "$tmp_a" "$tmp_b"
        return 1
    fi
    if ! tar xzf "$img_b" -C "$tmp_b" 2>/dev/null; then
        rm -rf "$tmp_a" "$tmp_b"
        return 1
    fi

    # Compare manifest.json (layer ordering and references)
    local manifest_a manifest_b
    manifest_a=$(cat "$tmp_a/manifest.json" 2>/dev/null) || { rm -rf "$tmp_a" "$tmp_b"; return 1; }
    manifest_b=$(cat "$tmp_b/manifest.json" 2>/dev/null) || { rm -rf "$tmp_a" "$tmp_b"; return 1; }

    # Compare config JSON (normalize: remove timestamps)
    local config_a config_b
    config_a=$(python3 -c "
import json, sys, glob, os, re
mf = json.load(open('$tmp_a/manifest.json'))
cfg_file = os.path.join('$tmp_a', mf[0]['Config'])
cfg = json.load(open(cfg_file))
# Remove fields that are expected to differ (timestamps, image IDs)
for key in ['created', 'container', 'docker_version']:
    cfg.pop(key, None)
import re
if 'history' in cfg:
    for entry in cfg['history']:
        entry.pop('created', None)
        if 'created_by' in entry:
            entry['created_by'] = re.sub(r'(?i)image_version=[^\s]+', 'IMAGE_VERSION=NORMALIZED', entry['created_by'])
# Normalize labels: remove pipeline-specific labels (Gap 11 fix)
labels = cfg.get('config', {}).get('Labels', {})
for label_key in list(labels.keys()):
    # Remove build-specific labels that differ between pipeline runs
    if label_key in ('Tag', 'build_date', 'build_number', 'build_id'):
        del labels[label_key]
# Normalize Env: remove IMAGE_VERSION (contains pipeline ID and branch)
env_list = cfg.get('config', {}).get('Env', [])
cfg.get('config', {})['Env'] = [e for e in env_list if not e.startswith('IMAGE_VERSION=')]
# Normalize rootfs diff_ids — these are content-addressed layer hashes
# that will differ when layers contain different timestamps/logs (expected)
cfg.pop('rootfs', None)
print(json.dumps(cfg, sort_keys=True))
" 2>/dev/null) || { rm -rf "$tmp_a" "$tmp_b"; return 1; }

    config_b=$(python3 -c "
import json, sys, glob, os, re
mf = json.load(open('$tmp_b/manifest.json'))
cfg_file = os.path.join('$tmp_b', mf[0]['Config'])
cfg = json.load(open(cfg_file))
for key in ['created', 'container', 'docker_version']:
    cfg.pop(key, None)
if 'history' in cfg:
    for entry in cfg['history']:
        entry.pop('created', None)
        if 'created_by' in entry:
            entry['created_by'] = re.sub(r'(?i)image_version=[^\s]+', 'IMAGE_VERSION=NORMALIZED', entry['created_by'])
labels = cfg.get('config', {}).get('Labels', {})
for label_key in list(labels.keys()):
    if label_key in ('Tag', 'build_date', 'build_number', 'build_id'):
        del labels[label_key]
env_list = cfg.get('config', {}).get('Env', [])
cfg.get('config', {})['Env'] = [e for e in env_list if not e.startswith('IMAGE_VERSION=')]
cfg.pop('rootfs', None)
print(json.dumps(cfg, sort_keys=True))
" 2>/dev/null) || { rm -rf "$tmp_a" "$tmp_b"; return 1; }

    # Ordered list of layers as referenced by the manifest (detects same layer
    # SET applied in a different ORDER, which yields a different final fs).
    local layer_order_a layer_order_b
    layer_order_a=$(python3 -c "import json,hashlib,os;m=json.load(open('$tmp_a/manifest.json'))[0]['Layers'];print('\n'.join(hashlib.sha256(open(os.path.join('$tmp_a',l),'rb').read()).hexdigest() for l in m))" 2>/dev/null || echo "ERR_A")
    layer_order_b=$(python3 -c "import json,hashlib,os;m=json.load(open('$tmp_b/manifest.json'))[0]['Layers'];print('\n'.join(hashlib.sha256(open(os.path.join('$tmp_b',l),'rb').read()).hexdigest() for l in m))" 2>/dev/null || echo "ERR_B")

    local has_semantic_diff=false
    local diff_details=""

    # When the ordered layer hashes match exactly, the final filesystems are
    # byte-identical and only metadata (config/manifest) can differ — skip the
    # expensive flatten. Otherwise, flatten BOTH images to their final rootfs
    # (manifest order + whiteouts) and compare full content + metadata.
    if [[ "$layer_order_a" != "$layer_order_b" || "$layer_order_a" == ERR_A ]]; then
        local root_a="$tmp_a/_merged" root_b="$tmp_b/_merged"
        local meta_a="$tmp_a/_meta.tsv" meta_b="$tmp_b/_meta.tsv"

        if ! flatten_docker_image "$tmp_a" "$root_a" "$meta_a" \
            || ! flatten_docker_image "$tmp_b" "$root_b" "$meta_b"; then
            # Flatten unavailable (no python3) — cannot prove equivalence here.
            rm -rf "$tmp_a" "$tmp_b"
            return 1   # let the caller fall back to the docker-daemon path
        fi

        # 1) Metadata comparison: path presence, type, mode, owner, symlink
        #    target. Ignore inherently non-deterministic build-state paths.
        local meta_diff
        meta_diff=$(diff "$meta_a" "$meta_b" 2>/dev/null | grep "^[<>]" || true)
        if [[ -n "$meta_diff" ]]; then
            local meta_sig=0
            while IFS= read -r line; do
                [[ -z "$line" ]] && continue
                local rel="${line#? }"; rel="${rel%%$'\t'*}"
                docker_path_is_noise "$rel" || ((meta_sig++))
            done <<< "$meta_diff"
            if [[ $meta_sig -gt 0 ]]; then
                has_semantic_diff=true
                diff_details="Filesystem metadata differs ($meta_sig path/mode/owner/symlink entries)"
            fi
        fi

        # 2) Content comparison over the merged rootfs for files present on
        #    both sides; ELF-/cosmetic-aware. Bidirectional presence is already
        #    covered by the metadata diff above.
        if ! $has_semantic_diff; then
            local content_sig=0 first_sig=""
            while IFS= read -r fa; do
                [[ -z "$fa" ]] && continue
                local rel="${fa#$root_a/}"
                docker_path_is_noise "$rel" && continue
                local fb="$root_b/$rel"
                [[ -f "$fb" ]] || continue          # presence handled by metadata diff
                cmp -s "$fa" "$fb" && continue
                if ! regular_files_cosmetic "$fa" "$fb" "$rel"; then
                    ((content_sig++))
                    [[ -z "$first_sig" ]] && first_sig="$rel"
                    [[ $content_sig -ge 10 ]] && break
                fi
            done < <(find "$root_a" -type f)
            if [[ $content_sig -gt 0 ]]; then
                has_semantic_diff=true
                diff_details="Filesystem content differs ($content_sig files, e.g. $first_sig)"
            fi
        fi
    fi

    # Check config (env, cmd, entrypoint, labels — excluding timestamps)
    if [[ "$config_a" != "$config_b" ]]; then
        has_semantic_diff=true
        diff_details="${diff_details:+$diff_details; }Config differs (Env/Cmd/Labels/Entrypoint)"
    fi

    # Determine result
    if [[ "$has_semantic_diff" == "true" ]]; then
        record_result "$name" "SEMANTIC" "$diff_details"
    elif [[ "$layer_order_a" != "$layer_order_b" || "$manifest_a" != "$manifest_b" ]]; then
        # Final filesystem + config are equivalent; only timestamps/metadata in
        # the archive differ.
        record_result "$name" "COSMETIC" "Layer/manifest timestamps only; merged filesystem equivalent (tar-based)"
    else
        record_result "$name" "IDENTICAL" "All layers and config match (tar-based)"
    fi

    rm -rf "$tmp_a" "$tmp_b"
    return 0
}

# Docker daemon-based comparison (original approach, used as fallback)
compare_docker_with_daemon() {
    local img_a="$1" img_b="$2" name="$3"

    local tag_a="verify-cache-a/${name%.gz}:test"
    local tag_b="verify-cache-b/${name%.gz}:test"

    docker load -i "$img_a" 2>/dev/null | grep -oP "Loaded image: \K.*" > /dev/null || {
        local loaded_id
        loaded_id=$(docker load -i "$img_a" 2>/dev/null | grep -oP "Loaded image ID: sha256:\K[a-f0-9]+" | head -1)
        if [[ -n "$loaded_id" ]]; then
            docker tag "$loaded_id" "$tag_a" 2>/dev/null || true
        else
            record_result "$name" "ERROR" "Failed to load Docker image A"
            return
        fi
    }

    docker load -i "$img_b" 2>/dev/null | grep -oP "Loaded image: \K.*" > /dev/null || {
        local loaded_id
        loaded_id=$(docker load -i "$img_b" 2>/dev/null | grep -oP "Loaded image ID: sha256:\K[a-f0-9]+" | head -1)
        if [[ -n "$loaded_id" ]]; then
            docker tag "$loaded_id" "$tag_b" 2>/dev/null || true
        else
            record_result "$name" "ERROR" "Failed to load Docker image B"
            return
        fi
    }

    # Compare using container-diff if available
    if has_tool container-diff; then
        local diff_output="$OUTPUT_DIR/container-diff/${name}.txt"
        mkdir -p "$(dirname "$diff_output")"
        timeout "$TIMEOUT" container-diff diff \
            "daemon://$tag_a" "daemon://$tag_b" \
            --type=file --type=apt 2>/dev/null > "$diff_output" || true

        if [[ -s "$diff_output" ]]; then
            local pkg_diffs
            pkg_diffs=$(grep -c "^-\|^+" "$diff_output" 2>/dev/null || echo "0")
            if [[ $pkg_diffs -gt 0 ]]; then
                record_result "$name" "SEMANTIC" "container-diff found $pkg_diffs differences (see $diff_output)"
            else
                record_result "$name" "COSMETIC" "Docker layer timestamps only"
            fi
        else
            record_result "$name" "COSMETIC" "No filesystem/package differences detected"
        fi
    else
        # Fallback: compare docker inspect (normalized)
        local inspect_a inspect_b
        inspect_a=$(docker inspect "$tag_a" 2>/dev/null | python3 -c "
import json, sys
data = json.load(sys.stdin)[0]['Config']
for key in ['Hostname', 'Image']:
    data.pop(key, None)
print(json.dumps(data, sort_keys=True, indent=2))
" 2>/dev/null || echo "ERROR")

        inspect_b=$(docker inspect "$tag_b" 2>/dev/null | python3 -c "
import json, sys
data = json.load(sys.stdin)[0]['Config']
for key in ['Hostname', 'Image']:
    data.pop(key, None)
print(json.dumps(data, sort_keys=True, indent=2))
" 2>/dev/null || echo "ERROR")

        if [[ "$inspect_a" == "$inspect_b" ]]; then
            record_result "$name" "COSMETIC" "docker inspect configs match (timestamp diffs only)"
        elif [[ "$inspect_a" == "ERROR" || "$inspect_b" == "ERROR" ]]; then
            record_result "$name" "ERROR" "Failed to inspect Docker images"
        else
            record_result "$name" "SEMANTIC" "Docker config differs (Env/Cmd/Labels/etc.)"
        fi
    fi

    # Cleanup loaded images
    docker rmi "$tag_a" "$tag_b" 2>/dev/null || true
}

# ═══════════════════════════════════════════════════════════════════════════════
# LEVEL 5: INSTALLER IMAGE COMPARISON
# ═══════════════════════════════════════════════════════════════════════════════
compare_installers() {
    echo ""
    echo -e "${BOLD}━━━ Level 5: Installer Image Comparison ━━━${NC}"
    echo ""

    local images_a images_b
    images_a=$(find "$DIR_A" -maxdepth 2 \( -name "sonic-*.img.gz" -o -name "sonic-*.bin" \) -type f 2>/dev/null | sort)
    images_b=$(find "$DIR_B" -maxdepth 2 \( -name "sonic-*.img.gz" -o -name "sonic-*.bin" \) -type f 2>/dev/null | sort)

    if [[ -z "$images_a" && -z "$images_b" ]]; then
        log_warn "No installer images found in either directory"
        return
    fi

    # Match by filename
    local names_a names_b
    names_a=$(echo "$images_a" | xargs -I{} basename {} | sort)
    names_b=$(echo "$images_b" | xargs -I{} basename {} | sort)

    local common_names
    common_names=$(comm -12 <(echo "$names_a") <(echo "$names_b"))

    # Report missing
    local only_a only_b
    only_a=$(comm -23 <(echo "$names_a") <(echo "$names_b"))
    only_b=$(comm -13 <(echo "$names_a") <(echo "$names_b"))
    while IFS= read -r name; do
        [[ -z "$name" ]] && continue
        record_result "$name" "MISSING" "Only in A"
    done <<< "$only_a"
    while IFS= read -r name; do
        [[ -z "$name" ]] && continue
        record_result "$name" "MISSING" "Only in B"
    done <<< "$only_b"

    # Compare common images
    while IFS= read -r name; do
        [[ -z "$name" ]] && continue
        local file_a file_b
        file_a=$(find "$DIR_A" -maxdepth 2 -name "$name" -type f | head -1)
        file_b=$(find "$DIR_B" -maxdepth 2 -name "$name" -type f | head -1)

        echo -n "  Comparing $name ... "

        # Quick SHA check
        local sha_a sha_b
        sha_a=$(sha256sum "$file_a" | cut -d' ' -f1)
        sha_b=$(sha256sum "$file_b" | cut -d' ' -f1)

        if [[ "$sha_a" == "$sha_b" ]]; then
            echo "IDENTICAL"
            record_result "$name" "IDENTICAL" "SHA256 match"
            continue
        fi

        # Deep comparison: extract and compare squashfs contents
        compare_installer_deep "$name" "$file_a" "$file_b"
    done <<< "$common_names"
}

compare_installer_deep() {
    local name="$1" file_a="$2" file_b="$3"
    local tmp_a tmp_b
    # Use OUTPUT_DIR parent for temp (in case /tmp is full)
    local tmpbase="${TMPDIR:-${OUTPUT_DIR:-/tmp}}"
    mkdir -p "$tmpbase" 2>/dev/null || tmpbase="/tmp"
    tmp_a=$(mktemp -d -p "$tmpbase")
    tmp_b=$(mktemp -d -p "$tmpbase")

    local is_img_gz=false
    [[ "$name" == *.img.gz ]] && is_img_gz=true

    local has_semantic=false
    local diff_details=""

    if $is_img_gz; then
        # .img.gz: decompress, find squashfs, extract file/package lists.
        # Decompression MUST succeed — a failed/partial gunzip would otherwise
        # let two corrupt images look "equal" (e.g. both empty) and PASS.
        if ! gunzip -c "$file_a" > "$tmp_a/image.img" 2>/dev/null; then
            echo "ERROR (gunzip A failed)"
            record_result "$name" "ERROR" "Failed to decompress installer image A (corrupt .img.gz?)"
            rm -rf "$tmp_a" "$tmp_b"
            return
        fi
        if ! gunzip -c "$file_b" > "$tmp_b/image.img" 2>/dev/null; then
            echo "ERROR (gunzip B failed)"
            record_result "$name" "ERROR" "Failed to decompress installer image B (corrupt .img.gz?)"
            rm -rf "$tmp_a" "$tmp_b"
            return
        fi
        if [[ ! -s "$tmp_a/image.img" || ! -s "$tmp_b/image.img" ]]; then
            echo "ERROR (empty decompressed image)"
            record_result "$name" "ERROR" "Decompressed installer image is empty (corrupt .img.gz?)"
            rm -rf "$tmp_a" "$tmp_b"
            return
        fi

        # Extract squashfs from raw image (offset varies; use unsquashfs -s to find it)
        # The squashfs is typically at a known offset or we can scan for the magic bytes
        local sqfs_a sqfs_b
        sqfs_a=$(extract_installer_squashfs "$tmp_a/image.img" "$tmp_a")
        sqfs_b=$(extract_installer_squashfs "$tmp_b/image.img" "$tmp_b")

        if [[ -z "$sqfs_a" || -z "$sqfs_b" ]]; then
            # Fallback: compare decompressed image sizes
            local size_a size_b
            size_a=$(stat -c%s "$tmp_a/image.img" 2>/dev/null || echo "0")
            size_b=$(stat -c%s "$tmp_b/image.img" 2>/dev/null || echo "0")
            if [[ "$size_a" == "$size_b" ]]; then
                # Same size — check if identical
                if cmp -s "$tmp_a/image.img" "$tmp_b/image.img"; then
                    echo "COSMETIC (gzip header only)"
                    record_result "$name" "COSMETIC" "Decompressed content identical; gzip header differs"
                else
                    # Same size but different content and we can't extract — flag as SEMANTIC
                    # (we have no way to prove it's cosmetic without payload inspection)
                    echo "SEMANTIC (content differs, extraction unavailable)"
                    record_result "$name" "SEMANTIC" "Decompressed content differs ($size_a bytes); squashfs extraction unavailable for deeper analysis"
                fi
            else
                # Different decompressed image size with no way to extract the
                # squashfs payload. The raw installer image size is deterministic,
                # so ANY size delta is evidence of a real content change — we
                # cannot prove it cosmetic without payload inspection. Flag
                # SEMANTIC rather than masking small deltas as "alignment".
                local size_diff pct_diff
                size_diff=$((size_a > size_b ? size_a - size_b : size_b - size_a))
                local larger=$((size_a > size_b ? size_a : size_b))
                pct_diff=$(python3 -c "print(f'{100*$size_diff/$larger:.2f}')" 2>/dev/null || echo "0")
                echo "SEMANTIC (size diff: $size_diff bytes / ${pct_diff}%, extraction unavailable)"
                record_result "$name" "SEMANTIC" "Decompressed size differs by $size_diff bytes (${pct_diff}%); squashfs extraction unavailable to prove equivalence"
                has_semantic=true
            fi
            rm -rf "$tmp_a" "$tmp_b"
            return
        fi

        # Compare squashfs file listings
        local files_a files_b
        files_a=$(find "$sqfs_a" -type f | sed "s|$sqfs_a||" | sort)
        files_b=$(find "$sqfs_b" -type f | sed "s|$sqfs_b||" | sort)

        local file_diff
        file_diff=$(diff <(echo "$files_a") <(echo "$files_b") 2>/dev/null || true)

        if [[ -n "$file_diff" ]]; then
            local diff_count
            diff_count=$(echo "$file_diff" | grep -c "^[<>]" || true)
            has_semantic=true
            diff_details="File list differs by $diff_count entries"
        fi

        # Content hash comparison: hash key configs/scripts in the squashfs
        # For binaries, strip debug info before hashing to avoid build-id noise
        if ! $has_semantic; then
            local hash_list_a hash_list_b
            hash_list_a=$(mktemp)
            hash_list_b=$(mktemp)

            # Hash config/script files directly (text — no normalization needed)
            find "$sqfs_a" \( -path "*/etc/sonic/*" -o -path "*/etc/supervisor/*" \) \
                -type f -exec sha256sum {} \; 2>/dev/null | \
                sed "s|$sqfs_a||" | sort -k2 > "$hash_list_a"
            find "$sqfs_b" \( -path "*/etc/sonic/*" -o -path "*/etc/supervisor/*" \) \
                -type f -exec sha256sum {} \; 2>/dev/null | \
                sed "s|$sqfs_b||" | sort -k2 > "$hash_list_b"

            # For binaries, strip debug/build-id then hash (avoids build-id false positives)
            if has_tool objcopy; then
                local bin_tmp
                bin_tmp=$(mktemp -d -p "${TMPDIR:-${OUTPUT_DIR:-/tmp}}" 2>/dev/null || mktemp -d)
                for dir_pair in "$sqfs_a:a" "$sqfs_b:b"; do
                    local sqfs_dir="${dir_pair%%:*}" side="${dir_pair##*:}"
                    local target_file="$hash_list_a"
                    [[ "$side" == "b" ]] && target_file="$hash_list_b"
                    find "$sqfs_dir" \( -path "*/usr/bin/*" -o -path "*/usr/sbin/*" \) \
                        -type f | while read -r bin_file; do
                        local stripped="$bin_tmp/stripped_$$"
                        if file "$bin_file" 2>/dev/null | grep -q "ELF"; then
                            objcopy --strip-debug --remove-section=.note.gnu.build-id \
                                "$bin_file" "$stripped" 2>/dev/null || cp "$bin_file" "$stripped"
                        else
                            cp "$bin_file" "$stripped"
                        fi
                        local h
                        h=$(sha256sum "$stripped" | cut -d' ' -f1)
                        echo "$h  ${bin_file#$sqfs_dir}" >> "$target_file"
                        rm -f "$stripped"
                    done
                done
                rm -rf "$bin_tmp"
            else
                # No objcopy — hash raw binaries (may produce false positives from build-ids)
                find "$sqfs_a" \( -path "*/usr/bin/*" -o -path "*/usr/sbin/*" \) \
                    -type f -exec sha256sum {} \; 2>/dev/null | \
                    sed "s|$sqfs_a||" >> "$hash_list_a"
                find "$sqfs_b" \( -path "*/usr/bin/*" -o -path "*/usr/sbin/*" \) \
                    -type f -exec sha256sum {} \; 2>/dev/null | \
                    sed "s|$sqfs_b||" >> "$hash_list_b"
            fi

            # Sort and compare
            local content_diff
            content_diff=$(diff <(sort -k2 "$hash_list_a") <(sort -k2 "$hash_list_b") 2>/dev/null || true)
            if [[ -n "$content_diff" ]]; then
                local content_diff_count
                content_diff_count=$(echo "$content_diff" | grep -c "^[<>]" || true)
                content_diff_count=${content_diff_count:-0}
                if [[ $content_diff_count -gt 0 ]]; then
                    has_semantic=true
                    diff_details="${diff_details:+$diff_details; }Content hash differs in $((content_diff_count / 2)) files"
                fi
            fi
            rm -f "$hash_list_a" "$hash_list_b"
        fi

        # Compare installed package lists with versions (dpkg status)
        local pkgs_a pkgs_b
        pkgs_a=$(find "$sqfs_a" -path "*/var/lib/dpkg/status" -exec grep -E "^(Package|Version):" {} \; 2>/dev/null | \
            paste -d' ' - - | sort)
        pkgs_b=$(find "$sqfs_b" -path "*/var/lib/dpkg/status" -exec grep -E "^(Package|Version):" {} \; 2>/dev/null | \
            paste -d' ' - - | sort)

        if [[ -n "$pkgs_a" && -n "$pkgs_b" ]]; then
            local pkg_diff
            pkg_diff=$(diff <(echo "$pkgs_a") <(echo "$pkgs_b") 2>/dev/null || true)
            if [[ -n "$pkg_diff" ]]; then
                local pkg_diff_count
                pkg_diff_count=$(echo "$pkg_diff" | grep -c "^[<>]" || true)
                has_semantic=true
                diff_details="${diff_details:+$diff_details; }Package list differs by $pkg_diff_count entries"
            fi
        fi
    else
        # .bin files: ONIE installer script + payload
        # Compare decompressed content
        local size_a size_b
        size_a=$(stat -c%s "$file_a")
        size_b=$(stat -c%s "$file_b")

        if [[ "$size_a" == "$size_b" ]]; then
            # Same size, check if content identical after skipping ONIE shell script header
            # ONIE .bin installers have a variable-length shell script header (typically 4KB-50KB)
            # terminated by a binary payload marker.
            local sha_payload_a sha_payload_b
            local skip_bytes_a skip_bytes_b
            # Find payload offset independently for each file
            skip_bytes_a=$(python3 -c "
import sys
data = open('$file_a', 'rb').read(65536)
for i in range(512, len(data) - 2):
    if data[i:i+2] == b'\x1f\x8b' or data[i:i+6] == b'\xfd7zXZ\x00':
        print(i); sys.exit(0)
print(512)
" 2>/dev/null || echo "512")
            skip_bytes_b=$(python3 -c "
import sys
data = open('$file_b', 'rb').read(65536)
for i in range(512, len(data) - 2):
    if data[i:i+2] == b'\x1f\x8b' or data[i:i+6] == b'\xfd7zXZ\x00':
        print(i); sys.exit(0)
print(512)
" 2>/dev/null || echo "512")
            sha_payload_a=$(tail -c "+$((skip_bytes_a + 1))" "$file_a" | sha256sum | cut -d' ' -f1)
            sha_payload_b=$(tail -c "+$((skip_bytes_b + 1))" "$file_b" | sha256sum | cut -d' ' -f1)
            if [[ "$sha_payload_a" == "$sha_payload_b" ]]; then
                echo "COSMETIC (header differs, payload identical)"
                record_result "$name" "COSMETIC" "Payload identical; header/script differs (skip_a=$skip_bytes_a, skip_b=$skip_bytes_b)"
                rm -rf "$tmp_a" "$tmp_b"
                return
            fi
            # Payload differs — this is SEMANTIC (don't fall through to header-only check)
            has_semantic=true
            diff_details="Binary payload differs after header (skip_a=$skip_bytes_a, skip_b=$skip_bytes_b)"
        fi

        # Different size — always SEMANTIC for .bin
        if ! $has_semantic; then
            has_semantic=true
            diff_details="Binary content differs (size A: $size_a, B: $size_b)"
        fi
    fi

    if $has_semantic; then
        echo "SEMANTIC — $diff_details"
        record_result "$name" "SEMANTIC" "$diff_details"
    else
        echo "COSMETIC"
        record_result "$name" "COSMETIC" "Squashfs content identical; metadata/compression differs"
    fi

    rm -rf "$tmp_a" "$tmp_b"
}

# Extract squashfs from a raw disk image
# Returns path to extracted squashfs root, or empty string on failure
extract_squashfs() {
    local image="$1" dest="$2"
    local sqfs_dir="$dest/squashfs-root"

    # Method 1: Find squashfs magic (hsqs) using grep -boa (fast, no Python memory load)
    local offset
    offset=$(grep -boa 'hsqs' "$image" 2>/dev/null | head -1 | cut -d: -f1)

    if [[ -n "$offset" ]]; then
        # Extract squashfs starting at offset
        dd if="$image" bs=1 skip="$offset" 2>/dev/null | unsquashfs -d "$sqfs_dir" -f /dev/stdin >/dev/null 2>&1
        if [[ -d "$sqfs_dir" && $(find "$sqfs_dir" -type f | wc -l) -gt 100 ]]; then
            echo "$sqfs_dir"
            return
        fi
    fi

    # Method 2: Try unsquashfs directly (works if image IS a squashfs)
    unsquashfs -d "$sqfs_dir" -f "$image" >/dev/null 2>&1
    if [[ -d "$sqfs_dir" && $(find "$sqfs_dir" -type f | wc -l) -gt 100 ]]; then
        echo "$sqfs_dir"
        return
    fi

    echo ""
}

# Extract the installer's embedded rootfs squashfs from either:
#   - a raw/squashfs image, or
#   - a qcow2/raw disk image containing /image-*/fs.squashfs in the rootfs partition.
extract_installer_squashfs() {
    local image="$1" dest="$2"
    local sqfs_dir

    sqfs_dir=$(extract_squashfs "$image" "$dest")
    if [[ -n "$sqfs_dir" ]]; then
        echo "$sqfs_dir"
        return
    fi

    if has_tool qemu-img && has_tool fdisk && has_tool debugfs; then
        local inspect_image="$image"
        local raw_image="$dest/image.raw"

        if qemu-img info "$image" 2>/dev/null | grep -q "file format: qcow2"; then
            qemu-img convert -O raw "$image" "$raw_image" >/dev/null 2>&1 || {
                echo ""
                return
            }
            inspect_image="$raw_image"
        fi

        local part_info part_start part_sectors
        part_info=$(fdisk -l "$inspect_image" 2>/dev/null | awk '/ Linux filesystem$/ {print $2, $4; exit}')
        if [[ -n "$part_info" ]]; then
            read -r part_start part_sectors <<< "$part_info"

            local part_image="$dest/rootfs-partition.ext4"
            dd if="$inspect_image" of="$part_image" bs=512 skip="$part_start" count="$part_sectors" conv=sparse status=none 2>/dev/null || {
                echo ""
                return
            }

            local image_dir
            image_dir=$(printf 'ls -p /\nquit\n' | debugfs -f - "$part_image" 2>/dev/null | \
                awk -F/ '/image-.*\/\/$/ {print $5; exit}')

            if [[ -n "$image_dir" ]]; then
                local embedded_sqfs="$dest/fs.squashfs"
                debugfs -R "dump /$image_dir/fs.squashfs $embedded_sqfs" "$part_image" >/dev/null 2>&1 || true
                if [[ -s "$embedded_sqfs" ]]; then
                    sqfs_dir="$dest/squashfs-root"
                    unsquashfs -d "$sqfs_dir" -f "$embedded_sqfs" >/dev/null 2>&1 || true
                    if [[ -d "$sqfs_dir" && $(find "$sqfs_dir" -type f | wc -l) -gt 100 ]]; then
                        echo "$sqfs_dir"
                        return
                    fi
                fi
            fi
        fi
    fi

    echo ""
}

# ═══════════════════════════════════════════════════════════════════════════════
# REPORT GENERATION
# ═══════════════════════════════════════════════════════════════════════════════
generate_report() {
    echo ""
    echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}  EQUIVALENCE REPORT${NC}"
    echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
    echo "  Directory A: $DIR_A"
    echo "  Directory B: $DIR_B"
    echo "  Levels compared: $LEVELS"
    echo ""
    echo -e "  ${GREEN}IDENTICAL:${NC}  $COUNT_IDENTICAL"
    echo -e "  ${YELLOW}COSMETIC:${NC}   $COUNT_COSMETIC  (timestamp/metadata diffs — safe)"
    echo -e "  ${RED}SEMANTIC:${NC}   $COUNT_SEMANTIC  (real content differences — cache bug)"
    if [[ $COUNT_BASELINE_NONDETERMINISM -gt 0 ]]; then
        echo -e "  ${YELLOW}BASELINE:${NC}   $COUNT_BASELINE_NONDETERMINISM  (known non-determinism — not cache-related)"
    fi
    echo -e "  ${RED}MISSING:${NC}    $COUNT_MISSING  (artifact in one build only)"
    if [[ $COUNT_EXPECTED_MISSING -gt 0 ]]; then
        echo -e "  ${YELLOW}EXPECTED_MISSING:${NC} $COUNT_EXPECTED_MISSING  (dbgsym companions not restored by cache — non-fatal)"
    fi
    echo -e "  ERROR:      $COUNT_ERROR"
    echo -e "  ─────────────────"
    echo -e "  TOTAL:      $TOTAL_ARTIFACTS"
    echo ""

    if [[ $COUNT_SEMANTIC -eq 0 && $COUNT_MISSING -eq 0 && $COUNT_ERROR -eq 0 ]]; then
        echo -e "  ${GREEN}${BOLD}VERDICT: PASS — Cache produces equivalent artifacts${NC}"
        if [[ $COUNT_BASELINE_NONDETERMINISM -gt 0 ]]; then
            echo "  $COUNT_BASELINE_NONDETERMINISM diff(s) matched baseline (build non-determinism, not cache-related)"
        else
            echo "  All differences are known-cosmetic (timestamps, gzip headers, etc.)"
        fi
        if [[ $COUNT_EXPECTED_MISSING -gt 0 ]]; then
            echo "  $COUNT_EXPECTED_MISSING dbgsym companion(s) not restored by cache (non-fatal; not in runtime image)"
        fi
    elif [[ $COUNT_ERROR -gt 0 && $COUNT_SEMANTIC -eq 0 && $COUNT_MISSING -eq 0 ]]; then
        echo -e "  ${YELLOW}${BOLD}VERDICT: INCONCLUSIVE — $COUNT_ERROR artifacts had errors${NC}"
        echo ""
        echo "  Errors (could not compare — fix tool availability or inputs):"
        for result in "${RESULTS[@]}"; do
            IFS='|' read -r class artifact detail <<< "$result"
            detail="${detail%%@@SIG@@*}"
            [[ "$class" == "ERROR" ]] && echo "    • $artifact: $detail"
        done
    else
        echo -e "  ${RED}${BOLD}VERDICT: FAIL — Cache has correctness issues${NC}"
        echo ""
        echo "  Semantic differences found in:"
        for result in "${RESULTS[@]}"; do
            IFS='|' read -r class artifact detail <<< "$result"
            detail="${detail%%@@SIG@@*}"
            if [[ "$class" == "SEMANTIC" || "$class" == "MISSING" ]]; then
                echo -e "    ${RED}•${NC} $artifact: $detail"
            fi
        done
    fi
    echo ""

    # Save detailed report
    mkdir -p "$OUTPUT_DIR"
    local report_file="$OUTPUT_DIR/equivalence-report.txt"
    {
        echo "═══════════════════════════════════════════════════════════════"
        echo "  DPKG Cache Equivalence Report"
        echo "  Generated: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
        echo "═══════════════════════════════════════════════════════════════"
        echo ""
        echo "Dir A: $DIR_A"
        echo "Dir B: $DIR_B"
        echo ""
        echo "Summary: $COUNT_IDENTICAL identical, $COUNT_COSMETIC cosmetic, $COUNT_SEMANTIC semantic, $COUNT_BASELINE_NONDETERMINISM baseline, $COUNT_MISSING missing, $COUNT_EXPECTED_MISSING expected-missing(dbgsym), $COUNT_ERROR errors"
        echo ""
        printf "%-12s %-50s %s\n" "STATUS" "ARTIFACT" "DETAIL"
        printf "%-12s %-50s %s\n" "────────────" "──────────────────────────────────────────────────" "──────"
        for result in "${RESULTS[@]}"; do
            IFS='|' read -r class artifact detail <<< "$result"
            detail="${detail%%@@SIG@@*}"
            printf "%-12s %-50s %s\n" "$class" "$artifact" "$detail"
            # Link to diffoscope report if one was generated
            if [[ "$class" == "SEMANTIC" && -f "$OUTPUT_DIR/diffoscope/${artifact}.html" ]]; then
                printf "%-12s %-50s %s\n" "" "" "→ diffoscope: diffoscope/${artifact}.html"
            fi
        done
        # Note diffoscope reports if any were generated
        if [[ -d "$OUTPUT_DIR/diffoscope" ]] && ls "$OUTPUT_DIR/diffoscope"/*.html &>/dev/null; then
            echo ""
            echo "Detailed diffoscope reports: $OUTPUT_DIR/diffoscope/"
            ls "$OUTPUT_DIR/diffoscope/"*.html 2>/dev/null | while read -r f; do
                echo "  • $(basename "$f")"
            done
        fi
    } > "$report_file"

    log_info "Full report saved to: $report_file"

    # JSON output if requested
    if $JSON_OUTPUT; then
        local json_file="$OUTPUT_DIR/equivalence-report.json"
        # Generate JSON via stdin to avoid injection from paths/details containing quotes
        _JSON_DIR_A="$DIR_A" _JSON_DIR_B="$DIR_B" _JSON_LEVELS="$LEVELS" \
        _JSON_TOTAL="$TOTAL_ARTIFACTS" _JSON_IDENTICAL="$COUNT_IDENTICAL" \
        _JSON_COSMETIC="$COUNT_COSMETIC" _JSON_SEMANTIC="$COUNT_SEMANTIC" \
        _JSON_MISSING="$COUNT_MISSING" _JSON_ERROR="$COUNT_ERROR" \
        _JSON_BASELINE_ND="$COUNT_BASELINE_NONDETERMINISM" \
        _JSON_EXPECTED_MISSING="$COUNT_EXPECTED_MISSING" \
        _JSON_BASELINE_FILE="$BASELINE_FILE" \
        _JSON_IS_BASELINE="$( $GENERATE_BASELINE && echo true || echo false )" \
        python3 -c "
import json, sys, os
lines = sys.stdin.read().strip().split('\n')
results = []
for line in lines:
    parts = line.split('|', 2)
    if len(parts) == 3:
        detail = parts[2]
        signature = ''
        if '@@SIG@@' in detail:
            detail, signature = detail.split('@@SIG@@', 1)
        results.append({'classification': parts[0], 'artifact': parts[1], 'detail': detail, 'signature': signature})
semantic = int(os.environ['_JSON_SEMANTIC'])
missing = int(os.environ['_JSON_MISSING'])
error = int(os.environ['_JSON_ERROR'])
if semantic == 0 and missing == 0 and error == 0:
    verdict = 'PASS'
elif error > 0 and semantic == 0 and missing == 0:
    verdict = 'INCONCLUSIVE'
else:
    verdict = 'FAIL'
baseline_file = os.environ.get('_JSON_BASELINE_FILE') or None
report = {
    'dir_a': os.environ['_JSON_DIR_A'],
    'dir_b': os.environ['_JSON_DIR_B'],
    'levels': os.environ['_JSON_LEVELS'],
    'summary': {
        'total': int(os.environ['_JSON_TOTAL']),
        'identical': int(os.environ['_JSON_IDENTICAL']),
        'cosmetic': int(os.environ['_JSON_COSMETIC']),
        'semantic': semantic,
        'baseline_nondeterminism': int(os.environ['_JSON_BASELINE_ND']),
        'missing': missing,
        'expected_missing': int(os.environ['_JSON_EXPECTED_MISSING']),
        'error': error
    },
    'baseline_file': baseline_file,
    'is_baseline': os.environ['_JSON_IS_BASELINE'] == 'true',
    'verdict': verdict,
    'results': results
}
print(json.dumps(report, indent=2))
" > "$json_file" < <(printf '%s\n' "${RESULTS[@]}") || log_warn "Failed to generate JSON report"
        log_info "JSON report saved to: $json_file"
    fi
}

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════
main() {
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║  SONiC DPKG Cache — Artifact Equivalence Verification         ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "  Dir A: $DIR_A"
    echo "  Dir B: $DIR_B"
    echo "  Levels: $LEVELS"
    echo "  Mode: $( $QUICK_MODE && echo "quick (hash only)" || echo "deep analysis" )"
    echo "  diffoscope: $( $USE_DIFFOSCOPE && echo "enabled" || echo "disabled" )"
    if [[ -n "$BASELINE_FILE" ]]; then
        echo "  Baseline: $BASELINE_FILE (${#BASELINE_SEMANTICS[@]} known non-deterministic)"
    fi
    if $GENERATE_BASELINE; then
        echo "  Mode: GENERATING BASELINE (fresh-vs-fresh — record inherent non-determinism)"
    fi

    mkdir -p "$OUTPUT_DIR"

    # Run requested levels (use comma-boundary matching to avoid greedy patterns)
    local level_csv=",$LEVELS,"
    if [[ "$level_csv" == *",1,"* ]]; then
        compare_debs
    fi
    if [[ "$level_csv" == *",2,"* ]]; then
        compare_wheels
    fi
    if [[ "$level_csv" == *",3,"* ]]; then
        compare_dockers
    fi
    if [[ "$level_csv" == *",5,"* ]]; then
        compare_installers
    fi

    # Generate report
    generate_report

    # Exit code: fail on SEMANTIC, MISSING, ERROR, or zero artifacts compared.
    # (baseline-matched SEMANTIC and dbgsym EXPECTED_MISSING were already
    # downgraded in record_result, so they do not count toward failure.)
    if $GENERATE_BASELINE; then
        # Baseline generation is fresh-vs-fresh: SEMANTIC/MISSING diffs are the
        # inherent build non-determinism we are recording, not failures. Only a
        # broken run (extraction errors or nothing compared) is fatal — so under
        # the pipeline's `set -e` the report is emitted and the subsequent
        # A-vs-C comparison step still runs.
        if [[ $COUNT_ERROR -gt 0 ]]; then
            log_warn "Exiting with code 2: $COUNT_ERROR artifacts had extraction/comparison errors"
            exit 2
        elif [[ $TOTAL_ARTIFACTS -eq 0 ]]; then
            log_warn "Exiting with code 2: No artifacts were compared (check --dir-a/--dir-b paths)"
            exit 2
        fi
        echo ""
        echo -e "${GREEN}Baseline generated: recorded $COUNT_SEMANTIC semantic diff(s) as known non-determinism${NC}"
        exit 0
    fi
    if [[ $COUNT_SEMANTIC -gt 0 || $COUNT_MISSING -gt 0 ]]; then
        if [[ -n "$BASELINE_FILE" ]]; then
            echo ""
            echo -e "${RED}FAIL: $COUNT_SEMANTIC new semantic difference(s) not in baseline${NC}"
            if [[ $COUNT_BASELINE_NONDETERMINISM -gt 0 ]]; then
                echo -e "${YELLOW}      ($COUNT_BASELINE_NONDETERMINISM additional diffs matched baseline — expected non-determinism)${NC}"
            fi
        fi
        exit 1
    elif [[ $COUNT_ERROR -gt 0 ]]; then
        log_warn "Exiting with code 2: $COUNT_ERROR artifacts had extraction/comparison errors"
        exit 2
    elif [[ $TOTAL_ARTIFACTS -eq 0 ]]; then
        log_warn "Exiting with code 2: No artifacts were compared (check --dir-a/--dir-b paths)"
        exit 2
    else
        if [[ $COUNT_BASELINE_NONDETERMINISM -gt 0 ]]; then
            echo ""
            echo -e "${GREEN}PASS: All $COUNT_BASELINE_NONDETERMINISM semantic diff(s) matched the baseline (known non-determinism)${NC}"
        fi
        exit 0
    fi
}

main "$@"
