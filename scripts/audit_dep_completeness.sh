#!/bin/bash
#
# audit_dep_completeness.sh — Per-Package .dep File Completeness Audit
#
# ═══════════════════════════════════════════════════════════════════════════════
# PURPOSE
# ═══════════════════════════════════════════════════════════════════════════════
#
# In SONiC's DPKG caching system (Makefile.cache), each package has a .dep file
# (rules/<pkg>.dep) that declares what inputs affect its cache key. If a .dep file
# is incomplete — missing a build flag, a source file pattern, or a dependency —
# the cache key won't change when it should, and stale artifacts will be served.
#
# This script performs static analysis across ALL rules/*.dep files to identify
# gaps where build inputs exist in .mk files but are not reflected in cache key
# computation. It cross-references:
#   - rules/*.mk  (what flags/deps/sources actually affect each package)
#   - rules/*.dep (what the cache system tracks)
#   - Makefile.cache (global lists and hash computation logic)
#   - Source directories (whether packages have source code at all)
#
# ═══════════════════════════════════════════════════════════════════════════════
# WHAT IT CHECKS (9 checks)
# ═══════════════════════════════════════════════════════════════════════════════
#
# Check 1: Packages registered in build categories without a .dep file
#           These packages have NO cache key computation at all — they'll always
#           get a cache hit on first write, regardless of source changes.
#
# Check 2: .dep files that don't include their own .mk file in DEP_FILES
#           The .mk file defines dependencies and build flags — if it changes
#           (e.g., new _DEPENDS added), the cache key must reflect that.
#
# Check 3: Build flags (ifeq/ifneq conditionals) in .mk not tracked in .dep
#           Analyzes WHAT each conditional does to distinguish:
#           - Build-affecting: changes _DEPENDS, _RDEPENDS, build commands → REAL gap
#           - Assembly-only: changes SONIC_INSTALL_DOCKER_IMAGES → safe to ignore
#
# Check 4: _DEPENDS declared in .mk but not reflected in .dep DEP_FILES
#           If package A depends on package B, A's cache key should include B.
#
# Check 5: Source directory patterns — .dep declares _SRC_PATH or git ls-files
#           but the pattern may miss files (e.g., generated sources, submodules).
#
# Check 6: CACHE_MODE consistency — GIT_COMMIT_SHA vs GIT_CONTENT_SHA
#           GIT_COMMIT_SHA mode uses the git commit hash (fast but coarse).
#           GIT_CONTENT_SHA hashes actual file content (precise but slower).
#
# Check 7: Cross-package dependency completeness — are transitive deps covered?
#           If A depends on B depends on C, does A's cache key transitively
#           include C's changes? (Makefile.cache handles this via DEP_MOD_SHA
#           recursion, but only if B is declared as a dep of A.)
#
# Check 8: slave.mk flags used in package build rules but not tracked
#           Scans for flags like SONIC_BUILD_JOBS, ENABLE_SYNCD_RPC that are
#           used in conditionals but not in SONIC_COMMON_FLAGS_LIST or DEP_FLAGS.
#
# Check 9: Nested derived packages not covered by cache save/restore
#           add_derived_package(X, Y) only adds Y to X's first-level
#           _DERIVED_DEBS, and the cache save does not recurse. If X is itself a
#           derived deb, Y is dropped from the top-level package's cache tarball.
#           Confirmed by negative control NC-6 (libnl3 nested derived debs).
#
# Check 10: Exported build-env flags not tracked in any cache key
#           Flags passed via `export FLAG` (not an ifeq($(FLAG)) conditional) are
#           consumed inside debian/rules or sub-Makefiles, so the ifeq-based
#           Checks 3 and 8 cannot see them. Closes the "export-only" blind spot
#           (e.g. ENABLE_FRR_TCMALLOC on frr, whose _DEP_FLAGS is common-only).
#
# ═══════════════════════════════════════════════════════════════════════════════
# USAGE
# ═══════════════════════════════════════════════════════════════════════════════
#
#   ./scripts/audit_dep_completeness.sh [OPTIONS]
#
# Options:
#   --verbose        Show detailed explanations for each finding
#   --json           Output findings in JSON format (for downstream tooling)
#   --package NAME   Audit only the specified package (e.g., --package swss)
#
# Examples:
#   # Full audit of all ~165 .dep files
#   ./scripts/audit_dep_completeness.sh
#
#   # Audit a specific package with details
#   ./scripts/audit_dep_completeness.sh --verbose --package docker-orchagent
#
#   # Machine-readable output for CI integration
#   ./scripts/audit_dep_completeness.sh --json > findings.json
#
# ═══════════════════════════════════════════════════════════════════════════════
# INTERPRETING RESULTS
# ═══════════════════════════════════════════════════════════════════════════════
#
# Findings are classified by severity:
#
#   P0 = Confirmed stale cache risk
#        The gap WILL cause incorrect cache hits under normal usage.
#        Example: A package has no .dep file at all.
#
#   P1 = Likely stale cache risk (needs verification)
#        Strong evidence of a gap, but needs Phase 2 PoC to confirm binary diff.
#        Example: ENABLE_SYNCD_RPC changes build profile but isn't in syncd.dep.
#
#   P2 = Potential risk / cosmetic concern
#        May or may not cause issues depending on usage patterns.
#        Example: A .dep doesn't include its own .mk file (redundant if .mk
#        rarely changes build output independently of source).
#
#   P3 = Informational / design observation
#        Not a bug, but worth knowing for completeness.
#        Example: Deprecated flag still tracked in some .dep files.
#
# The summary table at the end shows counts by severity. Focus on P0/P1 first.
#
# Exit codes:
#   0 = No P0 findings (P1 warnings may exist but are not blocking)
#   1 = P0 findings present (confirmed stale cache risk)
#   2 = Script error (e.g., not run from repo root, missing Makefile.cache)
#
# ═══════════════════════════════════════════════════════════════════════════════
# FALSE POSITIVE FILTERING
# ═══════════════════════════════════════════════════════════════════════════════
#
# The script includes filters to reduce noise from known non-issues:
#
# - Assembly-only flags: INCLUDE_* flags that only control whether a Docker image
#   is INSTALLED into the final NOS image (SONIC_INSTALL_DOCKER_IMAGES) — they
#   don't change what's INSIDE the image during build.
#
# - Block-content analysis (data-driven, no hand-maintained flag lists): the script
#   inspects WHAT a conditional block DOES rather than matching flag names. A block is
#   "build-affecting" only if it touches a stable SONiC build variable (see
#   BUILD_SIGNAL_RE: *_DEPENDS/_RDEPENDS/_DBG_PACKAGES/_EXTRA_DEBS/_DERIVED_DEBS/
#   _SRC_PATH/_BUILD_ENV/_MAKE_ENV/_CFLAGS/_MAKE_TARGET, add_derived_package /
#   add_extra_package, dpkg-buildpackage, DEB_BUILD_OPTIONS). Blocks that only select/assemble packages (SONIC_MAKE_DEBS,
#   SONIC_INSTALL_DOCKER_IMAGES, SONIC_PACKAGES_LOCAL, DEFAULT_FEATURE_STATE/OWNER)
#   carry no build signal and are classified assembly-only (not a gap). These naming
#   conventions are fixed by Makefile.cache/slave.mk, so the audit needs no upkeep as
#   new feature flags are added.
#
# - Deprecated flags: detected generically — any flag pinned to 'n' for the modern
#   build envs (an if(n)eq filter on $(BLDENV) naming bookworm/trixie that sets the
#   flag = n in slave.mk) is reported as P3 informational only.
#
# ═══════════════════════════════════════════════════════════════════════════════
# CONTEXT
# ═══════════════════════════════════════════════════════════════════════════════
#
# This script is part of the DPKG Cache Validation toolkit:
#   - audit_dep_completeness.sh  → per-package .dep file audit (this script)
#   - check_common_files.sh      → global cache input audit (5 checks)
#
# Together they form Phase 1 (Static Analysis) of the DPKG Cache Equivalence
# verification plan. Their output feeds into Phase 2 (PoC builds) to confirm
# whether identified gaps cause actual binary differences.
#

