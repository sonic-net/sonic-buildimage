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
# ── The following checks close blind spots an independent multi-model LLM audit
#    found that the flag-oriented checks above cannot model (file / dep-edge /
#    non-determinism classes). Each is data-driven and evidence-gated. ──
#
# Check 11: Build flags consumed ONLY inside a package's source recipe
#           (debian/rules, source Makefile, setup.py) but absent from _DEP_FLAGS.
#           Checks 3/8 only read .mk ifeq blocks and slave.mk; a flag read straight
#           from the environment inside the submodule recipe (e.g. ENABLE_ASAN in
#           sonic-sairedis/debian/rules, ENABLE_GCOV in sonic-swss) is invisible to
#           them, so toggling it silently reuses a stale artifact.
#
# Check 12: In-repo Jinja2 templates a Dockerfile.j2 {% include/import/from %}s
#           but the image's .dep does not track. dockers/dockerfile-macros.j2 is
#           imported by ~50 images yet lives outside each image's $(DPATH) glob, so
#           editing the shared macro invalidates almost no image cache.
#
# Check 13: Package built from a real source tree (_SRC_PATH set) with caching ON
#           but whose .dep hashes NO source at all (no _SMDEP_FILES and no
#           `git ls-files` of the source) — the entire source is outside the key
#           (e.g. bmcweb / sonic-dbus-bridge in rules/sonic-redfish.dep).
#
# Check 14: Moving-ref / non-deterministic fetch behind a stable cache key —
#           raw/master|main URLs, :latest base images, unpinned `go get`, meson
#           wrap revision=HEAD/main. The artifact changes upstream while the key
#           does not, so a stale build is reused (e.g. gobgp `go get`).
#
# Check 15: Structural DEP_FILES bugs that silently drop source from the key:
#           (a) aliasing another package's SPATH-derived DEP_FILES so the package's
#               OWN source is never hashed (sflowtool/psample, libnss-radius);
#           (b) `SPATH := $(PKG)_SRC_PATH` missing the inner `$(...)` so the glob
#               resolves to nothing (platform/mellanox/rshim.dep);
#           (c) a second `DEP_FILES :=` (colon-equals, not `+=`) that discards the
#               already-accumulated common/rule files (rules/eventd.dep).
#
# ── Checks 16–17 extend the same "recipe consumes an input outside the key"
#    principle to the two build targets whose inputs are NOT declared in a
#    rules/*.dep at all (so Checks 1–15 never look at them): the host
#    root-filesystem image, and the shared docker build-context tooling. ──
#
# Check 16: Host root-filesystem (SONIC_RFS_TARGETS) files consumed by
#           build_debian.sh (or a script it invokes) but absent from the
#           hand-maintained RFS_DEP_FILES list in Makefile.cache. The rootfs key
#           is GIT_CONTENT_SHA over RFS_DEP_FILES, yet build_debian.sh sources
#           functions.sh, copies files/scripts/*, renders extra
#           files/build_templates/*.j2, and runs several scripts/*.{sh,py} — none
#           tracked, so editing them reuses a stale squashfs. Data-driven: parses
#           the actual RFS_DEP_FILES globs and the actual recipe's file reads.
#
# Check 17: Docker build-context generators consumed by EVERY docker image build
#           but hashed into NO docker cache key. slave.mk runs
#           scripts/prepare_docker_buildinfo.sh, which invokes
#           scripts/versions_manager.py / docker_version_control.sh and copies
#           src/sonic-build-hooks/buildinfo/* into the context. None are in
#           SONIC_COMMON_FILES_LIST (only .platform rules/functions Makefile.cache)
#           nor under any image's $(DPATH), so editing them silently reuses stale
#           images. (The version OUTPUT under files/build/versions/** IS folded in
#           at Makefile.cache; only the generator logic is untracked.)
#
# ── Check 19 closes a further "consumed input outside the key" gap that an
#    independent multi-model audit surfaced after Checks 16–17. ──
#
# Check 18: (invariant verifier — NOT a gap) Docker _INSTALL_PYTHON_WHEELS /
#           _INSTALL_DEBS packages are NOT baked into the image, so Makefile.cache's
#           docker dependency-SHA rollup (GET_MOD_DEP_SHA -> MOD_DEP_PKGS) correctly
#           omits them. A multi-model review (opus/gpt/gemini, unanimous) plus direct
#           tracing established: slave.mk's docker recipe lists _INSTALL_* only as
#           "%-install" PREREQUISITES (slave.mk ~L1237-1238); those %-install rules
#           run `dpkg -i` / `pip install` on the build HOST, never inside the image.
#           The image content is produced solely from the j2 vars _debs/_whls/_pydebs,
#           which derive strictly from _DEPENDS/_RDEPENDS, _PYTHON_WHEELS and
#           _PYTHON_DEBS (slave.mk ~L1263-1265) — _INSTALL_* is never referenced by
#           any Dockerfile.j2 (proof: docker-gnmi-sidecar declares both _INSTALL_*
#           lists yet its Dockerfile installs zero packages). Their real purpose is
#           host-side provisioning for the build-time cli-plugin-tests pytest run
#           (slave.mk ~L1270). Folding them into the docker key (as an earlier draft
#           proposed) would only OVER-invalidate the image cache for zero correctness
#           gain. This check therefore verifies the host-only invariant and flags ONLY
#           the genuine defect: an _INSTALL_* package that IS also baked into the image
#           (appears in an image-content role) yet is somehow not folded — a condition
#           that does not currently occur.
#
# Check 19: An online-deb target that reuses another target's HTTP header
#           fingerprint (wget --spider --server-response, used to defeat a moving
#           upstream softlink) in its _DEP_FLAGS while its OWN artifact URL is
#           never spidered — so its real moving remote identity is not in the key
#           (e.g. BRCM_DNX_SAI reuses the XGS-derived SAI_FLAGS). Data-driven:
#           parses the spidered URL set and the consuming targets from the recipe.
#
# ── Check 20 closes the "in-repo patch content applied before the build but not
#    folded into the artifact's cache key" class (two mechanisms). ──
#
# Check 20A (quilt patch series): slave.mk applies $(pkg)_SRC_PATH).patch/series
#           to a source-built package via 'QUILT_PATCHES=... quilt push -a' right
#           before dpkg-buildpackage / make / wheel builds. When the source root
#           is a submodule, the sibling <root>.patch/ dir lives in the PARENT
#           repo, so _SMDEP_FILES (which runs 'git ls-files' INSIDE the submodule)
#           can never see it, and no _DEP_FILES enumerates it — editing a patch
#           reuses a stale cached deb (e.g. src/sonic-swss.patch, ptf, scapy,
#           supervisor, redis-dump-load, sonic-dash-ha, ptf-py3). Coverage is
#           decided empirically per patch dir: a series is covered iff some
#           resolved source root T satisfies 'git -C T ls-files <rel>' (which,
#           for a submodule root, excludes a parent-repo sibling) — so nested
#           patch dirs under a normal (non-submodule) root (p4lang/*, thrift_0_14_1)
#           are correctly NOT flagged. Evidence-gated on the slave.mk quilt recipe.
#
# Check 20B (external kernel patches): a platform overrides EXTERNAL_KERNEL_PATCH_LOC
#           (e.g. platform/mellanox/non-upstream-patches/); src/sonic-linux-kernel/
#           Makefile copies + applies that dir's *.patch when INCLUDE_EXTERNAL_PATCHES=y,
#           but the linux-kernel cache key folds only the INCLUDE_EXTERNAL_PATCHES
#           *flag* in _DEP_FLAGS — never the tracked patch content — so editing the
#           patch while the flag stays 'y' reuses a stale cached kernel. Evidence-
#           gated on the linux-kernel recipe consuming EXTERNAL_KERNEL_PATCH_LOC.
#
# ── Check 21 closes the "inline (non-export) env VALUE consumed by a cached recipe
#    but absent from the key" class that Check 10 (exported-only) is blind to. ──
#
# Check 21: slave.mk's rootfs (RFS) recipe invokes ./build_debian.sh with a long
#           prefix of INLINE, per-command environment assignments
#           (VAR=$(VAR) ./build_debian.sh) instead of `export`ed variables.
#           build_debian.sh reads those VALUES and bakes them into the rootfs that
#           the recipe then SAVE_CACHEs, yet the RFS target's cache key folds only
#           SONIC_COMMON_FLAGS_LIST. Because Check 10 scans EXPORTED variables only,
#           these inline values are a structural blind spot — e.g. the default admin
#           USERNAME/PASSWORD, CHANGE_DEFAULT_PASSWORD, the BMC account credentials,
#           MASTER_KUBERNETES_VERSION and MASTER_CRI_DOCKERD, IMAGE_TYPE, DBGOPT:
#           changing any of them bakes new content into the rootfs while the same
#           stale cached squashfs is restored. Data-driven: parse the inline env
#           prefix of every SAVE_CACHE-backed ./build_debian.sh recipe, keep only the
#           vars build_debian.sh actually reads, then drop those already in the key,
#           those exported (deferred to Check 10), and those human-waived. Identity /
#           plumbing values ($* target-stem name, its derived TARGET_MACHINE, the
#           TARGET_PATH output dir, SONIC_VERSION_CACHE) are cleared via reasoned
#           waivers in scripts/cache_key_export_waivers.tsv; everything else is
#           default-deny (P1).
#
# ── Checks 25–26 close the two remaining "build-environment identity not in the
#    key" classes: the slave.mk recipe body, and the slave/toolchain image. ──
#
# Check 25: slave.mk is intentionally NOT hashed into any DEP_FILES list; instead a
#           MANUAL version stamp, SONIC_CACHE_RECIPE_VER, is folded into every key via
#           SONIC_COMMON_FLAGS_LIST, and a git-object baseline (SONIC_CACHE_RECIPE_VER_
#           BASELINE) records the slave.mk content it was last reviewed against. Two
#           ways this silently fails: (a) the stamp is removed from SONIC_COMMON_FLAGS_
#           LIST — then no key tracks the recipe body at all; (b) slave.mk drifts from
#           the baseline without the stamp being bumped — packages built with the old
#           recipe are served stale. Data-driven: parse the stamp membership and
#           compare git hash-object slave.mk to the recorded baseline.
#
# Check 26: package keys capture the slave build environment only through the
#           sonic-slave-<distro> Dockerfile SOURCE (SONIC_COMMON_BASE_FILES_LIST).
#           The identity of the BUILT slave image (SLAVE_TAG / the resolved base-image
#           digest / the apt package versions installed into it) is in NO package key,
#           so a toolchain change that does not edit the Dockerfile source — a moving
#           base tag, an upstream apt update — reuses stale cached packages. Reports
#           the structural omission (no slave-image identity token in the common key)
#           and, as evidence, each sonic-slave base image pinned by a moving tag.
#
# Check 27: output-affecting GLOBAL build-mode selectors that are not encoded in the
#           output filename and not one of CONFIGURED_PLATFORM/CONFIGURED_ARCH/BLDENV
#           (which ARE in SONIC_COMMON_FLAGS_LIST). CROSS_BUILD_ENVIRON gates the
#           CROSS_COMPILE_FLAGS in the dpkg-buildpackage recipe, so a native vs cross
#           build of the SAME arch produces different binaries under the same key. If
#           such a selector is used by a build recipe but is absent from SONIC_COMMON_
#           FLAGS_LIST, the two builds collide on one cache entry. This is the residual
#           of the variant/mode-collision class NOT already covered by CONFIGURED_*.
#
# Check 28: helper scripts invoked by slave.mk build recipes (scripts/*, src/sonic-
#           build-hooks/*) that shape the CONTENT of produced artifacts — buildinfo /
#           version-pin / debug-file injection into images — but are not in SONIC_
#           COMMON_FILES_LIST nor any _DEP_FILES. Editing such a script changes every
#           affected output without moving any cache key. Data-driven: enumerate the
#           scripts referenced in recipes and verify each is hashed by a file list.
#
# Check 29: embed-without-edge. A package's debian/rules extracts a SIBLING built deb
#           (dpkg -x of target/debs/<env>/<name>_*.deb) and copies content out of it,
#           yet declares no _DEPENDS edge on <name>. The embedded binary is baked into
#           the package but rebuilding <name> does not invalidate this package's key,
#           so it is served stale. Data-driven: map each embedding debian/rules back to
#           its owning rules .mk (via _SRC_PATH), and flag every embedded sibling whose
#           name is absent from the aggregated _DEPENDS/_RDEPENDS of that package.
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

