#!/bin/bash
# build-timing-report.sh — Parse SONiC build logs and generate per-package timing report
#
# Usage: ./scripts/build-timing-report.sh [target_dir]
#   target_dir: path to sonic-buildimage/target (default: ./target)
#
# Parses HEADER/FOOTER timestamps from individual .log files and produces:
#   1. Sorted table of packages by duration (longest first)
#   2. Phase breakdown (bookworm vs trixie)
#   3. Build parallelism analysis
#   4. Parallelism efficiency stats
#   5. Gantt chart data (CSV)

set -euo pipefail

# Ensure scripts are executable
for script in "$(dirname "$0")"/build-dep-graph.py "$(dirname "$0")"/build-resource-monitor.sh; do
    [ -f "$script" ] && [ ! -x "$script" ] && chmod +x "$script"
done

TARGET_DIR="${1:-./target}"

if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: target directory '$TARGET_DIR' not found" >&2
    exit 1
fi

# Collect timing data from all .log files
collect_timings() {
    local dir="$1"
    local phase="$2"

    find "$dir" -maxdepth 1 -name '*.log' ! -name '*-install.log' 2>/dev/null | while read -r logfile; do
        local pkg
        pkg=$(basename "$logfile" .log)

        # Extract timestamps
        local start_line end_line
        start_line=$(grep -m1 '^Build start time:' "$logfile" 2>/dev/null || true)
        end_line=$(grep '^Build end time:' "$logfile" 2>/dev/null | tail -1 || true)
        if [ -z "$start_line" ] || [ -z "$end_line" ]; then
            continue
        fi

        # Parse timestamps
        local start_ts end_ts
        start_ts=$(date -d "${start_line#Build start time: }" +%s 2>/dev/null || true)
        end_ts=$(date -d "${end_line#Build end time: }" +%s 2>/dev/null || true)

        if [ -z "$start_ts" ] || [ -z "$end_ts" ]; then
            continue
        fi

        local duration=$(( end_ts - start_ts ))
        local start_fmt end_fmt
        start_fmt=$(date -d "@$start_ts" +%H:%M:%S 2>/dev/null)
        end_fmt=$(date -d "@$end_ts" +%H:%M:%S 2>/dev/null)

        # Check if it was cached
        local cached="no"
        if grep -q 'CACHE_LOADED\|Skipping' "$logfile" 2>/dev/null; then
            cached="yes"
        fi

        echo "${duration}|${phase}|${pkg}|${start_ts}|${end_ts}|${start_fmt}|${end_fmt}|${cached}"
    done
}

echo "=============================================="
echo "  SONiC Build Timing Report"
echo "  Generated: $(date)"
echo "  Target dir: $TARGET_DIR"
echo "=============================================="
echo ""

# Collect from all phases
ALL_TIMINGS=$(mktemp)
trap 'rm -f "$ALL_TIMINGS"' EXIT

for distro_dir in "$TARGET_DIR"/debs/*/; do
    [ -d "$distro_dir" ] || continue
    phase=$(basename "$distro_dir")
    collect_timings "$distro_dir" "$phase"
done >> "$ALL_TIMINGS"

for distro_dir in "$TARGET_DIR"/python-wheels/*/; do
    [ -d "$distro_dir" ] || continue
    phase=$(basename "$distro_dir")
    collect_timings "$distro_dir" "wheel-$phase"
done >> "$ALL_TIMINGS"

# Also check docker logs if they exist
for docker_dir in "$TARGET_DIR"/docker-*.gz.log; do
    [ -f "$docker_dir" ] || continue
    pkg=$(basename "$docker_dir" .log)
    start_line=$(grep -m1 '^Build start time:' "$docker_dir" 2>/dev/null || true)
    end_line=$(grep '^Build end time:' "$docker_dir" 2>/dev/null | tail -1 || true)
    if [ -n "$start_line" ] && [ -n "$end_line" ]; then
        start_ts=$(date -d "${start_line#Build start time: }" +%s 2>/dev/null || true)
        end_ts=$(date -d "${end_line#Build end time: }" +%s 2>/dev/null || true)
        if [ -n "$start_ts" ] && [ -n "$end_ts" ]; then
            duration=$(( end_ts - start_ts ))
            echo "${duration}|docker|${pkg}|${start_ts}|${end_ts}|$(date -d "@$start_ts" +%H:%M:%S)|$(date -d "@$end_ts" +%H:%M:%S)|no"
        fi
    fi
done >> "$ALL_TIMINGS"

TOTAL_PACKAGES=$(wc -l < "$ALL_TIMINGS")

if [ "$TOTAL_PACKAGES" -eq 0 ]; then
    echo "No timing data found in $TARGET_DIR"
    exit 0
fi

echo "Total packages with timing data: $TOTAL_PACKAGES"
echo ""

# ---- Section 1: Top packages by duration ----
echo "=============================================="
echo "  TOP 30 SLOWEST PACKAGES"
echo "=============================================="
printf "%-55s %8s  %-10s  %8s  %8s  %s\n" "PACKAGE" "DURATION" "PHASE" "START" "END" "CACHED"
printf "%-55s %8s  %-10s  %8s  %8s  %s\n" "-------" "--------" "-----" "-----" "---" "------"