set -uo pipefail

# --- Configuration ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
RULES_DIR="$REPO_ROOT/rules"
PLATFORM_DIR="$REPO_ROOT/platform/${CONFIGURED_PLATFORM:-vs}"
MAKEFILE_CACHE="$REPO_ROOT/Makefile.cache"
EXPORT_REGISTRY="$SCRIPT_DIR/cache_key_export_registry.tsv"   # informational snapshot only
EXPORT_SCANNER="$SCRIPT_DIR/cache_key_scan.py"
EXPORT_WAIVERS="$SCRIPT_DIR/cache_key_export_waivers.tsv"     # human-justified safe exports

# Colors for terminal output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Internal field separator for FINDINGS records. Uses ASCII Unit Separator
# (0x1f) instead of '|' because finding text (e.g. grep -Ev "a|b" exclusion
# patterns) can legitimately contain '|', which would corrupt the split.
readonly FINDING_FS=$'\037'

# Counters
TOTAL_DEP_FILES=0
FINDINGS_P0=0
FINDINGS_P1=0
FINDINGS_P2=0
FINDINGS_P3=0

# Options
VERBOSE=false
JSON_OUTPUT=false
FILTER_PACKAGE=""
FINDINGS=()
DIFF_MODE=false
DIFF_BASE=""
DIFF_VARS_CACHE=""
EXPORT_SCAN_CACHE=""

# --- Argument Parsing ---
while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --json)
            JSON_OUTPUT=true
            shift
            ;;
        --package|-p)
            FILTER_PACKAGE="$2"
            shift 2
            ;;
        --base|-b)
            DIFF_MODE=true
            DIFF_BASE="$2"
            shift 2
            ;;
        --refresh-registry)
            # Regenerate the comprehensive export registry from a whole-tree scan.
            if [[ ! -f "$EXPORT_SCANNER" ]]; then
                echo "Scanner not found: $EXPORT_SCANNER" >&2
                exit 1
            fi
            exec python3 "$EXPORT_SCANNER"
            ;;
        --help|-h)
            echo "Usage: $0 [--verbose] [--json] [--package PKGNAME] [--base REF] [--refresh-registry]"
            echo ""
            echo "Options:"
            echo "  --verbose, -v      Show detailed analysis for each .dep file"
            echo "  --json             Output findings as JSON (for programmatic consumption)"
            echo "  --package, -p      Only audit a specific package (e.g., 'swss')"
            echo "  --base, -b REF     PR/diff mode: only judge packages and exported build"
            echo "                     variables changed since git REF (e.g. origin/master)."
            echo "                     Any not-provably-safe export the PR touches -> P0."
            echo "  --refresh-registry Write an informational snapshot of the live export"
            echo "                     classification to scripts/cache_key_export_registry.tsv"
            echo "                     (scripts/cache_key_scan.py) and exit. The gate itself"
            echo "                     classifies LIVE and does not depend on this snapshot."
            echo "  --help, -h         Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# --- Helper Functions ---

log_verbose() {
    if $VERBOSE; then
        echo -e "  ${CYAN}[VERBOSE]${NC} $1"
    fi
}

add_finding() {
    local severity="$1"
    local package="$2"
    local issue="$3"
    local suggestion="$4"

    FINDINGS+=("${severity}${FINDING_FS}${package}${FINDING_FS}${issue}${FINDING_FS}${suggestion}")

    case $severity in
        P0) ((FINDINGS_P0++)) ;;
        P1) ((FINDINGS_P1++)) ;;
        P2) ((FINDINGS_P2++)) ;;
        P3) ((FINDINGS_P3++)) ;;
    esac
}

# --- Whole-tree corpus helpers -------------------------------------------------
# The original per-package checks (1,3,4,5,6,7,9) only scanned rules/*.mk and
# rules/*.dep. Real build rules also live under platform/** (hundreds of .mk and
# .dep files) and were structurally invisible — a package's cache-key gap in
# platform/ could never be reported. These helpers return the FULL git-tracked
# corpus (rules/ + platform/**) so every check applies the same whole-tree,
# no-heuristic principle already used by the export scanner (Check 10).
#
# Output is one absolute path per line (git-tracked only; ordering is stable).
# Results are computed once and memoised for the life of the process.
CORPUS_MK_CACHE=""
CORPUS_DEP_CACHE=""

all_rule_mk_files() {
    if [[ -z "$CORPUS_MK_CACHE" ]]; then
        CORPUS_MK_CACHE=$(git -C "$REPO_ROOT" ls-files 2>/dev/null \
            | grep -E '^(rules|platform)/.*\.mk$' \
            | sed "s|^|$REPO_ROOT/|")
    fi
    printf '%s\n' "$CORPUS_MK_CACHE"
}

all_dep_files() {
    if [[ -z "$CORPUS_DEP_CACHE" ]]; then
        CORPUS_DEP_CACHE=$(git -C "$REPO_ROOT" ls-files 2>/dev/null \
            | grep -E '^(rules|platform)/.*\.dep$' \
            | sed "s|^|$REPO_ROOT/|")
    fi
    printf '%s\n' "$CORPUS_DEP_CACHE"
}