# True (0) if $1 is a real package .dep (declares cache directives) rather than a
# pure include-aggregator fragment. Several platforms ship a rules.dep whose whole
# body is `include $(PLATFORM_PATH)/<pkg>.dep` lines — it defines no package and
# intentionally carries no CACHE_MODE/DEP_FILES, so per-package checks must not
# treat it as a package (doing so yields spurious "no CACHE_MODE" / exclusion
# findings). Decided structurally from the file's own directives, not by name.
is_package_dep() {
    grep -qE '(DEP_FILES|SMDEP_FILES|_CACHE_MODE|_DEP_FLAGS)' "$1" 2>/dev/null
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

# True (0) if $flag is used in an ifeq/ifneq/if(n)def conditional in the given .mk
# file — i.e. Check 3 already models it FOR THIS PACKAGE, so Check 11 (source-recipe
# scan) must not double-report it. Scoped per-package because Check 3 judges each
# package against its OWN .mk; a flag conditional in a different package's .mk does
# not cover this one.
flag_in_mk_conditional() {
    local flag="$1" mk="$2"
    [[ -f "$mk" ]] || return 1
    grep -qE "if(eq|neq|def|ndef)[[:space:]]*\(?\\\$\\(${flag}\\)" "$mk" 2>/dev/null
}

# True (0) if $flag is an EXPORTED build-env variable (Check 10's whole-tree export
# scanner lists it), so Check 11 should defer to Check 10 for it (no duplication).
flag_is_exported() {
    compute_export_scan || return 1
    awk -F'\t' -v v="$1" '$1==v {found=1} END{exit(found?0:1)}' <<< "$EXPORT_SCAN_CACHE"
}

# SONIC_COMMON_FLAGS_LIST contents, computed once (used by classifiers above).
compute_common_flags_list() {
    COMMON_FLAGS_LIST_CACHE=$(sed -n '/^SONIC_COMMON_FLAGS_LIST/,/[^\\]$/p' "$MAKEFILE_CACHE" \
        | grep -oP '\$\(\w+\)' | tr -d '$()' | sort -u)
}

# Resolve the on-disk source directory a package's *_SRC_PATH points at, from its
# .mk. Only the unambiguous `$(SRC_PATH)/<literal>` and `$(PLATFORM_PATH)/<literal>`
# forms are resolved (SRC_PATH=src; PLATFORM_PATH=platform/<vendor>); nested-variable
# forms (e.g. $($(X)_SRC_PATH)) are intentionally left unresolved and skipped so the
# checks never guess. Echoes an absolute dir that exists, or nothing.
resolve_src_path() {
    local mk_file="$1"
    local raw
    raw=$(grep -hoP '_SRC_PATH\s*[:?]?=\s*\K\S.*' "$mk_file" 2>/dev/null | head -1)
    [[ -n "$raw" ]] || return 0
    local rel=""
    if [[ "$raw" =~ ^\$\(SRC_PATH\)/([A-Za-z0-9._/-]+)$ ]]; then
        rel="src/${BASH_REMATCH[1]}"
    elif [[ "$raw" =~ ^\$\(PLATFORM_PATH\)/([A-Za-z0-9._/-]+)$ ]]; then
        local vendor_rel="${mk_file#"$REPO_ROOT"/}"
        vendor_rel="${vendor_rel%/*}"   # dir holding the .mk == PLATFORM_PATH
        rel="${vendor_rel}/${BASH_REMATCH[1]}"
    else
        return 0
    fi
    rel="${rel%/}"
    [[ -d "$REPO_ROOT/$rel" ]] && echo "$REPO_ROOT/$rel"
}

# The package variable names ($(FOO)) a .dep assigns cache directives to, e.g.
# BMCWEB from `$(BMCWEB)_DEP_FILES`. Used to relate DEP_FILES/SPATH to packages.
dep_pkg_vars() {
    grep -oP '^\s*\$\(\K[A-Z0-9_]+(?=\)_(CACHE_MODE|DEP_FLAGS|DEP_FILES|SMDEP_FILES))' \
        "$1" 2>/dev/null | sort -u
}

# Repo-relative source dir for a SPECIFIC package variable's *_SRC_PATH in a .mk,
# resolving only the unambiguous $(SRC_PATH)/... and $(PLATFORM_PATH)/... forms
# (else nothing). Used to prove two packages sharing one SPATH actually have
# DIFFERENT source trees before flagging an aliasing bug (FP-safe).
pkg_var_src_dir() {
    local mk_file="$1" var="$2" raw rel=""
    raw=$(grep -oP "\\\$\\($var\\)_SRC_PATH\s*[:?]?=\s*\K\S.*" "$mk_file" 2>/dev/null | head -1)
    [[ -n "$raw" ]] || return 0
    if [[ "$raw" =~ ^\$\(SRC_PATH\)/([A-Za-z0-9._/-]+)$ ]]; then
        rel="src/${BASH_REMATCH[1]}"
    elif [[ "$raw" =~ ^\$\(PLATFORM_PATH\)/([A-Za-z0-9._/-]+)$ ]]; then
        local vendor_rel="${mk_file#"$REPO_ROOT"/}"; vendor_rel="${vendor_rel%/*}"
        rel="${vendor_rel}/${BASH_REMATCH[1]}"
    else
        return 0
    fi
    echo "${rel%/}"
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
#
# Two classes of dependency are deliberately NOT flagged (they were previously
# reported but are false positives, confirmed by cache-mechanism review):
#   1. Docker-image dependencies. SONIC_DOCKER_IMAGES/*_DOCKERS targets are
#      content-keyed by their own mechanism (SHA_DEP_RULES folds
#      files/build/versions/** and _FILES into their DEP_FILES, and DEP_MOD_SHA
#      recursively folds their DEPENDS' hashes). They intentionally carry no
#      rules/*.dep, so "no .dep file" does not imply a stale cache key.
#   2. Dependencies whose defining .mk is already listed in the CONSUMER's own
#      .dep DEP_FILES. Editing that .mk (including a pinned vendor version bump)
#      already changes the consumer's cache key, so the dependency's missing
#      .dep is not a staleness gap for this consumer.
# The genuine gaps are from-source / version-pinned online libs (e.g. grpc,
# libsaithrift-dev, saiserver) with no .dep, embedded in a cacheable consumer
# that does not otherwise track them.

# Memoised set of every package variable registered into any SONiC docker-image
# list (SONIC_DOCKER_IMAGES, SONIC_DOCKER_DBG_IMAGES, SONIC_<DEB>_DOCKERS, ...).
DOCKER_TARGET_SET_CACHE=""
docker_target_set() {
    if [[ -z "$DOCKER_TARGET_SET_CACHE" ]]; then
        DOCKER_TARGET_SET_CACHE=$(all_rule_mk_files | tr '\n' '\0' | xargs -0 grep -rhoP \
            'SONIC_[A-Z_]*DOCKER[A-Z_]*\s*\+=\s*\$\(\K\w+(?=\))' 2>/dev/null | sort -u)
    fi
    printf '%s\n' "$DOCKER_TARGET_SET_CACHE"
}
is_docker_target() {
    [[ -n "$1" ]] && docker_target_set | grep -qx "$1"
}

# True when the consumer's .dep already tracks the dependency's defining .mk in
# its DEP_FILES (by repo-relative path or basename).
consumer_dep_covers_mk() {
    local consumer_dep="$1" dep_mk="$2"
    [[ -f "$consumer_dep" ]] || return 1
    local dep_mk_rel dep_mk_base
    dep_mk_rel="${dep_mk#"$REPO_ROOT"/}"
    dep_mk_base=$(basename "$dep_mk")
    grep -qF -e "$dep_mk_rel" -e "$dep_mk_base" "$consumer_dep"
}

check_dependency_chain_coverage() {
    echo -e "\n${CYAN}=== Check 4: Dependency chain — do all dependencies have .dep files? ===${NC}"

    local missing_dep_count=0
    local suppressed_docker=0
    local suppressed_covered=0

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
                    # Suppression 1: docker-image dependencies are keyed by their
                    # own mechanism (SHA_DEP_RULES versions/ injection + recursive
                    # DEP_MOD_SHA), not rules/*.dep — a missing .dep is an FP.
                    if is_docker_target "$dep_pkg_var"; then
                        ((suppressed_docker++))
                        log_verbose "$base → \$$dep_pkg_var is a docker target; keyed separately (suppressed)"
                        continue
                    fi
                    # Suppression 2: the consumer's own .dep already lists the
                    # dependency's defining .mk in DEP_FILES, so edits to it
                    # (incl. a pinned version bump) already invalidate the
                    # consumer's cache key — not a gap for this consumer.
                    if consumer_dep_covers_mk "${mk_file%.mk}.dep" "$defining_mk"; then
                        ((suppressed_covered++))
                        log_verbose "$base → $dep_base has no .dep, but $base.dep already tracks $dep_rel (suppressed)"
                        continue
                    fi
                    ((missing_dep_count++))
                    add_finding "P2" "$(pkg_label "$mk_file")" \
                        "Depends on \$($dep_pkg_var) (from $dep_rel) which has no .dep file" \
                        "Cache key may not reflect changes in $dep_base"
                    log_verbose "$base depends on $dep_pkg_var → $dep_base has no .dep"
                fi
            fi
        done
    done < <(all_rule_mk_files)

    echo "  Found $missing_dep_count dependency chain gaps (suppressed $suppressed_docker docker-target, $suppressed_covered consumer-covered)"
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

        # Skip pure include-aggregator .dep fragments (not real packages).
        is_package_dep "$dep_file" || { log_verbose "$base: include-aggregator .dep — skipping"; continue; }

        # Look for grep -Ev or grep -v (exclusion patterns)
        local exclusions
        exclusions=$(grep -oP 'grep\s+-[Ev]+\s+"[^"]*"' "$dep_file" 2>/dev/null || true)

        if [[ -n "$exclusions" ]]; then
            # Filter out benign patterns:
            # - `grep -v " "` / `grep -Ev " "` just remove filenames with spaces
            #   (sanitization). Match any -v/-Ev/-vE flag cluster, not just bare -v.
            local is_benign=false
            if echo "$exclusions" | grep -qP 'grep\s+-[Ev]*v[Ev]*\s+" "'; then
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

        # Skip pure include-aggregator .dep fragments (not real packages).
        is_package_dep "$dep_file" || { log_verbose "$base: include-aggregator .dep — skipping"; continue; }

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

# --- Check 11: Build flags consumed only inside a package's source recipe ---
check_submodule_recipe_flags() {
    echo -e "\n${CYAN}=== Check 11: Build flags consumed only inside source recipes ===${NC}"
    local found=0
    while IFS= read -r dep_file; do
        [[ -n "$dep_file" ]] || continue
        is_package_dep "$dep_file" || continue
        grep -qE "_CACHE_MODE[[:space:]]*:?=[[:space:]]*none" "$dep_file" && continue
        local mk_file="${dep_file%.dep}.mk"
        [[ -f "$mk_file" ]] || continue
        local base; base=$(basename "$dep_file" .dep)
        [[ -n "$FILTER_PACKAGE" && "$base" != "$FILTER_PACKAGE" ]] && continue
        local src; src=$(resolve_src_path "$mk_file")
        [[ -n "$src" ]] || continue

        local recipes=() r
        for r in "$src/debian/rules" "$src/Makefile" "$src/setup.py"; do
            [[ -f "$r" ]] && recipes+=("$r")
        done
        [[ ${#recipes[@]} -gt 0 ]] || continue

        # Flags the recipe CONSUMES as an actual make/env VARIABLE — $(FLAG),
        # ${FLAG}, $$FLAG, or a python environ/getenv read. Matching only these
        # consumption forms (never a bare token) avoids false hits on literal
        # strings that merely contain the name, e.g. a sed replacement
        # `s|ENABLE_SAITHRIFT=0|ENABLE_SAITHRIFT=1|` or a hardcoded CMake option
        # `-DENABLE_REDIS=ON`. Scoped to the SONiC feature-flag convention
        # (ENABLE_/INCLUDE_); INSTALL_/CROSS_/MULTIARCH_ are excluded (INSTALL_* is
        # dominated by kernel-Kbuild/CMake internals; CROSS_*/MULTIARCH_* are
        # exported build-env vars already owned by Check 10).
        local ref_flags
        ref_flags=$(
            { grep -hoP '\$[({]\K(ENABLE_|INCLUDE_)[A-Z0-9_]+' "${recipes[@]}" 2>/dev/null
              grep -hoP '\$\$\K(ENABLE_|INCLUDE_)[A-Z0-9_]+'   "${recipes[@]}" 2>/dev/null
              grep -hoP '(?:environ(?:\.get)?[\[(]|getenv[[:space:]]*\()[[:space:]]*["'\'']\K(ENABLE_|INCLUDE_)[A-Z0-9_]+' "${recipes[@]}" 2>/dev/null
            } | sort -u)

        # This package's tracked flags (its own _DEP_FLAGS + the global list).
        local dep_flags
        dep_flags=$(grep "_DEP_FLAGS" "$dep_file" 2>/dev/null | grep -oP '\$\(\w+\)' | tr -d '$()')

        local flag
        while IFS= read -r flag; do
            [[ -z "$flag" ]] && continue
            # Skip recipe-LOCAL variables (assigned with = or := inside the recipe);
            # those are internal, not an external toggle. (?= default is env-overridable
            # and intentionally NOT skipped.)
            grep -qE "^[[:space:]]*${flag}[[:space:]]*:?=" "${recipes[@]}" 2>/dev/null && continue
            echo "$dep_flags" | grep -qx "$flag" && continue
            echo "$COMMON_FLAGS_LIST_CACHE" | grep -qx "$flag" && continue
            # Skip flags already owned by other checks to avoid duplicate findings:
            #   - exported vars are Check 10's domain (whole-tree export scan);
            #   - flags used in an .mk ifeq/ifneq are Check 3/8's domain.
            flag_is_exported "$flag" && continue
            flag_in_mk_conditional "$flag" "$mk_file" && continue
            local where
            where=$(grep -nE "\b${flag}\b" "${recipes[@]}" 2>/dev/null | head -1 \
                | sed "s|$REPO_ROOT/||" | cut -d: -f1-2)
            if flag_forced_off_for_modern_bldenv "$flag"; then
                add_finding "P3" "$(pkg_label "$dep_file")" \
                    "Flag \$$flag consumed in source recipe ($where) but effectively deprecated (pinned off for modern BLDENV)" \
                    "Low priority unless a legacy build env sets it"
            else
                add_finding "P1" "$(pkg_label "$dep_file")" \
                    "Flag \$$flag consumed in source recipe ($where) but not in ${base}_DEP_FLAGS or SONIC_COMMON_FLAGS_LIST" \
                    "Add \$($flag) to ${base}_DEP_FLAGS in $base.dep"
                ((found++))
            fi
        done <<< "$ref_flags"
    done < <(all_dep_files)
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — recipe-consumed flags are all tracked${NC}" \
        || echo -e "  ${YELLOW}$found flag(s) consumed in source recipes but untracked${NC}"
}

# --- Check 12: Dockerfile.j2 template imports not tracked in .dep ---
check_docker_template_imports() {
    echo -e "\n${CYAN}=== Check 12: Dockerfile.j2 template imports not tracked in .dep ===${NC}"
    local found=0
    while IFS= read -r rel; do
        [[ -n "$rel" ]] || continue
        local j2="$REPO_ROOT/$rel"
        local dockerdir; dockerdir=$(dirname "$rel")     # dockers/docker-xxx
        local stem; stem=$(basename "$dockerdir")        # docker-xxx
        local dep_file="$RULES_DIR/${stem}.dep"
        [[ -f "$dep_file" ]] || continue
        [[ -n "$FILTER_PACKAGE" && "$stem" != "$FILTER_PACKAGE" ]] && continue
        grep -qE "_CACHE_MODE[[:space:]]*:?=[[:space:]]*none" "$dep_file" && continue

        local refs
        refs=$(grep -oP '\{%[-[:space:]]*(?:include|import|from)[[:space:]]+"\K[^"]+' "$j2" 2>/dev/null | sort -u)
        [[ -n "$refs" ]] || continue

        local ref
        while IFS= read -r ref; do
            [[ -z "$ref" ]] && continue
            [[ -f "$REPO_ROOT/$ref" ]] || continue          # only in-repo templates
            [[ "$ref" == "$dockerdir/"* ]] && continue       # covered by git ls-files $(DPATH)
            grep -qF "$ref" "$dep_file" && continue          # named literally
            grep -qF "$(basename "$ref")" "$dep_file" && continue
            add_finding "P1" "$stem" \
                "Dockerfile.j2 imports in-repo template '$ref' but $stem.dep does not track it (outside \$(DPATH))" \
                "Add $ref to ${stem}_DEP_FILES (or a shared DEP_FILES list) in $stem.dep"
            ((found++))
        done <<< "$refs"
    done < <(git -C "$REPO_ROOT" ls-files 'dockers/*/Dockerfile.j2' 2>/dev/null)
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — imported templates are tracked${NC}" \
        || echo -e "  ${YELLOW}$found untracked template import(s)${NC}"
}

# --- Check 13: Cached source-built package that hashes no source at all ---
check_untracked_source_tree() {
    echo -e "\n${CYAN}=== Check 13: Cached source-built package hashes no source ===${NC}"
    local found=0
    while IFS= read -r dep_file; do
        [[ -n "$dep_file" ]] || continue
        is_package_dep "$dep_file" || continue
        grep -qE "_CACHE_MODE[[:space:]]*:?=[[:space:]]*none" "$dep_file" && continue
        local mk_file="${dep_file%.dep}.mk"
        [[ -f "$mk_file" ]] || continue
        local base; base=$(basename "$dep_file" .dep)
        [[ -n "$FILTER_PACKAGE" && "$base" != "$FILTER_PACKAGE" ]] && continue

        # Skip if the .dep hashes source in any way.
        grep -qE "git ls-files" "$dep_file" && continue
        grep -qE "SMDEP_FILES" "$dep_file" && continue

        local src; src=$(resolve_src_path "$mk_file")
        [[ -n "$src" ]] || continue
        # Must be a real, git-tracked source tree (submodule gitlink or files).
        [[ -n "$(git -C "$REPO_ROOT" ls-files -- "${src#"$REPO_ROOT"/}" 2>/dev/null | head -1)" ]] || continue

        add_finding "P1" "$(pkg_label "$dep_file")" \
            "Built from source tree ${src#"$REPO_ROOT"/} with caching ON, but .dep hashes NO source (no git ls-files, no _SMDEP_FILES) — the entire source is outside the cache key" \
            "Add DEP_FILES += \$(shell git ls-files \$(${base}_SRC_PATH)), or _SMDEP_FILES for a submodule source tree"
        ((found++))
    done < <(all_dep_files)
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — cached source-built packages hash their source${NC}" \
        || echo -e "  ${YELLOW}$found package(s) whose source is unhashed${NC}"
}

# --- Check 14: Moving-ref / non-deterministic fetches behind a stable key ---
# True (0) if a file carrying a moving-ref fetch belongs to a CACHED package (so a
# stale reuse is actually possible). This mechanically resolves the "non-cached
# platforms can't serve stale" question: a platform .mk that declares a moving URL
# is only a gap when its sibling .dep enables caching (marvell-prestera/clounix SAI
# — cached), NOT when there is no caching sibling (nephos/barefoot/centec/teralynx).
moving_ref_is_cached() {
    local f="$1" rel="${1#"$REPO_ROOT"/}" sib
    case "$rel" in
        *.mk)
            sib="${f%.mk}.dep"
            [[ -f "$sib" ]] && is_package_dep "$sib" \
                && ! grep -qE "_CACHE_MODE[[:space:]]*:?=[[:space:]]*none" "$sib"
            ;;
        dockers/*/Dockerfile.j2)
            sib="$RULES_DIR/$(basename "$(dirname "$rel")").dep"
            [[ -f "$sib" ]] && ! grep -qE "_CACHE_MODE[[:space:]]*:?=[[:space:]]*none" "$sib"
            ;;
        *)  # source-build recipe (src/*/Makefile, debian/rules, meson .wrap) — a
            # source build fetched at build time is cached via its rules/platform .dep.
            return 0 ;;
    esac
}

check_moving_refs() {
    echo -e "\n${CYAN}=== Check 14: Moving-ref / non-deterministic fetches behind a stable key ===${NC}"
    local found=0
    local rel f hits
    while IFS= read -r rel; do
        [[ -n "$rel" ]] || continue
        f="$REPO_ROOT/$rel"
        [[ -f "$f" ]] || continue
        hits=""
        grep -qEi 'raw/(master|main)/' "$f" 2>/dev/null && hits+="raw/master|main URL; "
        grep -qEi '(^|[[:space:]])(FROM|docker[[:space:]]+pull)[^\n]*:latest([[:space:]]|$)' "$f" 2>/dev/null && hits+=":latest image ref; "
        grep -qE 'go[[:space:]]+get([[:space:]]+-[^[:space:]]+)*[[:space:]]+[^@[:space:]]+$' "$f" 2>/dev/null && hits+="unpinned 'go get'; "
        grep -qEi 'revision[[:space:]]*=[[:space:]]*(HEAD|main|master)([[:space:]]|$)' "$f" 2>/dev/null && hits+="wrap revision=HEAD/main/master; "
        [[ -n "$hits" ]] || continue
        # Only a gap if the artifact is actually cached (else it can't serve stale).
        moving_ref_is_cached "$f" || { log_verbose "$rel: moving ref but package not cached — skipping"; continue; }
        add_finding "P2" "$rel" \
            "Non-deterministic fetch behind a stable cache key: ${hits%; }" \
            "Pin to an immutable commit/tag/digest and fold it into the cache key (_DEP_FLAGS), or record & verify a SHA"
        ((found++))
    done < <(git -C "$REPO_ROOT" ls-files \
                'src/*/Makefile' 'src/*/debian/rules' 'platform/*/Makefile' \
                'platform/*/*/Makefile' 'dockers/*/Dockerfile.j2' \
                'rules/*.mk' 'platform/*/*.mk' 'src/**/*.wrap' 2>/dev/null | sort -u)
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — no moving-ref fetches detected${NC}" \
        || echo -e "  ${YELLOW}$found file(s) with non-deterministic fetches${NC}"
}

# --- Check 15: Structural DEP_FILES bugs (aliasing / expansion / reassignment) ---
check_dep_files_structural() {
    echo -e "\n${CYAN}=== Check 15: Structural DEP_FILES bugs (aliasing / expansion / reassignment) ===${NC}"
    local found=0
    while IFS= read -r dep_file; do
        [[ -n "$dep_file" ]] || continue
        is_package_dep "$dep_file" || continue
        local base; base=$(basename "$dep_file" .dep)
        [[ -n "$FILTER_PACKAGE" && "$base" != "$FILTER_PACKAGE" ]] && continue
        local mk_file="${dep_file%.dep}.mk"

        # (b) make-expansion bug: SPATH := $(VAR)_SRC_PATH  (missing inner $()).
        if grep -qE 'SPATH[[:space:]]*:?=[[:space:]]*\$\([A-Z0-9_]+\)_SRC_PATH([[:space:]]|$)' "$dep_file" 2>/dev/null; then
            local bln; bln=$(grep -nE 'SPATH[[:space:]]*:?=[[:space:]]*\$\([A-Z0-9_]+\)_SRC_PATH' "$dep_file" | head -1 | cut -d: -f1)
            add_finding "P1" "$(pkg_label "$dep_file")" \
                "Make-expansion bug at $base.dep:$bln — 'SPATH := \$(VAR)_SRC_PATH' is missing the inner \$(): it expands to a literal '<deb-name>_SRC_PATH' so git ls-files hashes NO source" \
                "Change to SPATH := \$(\$(VAR)_SRC_PATH)"
            ((found++))
        fi

        # (a) aliasing bug: SPATH derived from package X's _SRC_PATH, but another
        #     package Y in the same .dep assigns its DEP_FILES from that shared
        #     SPATH/DEP_FILES while Y has a DIFFERENT source tree (proven below).
        local spath_owner
        spath_owner=$(grep -oP 'SPATH[[:space:]]*:?=[[:space:]]*\$\(\$\(\K[A-Z0-9_]+(?=\)_SRC_PATH)' "$dep_file" | head -1)
        if [[ -n "$spath_owner" && -f "$mk_file" ]]; then
            local owner_dir; owner_dir=$(pkg_var_src_dir "$mk_file" "$spath_owner")
            local pv
            while IFS= read -r pv; do
                [[ -z "$pv" || "$pv" == "$spath_owner" ]] && continue
                local y_dir; y_dir=$(pkg_var_src_dir "$mk_file" "$pv")
                # Only flag when BOTH resolve and Y's tree is neither the owner's
                # tree nor a subdir of it (so SPATH genuinely misses Y's source).
                [[ -n "$owner_dir" && -n "$y_dir" ]] || continue
                [[ "$y_dir" == "$owner_dir" || "$y_dir" == "$owner_dir"/* ]] && continue
                add_finding "P1" "$(pkg_label "$dep_file")" \
                    "DEP_FILES aliasing — \$($pv)_DEP_FILES reuses SPATH from \$($spath_owner)_SRC_PATH ($owner_dir), so $pv's own source ($y_dir) is never hashed" \
                    "Recompute git ls-files from \$($pv)_SRC_PATH for \$($pv)_DEP_FILES"
                ((found++))
            done < <(grep -oP '^\s*\$\(\K[A-Z0-9_]+(?=\)_DEP_FILES[[:space:]]*:?=[[:space:]]*.*\$\((DEP_FILES|SPATH)\))' "$dep_file" | sort -u)
        fi

        # (c) reassignment bug: a `DEP_FILES :=` AFTER the first whose RHS neither
        #     carries forward $(DEP_FILES) NOR re-includes a *COMMON_FILES_LIST —
        #     it silently drops the accumulated common/rule files. Re-initialising
        #     each package section to the common list (the standard idiom, e.g.
        #     p4lang/tacacs/sai-modules) is safe and NOT flagged.
        local idx=0 rln rline
        while IFS= read -r rln; do
            ((idx++))
            [[ $idx -eq 1 ]] && continue          # first := is the legitimate init
            rline=$(sed -n "${rln}p" "$dep_file")
            echo "$rline" | grep -qE '\$\(DEP_FILES\)|COMMON_FILES_LIST' && continue
            add_finding "P2" "$(pkg_label "$dep_file")" \
                "DEP_FILES reassigned with ':=' at $base.dep:$rln (drops accumulated files: RHS carries neither \$(DEP_FILES) nor a *COMMON_FILES_LIST) — the SONIC_COMMON_FILES_LIST/rule files fall out of the key" \
                "Use 'DEP_FILES +=' here so common and rule files are retained"
            ((found++))
        done < <(grep -nE '^[[:space:]]*DEP_FILES[[:space:]]*:=' "$dep_file" | cut -d: -f1)
    done < <(all_dep_files)
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — no structural DEP_FILES bugs${NC}" \
        || echo -e "  ${YELLOW}$found structural DEP_FILES bug(s)${NC}"
}

# --- Check 16: Host root-filesystem files consumed by build_debian.sh but untracked ---
# The RFS target (SONIC_RFS_TARGETS) keys off a hand-maintained RFS_DEP_FILES
# wildcard in Makefile.cache — NOT a rules/*.dep — so Checks 1–15 never see it.
# We parse the real RFS_DEP_FILES coverage (explicit files + `git ls-files DIR`
# globs) and diff it against the in-repo files build_debian.sh (and the scripts it
# invokes, transitively over *.sh) actually consumes. Every consumed, git-tracked
# file outside the coverage is a rootfs cache-key gap.
check_rfs_untracked_files() {
    echo -e "\n${CYAN}=== Check 16: Host rootfs files consumed by build_debian.sh but untracked ===${NC}"
    local found=0
    if [[ ! -f "$REPO_ROOT/build_debian.sh" ]] || ! grep -qE '^RFS_DEP_FILES[[:space:]]*:?=' "$MAKEFILE_CACHE"; then
        echo -e "  ${GREEN}N/A — no build_debian.sh / RFS_DEP_FILES in this tree${NC}"
        return
    fi
    if [[ -n "$FILTER_PACKAGE" ]]; then
        echo -e "  ${GREEN}Skipped (whole-image check) under --package${NC}"
        return
    fi

    # 1) RFS_DEP_FILES coverage: explicit file tokens + globbed directories.
    local block
    block=$(awk '/^RFS_DEP_FILES[[:space:]]*:?=/{f=1}
                 f{print}
                 f && /\)[[:space:]]*$/ && $0 !~ /\\[[:space:]]*$/ {exit}' "$MAKEFILE_CACHE")
    local glob_dirs explicit
    glob_dirs=$(echo "$block" | grep -oP 'git ls-files[[:space:]]+\K[A-Za-z0-9._/-]+' | sort -u)
    explicit=$(echo "$block" | grep -oP '(?<![\w./-])(?:\./)?(?:files|scripts)/[\w./-]+' | sed 's#^\./##' | sort -u)
    # Expand $(addprefix PFX, a b c) — RFS_DEP_FILES lists some scripts this way,
    # e.g. $(addprefix scripts/, build_debian_base_system.sh build_mirror_config.sh).
    local addpre_line pfx items it
    while IFS= read -r addpre_line; do
        [[ -z "$addpre_line" ]] && continue
        pfx="${addpre_line%%,*}"
        items="${addpre_line#*,}"
        for it in $items; do
            [[ -n "$it" ]] && explicit+=$'\n'"${pfx}${it}"
        done
    done < <(echo "$block" | grep -oE '\$\(addprefix[[:space:]]+(files|scripts)/[^,]*,[^)]*' \
                 | sed -E 's/\$\(addprefix[[:space:]]+//')

    # 2) Transitively scan build_debian.sh over the *.sh scripts it invokes,
    #    collecting every files/… or scripts/… path token it reads/copies/renders.
    declare -A visited reported
    local queue=("build_debian.sh") consumed=() cur f tok
    while [[ ${#queue[@]} -gt 0 ]]; do
        cur="${queue[0]}"; queue=("${queue[@]:1}")
        [[ -n "${visited[$cur]:-}" ]] && continue
        visited[$cur]=1
        [[ -f "$REPO_ROOT/$cur" ]] || continue
        f="$REPO_ROOT/$cur"
        while IFS= read -r tok; do
            tok="${tok#./}"
            [[ -z "$tok" ]] && continue
            consumed+=("$tok")
            [[ "$tok" == *.sh && -z "${visited[$tok]:-}" ]] && queue+=("$tok")
        done < <(grep -hoP '(?<![\w./-])(?:\./)?(?:files|scripts)/[\w./-]+' "$f" 2>/dev/null | sort -u)
        # bare functions.sh consumed only when actually sourced ('. functions.sh')
        if grep -qE '^[[:space:]]*(\.|source)[[:space:]]+functions\.sh([[:space:]]|$)' "$f" 2>/dev/null; then
            consumed+=("functions.sh")
        fi
    done
    [[ ${#consumed[@]} -gt 0 ]] || { echo -e "  ${GREEN}None${NC}"; return; }

    # 3) Report each consumed, git-tracked, uncovered file.
    local cf covered d
    while IFS= read -r cf; do
        [[ -z "$cf" || -n "${reported[$cf]:-}" ]] && continue
        [[ -f "$REPO_ROOT/$cf" ]] || continue
        [[ -n "$(git -C "$REPO_ROOT" ls-files -- "$cf" 2>/dev/null)" ]] || continue
        [[ "$cf" == "build_debian.sh" || "$cf" == "onie-image.conf" ]] && continue
        echo "$explicit" | grep -qxF "$cf" && continue
        covered=0
        while IFS= read -r d; do
            [[ -z "$d" ]] && continue
            [[ "$cf" == "$d/"* ]] && { covered=1; break; }
        done <<< "$glob_dirs"
        [[ $covered -eq 1 ]] && continue
        reported[$cf]=1
        add_finding "P1" "sonic-rootfs (RFS)" \
            "build_debian.sh (or a script it invokes) consumes in-repo file '$cf', but it is not in RFS_DEP_FILES / SONIC_COMMON_BASE_FILES_LIST — the host rootfs/squashfs cache key ignores it, so editing '$cf' reuses a stale image" \
            "Add '$cf' to RFS_DEP_FILES in Makefile.cache (or a \$(shell git ls-files <dir>) glob that covers it)"
        ((found++))
    done < <(printf '%s\n' "${consumed[@]}" | sort -u)
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — rootfs recipe inputs are all tracked${NC}" \
        || echo -e "  ${YELLOW}$found rootfs input(s) consumed but untracked${NC}"
}

# --- Check 17: Docker build-context generators not hashed into any docker key ---
# Every docker image build runs scripts/prepare_docker_buildinfo.sh (slave.mk),
# which transitively invokes other scripts/*.{sh,py} and copies build-hooks
# buildinfo into the image build context. None of these live under an image's
# $(DPATH) nor in SONIC_COMMON_FILES_LIST, so they are in NO docker cache key —
# editing the generator logic reuses stale images. Data-driven: seed from the
# docker recipe in slave.mk, take the transitive closure over invoked scripts,
# and flag any that the docker key (SONIC_COMMON_FILES_LIST) does not contain.
check_docker_buildinfo_tracking() {
    echo -e "\n${CYAN}=== Check 17: Docker build-context generators not in any docker key ===${NC}"
    local found=0
    [[ -f "$REPO_ROOT/slave.mk" ]] || { echo -e "  ${GREEN}N/A — no slave.mk${NC}"; return; }
    if [[ -n "$FILTER_PACKAGE" ]]; then
        echo -e "  ${GREEN}Skipped (whole-image check) under --package${NC}"
        return
    fi

    # Docker-key common files (folded into every SONIC_DOCKER_IMAGES key).
    local common_files
    common_files=$(grep -E 'SONIC_COMMON_FILES_LIST[[:space:]]*:?=' "$MAKEFILE_CACHE" | head -1)

    # Seed: the build-context preparer(s) the docker recipe runs in slave.mk.
    local seeds
    seeds=$(grep -hoP '(?<![\w./-])scripts/[\w./-]+\.(?:sh|py)' "$REPO_ROOT/slave.mk" 2>/dev/null \
              | grep -iE 'docker.*buildinfo|buildinfo.*docker' | sort -u)
    [[ -n "$seeds" ]] || { echo -e "  ${GREEN}None — no docker build-context preparer found${NC}"; return; }

    # Transitive closure over invoked scripts + build-context copies from src/.
    declare -A visited
    local queue=() consumed_scripts=() consumed_trees=() s cur f ref
    while IFS= read -r s; do [[ -n "$s" ]] && queue+=("$s"); done <<< "$seeds"
    while [[ ${#queue[@]} -gt 0 ]]; do
        cur="${queue[0]}"; queue=("${queue[@]:1}")
        [[ -n "${visited[$cur]:-}" ]] && continue
        visited[$cur]=1
        [[ -n "$(git -C "$REPO_ROOT" ls-files -- "$cur" 2>/dev/null)" ]] || continue
        consumed_scripts+=("$cur")
        f="$REPO_ROOT/$cur"
        while IFS= read -r ref; do
            [[ -n "$ref" && -z "${visited[$ref]:-}" ]] && queue+=("$ref")
        done < <(grep -hoP '(?<![\w./-])scripts/[\w./-]+\.(?:sh|py)' "$f" 2>/dev/null | sort -u)
        while IFS= read -r ref; do
            [[ -n "$ref" ]] && consumed_trees+=("${ref%/}")
        done < <(grep -hoE 'src/[A-Za-z0-9._/-]+/buildinfo(/[A-Za-z0-9._/-]*)?' "$f" 2>/dev/null | sort -u)
    done

    # Report each consumed script/tree not present in the docker key.
    declare -A reported
    local item base_tree
    for item in "${consumed_scripts[@]}"; do
        [[ -n "${reported[$item]:-}" ]] && continue
        echo "$common_files" | grep -qE "(^| )$(printf '%s' "$item" | sed 's/[.[]/\\&/g')( |$)" && continue
        reported[$item]=1
        add_finding "P2" "docker-images (all)" \
            "Docker build-context generator '$item' is invoked by every docker build (via scripts/prepare_docker_buildinfo.sh in slave.mk) but is not in any docker cache key (absent from SONIC_COMMON_FILES_LIST and every image's \$(DPATH)) — editing it changes the generated build context/version state without invalidating any image" \
            "Fold the docker build-context generators into SONIC_COMMON_FILES_LIST (or a shared docker DEP_FILES list) in Makefile.cache"
        ((found++))
    done
    # De-dup build-context trees to their top-level buildinfo dir.
    for item in "${consumed_trees[@]}"; do
        base_tree=$(echo "$item" | grep -oE 'src/[A-Za-z0-9._/-]+/buildinfo' | head -1)
        [[ -z "$base_tree" || -n "${reported[$base_tree]:-}" ]] && continue
        [[ -n "$(git -C "$REPO_ROOT" ls-files -- "$base_tree" 2>/dev/null | head -1)" ]] || continue
        reported[$base_tree]=1
        add_finding "P2" "docker-images (all)" \
            "Build-context tree '$base_tree/' is copied into every docker build context by scripts/prepare_docker_buildinfo.sh but is not in any docker cache key — editing these build hooks reuses stale images" \
            "Hash '$base_tree' (git ls-files) into a shared docker DEP_FILES list in Makefile.cache"
        ((found++))
    done
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — docker build-context tooling is tracked${NC}" \
        || echo -e "  ${YELLOW}$found docker build-context input(s) untracked${NC}"
}

# --- Check 18: Docker _INSTALL_PYTHON_WHEELS/_INSTALL_DEBS host-only invariant ---
# CORRECTED (was a false positive): a docker's $(DOCKER)_INSTALL_PYTHON_WHEELS /
# _INSTALL_DEBS lists are NOT baked into the produced image, so Makefile.cache's docker
# dependency-SHA rollup (GET_MOD_DEP_SHA -> MOD_DEP_PKGS) is CORRECT to omit them.
# Ground truth (multi-model review + direct tracing, unanimous):
#   * slave.mk's docker recipe lists _INSTALL_* only as "%-install" PREREQUISITES
#     (slave.mk ~L1237-1238). Those %-install rules run `dpkg -i` / `pip install` on the
#     build HOST (slave.mk ~L940-960, ~L1076-1082), never inside the image.
#   * The image content is generated solely from the j2 export vars _debs/_whls/_pydebs,
#     which slave.mk populates strictly from _DEPENDS/_RDEPENDS, _PYTHON_WHEELS and
#     _PYTHON_DEBS (slave.mk ~L1263-1265). No Dockerfile.j2 ever references _INSTALL_*
#     (proof: docker-gnmi-sidecar declares both _INSTALL_* lists yet its Dockerfile
#     installs zero packages).
#   * Their real purpose is host-side provisioning for the build-time cli-plugin-tests
#     pytest run (slave.mk ~L1270). Folding them into the docker key would only
#     OVER-invalidate the image cache (busting e.g. every dhcp-relay/eventd/macsec image
#     on any sonic-utilities change) for zero correctness gain.
# This check therefore VERIFIES the host-only invariant. It stays silent in the normal
# case and re-activates real gap detection ONLY if slave.mk is ever changed so that the
# image-content j2 exports (_debs/_whls/_pydebs) start deriving from an _INSTALL_* list —
# at which point those packages genuinely enter the image and must be folded into the key.
check_docker_install_pkgs_unhashed() {
    echo -e "\n${CYAN}=== Check 18: Docker _INSTALL_PYTHON_WHEELS/_INSTALL_DEBS host-only invariant ===${NC}"
    local found=0
    if [[ -n "$FILTER_PACKAGE" ]]; then
        echo -e "  ${GREEN}Skipped (whole-image check) under --package${NC}"; return
    fi
    [[ -f "$MAKEFILE_CACHE" ]] || { echo -e "  ${GREEN}N/A — no Makefile.cache${NC}"; return; }
    local slave_mk="$REPO_ROOT/slave.mk"
    [[ -f "$slave_mk" ]] || { echo -e "  ${GREEN}N/A — no slave.mk${NC}"; return; }

    # Invariant gate: are _INSTALL_* packages actually baked into the image? They are
    # only if slave.mk's image-content j2 exports (_debs=/_whls=/_pydebs=) reference an
    # _INSTALL_* list. In the current build they do not — the packages are host-only.
    if ! grep -E '_(debs|whls|pydebs)[[:space:]]*=' "$slave_mk" | grep -q 'INSTALL_PYTHON_WHEELS\|INSTALL_DEBS'; then
        echo -e "  ${GREEN}None — _INSTALL_PYTHON_WHEELS/_INSTALL_DEBS install to the build host for cli-plugin-tests (pytest) and are never baked into the image; correctly excluded from the docker key.${NC}"
        return
    fi

    # slave.mk now bakes _INSTALL_* content into the image -> those packages MUST be
    # folded into the docker key. Re-apply gap detection, unless GET_MOD_DEP_SHA already
    # folds the _INSTALL_* lists (auto-silences once fixed there too).
    local fold_block
    fold_block=$(grep -A8 '_MOD_DEP_PKGS[[:space:]]*:=' "$MAKEFILE_CACHE" | head -10)
    if echo "$fold_block" | grep -q 'INSTALL_PYTHON_WHEELS\|INSTALL_DEBS'; then
        echo -e "  ${GREEN}None — _INSTALL_* lists are folded into the docker dep SHA${NC}"; return
    fi

    declare -A folded installed
    local mk line var kind rhs tok key rest
    while IFS= read -r mk; do
        [[ -f "$mk" ]] || continue
        grep -qE '_INSTALL_(PYTHON_WHEELS|DEBS)[[:space:]]*[:+]?=' "$mk" || continue
        folded=(); installed=()
        # Package tokens declared in a FOLDED role, per docker variable.
        while IFS= read -r line; do
            [[ "$line" =~ ^[[:space:]]*\$\(([A-Za-z0-9_]+)\)_(DEPENDS|RDEPENDS|WHEEL_DEPENDS|PYTHON_DEBS|PYTHON_WHEELS|DBG_DEPENDS|DBG_IMAGE_PACKAGES|LOAD_DOCKERS)[[:space:]]*[:+]?=(.*)$ ]] || continue
            var="${BASH_REMATCH[1]}"; rhs="${BASH_REMATCH[3]}"
            for tok in $(echo "$rhs" | grep -oE '\$\([A-Za-z0-9_]+\)'); do
                folded["$var|$tok"]=1
            done
        done < "$mk"
        # Package tokens installed via _INSTALL_* but NOT also in a folded role.
        while IFS= read -r line; do
            [[ "$line" =~ ^[[:space:]]*\$\(([A-Za-z0-9_]+)\)_INSTALL_(PYTHON_WHEELS|DEBS)[[:space:]]*[:+]?=(.*)$ ]] || continue
            var="${BASH_REMATCH[1]}"; kind="${BASH_REMATCH[2]}"; rhs="${BASH_REMATCH[3]}"
            for tok in $(echo "$rhs" | grep -oE '\$\([A-Za-z0-9_]+\)'); do
                [[ -n "${folded["$var|$tok"]:-}" ]] && continue
                installed["$var|$kind|$tok"]=1
            done
        done < "$mk"
        for key in "${!installed[@]}"; do
            var="${key%%|*}"; rest="${key#*|}"; kind="${rest%%|*}"; tok="${rest#*|}"
            add_finding "P1" "$(pkg_label "$mk")" \
                "Docker '$var' now bakes package $tok into the image via _INSTALL_${kind} (slave.mk image-content export references _INSTALL_*), but Makefile.cache GET_MOD_DEP_SHA does not fold _INSTALL_PYTHON_WHEELS/_INSTALL_DEBS into the docker cache key — rebuilding $tok changes the image content without changing the docker key (stale-cache risk)" \
                "Add \$(${var}_INSTALL_PYTHON_WHEELS) and \$(${var}_INSTALL_DEBS) to the MOD_DEP_PKGS foreach in Makefile.cache GET_MOD_DEP_SHA (or add $tok to \$(${var})_DEPENDS)"
            ((found++))
        done
    done < <(all_rule_mk_files)

    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — installed wheels/debs are all folded into docker keys${NC}" \
        || echo -e "  ${YELLOW}$found docker install-package(s) baked into the image but not folded into the docker key${NC}"
}

# --- Check 19: Remote-content fingerprint reused by a target whose URL isn't sampled --
# Some online-deb targets defeat a moving upstream softlink by folding a shell-computed
# HTTP header fingerprint (wget --spider --server-response of the artifact URL) into
# their _DEP_FLAGS. That mitigation is only valid for the targets whose URL is actually
# spidered. If a DIFFERENT target reuses the same fingerprint variable in its _DEP_FLAGS
# while its own $(TARGET)_URL is never sampled, that target's real (moving) remote
# identity is absent from its key — a re-published artifact at the same version string
# reuses a stale cached deb. Data-driven: the sampled URL set and the consuming targets
# are both parsed from the recipe; a target whose own _URL is in the sampled set (or
# which has no _URL at all) is not flagged.
check_misscoped_remote_fingerprint() {
    echo -e "\n${CYAN}=== Check 19: Remote fingerprint reused by a target whose own URL is not sampled ===${NC}"
    local found=0
    local corpus f fvar block spidered_set t url_re
    corpus=$( { all_rule_mk_files; all_dep_files; } | sort -u )

    while IFS= read -r f; do
        [[ -f "$f" ]] || continue
        grep -qE 'wget[[:space:]]+--spider' "$f" || continue
        while IFS= read -r fvar; do
            [[ -z "$fvar" ]] && continue
            # URLs actually spidered on the fingerprint assignment (+ continuation line).
            block=$(grep -A1 -E "^[[:space:]]*$fvar[[:space:]]*:?=.*wget[[:space:]]+--spider" "$f")
            spidered_set=$(echo "$block" | grep -oP '\$\(\$\(\K[A-Za-z0-9_]+(?=\)_URL\))' | sort -u)
            # Targets that fold $(fvar) into their own _DEP_FLAGS (same line).
            while IFS= read -r t; do
                [[ -z "$t" ]] && continue
                echo "$spidered_set" | grep -qxF "$t" && continue
                # Gate: the target must define its own remote _URL (a distinct input).
                url_re='\$\('"$t"'\)_URL[[:space:]]*[:+]?='
                grep -hqE "$url_re" $corpus 2>/dev/null || continue
                add_finding "P1" "$(pkg_label "$f")" \
                    "Target '$t' folds the remote-content fingerprint \$($fvar) into its _DEP_FLAGS, but \$($fvar) is computed by 'wget --spider' of a URL set that does not include \$($t)_URL — the target's own (moving) remote artifact identity is never sampled, so a re-published artifact at the same version reuses a stale cached deb" \
                    "Add \$($t)_URL to the 'wget --spider' URL list feeding $fvar (or compute a dedicated fingerprint from \$($t)_URL for \$($t)_DEP_FLAGS)"
                ((found++))
            done < <(grep -oP '\$\(\K[A-Za-z0-9_]+(?=\)_DEP_FLAGS[[:space:]]*[:+]?=.*\$\('"$fvar"'\))' "$f" | sort -u)
        done < <(grep -oP '^[[:space:]]*\K[A-Za-z0-9_]+(?=[[:space:]]*:?=.*wget[[:space:]]+--spider)' "$f" | sort -u)
    done < <(echo "$corpus")

    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — remote fingerprints sample every consuming target's URL${NC}" \
        || echo -e "  ${YELLOW}$found target(s) reuse a fingerprint that omits their own URL${NC}"
}

check_unhashed_patch_content() {
    echo -e "\n${CYAN}=== Check 20: In-repo patch content applied before build but absent from the cache key ===${NC}"
    local found=0
    local line var val

    # ---- Phase A: quilt patch-series dirs -- $(pkg)_SRC_PATH).patch/ ----
    # Evidence gate: slave.mk must still apply patches via 'QUILT_PATCHES=... quilt push'.
    if grep -qE 'QUILT_PATCHES=.*quilt[[:space:]]+push' "$REPO_ROOT/slave.mk"; then
        # Global auto-silence: a repo-wide fold of a *.patch path into any DEP/SMDEP list.
        local patch_folded=0
        if { all_dep_files; echo "$MAKEFILE_CACHE"; } \
             | xargs grep -hE '(DEP_FILES|SMDEP_FILES)[[:space:]]*[:+]?=.*\.patch' 2>/dev/null | grep -q .; then
            patch_folded=1
        fi
        if [[ $patch_folded -eq 0 ]]; then
            # Resolve every cache-key source root ($(X)_SRC_PATH), $(SRC_PATH)->src, with one
            # level of $($(Y)_SRC_PATH) indirection. A patch series is COVERED iff some resolved
            # root T satisfies 'git -C T ls-files <rel>' -- which, for a submodule root, can never
            # list a parent-repo sibling <root>.patch, but for a normal dir root lists nested files.
            local -A VARSRC=()
            local -A RESOLVED=()
            local -A SRCROOT=()
            local f
            while IFS= read -r f; do
                [[ -f "$f" ]] || continue
                while IFS= read -r line; do
                    [[ "$line" =~ ^[[:space:]]*\$\(([A-Za-z0-9_]+)\)_SRC_PATH[[:space:]]*[:+]?=[[:space:]]*(.*)$ ]] || continue
                    var="${BASH_REMATCH[1]}"; val="${BASH_REMATCH[2]}"
                    val="${val%%#*}"; val="${val%"${val##*[![:space:]]}"}"
                    VARSRC["$var"]="$val"
                done < "$f"
            done < <(all_rule_mk_files)
            local changed=1 pass=0 rv other
            while [[ $changed -eq 1 && $pass -lt 6 ]]; do
                changed=0; ((pass++))
                for var in "${!VARSRC[@]}"; do
                    rv="${VARSRC[$var]}"
                    rv="${rv//\$(SRC_PATH)/src}"
                    if [[ "$rv" =~ \$\(\$\(([A-Za-z0-9_]+)\)_SRC_PATH\) ]]; then
                        other="${BASH_REMATCH[1]}"
                        [[ -n "${RESOLVED[$other]:-}" ]] && rv="${rv//\$(\$($other)_SRC_PATH)/${RESOLVED[$other]}}"
                    fi
                    if [[ "$rv" != *'$('* && -n "$rv" ]]; then
                        [[ "${RESOLVED[$var]:-}" != "$rv" ]] && { RESOLVED["$var"]="$rv"; changed=1; }
                    fi
                done
            done
            for var in "${!RESOLVED[@]}"; do SRCROOT["${RESOLVED[$var]}"]=1; done

            local series D base coveredby T rel
            while IFS= read -r series; do
                [[ -z "$series" ]] && continue
                D="${series%/series}"          # repo-rel patch dir, e.g. src/sonic-swss.patch
                base="${D%.patch}"             # source root it patches, e.g. src/sonic-swss
                # Only a sibling/nested patch of a real built source root is applied by slave.mk.
                [[ -n "${SRCROOT[$base]:-}" ]] || continue
                coveredby=""
                for T in "${!SRCROOT[@]}"; do
                    [[ "$D/" == "$T/"* ]] || continue         # T must be an ancestor dir of D
                    rel="${series#"$T"/}"
                    if git -C "$REPO_ROOT/$T" ls-files --error-unmatch "$rel" >/dev/null 2>&1; then
                        coveredby="$T"; break
                    fi
                done
                [[ -n "$coveredby" ]] && continue
                add_finding "P1" "$(basename "$base")" \
                    "quilt patch series '$D' is applied to \$(...)_SRC_PATH='$base' by slave.mk before the build (QUILT_PATCHES=... quilt push -a), but '$base' is a submodule root so the sibling '$D' lives in the parent repo: _SMDEP_FILES runs 'git ls-files' INSIDE '$base' and cannot see it, and no _DEP_FILES enumerates it -- editing/adding/removing a patch reuses a stale cached artifact" \
                    "Fold the patch dir into the package cache key, e.g. DEP_FILES += \$(shell git ls-files $D) in the package .dep (or \$(wildcard \$(<pkg>_SRC_PATH).patch/*) globally in Makefile.cache)"
                ((found++))
            done < <(git -C "$REPO_ROOT" ls-files '*.patch/series')
        fi
    fi

    # ---- Phase B: platform EXTERNAL_KERNEL_PATCH_LOC dirs ----
    # Evidence gate: the linux-kernel recipe must consume EXTERNAL_KERNEL_PATCH_LOC.
    local lk_mk="$REPO_ROOT/src/sonic-linux-kernel/Makefile"
    if [[ -f "$lk_mk" ]] && grep -qE 'EXTERNAL_KERNEL_PATCH_LOC' "$lk_mk"; then
        local mkf rhs suffix platdir locdir tracked
        while IFS= read -r mkf; do
            [[ -f "$mkf" ]] || continue
            while IFS= read -r line; do
                [[ "$line" =~ EXTERNAL_KERNEL_PATCH_LOC[[:space:]]*:?=[[:space:]]*(.*)$ ]] || continue
                rhs="${BASH_REMATCH[1]}"
                [[ "$rhs" == *'$(PLATFORM_PATH)/'* ]] || continue     # need a repo-relative loc
                suffix="${rhs#*\$(PLATFORM_PATH)/}"
                suffix="${suffix%%#*}"; suffix="${suffix%"${suffix##*[![:space:]]}"}"; suffix="${suffix%/}"
                platdir="$(dirname "${mkf#"$REPO_ROOT"/}")"           # e.g. platform/mellanox
                locdir="$platdir/$suffix"
                tracked=$(git -C "$REPO_ROOT" ls-files "$locdir" 2>/dev/null | grep -vEi '(^|/)README(\.md)?$')
                [[ -z "$tracked" ]] && continue
                # Coverage: does the linux-kernel key fold this path / loc content?
                if { echo "$REPO_ROOT/rules/linux-kernel.dep"; echo "$MAKEFILE_CACHE"; } \
                     | xargs grep -hE "(DEP_FILES|SMDEP_FILES)[[:space:]]*[:+]?=.*($suffix|EXTERNAL_KERNEL_PATCH_LOC|non-upstream)" 2>/dev/null | grep -q .; then
                    continue
                fi
                add_finding "P1" "$platdir/$suffix" \
                    "Platform kernel patch dir '$locdir' (EXTERNAL_KERNEL_PATCH_LOC) is copied into the kernel build and applied when INCLUDE_EXTERNAL_PATCHES=y (src/sonic-linux-kernel/Makefile), but the linux-kernel cache key folds only the INCLUDE_EXTERNAL_PATCHES *flag* in _DEP_FLAGS -- never the tracked patch content ($(echo "$tracked" | tr '\n' ' ')). Editing the patch while the flag stays 'y' reuses a stale cached kernel" \
                    "Add the tracked EXTERNAL_KERNEL_PATCH_LOC files to \$(LINUX_HEADERS_COMMON)_DEP_FILES, e.g. DEP_FILES += \$(shell git ls-files $locdir)"
                ((found++))
            done < "$mkf"
        done < <(grep -rlE 'EXTERNAL_KERNEL_PATCH_LOC[[:space:]]*:?=' $(all_rule_mk_files) 2>/dev/null)
    fi

    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — in-repo patch content applied before build is folded into the cache key${NC}" \
        || echo -e "  ${YELLOW}$found patch source(s) applied before build but absent from the cache key${NC}"
}

# --- Check 21: Inline (non-export) env VALUES passed to a cached build recipe ---
# The rootfs (RFS) recipe in slave.mk invokes ./build_debian.sh with a prefix of
# INLINE, per-command environment assignments (VAR=$(VAR) ./build_debian.sh) rather
# than `export`ed variables. build_debian.sh reads those values and bakes them into
# the rootfs the recipe SAVE_CACHEs, yet the RFS target key folds only
# SONIC_COMMON_FLAGS_LIST. Check 10 scans EXPORTED variables only, so these inline
# values are a structural blind spot. Data-driven: parse the inline env prefix of
# every SAVE_CACHE-backed ./build_debian.sh recipe, keep only vars the consumed
# script actually references, and default-deny those that are not in the key, not
# exported (Check 10), and not human-waived.
check_inline_recipe_env_vars() {
    echo -e "\n${CYAN}=== Check 21: Inline (non-export) recipe env values vs cache key ===${NC}"
    local sm="$REPO_ROOT/slave.mk"
    local consumer="$REPO_ROOT/build_debian.sh"
    local found=0 waived=0

    # Evidence gate: the cache-backed rootfs recipe and its consumed script must exist.
    if [[ ! -f "$sm" || ! -f "$consumer" ]]; then
        echo -e "  ${GREEN}None — no cache-backed build_debian.sh recipe present${NC}"
        return
    fi

    local -a SM_LINES=()
    mapfile -t SM_LINES < "$sm"
    local nlines=${#SM_LINES[@]}

    # Collect inline env var names from every SAVE_CACHE-backed ./build_debian.sh
    # recipe: for each invocation confirm a SAVE_CACHE within the next few lines
    # (only cached targets matter), then walk backward over the contiguous
    # 'VAR=... \' continuation block that forms the inline env prefix.
    local -A CAND=()
    local ln
    while IFS= read -r ln; do
        [[ -n "$ln" ]] || continue
        local cached=0 j peek
        for ((j=ln; j<ln+6 && j<nlines; j++)); do
            peek="${SM_LINES[$j]}"
            [[ "$peek" == *SAVE_CACHE* ]] && { cached=1; break; }
        done
        [[ $cached -eq 1 ]] || continue
        local k=$((ln-2)) body           # ln is 1-based build_debian.sh line; SM_LINES 0-based
        while ((k>=0)); do
            body="${SM_LINES[$k]}"
            if [[ "$body" =~ ^[[:space:]]*([A-Za-z_][A-Za-z0-9_]*)=.*\\[[:space:]]*$ ]]; then
                CAND["${BASH_REMATCH[1]}"]=1
                ((k--))
            else
                break
            fi
        done
    done < <(grep -nE '\./build_debian\.sh' "$sm" | cut -d: -f1)

    if [[ ${#CAND[@]} -eq 0 ]]; then
        echo -e "  ${GREEN}None — no cache-backed build_debian.sh recipe present${NC}"
        return
    fi

    local v reason
    for v in $(printf '%s\n' "${!CAND[@]}" | sort); do
        # Covered: folded into the RFS target's key (its DEP_FLAGS := SONIC_COMMON_FLAGS_LIST).
        if echo "$COMMON_FLAGS_LIST_CACHE" | grep -qx "$v"; then
            log_verbose "  $v — in SONIC_COMMON_FLAGS_LIST (OK)"; continue
        fi
        # Exported elsewhere -> Check 10 already models it; don't double-report.
        if flag_is_exported "$v"; then
            log_verbose "  $v — exported (Check 10 covers it)"; continue
        fi
        # Consumed? Data-driven: the value must actually be read by build_debian.sh.
        # A var that is passed but never referenced is not a content input -> skip.
        if ! grep -qE "(\\\$\{?${v}\b|[^A-Za-z0-9_]${v}=)" "$consumer" 2>/dev/null; then
            log_verbose "  $v — passed but not read by build_debian.sh (not a content input)"; continue
        fi
        # Human-justified waiver — clears identity/plumbing values (target-stem name,
        # output dir, version-cache path) that provably cannot alter rootfs content.
        reason=$(waiver_reason "$v")
        if [[ -n "$reason" ]]; then
            ((waived++)); log_verbose "  $v — waived: $reason"
            add_finding "P3" "$v" \
                "Inline recipe env value waived from cache-key tracking (human-justified): $reason" \
                "Re-verify the waiver reason still holds if this variable's build usage changes"
            continue
        fi
        add_finding "P1" "$v" \
            "Passed INLINE (non-export, '$v=\$($v)') to the cache-backed ./build_debian.sh rootfs recipe (SONIC_RFS_TARGETS) and read by build_debian.sh, but the RFS cache key folds only SONIC_COMMON_FLAGS_LIST — so the value is in neither the key nor any _DEP_FLAGS. Because it is not exported, Check 10 cannot see it: changing the value bakes new content into the rootfs while the same stale cached squashfs is restored" \
            "Add \$($v) to the RFS target DEP_FLAGS (SONIC_COMMON_FLAGS_LIST in Makefile.cache) so the value is folded into the rootfs cache key; if it provably cannot affect rootfs content, record it (with a reason) in scripts/cache_key_export_waivers.tsv"
        ((found++))
        $VERBOSE && echo -e "  ${RED}P1${NC} \$$v (inline, consumed by build_debian.sh, unkeyed)"
    done

    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — every inline recipe env value is keyed, exported (Check 10), or waived${NC}" \
        || echo -e "  ${YELLOW}$found inline recipe env value(s) consumed by build_debian.sh but absent from the cache key${NC}"
    [[ $waived -gt 0 ]] && log_verbose "$waived inline env value(s) cleared by recorded waivers"
}

# --- Shared resolvers for the docker cache-key checks (23-24) ---------------
# Repo-relative Dockerfile-context directory ($(DPATH)) for a docker .mk/.dep.
# The docker recipe dir is named after the file stem: rules/docker-X.mk builds
# dockers/docker-X, while a platform docker (platform/<vendor>/docker-X.mk) builds
# platform/<vendor>/docker-X. Derived structurally, no per-target table.
docker_dir_for_stem() {
    local rel="$1" stem="$2"
    case "$rel" in
        rules/*) echo "dockers/$stem" ;;
        *)       echo "$(dirname "$rel")/$stem" ;;
    esac
}

# Set of build-flag identifiers that at least one docker .dep folds into its own
# _DEP_FLAGS beyond SONIC_COMMON_FLAGS_LIST (e.g. ENABLE_ASAN). A flag in this set
# is a proven, keyed docker build input, so a sibling docker that consumes it in
# its Dockerfile but omits it from its key is a real per-target collision (Check 23).
build_docker_keyed_flags() {
    [[ "${DKF_BUILT:-0}" == "1" ]] && return
    declare -gA DOCKER_KEYED_FLAGS
    DKF_BUILT=1
    local dep line tok var
    while IFS= read -r dep; do
        [[ -f "$dep" ]] || continue
        [[ "$(basename "$dep")" == docker-*.dep ]] || continue
        while IFS= read -r line; do
            [[ "$line" =~ _DEP_FLAGS[[:space:]]*[:+]?=(.*)$ ]] || continue
            for tok in $(echo "${BASH_REMATCH[1]}" | grep -oE '\$\([A-Za-z0-9_]+\)'); do
                var="${tok#\$(}"; var="${var%)}"
                [[ "$var" == "SONIC_COMMON_FLAGS_LIST" ]] && continue
                echo "$COMMON_FLAGS_LIST_CACHE" | grep -qx "$var" && continue
                DOCKER_KEYED_FLAGS["$var"]=1
            done
        done < "$dep"
    done < <(all_dep_files)
}

# NOTE: A former "Check 22" flagged docker $(IMG)_FILES sources as absent from the
# cache key. It was WITHDRAWN as a false positive: Makefile.cache SHA_DEP_RULES
# (lines ~638-645) already appends every $(IMG)_FILES source — resolved via its
# $(VAR)_PATH, else $(FILES_PATH) — into $(IMG)_DEP_FILES, which is git-hashed into
# the docker dep SHA (upstream fix #15473). Verified empirically: the generated
# target/docker-*.gz.dep lists those sources (e.g. files/build_templates/swss_vars.j2,
# src/sonic-ctrmgrd/*). Editing such a file therefore DOES move the key. Numbering
# below is preserved for continuity with recorded findings.

# --- Check 23: Docker build-arg flag consumed by Dockerfile but not in its key --
# A docker image's rendered Dockerfile.j2 can branch on a build flag ({% if FLAG %}
# / {{FLAG}}). When sibling docker images DO fold that flag into their _DEP_FLAGS
# (proving it is a real keyed build input — e.g. ENABLE_ASAN in docker-orchagent /
# docker-syncd-mlnx) but THIS image omits it, the image's own _MOD_HASH does not
# separate the two flag renderings. NOTE (P3, hygiene): a full collision usually
# does NOT occur, because the docker cache filename also folds the flags/SHAs of
# every _DEPENDS package via GET_MOD_DEP_SHA — if any dependency keys the flag
# (e.g. docker-sonic-vs depends on SYSMGR, whose rules/sysmgr.dep carries
# $(ENABLE_ASAN)), the transitive hash already separates the keys. This check is
# therefore defense-in-depth: fix the direct omission so correctness does not rely
# on an incidental dependency. Data-driven: keyed-flag set parsed from real _DEP_FLAGS.
check_docker_flag_not_keyed() {
    echo -e "\n${CYAN}=== Check 23: Docker Dockerfile build flag not in that image's _DEP_FLAGS (hygiene) ===${NC}"
    local found=0
    build_docker_keyed_flags
    local dep dep_rel stem ddir j2 F
    while IFS= read -r dep; do
        [[ -f "$dep" ]] || continue
        [[ "$(basename "$dep")" == docker-*.dep ]] || continue
        grep -qE "_CACHE_MODE[[:space:]]*:?=[[:space:]]*none" "$dep" && continue
        stem=$(basename "$dep" .dep)
        [[ -n "$FILTER_PACKAGE" && "$stem" != "$FILTER_PACKAGE" ]] && continue
        dep_rel="${dep#"$REPO_ROOT"/}"
        ddir=$(docker_dir_for_stem "$dep_rel" "$stem")
        j2="$REPO_ROOT/$ddir/Dockerfile.j2"
        [[ -f "$j2" ]] || continue
        for F in "${!DOCKER_KEYED_FLAGS[@]}"; do
            grep -qE "\{[%{][^}]*\b${F}\b" "$j2" || continue          # consumed in a Jinja directive
            grep -qE "_DEP_FLAGS[[:space:]]*[:+]?=.*\\\$\\(${F}\\)" "$dep" && continue  # keyed here
            add_finding "P3" "$(pkg_label "$dep")" \
                "Dockerfile.j2 branches on build flag \$($F) (which sibling docker images fold into their cache key) but ${stem}.dep _DEP_FLAGS omits it. A direct collision is usually blocked because a _DEPENDS package that keys \$($F) already separates the key via GET_MOD_DEP_SHA — but this is incidental; add the flag directly as defense-in-depth" \
                "Append \$($F) to \$(IMG)_DEP_FLAGS in ${stem}.dep (as the sibling docker .dep files already do)"
            ((found++))
        done
    done < <(all_dep_files)
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — Dockerfile build flags are all keyed per image${NC}" \
        || echo -e "  ${YELLOW}$found docker image(s) omit a consumed build flag from their key${NC}"
}

# --- Check 24: include'd recipe .mk fragment not tracked in the target's key ---
# A rule/platform .mk can 'include' a shared recipe fragment (e.g. platform docker
# targets include platform/template/docker-syncd-*.mk, which supplies _LOAD_DOCKERS/
# _CONTAINER_NAME/_RUN_OPT). The fragment lives outside $(DPATH) and is not in the
# target's DEP_FILES, so editing it is not directly hashed. NOTE (P3, narrow): most
# of the fragment's content is already covered or harmless — _LOAD_DOCKERS (base
# image) is keyed via MOD_DEP_PKGS/GET_MOD_DEP_SHA, and _RUN_OPT is runtime-only
# (does not affect the built .gz). The only build-affecting, un-keyed variable is
# _CONTAINER_NAME (passed as --build-arg docker_container_name), and only images
# whose Dockerfile actually consumes that ARG bake it in. Blast radius is therefore
# small. Data-driven: resolve each include (expanding $(PLATFORM_PATH)/$(DOCKERS_PATH)),
# require a git-tracked .mk, and flag those the paired .dep does not list.
check_included_mk_unhashed() {
    echo -e "\n${CYAN}=== Check 24: include'd recipe .mk fragment not in the target's cache key (narrow) ===${NC}"
    local found=0
    local mk mk_rel dep stem plat inc resolved rel bn
    while IFS= read -r mk; do
        [[ -f "$mk" ]] || continue
        grep -qE '^[[:space:]]*include[[:space:]]+' "$mk" || continue
        dep="${mk%.mk}.dep"
        [[ -f "$dep" ]] || continue
        is_package_dep "$dep" || continue
        grep -qE "_CACHE_MODE[[:space:]]*:?=[[:space:]]*none" "$dep" && continue
        mk_rel="${mk#"$REPO_ROOT"/}"
        stem=$(basename "$mk_rel" .mk)
        [[ -n "$FILTER_PACKAGE" && "$stem" != "$FILTER_PACKAGE" ]] && continue
        plat=""
        case "$mk_rel" in
            platform/*) plat="platform/$(echo "$mk_rel" | cut -d/ -f2)" ;;
        esac
        while IFS= read -r inc; do
            [[ -n "$inc" ]] || continue
            resolved="${inc//\$(PLATFORM_PATH)/$plat}"
            resolved="${resolved//\$(DOCKERS_PATH)/dockers}"
            [[ "$resolved" == *'$('* ]] && continue        # unresolved make var
            rel=$(realpath -m --relative-to="$REPO_ROOT" "$REPO_ROOT/$resolved" 2>/dev/null)
            [[ -n "$rel" && "$rel" == *.mk ]] || continue
            [[ -n "$(git -C "$REPO_ROOT" ls-files -- "$rel" 2>/dev/null | head -1)" ]] || continue
            grep -qF "$rel" "$dep" && continue             # tracked by full path
            bn=$(basename "$rel")
            grep -qF "$bn" "$dep" && continue              # tracked by basename
            add_finding "P3" "$(pkg_label "$dep")" \
                "${stem}.mk 'include's recipe fragment $rel but ${stem}.dep does not track it (it lives outside \$(DPATH)). Most of the fragment is already covered — _LOAD_DOCKERS is keyed via MOD_DEP_PKGS and _RUN_OPT is runtime-only; the only un-keyed build-affecting var is _CONTAINER_NAME (--build-arg docker_container_name), baked in only where the Dockerfile consumes that ARG" \
                "Add $rel to ${stem}_DEP_FILES in ${stem}.dep (or hash every include'd .mk into the target key)"
            ((found++))
        done < <(grep -oP '^[[:space:]]*include[[:space:]]+\K\S+' "$mk")
    done < <(all_rule_mk_files)
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — include'd recipe fragments are tracked${NC}" \
        || echo -e "  ${YELLOW}$found include'd recipe fragment(s) not in the target key${NC}"
}

# --- Check 25: slave.mk recipe-body version stamp integrity + drift ---
check_recipe_body_drift() {
    echo -e "\n${CYAN}=== Check 25: slave.mk recipe-body drift vs SONIC_CACHE_RECIPE_VER stamp ===${NC}"
    local found=0

    # (a) The recipe body of slave.mk is folded into every package key ONLY through the
    #     manual SONIC_CACHE_RECIPE_VER stamp (slave.mk itself is intentionally not in
    #     any DEP_FILES). If the stamp is not in SONIC_COMMON_FLAGS_LIST the whole guard
    #     is silently defeated and no key tracks the recipe body.
    if [[ -n "$COMMON_FLAGS_LIST_CACHE" ]] && ! echo "$COMMON_FLAGS_LIST_CACHE" | grep -qx 'SONIC_CACHE_RECIPE_VER'; then
        add_finding "P1" "Makefile.cache" \
            "SONIC_CACHE_RECIPE_VER is not part of SONIC_COMMON_FLAGS_LIST, so the manual slave.mk recipe-body version stamp is folded into NO package cache key — every recipe-body change in slave.mk is invisible to the cache" \
            "Add \$(SONIC_CACHE_RECIPE_VER) to SONIC_COMMON_FLAGS_LIST in Makefile.cache"
        ((found++))
    fi

    # (b) slave.mk drifted from the reviewed baseline; if the stamp was not bumped,
    #     packages built with the old recipe are served stale.
    local baseline cur
    baseline=$(grep -oP '^\s*SONIC_CACHE_RECIPE_VER_BASELINE\s*:?=\s*\K[0-9a-f]+' "$MAKEFILE_CACHE" 2>/dev/null | head -1)
    cur=$(git -C "$REPO_ROOT" hash-object slave.mk 2>/dev/null)
    if [[ -n "$baseline" && -n "$cur" && "$baseline" != "$cur" ]]; then
        add_finding "P1" "slave.mk" \
            "slave.mk (git-object $cur) has drifted from SONIC_CACHE_RECIPE_VER_BASELINE ($baseline) — if the change alters any package build recipe and SONIC_CACHE_RECIPE_VER was not bumped, cached packages built with the old recipe are served stale" \
            "Review the slave.mk change: if it affects package output, bump SONIC_CACHE_RECIPE_VER and set SONIC_CACHE_RECIPE_VER_BASELINE=$cur; otherwise just update the baseline to $cur"
        ((found++))
    fi

    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — slave.mk recipe body is stamped and in sync with the baseline${NC}" \
        || echo -e "  ${YELLOW}$found recipe-body stamp/drift issue(s)${NC}"
}

# --- Check 26: slave/toolchain image identity not folded into any package key ---
check_slave_toolchain_identity() {
    echo -e "\n${CYAN}=== Check 26: slave/toolchain image identity not in any package key ===${NC}"
    local found=0

    # (a) Package keys capture the slave build environment only through the
    #     sonic-slave-<distro> Dockerfile SOURCE (SONIC_COMMON_BASE_FILES_LIST). The
    #     identity of the BUILT slave image (SLAVE_TAG / resolved base-image digest /
    #     installed apt versions) is in NO package key.
    if [[ -n "$COMMON_FLAGS_LIST_CACHE" ]] && ! echo "$COMMON_FLAGS_LIST_CACHE" | grep -qiE 'SLAVE_TAG|SLAVE_HASH|SLAVE_BASE'; then
        add_finding "P2" "Makefile.cache" \
            "No slave build-image identity (SLAVE_TAG / built-image digest) is in SONIC_COMMON_FLAGS_LIST — package keys track only the sonic-slave Dockerfile SOURCE, not the resolved toolchain. A moving base image or an upstream apt update changes the toolchain without moving any package cache key" \
            "Fold a slave-image identity token (e.g. SLAVE_TAG, which already hashes the slave Dockerfile + build args) into SONIC_COMMON_FLAGS_LIST"
        ((found++))
    fi

    # (b) Evidence: sonic-slave-<distro> distro base images pinned by a moving tag.
    local d name fromline img seen
    for d in "$REPO_ROOT"/sonic-slave-*; do
        [[ -d "$d" && -f "$d/Dockerfile.j2" ]] || continue
        name=$(basename "$d")
        seen=""
        while IFS= read -r fromline; do
            # Strip: FROM keyword, --platform flags, jinja {{ registry prefix }},
            # and a trailing "as <stage>" alias — leaving the bare image ref.
            img=$(echo "$fromline" \
                | sed -E 's/^[[:space:]]*FROM[[:space:]]+//I' \
                | sed -E 's/--platform=[^[:space:]]+[[:space:]]*//g' \
                | sed -E 's/\{\{[^}]*\}\}//g' \
                | sed -E 's/[[:space:]]+as[[:space:]]+.*$//I' \
                | awk '{print $1}')
            [[ -n "$img" ]] || continue
            [[ "$img" == *'{{'* || "$img" == *'$'* ]] && continue    # unresolved template
            [[ "$img" == *@sha256:* ]] && continue                    # digest-pinned, ok
            case "$img" in
                *debian*:*|*ubuntu*:*) ;;                              # a distro base image
                *) continue ;;
            esac
            [[ " $seen " == *" $img "* ]] && continue                 # dedupe per slave dir
            seen="$seen $img"
            add_finding "P3" "$name" \
                "$name/Dockerfile.j2 base image '$img' is pinned by a MOVING tag (no @sha256 digest); the slave toolchain can change under an unchanged Dockerfile source, and no package key tracks the resolved image" \
                "Pin the base image by @sha256 digest, or fold SLAVE_TAG into the package key so a rebuilt slave invalidates caches"
            ((found++))
        done < <(grep -iE '^[[:space:]]*FROM[[:space:]]' "$d/Dockerfile.j2")
    done

    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — slave toolchain identity is captured in the key${NC}" \
        || echo -e "  ${YELLOW}$found toolchain-identity gap(s)${NC}"
}

check_build_mode_selectors() {
    echo -e "\n${CYAN}=== Check 27: output-affecting global build-mode selectors not in the key ===${NC}"
    local found=0
    # Global toggles that change the CONTENT of a produced artifact but are neither
    # encoded in the output filename nor selected by CONFIGURED_PLATFORM/CONFIGURED_
    # ARCH/BLDENV (all three ARE in SONIC_COMMON_FLAGS_LIST). Platform-specific
    # _BUILD_ENV (e.g. libsaithrift-dev's platform=vs vs platform=vpp) is therefore
    # already keyed via CONFIGURED_PLATFORM and is intentionally NOT flagged here.
    # The residual is a selector that varies WITHIN one platform/arch.
    local sel used_in_recipe
    for sel in CROSS_BUILD_ENVIRON; do
        # Only matters if a build recipe actually consumes it.
        used_in_recipe=$(grep -cE "\b$sel\b" "$REPO_ROOT/slave.mk" 2>/dev/null)
        [[ "${used_in_recipe:-0}" -gt 0 ]] || continue
        if [[ -z "$COMMON_FLAGS_LIST_CACHE" ]] || ! echo "$COMMON_FLAGS_LIST_CACHE" | grep -qw "$sel"; then
            add_finding "P2" "Makefile.cache" \
                "Global build-mode selector '$sel' alters produced artifacts (consumed by the slave.mk build recipe) but is absent from SONIC_COMMON_FLAGS_LIST and not encoded in output filenames — a build toggled only by '$sel' (native vs cross for the SAME arch) reuses a stale cache key" \
                "Fold \$($sel) into SONIC_COMMON_FLAGS_LIST so the selector participates in every package key"
            ((found++))
        fi
    done
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — build-mode selectors are keyed${NC}" \
        || echo -e "  ${YELLOW}$found build-mode-selector gap(s)${NC}"
}

check_recipe_invoked_scripts() {
    echo -e "\n${CYAN}=== Check 28: recipe-invoked build scripts not hashed into any key ===${NC}"
    local found=0
    # slave.mk recipes invoke helper scripts that shape the CONTENT of produced
    # artifacts (buildinfo / version-pin / debug-file injection). Only .platform,
    # rules/functions and Makefile.cache are in SONIC_COMMON_FILES_LIST, so edits to
    # these scripts change every affected output without moving any cache key.
    local script scripts_used
    # Pure control-flow helpers with no effect on artifact content.
    local denylist=" scripts/wait_for_docker.sh "
    scripts_used=$(grep -hoE '(scripts|src/sonic-build-hooks)/[A-Za-z0-9_./-]+\.(sh|py)' \
        "$REPO_ROOT/slave.mk" 2>/dev/null | sort -u)
    while IFS= read -r script; do
        [[ -n "$script" ]] || continue
        [[ "$denylist" == *" $script "* ]] && continue
        [[ -f "$REPO_ROOT/$script" ]] || continue
        # "Hashed" == referenced by a file list in Makefile.cache or any .dep. The
        # slave.mk invocation line itself is recipe usage, NOT tracking, so it is not
        # consulted here.
        if grep -qF "$script" "$MAKEFILE_CACHE" 2>/dev/null || \
           grep -rqF --include='*.dep' "$script" "$REPO_ROOT/rules" "$REPO_ROOT/platform" 2>/dev/null; then
            continue
        fi
        add_finding "P2" "$script" \
            "'$script' is invoked by a slave.mk build recipe and shapes produced artifacts (buildinfo/version/debug injection) but is not in SONIC_COMMON_FILES_LIST nor any _DEP_FILES — editing it changes outputs without invalidating any cache key" \
            "Add '$script' (and the helper library it sources) to SONIC_COMMON_FILES_LIST so recipe-invoked build scripts are hashed into every key"
        ((found++))
    done <<< "$scripts_used"
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — recipe-invoked build scripts are hashed${NC}" \
        || echo -e "  ${YELLOW}$found untracked build-script(s)${NC}"
}

check_embedded_sibling_deb() {
    echo -e "\n${CYAN}=== Check 29: package embeds a sibling deb with no _DEPENDS edge ===${NC}"
    local found=0
    # A debian/rules that dpkg-extracts a sibling built artifact
    # (target/debs/<env>/<name>_*.deb) and copies content out of it bakes that binary
    # into the package. Without a _DEPENDS edge on <name>, rebuilding <name> leaves
    # this package's cache key unchanged and it is served stale.
    local rulesfile srcdir base names name mk_all deplines candidates
    candidates=$(grep -rlE "target/debs/[^ ]*_[^ ]*\.deb" \
        "$REPO_ROOT"/src/*/debian/rules \
        "$REPO_ROOT"/platform/*/*/debian/rules 2>/dev/null)
    while IFS= read -r rulesfile; do
        [[ -n "$rulesfile" ]] || continue
        srcdir=$(dirname "$(dirname "$rulesfile")")      # strip /debian/rules
        base=$(basename "$srcdir")
        names=$(grep -oE "target/debs/[^ ]*/[a-z0-9.+_-]+_[^ ]*\.deb" "$rulesfile" \
            | sed -E 's#.*/##; s/_.*//' | sort -u)
        [[ -n "$names" ]] || continue
        # Owning rules .mk(s): more than one platform .mk may reuse the same source
        # dir, so aggregate the dependency edges declared across ALL of them.
        mk_all=$(grep -rlE "_SRC_PATH[[:space:]]*[:+]?=.*$base" \
            "$REPO_ROOT"/rules/*.mk "$REPO_ROOT"/platform/*/*.mk 2>/dev/null)
        deplines=""
        for mk in $mk_all; do
            deplines+=$'\n'$(grep -hE "_R?DEPENDS" "$mk" 2>/dev/null)
        done
        for name in $names; do
            # Dep edge present if any DEPENDS/RDEPENDS token names the sibling.
            echo "$deplines" | grep -iq "$name" && continue
            add_finding "P2" "$base" \
                "$base's debian/rules extracts sibling artifact '$name' (dpkg -x of target/debs/.../${name}_*.deb) and copies content into the package, but declares no _DEPENDS edge on '$name' — rebuilding '$name' does not invalidate $base's cache key, serving a stale embed" \
                "Add the '$name' package variable to $base's _DEPENDS so the embedded sibling participates in the cache key"
            ((found++))
        done
    done <<< "$candidates"
    [[ $found -eq 0 ]] && echo -e "  ${GREEN}None — embedded sibling debs have dep edges${NC}" \
        || echo -e "  ${YELLOW}$found embed-without-edge gap(s)${NC}"
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
    check_submodule_recipe_flags
    check_docker_template_imports
    check_untracked_source_tree
    check_moving_refs
    check_dep_files_structural
    check_rfs_untracked_files
    check_docker_buildinfo_tracking
    check_docker_install_pkgs_unhashed
    check_misscoped_remote_fingerprint
    check_unhashed_patch_content
    check_inline_recipe_env_vars
    check_docker_flag_not_keyed
    check_included_mk_unhashed
    check_recipe_body_drift
    check_slave_toolchain_identity
    check_build_mode_selectors
    check_recipe_invoked_scripts
    check_embedded_sibling_deb

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
