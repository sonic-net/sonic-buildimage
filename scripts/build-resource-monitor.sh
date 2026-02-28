#!/bin/bash
# build-resource-monitor.sh — Sample system resources during SONiC build
#
# Usage: ./scripts/build-resource-monitor.sh [interval_seconds] [output_file]
#   Runs in background, samples every N seconds until killed.
#   Designed to run alongside a build: start before, kill after.
#
# Output: CSV with timestamp, CPU%, memory_used_gb, memory_total_gb,
#         load_avg_1m, docker_containers, disk_io_mb_s

set -euo pipefail

INTERVAL="${1:-10}"
OUTPUT="${2:-./target/resource-monitor.csv}"

mkdir -p "$(dirname "$OUTPUT")"

echo "timestamp,epoch,cpu_pct,mem_used_gb,mem_total_gb,mem_pct,load_1m,load_5m,docker_containers,disk_read_mb_s,disk_write_mb_s" > "$OUTPUT"

echo "Resource monitor started (interval: ${INTERVAL}s, output: $OUTPUT)"
echo "Kill with: kill $$"

# Track previous disk stats for delta
prev_read=0
prev_write=0
first=true

while true; do
    ts=$(date +%H:%M:%S)
    epoch=$(date +%s)

    # CPU - delta between two /proc/stat reads (1 second apart)
    read_cpu_stats() { awk '/^cpu / {print $2, $3, $4, $5, $6, $7, $8}' /proc/stat; }
    cpu1=$(read_cpu_stats)
    sleep 1
    cpu2=$(read_cpu_stats)
    cpu_pct=$(echo "$cpu1" "$cpu2" | awk '{
        u1=$1+$2+$3; i1=$4; s1=$5+$6+$7;
        u2=$8+$9+$10; i2=$11; s2=$12+$13+$14;
        total=(u2+i2+s2)-(u1+i1+s1);
        if (total > 0) printf "%.0f", ((u2+s2)-(u1+s1))/total*100;
        else print "0";
    }')

    # Memory
    mem_info=$(free -m | awk '/^Mem:/ {printf "%.1f,%.1f,%.0f", $3/1024, $2/1024, $3/$2*100}')

    # Load average
    load=$(awk '{printf "%s,%s", $1, $2}' /proc/loadavg)

    # Docker containers running
    docker_count=$(docker ps -q 2>/dev/null | wc -l || echo 0)

    # Disk I/O (from /proc/diskstats, aggregate all devices)
    disk_stats=$(awk '{read+=$6; write+=$10} END {printf "%d,%d", read, write}' /proc/diskstats)
    cur_read=$(echo "$disk_stats" | cut -d, -f1)
    cur_write=$(echo "$disk_stats" | cut -d, -f2)

    if [ "$first" = true ]; then
        disk_read_mb=0
        disk_write_mb=0
        first=false
    else
        # Sectors are 512 bytes, convert to MB over interval
        if [ "$INTERVAL" -gt 0 ]; then
            disk_read_mb=$(( (cur_read - prev_read) * 512 / 1048576 / INTERVAL ))
            disk_write_mb=$(( (cur_write - prev_write) * 512 / 1048576 / INTERVAL ))
        else
            disk_read_mb=$(( (cur_read - prev_read) * 512 / 1048576 ))
            disk_write_mb=$(( (cur_write - prev_write) * 512 / 1048576 ))
        fi
    fi
    prev_read=$cur_read
    prev_write=$cur_write

    echo "${ts},${epoch},${cpu_pct},${mem_info},${load},${docker_count},${disk_read_mb},${disk_write_mb}" >> "$OUTPUT"

    sleep "$INTERVAL"
done