# Human-facing package label for a .mk/.dep path. rules/ files keep their bare
# basename (stable, historically used). platform/** files carry their repo-
# relative path (minus extension) because the SAME basename (e.g. libsaithrift-dev)
# exists under many vendor dirs — a bare basename would be ambiguous and collapse
# distinct packages together. Findings must be actionable to a specific file.
pkg_label() {
    local rel="${1#"$REPO_ROOT"/}"
    case "$rel" in
        rules/*) basename "$rel" | sed -E 's/\.(mk|dep)$//' ;;
        *)       echo "${rel%.*}" ;;
    esac
}

# Run the comprehensive whole-tree scanner ONCE (live) and cache its output. The
# gate classifies on every run from live code — there is no committed trust-store
# to drift out of date. Output rows: variable<TAB>disposition<TAB>evidence.
compute_export_scan() {
    [[ -n "$EXPORT_SCAN_CACHE" ]] && return 0
    [[ -f "$EXPORT_SCANNER" ]] || return 1
    EXPORT_SCAN_CACHE=$(python3 "$EXPORT_SCANNER" --stdout 2>/dev/null)
    [[ -n "$EXPORT_SCAN_CACHE" ]]
}

# Echo the human-recorded justification if $1 is waived in cache_key_export_waivers.tsv
# (variable<TAB>reason, '#' comments ignored), else nothing. A waiver is the ONLY
# way a not-provably-safe export is cleared, and it must carry a reason.
waiver_reason() {
    [[ -f "$EXPORT_WAIVERS" ]] || return 0
    awk -F'\t' -v v="$1" '!/^[[:space:]]*#/ && $1==v {print $2; exit}' "$EXPORT_WAIVERS"
}

# Populate DIFF_VARS_CACHE with the set of build-env variables the diff touches:
# any variable whose `export`/assignment/use lines were added or removed between
# DIFF_BASE and the working tree in slave.mk / rules/*.mk / Makefile.cache. Used
# by --base (PR) mode to scope Check 10 to what the PR actually changes.
compute_diff_vars() {
    $DIFF_MODE || return 0
    local range="$DIFF_BASE"
    # Resolve a merge-base when given a branch ref, so we only see PR-local changes.
    if git -C "$REPO_ROOT" rev-parse --verify -q "$DIFF_BASE" >/dev/null 2>&1; then
        local mb; mb=$(git -C "$REPO_ROOT" merge-base "$DIFF_BASE" HEAD 2>/dev/null)
        [[ -n "$mb" ]] && range="$mb"
    fi
    DIFF_VARS_CACHE=$(git -C "$REPO_ROOT" diff --unified=0 "$range" -- \
            slave.mk 'rules/*.mk' Makefile.cache 2>/dev/null \
        | grep -E '^[-+]' | grep -vE '^[-+]{3} ' \
        | grep -oP '(?:export\s+|\$\()\K[A-Za-z_][A-Za-z0-9_]*' \
        | sort -u)
    log_verbose "diff-mode variables touched since $DIFF_BASE: $(echo "$DIFF_VARS_CACHE" | tr '\n' ' ')"
}

# True (0) if $1 is one of the variables the diff touched (see compute_diff_vars).
diff_touches_var() {
    grep -qx "$1" <<< "$DIFF_VARS_CACHE"
}

# Escape a string for safe embedding inside a JSON double-quoted value.
json_escape() {
    local s="$1"
    s="${s//\\/\\\\}"   # backslash first
    s="${s//\"/\\\"}"   # then double quotes
    s="${s//	/\\t}"    # literal tab -> \t
    printf '%s' "$s"
}

# ─────────────────────────────────────────────────────────────────────────────
# Data-driven flag classifiers (no hand-maintained allow-lists)
# ─────────────────────────────────────────────────────────────────────────────
# Whether a build flag affects cached package output is decided by INSPECTING
# what its ifeq/ifneq blocks actually DO, using the SONiC Make build-variable
# naming convention as the signal. That convention (e.g. *_DEPENDS, *_DBG_PACKAGES)
# is fixed by Makefile.cache/slave.mk and effectively never changes, whereas the
# set of feature flags grows constantly — so this needs no manual upkeep.

# A conditional block is "build-affecting" if it assigns to a package build
# input: dependency lists, derived/extra/dbg debs, source paths, build env/flags,
# or runs a build recipe. It is ALSO build-affecting if it registers a derived or
# extra package via the add_derived_package / add_extra_package helpers (which
# expand to *_DERIVED_DEBS / *_EXTRA_DEBS and change a package's cached payload) —
# matched explicitly because the helper call site does not contain the literal
# *_DERIVED_DEBS/*_EXTRA_DEBS token. Blocks that only touch image-assembly /
# installer / feature-inclusion variables (e.g. SONIC_INSTALL_DOCKER_IMAGES,
# *_DOCKERS, SONIC_PACKAGES*, DEFAULT_FEATURE_*) do NOT match and are cosmetic.
readonly BUILD_SIGNAL_RE='_(DEPENDS|RDEPENDS|DBG_DEPENDS|DBG_PACKAGES|EXTRA_DEBS|DERIVED_DEBS|SRC_PATH|BUILD_ENV|MAKE_ENV|CFLAGS|MAKE_TARGET)[[:space:]+:=]|add_(derived|extra)_package|dpkg-buildpackage|DEB_BUILD_OPTIONS'

# Print the ifeq/ifneq($(flag)...) ... endif block(s) that reference $flag in $file.
# Captures ALL such blocks (not just the first) and tolerates nested conditionals
# via depth tracking, so build signals in later blocks are not missed.
flag_blocks_in_file() {
    local flag="$1" file="$2"
    [[ -f "$file" ]] || return 0
    awk -v flag="$flag" '
        function isif(l){ return (l ~ /^[[:space:]]*if(eq|neq|def|ndef)([[:space:]]|\()/) }
        depth==0 && $0 ~ ("if(eq|neq).*\\$\\(" flag "\\)") { depth=1; print; next }
        depth>0 {
            print
            if (isif($0)) depth++
            else if ($0 ~ /^[[:space:]]*endif([[:space:]]|$)/) depth--
        }
    ' "$file" 2>/dev/null || true
}

# True (0) if any block content carries a build signal (i.e. affects build output).
block_is_build_affecting() {
    echo "$1" | grep -qE "$BUILD_SIGNAL_RE"
}

# True (0) if $flag gates a package BUILD input anywhere in the whole-tree corpus
# (rules/ + platform/**) or slave.mk. Broadened from a rules-only scan so a flag
# that is build-affecting only in a platform .mk is still recognised.
flag_affects_build() {
    local flag="$1" f
    while IFS= read -r f; do
        [[ -n "$f" ]] || continue
        if block_is_build_affecting "$(flag_blocks_in_file "$flag" "$f")"; then
            return 0
        fi
    done < <(all_rule_mk_files; echo "$REPO_ROOT/slave.mk")
    return 1
}

# True (0) if $flag is tracked in some cache key: SONIC_COMMON_FLAGS_LIST or any
# per-package *_DEP_FLAGS across the whole-tree corpus (rules/ + platform/**).
flag_tracked_anywhere() {
    local flag="$1"
    if echo "$COMMON_FLAGS_LIST_CACHE" | grep -qx "$flag"; then
        return 0
    fi
    all_dep_files | xargs -r grep -lE "_DEP_FLAGS" 2>/dev/null \
        | xargs -r grep -lE "\\\$\\(${flag}\\)" 2>/dev/null | grep -q . && return 0
    return 1
}

# True (0) if slave.mk pins $flag to 'n' for all modern build envs
# (bookworm/trixie), making it effectively deprecated/dead (e.g. ENABLE_PY2_MODULES).
flag_forced_off_for_modern_bldenv() {
    local flag="$1"
    awk -v flag="$flag" '
        /if(eq|neq).*filter.*\$\(BLDENV\)/ { inblk = (/bookworm/ && /trixie/) ? 1 : 0; next }
        inblk && $0 ~ ("^[[:space:]]*" flag "[[:space:]]*=[[:space:]]*n([[:space:]]|$)") { found=1 }
        /^endif/ { inblk=0 }
        END { exit(found?0:1) }
    ' "$REPO_ROOT/slave.mk" 2>/dev/null
}

# SONIC_COMMON_FLAGS_LIST contents, computed once (used by classifiers above).
compute_common_flags_list() {
    COMMON_FLAGS_LIST_CACHE=$(sed -n '/^SONIC_COMMON_FLAGS_LIST/,/[^\\]$/p' "$MAKEFILE_CACHE" \
        | grep -oP '\$\(\w+\)' | tr -d '$()' | sort -u)
}
COMMON_FLAGS_LIST_CACHE=""

# --- Check 1: Missing .dep files ---
# Packages with .mk files but no .dep file are NEVER cached.
check_missing_dep_files() {
    echo -e "\n${CYAN}=== Check 1: Packages without .dep files (never cached) ===${NC}"

    local count=0
    while IFS= read -r mk_file; do
        [[ -n "$mk_file" ]] || continue
        local base
        base=$(basename "$mk_file" .mk)
        local dep_file="${mk_file%.mk}.dep"

        # Skip non-package mk files (config, functions, etc.)
        if [[ "$base" == "config" ]] || [[ "$base" == "functions" ]]; then
            continue
        fi

        # Check if .mk defines any SONIC_* target category
        if ! grep -qE "SONIC_(DPKG_DEBS|MAKE_DEBS|ONLINE_DEBS|COPY_DEBS|PYTHON_STDEB_DEBS|PYTHON_WHEELS|DOCKER_IMAGES|MAKE_FILES)" "$mk_file"; then
            continue
        fi

        if [[ ! -f "$dep_file" ]]; then
            ((count++))
            # Check if any cached package depends on this uncached package.
            # If yes → P1 (downstream stale risk). If no → P2 (performance only).
            # Strategy: extract variable names assigned in this .mk file, then check
            # if other .mk files reference them in DEPENDS lines.
            local has_dependents=false
            local defined_vars
            # Match both "$(VAR)_DEPENDS" style and "VAR = value" style assignments
            defined_vars=$(grep -oP '^\s*\$\(\K[A-Z][A-Z0-9_]+' "$mk_file" 2>/dev/null | sort -u)
            if [[ -z "$defined_vars" ]]; then
                # Try bare variable assignments (VAR = value)
                defined_vars=$(grep -oP '^\s*\K[A-Z][A-Z0-9_]+(?=\s*[:+?]?=)' "$mk_file" 2>/dev/null | sort -u)
            fi
            for var in $defined_vars; do
                # Skip common non-package variables
                [[ "$var" =~ ^(SONIC_|BLDENV|CONFIGURED|PATH|SRC_PATH|VERSION) ]] && continue
                if grep -rl "DEPENDS.*\$($var)" $(all_rule_mk_files) 2>/dev/null | grep -qv "$mk_file"; then
                    has_dependents=true
                    break
                fi
            done

            if $has_dependents; then
                add_finding "P1" "$(pkg_label "$mk_file")" "No .dep file — package is never cached (other cached packages depend on it)" "Create $dep_file to enable caching"
            else
                add_finding "P2" "$(pkg_label "$mk_file")" "No .dep file — package is never cached (performance only, no downstream dependents)" "Create $dep_file to enable caching"
            fi
            if $VERBOSE; then
                echo -e "  ${YELLOW}MISSING${NC}: $base.dep (targets defined in $base.mk)"
            fi
        fi
    done < <(all_rule_mk_files)

    echo "  Found $count packages without .dep files"
}

# --- Check 2: SONIC_COMMON_BASE_FILES_LIST completeness ---
# Verify all sonic-slave-* directories are tracked
check_common_base_files() {
    echo -e "\n${CYAN}=== Check 2: SONIC_COMMON_BASE_FILES_LIST completeness ===${NC}"

    # Extract currently tracked slave containers from Makefile.cache
    local tracked_slaves
    tracked_slaves=$(grep -oP 'sonic-slave-\w+' "$MAKEFILE_CACHE" | sort -u)

    # Find all sonic-slave-* directories that exist
    local existing_slaves
    existing_slaves=$(find "$REPO_ROOT" -maxdepth 1 -type d -name "sonic-slave-*" -exec basename {} \; | sort)

    echo "  Tracked in SONIC_COMMON_BASE_FILES_LIST:"
    echo "$tracked_slaves" | sed 's/^/    /'

    echo "  Existing sonic-slave-* directories:"
    echo "$existing_slaves" | sed 's/^/    /'

    # Find gaps
    while IFS= read -r slave_dir; do
        if ! echo "$tracked_slaves" | grep -q "^${slave_dir}$"; then
            add_finding "P1" "$slave_dir" "Missing from SONIC_COMMON_BASE_FILES_LIST in Makefile.cache" \
                "Add ${slave_dir}/Dockerfile.j2 and ${slave_dir}/Dockerfile.user.j2 to the list"
            echo -e "  ${RED}GAP${NC}: $slave_dir exists but is NOT tracked!"
        fi
    done <<< "$existing_slaves"
}

# --- Check 3: DEP_FLAGS vs actual build-affecting flags ---
# For each .dep, check if the .mk uses conditional flags not declared in DEP_FLAGS
check_dep_flags_coverage() {
    echo -e "\n${CYAN}=== Check 3: DEP_FLAGS coverage vs conditional build flags ===${NC}"

    # Extract SONIC_COMMON_FLAGS_LIST once for lookups
    local common_flags_list
    common_flags_list=$(sed -n '/^SONIC_COMMON_FLAGS_LIST/,/[^\\]$/p' "$MAKEFILE_CACHE" | \
        grep -oP '\$\(\w+\)' | tr -d '$()' | sort -u)

    while IFS= read -r dep_file; do
        [[ -n "$dep_file" ]] || continue
        local base
        base=$(basename "$dep_file" .dep)
        local mk_file="${dep_file%.dep}.mk"

        # Skip if no corresponding .mk
        [[ ! -f "$mk_file" ]] && continue

        local label
        label=$(pkg_label "$dep_file")

        # Filter if specific package requested
        if [[ -n "$FILTER_PACKAGE" ]] && [[ "$base" != "$FILTER_PACKAGE" ]]; then
            continue
        fi

        # Extract flags declared in .dep
        local dep_flags
        dep_flags=$(grep "_DEP_FLAGS" "$dep_file" 2>/dev/null | grep -oP '\$\(\w+\)' | tr -d '$()')

        # Extract conditional flags used in .mk. The original check only saw
        # ifeq/ifneq ($(FLAG)) directives; flags also gate builds via inline
        # $(if $(FLAG),...) and $(filter ...,$(FLAG)) functions and in recipe
        # value lines. Capture all of these forms so the coverage judgement is
        # comprehensive rather than conditional-directive-only.
        local mk_flags
        mk_flags=$(grep -oP '(?<=ifeq \(\$\()[\w]+(?=\))' "$mk_file" 2>/dev/null || true)
        mk_flags+=$'\n'
        mk_flags+=$(grep -oP '(?<=ifneq \(\$\()[\w]+(?=\))' "$mk_file" 2>/dev/null || true)
        mk_flags+=$'\n'
        # Inline $(if $(FLAG),...) function calls
        mk_flags+=$(grep -oP '\$\(if\s+\$\(\K[\w]+(?=\))' "$mk_file" 2>/dev/null || true)
        mk_flags+=$'\n'
        # $(filter ...,$(FLAG)) / $(filter-out ...,$(FLAG)) selectors
        mk_flags+=$(grep -oP '\$\(filter(?:-out)?[^,]*,\s*\$\(\K[\w]+(?=\))' "$mk_file" 2>/dev/null || true)

        # Filter to build-affecting flags. INSTALL_ is included so that
        # INSTALL_DEBUG_TOOLS (which adds debug packages to docker image content,
        # e.g. docker-base _DBG_PACKAGES) is evaluated; the install_only filter
        # below still suppresses cases where it merely selects an image to ship.
        local build_flags
        build_flags=$(echo "$mk_flags" | grep -E "^(ENABLE_|INCLUDE_|INSTALL_|SONIC_)" | sort -u)

        if [[ -z "$build_flags" ]]; then
            log_verbose "$base: No conditional build flags in .mk"
            continue
        fi

        # Check each build flag is covered
        while IFS= read -r flag; do
            [[ -z "$flag" ]] && continue

            # --- FALSE POSITIVE FILTERING (data-driven, no flag allow-lists) ---

            # Pattern 0: Effectively deprecated flags — pinned off for all modern
            # build envs (bookworm/trixie) in slave.mk, so their conditional is
            # dead code. Detected from slave.mk rather than a hardcoded name list.
            if flag_forced_off_for_modern_bldenv "$flag"; then
                log_verbose "$base: \$$flag is pinned off for modern BLDENV — downgrading to P3"
                add_finding "P3" "$label" \
                    "Flag \$$flag used but effectively deprecated (pinned to 'n' for bookworm/trixie in slave.mk)" \
                    "Low priority — only matters for legacy build envs where the flag can be 'y'"
                continue
            fi

            # Pattern 1: Flag's conditional in this .mk only controls image
            # assembly / installer inclusion (e.g. SONIC_INSTALL_DOCKER_IMAGES,
            # SONIC_PACKAGES_LOCAL) and does NOT touch any package build input.
            # Classified by build-signal inspection, not a hardcoded variable list.
            local block_content
            block_content=$(flag_blocks_in_file "$flag" "$mk_file")
            if [[ -n "$block_content" ]] && ! block_is_build_affecting "$block_content"; then
                log_verbose "$base: \$$flag only controls assembly/inclusion (no build signal) — skipping"
                continue
            fi

            # Pattern 1b: The flag may have been picked up from an inline
            # $(if $(FLAG),...) / $(filter ...,$(FLAG)) usage that has no ifeq
            # block for Pattern 1 to inspect. Fall back to the whole-tree
            # build-signal test: if the flag gates no package build input
            # ANYWHERE (rules/ + platform/**), it is assembly/inclusion-only
            # (e.g. SONIC_INSTALL_DOCKER_IMAGES, SONIC_PACKAGES_LOCAL) and is not
            # a cache-key gap. Data-driven — no variable allow-list.
            if [[ -z "$block_content" ]] && ! flag_affects_build "$flag"; then
                log_verbose "$base: \$$flag has no build signal anywhere (assembly/inclusion only) — skipping"
                continue
            fi

            # Check if flag is in DEP_FLAGS (directly or via SONIC_COMMON_FLAGS_LIST)
            if ! echo "$dep_flags" | grep -q "$flag"; then
                # Check if it's already in SONIC_COMMON_FLAGS_LIST (also check SONIC_ prefix variant)
                local flag_in_common=false
                if echo "$common_flags_list" | grep -q "^${flag}$"; then
                    flag_in_common=true
                elif echo "$common_flags_list" | grep -q "^SONIC_${flag}$"; then
                    flag_in_common=true
                fi

                if $flag_in_common; then
                    log_verbose "$base: $flag is in SONIC_COMMON_FLAGS_LIST (OK)"
                elif echo "$dep_flags" | grep -q "SONIC_COMMON_FLAGS_LIST"; then
                    # The dep uses SONIC_COMMON_FLAGS_LIST — check if the flag is there
                    if echo "$common_flags_list" | grep -q "^${flag}$" || \
                       echo "$common_flags_list" | grep -q "^SONIC_${flag}$"; then
                        log_verbose "$base: $flag covered via SONIC_COMMON_FLAGS_LIST"
                    else
                        add_finding "P1" "$label" \
                            "Flag \$$flag used in .mk conditional but not in DEP_FLAGS or SONIC_COMMON_FLAGS_LIST" \
                            "Add \$($flag) to ${base}_DEP_FLAGS in $base.dep, or add to SONIC_COMMON_FLAGS_LIST"
                        if $VERBOSE; then
                            echo -e "  ${YELLOW}GAP${NC}: $base uses \$$flag but doesn't track it"
                        fi
                    fi
                else
                    add_finding "P1" "$label" \
                        "Flag \$$flag used in .mk conditional but not tracked in DEP_FLAGS" \
                        "Add \$($flag) to DEP_FLAGS in $base.dep"
                fi
            fi
        done <<< "$build_flags"
    done < <(all_dep_files)
}

# --- Check 4: _DEPENDS declared in .mk vs what .dep tracks ---
# The .dep tracks file-level inputs. But _DEPENDS in .mk declares package-level deps.
# Those package deps are handled separately by Makefile.cache (via DEP_MOD_SHA).
# This check verifies that all _DEPENDS packages themselves have .dep files.
check_dependency_chain_coverage() {
    echo -e "\n${CYAN}=== Check 4: Dependency chain — do all dependencies have .dep files? ===${NC}"

    local missing_dep_count=0

    while IFS= read -r mk_file; do
        [[ -n "$mk_file" ]] || continue
        local base
        base=$(basename "$mk_file" .mk)

        [[ "$base" == "config" ]] || [[ "$base" == "functions" ]] && continue
        if [[ -n "$FILTER_PACKAGE" ]] && [[ "$base" != "$FILTER_PACKAGE" ]]; then
            continue
        fi

        # Extract _DEPENDS targets (the variable names, not filenames)
        local depends
        depends=$(grep -oP '\$\((\w+)\)_DEPENDS' "$mk_file" 2>/dev/null | head -1 | grep -oP '(?<=\$\()\w+(?=\))' || true)

        if [[ -z "$depends" ]]; then
            continue
        fi

        # Get the dependency list (right side of _DEPENDS assignment)
        local dep_packages
        dep_packages=$(grep "_DEPENDS" "$mk_file" | grep -oP '\$\(\w+\)' | grep -v "_DEPENDS\|_RDEPENDS\|_UNINSTALLS" | tr -d '$()')

        for dep_pkg_var in $dep_packages; do
            # Find which .mk defines this variable (assignment may be `=`, `:=`,
            # `+=` or `?=`), searching the whole corpus (rules/ + platform/**).
            local defining_mk
            defining_mk=$(grep -lP "^\s*${dep_pkg_var}\s*[:+?]?=" $(all_rule_mk_files) 2>/dev/null | head -1 || true)

            if [[ -n "$defining_mk" ]]; then
                local dep_base dep_rel
                dep_base=$(basename "$defining_mk" .mk)
                dep_rel="${defining_mk#"$REPO_ROOT"/}"
                if [[ ! -f "${defining_mk%.mk}.dep" ]]; then
                    ((missing_dep_count++))
                    add_finding "P2" "$(pkg_label "$mk_file")" \
                        "Depends on \$($dep_pkg_var) (from $dep_rel) which has no .dep file" \
                        "Cache key may not reflect changes in $dep_base"
                    log_verbose "$base depends on $dep_pkg_var → $dep_base has no .dep"
                fi
            fi
        done
    done < <(all_rule_mk_files)

    echo "  Found $missing_dep_count dependency chain gaps"
}

# --- Check 5: Source path exclusion patterns ---
# Some .dep files use grep -Ev to exclude files. Flag these for review.
# Filter out benign patterns like `grep -v " "` (filename sanitization).
check_exclusion_patterns() {
    echo -e "\n${CYAN}=== Check 5: Source file exclusion patterns in .dep files ===${NC}"

    while IFS= read -r dep_file; do
        [[ -n "$dep_file" ]] || continue
        local base
        base=$(basename "$dep_file" .dep)

        if [[ -n "$FILTER_PACKAGE" ]] && [[ "$base" != "$FILTER_PACKAGE" ]]; then
            continue
        fi

        # Look for grep -Ev or grep -v (exclusion patterns)
        local exclusions
        exclusions=$(grep -oP 'grep\s+-[Ev]+\s+"[^"]*"' "$dep_file" 2>/dev/null || true)

        if [[ -n "$exclusions" ]]; then
            # Filter out benign patterns:
            # - `grep -v " "` just removes filenames with spaces (sanitization)
            local is_benign=false
            if echo "$exclusions" | grep -qP 'grep\s+-v\s+" "'; then
                is_benign=true
                log_verbose "$base: Exclusion is just space-filtering (benign)"
            fi

            if ! $is_benign; then
                add_finding "P2" "$(pkg_label "$dep_file")" \
                    "Uses exclusion pattern: $exclusions" \
                    "Verify excluded files don't affect build output"
                if $VERBOSE; then
                    echo -e "  ${YELLOW}EXCLUSION${NC}: $base — $exclusions"
                fi
            fi
        fi
    done < <(all_dep_files)
}

# --- Check 6: CACHE_MODE analysis ---
# GIT_CONTENT_SHA vs GIT_COMMIT_SHA — the latter is stricter but causes more cache misses
check_cache_modes() {
    echo -e "\n${CYAN}=== Check 6: Cache mode analysis ===${NC}"

    local content_sha_count=0
    local commit_sha_count=0
    local no_mode_count=0
    local disabled_mode_count=0

    while IFS= read -r dep_file; do
        [[ -n "$dep_file" ]] || continue
        local base
        base=$(basename "$dep_file" .dep)

        if [[ -n "$FILTER_PACKAGE" ]] && [[ "$base" != "$FILTER_PACKAGE" ]]; then
            continue
        fi

        if grep -q "GIT_CONTENT_SHA" "$dep_file"; then
            ((content_sha_count++))
        elif grep -q "GIT_COMMIT_SHA" "$dep_file"; then
            ((commit_sha_count++))
            add_finding "P3" "$(pkg_label "$dep_file")" \
                "Uses GIT_COMMIT_SHA mode — any commit (even non-functional) invalidates cache" \
                "Consider GIT_CONTENT_SHA if commit metadata doesn't affect output"
        elif grep -qE "_CACHE_MODE[[:space:]]*:?=[[:space:]]*none" "$dep_file"; then
            # Explicitly opts out of caching — an intentional, clearly-set mode,
            # not a "missing mode" gap.
            ((disabled_mode_count++))
        else
            ((no_mode_count++))
            add_finding "P3" "$(pkg_label "$dep_file")" \
                "No explicit CACHE_MODE set (defaults may apply)" \
                "Consider explicitly setting CACHE_MODE for clarity"
        fi
    done < <(all_dep_files)

    echo "  GIT_CONTENT_SHA: $content_sha_count packages"
    echo "  GIT_COMMIT_SHA:  $commit_sha_count packages"
    echo "  CACHE_MODE=none: $disabled_mode_count packages"
    echo "  No explicit mode: $no_mode_count packages"
}

# --- Check 7: Docker images — verify Dockerfile.j2 directory is fully tracked ---
check_docker_dep_tracking() {
    echo -e "\n${CYAN}=== Check 7: Docker image .dep file completeness ===${NC}"

    while IFS= read -r dep_file; do
        [[ -n "$dep_file" ]] || continue
        [[ "$(basename "$dep_file")" == docker-*.dep ]] || continue
        [[ ! -f "$dep_file" ]] && continue
        local base
        base=$(basename "$dep_file" .dep)

        if [[ -n "$FILTER_PACKAGE" ]] && [[ "$base" != "$FILTER_PACKAGE" ]]; then
            continue
        fi

        # If the package explicitly disables caching (CACHE_MODE := none) there is
        # no stale-cache risk regardless of what the .dep tracks — skip it rather
        # than raise a spurious "doesn't track dir contents" gap.
        if grep -qE "_CACHE_MODE[[:space:]]*:?=[[:space:]]*none" "$dep_file"; then
            log_verbose "$base: CACHE_MODE=none (caching disabled) — not a tracking gap"
            continue
        fi

        # Check if dep tracks the Docker directory via git ls-files
        if ! grep -q "git ls-files" "$dep_file" && ! grep -q "shell.*ls" "$dep_file"; then
            # Check if it at least references the DPATH
            if ! grep -q "DPATH\|_PATH" "$dep_file"; then
                add_finding "P1" "$(pkg_label "$dep_file")" \
                    "Docker .dep doesn't appear to track Dockerfile directory contents" \
                    "Add: DEP_FILES += \$(shell git ls-files \$(DPATH))"
            fi
        fi

        # Check for SMDEP_FILES (docker images usually don't have them — they use DEP_FILES)
        # This is expected behavior, not a gap
        log_verbose "$base: Docker dep structure OK"
    done < <(all_dep_files)
}

# --- Check 8: Validate SONIC_COMMON_FLAGS_LIST against slave.mk usage ---
check_common_flags_completeness() {
    echo -e "\n${CYAN}=== Check 8: SONIC_COMMON_FLAGS_LIST vs actual build-affecting variables ===${NC}"

    local common_flags="$COMMON_FLAGS_LIST_CACHE"

    echo "  Current SONIC_COMMON_FLAGS_LIST:"
    echo "$common_flags" | sed 's/^/    /'

    # Look for ENABLE_*/INCLUDE_*/INSTALL_*/SONIC_* build flags used in slave.mk.
    # The original check only saw ifeq directives; flags also gate builds via
    # ifneq, inline $(if $(FLAG),...) and $(filter ...,$(FLAG)) forms. Capture
    # all of them so a flag hidden in a function call isn't silently missed.
    local slave_mk="$REPO_ROOT/slave.mk"
    local slave_flags
    slave_flags=$(grep -oP '(?<=ifeq \(\$\()(ENABLE_\w+|INCLUDE_\w+|INSTALL_\w+|SONIC_\w+)(?=\))' \
        "$slave_mk" 2>/dev/null)
    slave_flags+=$'\n'
    slave_flags+=$(grep -oP '(?<=ifneq \(\$\()(ENABLE_\w+|INCLUDE_\w+|INSTALL_\w+|SONIC_\w+)(?=\))' \
        "$slave_mk" 2>/dev/null)
    slave_flags+=$'\n'
    slave_flags+=$(grep -oP '\$\(if\s+\$\(\K(ENABLE_\w+|INCLUDE_\w+|INSTALL_\w+|SONIC_\w+)(?=\))' \
        "$slave_mk" 2>/dev/null)
    slave_flags+=$'\n'
    slave_flags+=$(grep -oP '\$\(filter(?:-out)?[^,]*,\s*\$\(\K(ENABLE_\w+|INCLUDE_\w+|INSTALL_\w+|SONIC_\w+)(?=\))' \
        "$slave_mk" 2>/dev/null)
    slave_flags=$(echo "$slave_flags" | grep -v '^$' | sort -u)

    echo ""
    echo "  Flags used in slave.mk conditionals but NOT tracked in any cache key:"

    # A slave.mk flag is only a real gap if it (a) actually affects package build
    # output AND (b) is not tracked anywhere (global list or per-package DEP_FLAGS).
    # Both tests are data-driven, so no assembly-only / per-package allow-lists are
    # maintained: flags that merely drive image assembly are filtered by
    # flag_affects_build, and flags tracked per-package are filtered by
    # flag_tracked_anywhere.
    local gap_count=0
    while IFS= read -r flag; do
        [[ -z "$flag" ]] && continue

        if ! flag_affects_build "$flag"; then
            log_verbose "  $flag — no build signal (assembly/inclusion/orchestration only)"
            continue
        fi
        if flag_tracked_anywhere "$flag"; then
            log_verbose "  $flag — tracked globally or per-package (OK)"
            continue
        fi

        ((gap_count++))
        echo -e "    ${YELLOW}$flag${NC}"
        add_finding "P2" "Makefile.cache" \
            "slave.mk uses \$$flag in a build-affecting conditional but it's tracked in neither SONIC_COMMON_FLAGS_LIST nor any package DEP_FLAGS" \
            "Add \$($flag) to SONIC_COMMON_FLAGS_LIST, or to the DEP_FLAGS of each affected package"
    done <<< "$slave_flags"

    if [[ $gap_count -eq 0 ]]; then
        echo -e "    ${GREEN}None — all covered (or tracked per-package / assembly-only)${NC}"
    fi
}

