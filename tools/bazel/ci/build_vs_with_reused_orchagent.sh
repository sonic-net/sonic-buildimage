#!/usr/bin/env bash
# Build the fixed native VS CI profile and retain its verified Orchagent archive.
set -euo pipefail

measure=false
if [[ ${1:-} == --measure-redundant-request && $# == 1 ]]; then
    measure=true
elif (( $# )); then
    echo 'Usage: build_vs_with_reused_orchagent.sh [--measure-redundant-request]' >&2
    exit 2
fi

mkdir -p artifacts/vs
options=(
    BUILD_WITH_BAZEL_WHEN_AVAILABLE=y INCLUDE_VS_DASH_SAI=y INCLUDE_FIPS=y
    ENABLE_ASAN=n ENABLE_SYNCD_RPC=n
    "SONIC_BUILDER_EXTRA_CMDLINE=--label ${SONIC_CI_CONTAINER_LABEL:?} -e BAZEL=${BAZEL:?} -e BAZEL_CONTAINER_ARGS"
)
receipt=artifacts/vs/orchagent-reuse.json
make target/sonic-vs.img.gz "${options[@]}"
capture_start=$EPOCHREALTIME
python3 -B tools/bazel/ci/vs_archive_reuse.py capture --workspace "$PWD" --receipt "$receipt"
capture_end=$EPOCHREALTIME

# Opt-in calibration: execute the old redundant request against the very same
# revision, flags, worker and warmed caches. Verification rejects changed bytes.
# Ordinary PR builds never execute this request.
baseline_seconds=
if $measure; then
    baseline_start=$EPOCHREALTIME
    make target/docker-orchagent.gz "${options[@]}"
    baseline_end=$EPOCHREALTIME
    python3 -B tools/bazel/ci/vs_archive_reuse.py verify-runtime --workspace "$PWD" --receipt "$receipt"
    baseline_seconds=$(python3 - "$baseline_start" "$baseline_end" <<'PY'
import sys
print(float(sys.argv[2]) - float(sys.argv[1]))
PY
    )
fi

make target/docker-orchagent-dbg.gz \
    target/docker-syncd-vs.gz target/docker-syncd-vs-dbg.gz \
    target/debs/trixie/sonic-p4rt_0.0.1_amd64.deb \
    target/debs/trixie/sonic-p4rt-dbgsym_0.0.1_amd64.deb "${options[@]}"
verify_start=$EPOCHREALTIME
python3 -B tools/bazel/ci/vs_archive_reuse.py verify --workspace "$PWD" --receipt "$receipt"
verify_end=$EPOCHREALTIME
python3 - "$capture_start" "$capture_end" "$verify_start" "$verify_end" "$baseline_seconds" <<'PY'
import json
from pathlib import Path
import sys

capture_start, capture_end, verify_start, verify_end = map(float, sys.argv[1:5])
validation_seconds = capture_end - capture_start + verify_end - verify_start
baseline_seconds = float(sys.argv[5]) if sys.argv[5] else None
report = {
    "schema": 1,
    "validation_seconds": validation_seconds,
    "redundant_request_seconds": baseline_seconds,
    "net_avoided_seconds": None if baseline_seconds is None else baseline_seconds - validation_seconds,
    "scope": "Same-run redundant request minus archive validation; not a paired full-job speedup.",
}
Path("artifacts/vs/orchagent-reuse-timing.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, sort_keys=True))
PY