sort -t'|' -k1 -rn "$ALL_TIMINGS" | head -30 | while IFS='|' read -r duration phase pkg start_ts end_ts start_fmt end_fmt cached; do
    mins=$(( duration / 60 ))
    secs=$(( duration % 60 ))
    printf "%-55s %4dm %02ds  %-10s  %8s  %8s  %s\n" "$pkg" "$mins" "$secs" "$phase" "$start_fmt" "$end_fmt" "$cached"
done

echo ""

# ---- Section 2: Phase breakdown ----
echo "=============================================="
echo "  PHASE BREAKDOWN"
echo "=============================================="

for phase in $(cut -d'|' -f2 "$ALL_TIMINGS" | sort -u); do
    phase_data=$(grep "|${phase}|" "$ALL_TIMINGS")
    count=$(echo "$phase_data" | wc -l)
    total_cpu=$(echo "$phase_data" | awk -F'|' '{sum+=$1} END {print sum}')
    earliest=$(echo "$phase_data" | awk -F'|' '{print $4}' | sort -n | head -1)
    latest=$(echo "$phase_data" | awk -F'|' '{print $5}' | sort -n | tail -1)
    wall_clock=$(( latest - earliest ))

    total_mins=$(( total_cpu / 60 ))
    wall_mins=$(( wall_clock / 60 ))
    wall_secs=$(( wall_clock % 60 ))

    if [ "$wall_clock" -gt 0 ]; then
        efficiency=$(( total_cpu * 100 / wall_clock ))
    else
        efficiency=0
    fi

    printf "  %-15s  %3d pkgs  %4d min CPU  %3dm %02ds wall  %3d%% parallelism\n" \
        "$phase" "$count" "$total_mins" "$wall_mins" "$wall_secs" "$efficiency"
done

echo ""

# ---- Section 3: Parallelism timeline (60-second samples) ----
echo "=============================================="
echo "  PARALLELISM OVER TIME"
echo "=============================================="

GLOBAL_START=$(awk -F'|' '{print $4}' "$ALL_TIMINGS" | sort -n | head -1)
GLOBAL_END=$(awk -F'|' '{print $5}' "$ALL_TIMINGS" | sort -n | tail -1)
TOTAL_WALL=$(( GLOBAL_END - GLOBAL_START ))

echo "  Build window: $(date -d "@$GLOBAL_START" +%H:%M:%S) → $(date -d "@$GLOBAL_END" +%H:%M:%S) ($(( TOTAL_WALL / 60 ))m $(( TOTAL_WALL % 60 ))s)"
echo ""

# Sample concurrency at 60-second intervals
echo "  Time        Concurrent Builds"
echo "  ----        -----------------"
for offset in $(seq 0 60 "$TOTAL_WALL"); do
    ts=$(( GLOBAL_START + offset ))
    concurrent=$(awk -F'|' -v t="$ts" '$4 <= t && $5 > t' "$ALL_TIMINGS" | wc -l)
    bar=$(printf '%*s' "$concurrent" '' | tr ' ' '█')
    printf "  %s  %2d  %s\n" "$(date -d "@$ts" +%H:%M:%S)" "$concurrent" "$bar"
done

echo ""

# ---- Section 4: Summary stats ----
echo "=============================================="
echo "  SUMMARY"
echo "=============================================="

TOTAL_CPU=$(awk -F'|' '{sum+=$1} END {print sum}' "$ALL_TIMINGS")
CACHED_COUNT=$(grep '|yes$' "$ALL_TIMINGS" | wc -l)
MAX_CONCURRENT=$(
    for offset in $(seq 0 10 "$TOTAL_WALL"); do
        ts=$(( GLOBAL_START + offset ))
        awk -F'|' -v t="$ts" '$4 <= t && $5 > t' "$ALL_TIMINGS" | wc -l
    done | sort -n | tail -1
)

echo "  Total packages:       $TOTAL_PACKAGES"
echo "  Cached packages:      $CACHED_COUNT"
echo "  Total CPU time:       $(( TOTAL_CPU / 60 ))m $(( TOTAL_CPU % 60 ))s"
echo "  Total wall time:      $(( TOTAL_WALL / 60 ))m $(( TOTAL_WALL % 60 ))s"
echo "  Max concurrent:       $MAX_CONCURRENT"
if [ "$TOTAL_WALL" -gt 0 ]; then
    echo "  Parallelism ratio:    $(( TOTAL_CPU * 100 / TOTAL_WALL ))% (CPU/wall)"
fi
echo ""

# ---- Section 5: CSV export ----
CSV_FILE="${TARGET_DIR}/build-timing.csv"
echo "package,phase,duration_sec,start_ts,end_ts,start_time,end_time,cached" > "$CSV_FILE"
sort -t'|' -k1 -rn "$ALL_TIMINGS" | while IFS='|' read -r duration phase pkg start_ts end_ts start_fmt end_fmt cached; do
    echo "$pkg,$phase,$duration,$start_ts,$end_ts,$start_fmt,$end_fmt,$cached"
done >> "$CSV_FILE"
echo "  CSV exported to: $CSV_FILE"
echo ""
echo "=============================================="