# --- Check 9: Nested derived packages (cache save/restore completeness) ---
# add_derived_package(X, Y) only appends Y to X's first-level _DERIVED_DEBS.
# The cache save logic (Makefile.cache) tars the MAIN package plus its
# _DERIVED_DEBS only — it does NOT recurse. If X is itself a derived package
# (i.e. X appeared as the child/2nd arg of another add_derived_package call),
# then Y lives under X's _DERIVED_DEBS, not the top-level main package's, so Y
# is silently dropped from the cache tarball. A cache HIT then restores the
# main deb and its first-level derived debs but MISSES the nested ones.
# This is the exact class of bug confirmed by negative control NC-6 (libnl3).
check_nested_derived_packages() {
    echo -e "\n${CYAN}=== Check 9: Nested derived packages (cache completeness) ===${NC}"

    local found=0
    local mk
    while IFS= read -r mk; do
        [[ -n "$mk" ]] || continue
        [[ -f "$mk" ]] || continue
        grep -q 'add_derived_package' "$mk" || continue
        if [[ -n "$FILTER_PACKAGE" && "$(basename "$mk" .mk)" != "$FILTER_PACKAGE" ]]; then
            continue
        fi

        # Build PARENT,CHILD pairs from each add_derived_package(parent, child) call
        local pairs
        pairs=$(grep -oE 'add_derived_package,[^,]+,[^)]+\)' "$mk" \
            | sed -E 's/add_derived_package,//' | tr -d ' $()')
        [[ -z "$pairs" ]] && continue

        local parents children
        parents=$(echo "$pairs" | cut -d, -f1 | sort -u)
        children=$(echo "$pairs" | cut -d, -f2 | sort -u)

        local p
        for p in $parents; do
            [[ -z "$p" ]] && continue
            # A parent that is also a child = nested derivation
            if echo "$children" | grep -qxF "$p"; then
                local grandchildren
                grandchildren=$(echo "$pairs" | awk -F, -v par="$p" '$1==par {print $2}' \
                    | sort -u | tr '\n' ' ')
                add_finding "P1" "$(pkg_label "$mk")" \
                    "Nested add_derived_package: \$$p is itself a derived deb; its derived debs ($grandchildren) are absent from the top-level package _DERIVED_DEBS and may be dropped from cache save/restore" \
                    "Register nested derived debs against the top-level main package, or confirm coverage with negative control NC-6"
                echo -e "  ${RED}GAP${NC}: $(basename "$mk"): nested parent \$$p -> derived debs: $grandchildren"
                ((found++))
            fi
        done
    done < <(all_rule_mk_files)

    if [[ $found -eq 0 ]]; then
        echo -e "  ${GREEN}None — no nested add_derived_package chains found${NC}"
    fi
}

# --- Check 10: Exported build-env variables vs the cache key (live whole-tree scan) ---
# Some build inputs never appear in an ifeq($(FLAG)) conditional; they are simply
# `export`ed from slave.mk / a rules/*.mk and consumed inside a package's
# debian/rules, source Makefile, or Dockerfile.j2 — or read straight from the
# environment by an external tool (docker, cargo, dpkg). Checks 3/8 only inspect
# ifeq/ifneq blocks, so such an export-only input that changes build output but is
# absent from SONIC_COMMON_FLAGS_LIST and every per-package *_DEP_FLAGS is invisible
# to them — flipping it silently reuses a stale cached artifact (the
# DOCKER_DEFAULT_PLATFORM class of bug).
#
# DESIGN: NO name heuristics, NO reliance on the Tier-2 rebuild, NO committed
# trust-store that can drift. scripts/cache_key_scan.py enumerates EVERY exported
# variable from live git-tracked source and classifies each by concrete evidence:
#   in-key    -> literally in SONIC_COMMON_FLAGS_LIST / a *_DEP_FLAGS. PROVABLY safe.
#   filename  -> value flows into a cached target's *.deb/*.whl/*.gz name. PROVABLY safe.
#   gap       -> referenced by a cached build recipe but untracked. REPORTED (P1).
#   assembly  -> referenced in the make/assembly layer, no cached-recipe consumer.
#                NOT provably safe (can still reach a cached `docker build`). REPORTED (P2).
#   external-env -> exported, NO in-repo consumer (read from the env by an external
#                tool). Cannot be proven safe by static analysis. REPORTED (P2).
# Only in-key/filename auto-clear (both recomputed live). Everything else is a
# finding (default-deny). A human clears a specific variable by recording it — WITH
# A REASON — in scripts/cache_key_export_waivers.tsv.
#
# PR/diff mode (--base REF): any not-provably-safe, un-waived export the PR TOUCHES
# is escalated to P0 (a hard gate) — this is exactly how DOCKER_DEFAULT_PLATFORM
# would be caught on the PR that introduces it. A full run (no --base) reports the
# whole standing problem set: gap=P1, assembly/external-env=P2.
check_exported_flags() {
    echo -e "\n${CYAN}=== Check 10: Exported build-env variables vs cache key (live scan) ===${NC}"

    if ! compute_export_scan; then
        add_finding "P1" "cache_key_scan.py" \
            "Export scanner $EXPORT_SCANNER did not run — cannot verify exported build-env variables against the cache key" \
            "Ensure python3 and scripts/cache_key_scan.py are present; run: $0 --refresh-registry to test it"
        echo -e "  ${YELLOW}Scanner unavailable${NC}"
        return
    fi

    local gap_count=0 unproven_count=0 p0_count=0 waived_count=0
    while IFS=$'\t' read -r flag disp ev; do
        [[ -z "$flag" ]] && continue
        # In diff mode, only judge variables the PR actually touches.
        if $DIFF_MODE && ! diff_touches_var "$flag"; then
            continue
        fi

        # Provably cache-safe — no finding.
        case "$disp" in
            in-key|filename)
                log_verbose "  $flag — $disp (provably cache-safe)"
                continue
                ;;
        esac
        # Belt-and-suspenders: honor a live cache-key membership too.
        if flag_tracked_anywhere "$flag"; then
            log_verbose "  $flag — tracked in a cache key (OK)"
            continue
        fi

        # Human-justified waiver — the only way a not-provable export is cleared.
        local reason
        reason=$(waiver_reason "$flag")
        if [[ -n "$reason" ]]; then
            ((waived_count++))
            log_verbose "  $flag — waived: $reason"
            add_finding "P3" "$flag" \
                "Exported build-env variable waived from cache-key tracking (human-justified): $reason" \
                "Re-verify the waiver reason still holds if this variable's build usage changes"
            continue
        fi

        # A real finding. Standing severity by evidence strength; PR-touched -> P0.
        local sev msg
        if $DIFF_MODE; then
            sev="P0"; ((p0_count++))
        elif [[ "$disp" == "gap" ]]; then
            sev="P1"; ((gap_count++))
        else
            sev="P2"; ((unproven_count++))
        fi
        case "$disp" in
            gap)
                msg="Exported and consumed by a cached build recipe (${ev:-recipe}) but tracked in neither SONIC_COMMON_FLAGS_LIST nor any package DEP_FLAGS — changing it alters the built artifact without flipping the cache key (stale-cache risk)"
                ;;
            assembly)
                msg="Exported into the build environment (referenced in the make/assembly layer) but not tracked in any cache key and not provably cache-safe — an assembly-layer variable can still reach a 'docker build' whose image is cached (the DOCKER_DEFAULT_PLATFORM class)"
                ;;
            *)
                msg="Exported into the build environment with NO in-repo consumer — it is read straight from the environment by an external build tool (docker/cargo/dpkg/...), which static analysis cannot see inside, so it cannot be proven cache-safe (the DOCKER_DEFAULT_PLATFORM class)"
                ;;
        esac
        add_finding "$sev" "$flag" "$msg" \
            "If it can affect any cached artifact, add \$($flag) to SONIC_COMMON_FLAGS_LIST or the relevant package DEP_FLAGS. If it provably cannot, record it (with a reason) in scripts/cache_key_export_waivers.tsv."
        $VERBOSE && echo -e "  ${RED}$sev${NC} \$$flag ($disp${ev:+: $ev})"
    done <<< "$EXPORT_SCAN_CACHE"

    if [[ $((gap_count + unproven_count + p0_count)) -eq 0 ]]; then
        echo -e "  ${GREEN}None — every exported build variable is provably cache-safe or waived${NC}"
    else
        [[ $p0_count -gt 0 ]] && echo -e "  ${RED}$p0_count PR-introduced untracked export(s) (P0)${NC}"
        [[ $gap_count -gt 0 ]] && echo -e "  ${YELLOW}$gap_count exported var(s) consumed by a cached recipe but untracked (P1)${NC}"
        [[ $unproven_count -gt 0 ]] && echo "  $unproven_count exported var(s) not provably cache-safe — review or waive (P2)"
    fi
    [[ $waived_count -gt 0 ]] && log_verbose "$waived_count exported var(s) cleared by recorded waivers"
}

# --- Output Results ---
print_summary() {
    echo -e "\n${CYAN}============================================${NC}"
    echo -e "${CYAN}=== AUDIT SUMMARY ===${NC}"
    echo -e "${CYAN}============================================${NC}"
    echo ""
    echo "  Total .dep files analyzed: $TOTAL_DEP_FILES"
    echo ""
    echo -e "  ${RED}P0 (Confirmed stale cache risk):${NC}  $FINDINGS_P0"
    echo -e "  ${YELLOW}P1 (Likely stale cache risk):${NC}     $FINDINGS_P1"
    echo -e "  P2 (Potential risk / verify):        $FINDINGS_P2"
    echo -e "  P3 (Informational):                  $FINDINGS_P3"
    echo ""

    if [[ ${#FINDINGS[@]} -eq 0 ]]; then
        echo -e "  ${GREEN}No findings! All .dep files appear complete.${NC}"
        return
    fi

    # Print findings table
    echo -e "  ${CYAN}--- Findings Detail ---${NC}"
    printf "  %-4s | %-30s | %-60s | %s\n" "SEV" "PACKAGE" "ISSUE" "SUGGESTION"
    printf "  %-4s-+-%-30s-+-%-60s-+-%s\n" "----" "------------------------------" "------------------------------------------------------------" "----------"

    # Sort by severity
    IFS=$'\n' sorted=($(sort <<< "${FINDINGS[*]}")); unset IFS

    for finding in "${sorted[@]}"; do
        IFS=$FINDING_FS read -r sev pkg issue suggestion <<< "$finding"
        local color="$NC"
        case $sev in
            P0) color="$RED" ;;
            P1) color="$YELLOW" ;;
        esac
        printf "  ${color}%-4s${NC} | %-30s | %-60s | %s\n" "$sev" "$pkg" "$issue" "$suggestion"
    done
}

print_json() {
    echo "["
    local first=true
    for finding in "${FINDINGS[@]}"; do
        IFS=$FINDING_FS read -r sev pkg issue suggestion <<< "$finding"
        if $first; then
            first=false
        else
            echo ","
        fi
        printf '  {"severity": "%s", "package": "%s", "issue": "%s", "suggestion": "%s"}' \
            "$(json_escape "$sev")" "$(json_escape "$pkg")" \
            "$(json_escape "$issue")" "$(json_escape "$suggestion")"
    done
    echo ""
    echo "]"
}

# --- Main ---
main() {
    # In JSON mode, route all human-readable progress to stderr so that stdout
    # carries only the JSON payload (emitted by print_json after restoring fd 1).
    if $JSON_OUTPUT; then
        exec 3>&1 1>&2
    fi

    echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║  SONiC DPKG Cache — Dependency Tracking Completeness Audit  ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "  Repository: $REPO_ROOT"
    echo "  Date: $(date -u '+%Y-%m-%d %H:%M UTC')"

    # Validate we're in the right directory
    if [[ ! -f "$MAKEFILE_CACHE" ]]; then
        echo -e "${RED}ERROR: Makefile.cache not found. Run from sonic-buildimage root.${NC}"
        exit 2
    fi

    TOTAL_DEP_FILES=$(all_dep_files | grep -c .)
    echo "  Total .dep files found: $TOTAL_DEP_FILES (rules/ + platform/**)"

    if [[ -n "$FILTER_PACKAGE" ]]; then
        echo -e "  ${CYAN}Filtering to package: $FILTER_PACKAGE${NC}"
    fi

    # Precompute the global flag list once for the data-driven classifiers.
    compute_common_flags_list
    compute_diff_vars

    # Run all checks
    check_missing_dep_files
    check_common_base_files
    check_dep_flags_coverage
    check_dependency_chain_coverage
    check_exclusion_patterns
    check_cache_modes
    check_docker_dep_tracking
    check_common_flags_completeness
    check_nested_derived_packages
    check_exported_flags

    # Output
    if $JSON_OUTPUT; then
        exec 1>&3 3>&-   # restore stdout for the JSON payload
        print_json
    else
        print_summary
    fi

    # Exit code based on findings
    if [[ $FINDINGS_P0 -gt 0 ]]; then
        exit 1
    elif [[ $FINDINGS_P1 -gt 0 ]]; then
        exit 0  # Warnings but not blocking
    else
        exit 0
    fi
}

main "$@"
