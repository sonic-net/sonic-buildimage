#!/usr/bin/env bash
# Validate the retained native VS container pairs and P4 debug artifacts.
set -euo pipefail

runtime_only=false
if [[ ${1:-} == --runtime-only ]]; then runtime_only=true; shift; fi
if (( $# )); then
    echo 'Usage: validate_vs.sh [--runtime-only]' >&2
    exit 2
fi
if [[ ${NOTRIXIE:-0} != 0 ]]; then
    echo 'VS validation requires the Trixie build environment' >&2
    exit 1
fi

mkdir -p artifacts/vs
# Preserve the outer Makefile's caller overrides when entering Makefile.work
# directly. An environment value still lets it append its version-cache settings.
export SONIC_OVERRIDE_BUILD_VARS="${SONIC_OVERRIDE_BUILD_VARS:-} ${SONIC_BUILD_VARS:-}"
builder_args="--label ${SONIC_CI_CONTAINER_LABEL:?} -e BAZEL=${BAZEL:?} -e BAZEL_CONTAINER_ARGS"
if [[ ${SONIC_VS_REUSE_BAZEL_PRODUCERS:-} == 1 ]]; then
    builder_args+=" -e SONIC_VS_REUSE_BAZEL_PRODUCERS"
fi
options=(
    BUILD_WITH_BAZEL_WHEN_AVAILABLE=y INCLUDE_VS_DASH_SAI=y INCLUDE_FIPS=y
    ENABLE_ASAN=n ENABLE_SYNCD_RPC=n
    "SONIC_BUILDER_EXTRA_CMDLINE=$builder_args"
)
receipt=artifacts/vs/orchagent-reuse.json
pairs=artifacts/vs/runtime-debug-pairs.json
if [[ ! -s artifacts/vs/build-revision.txt ]]; then
    echo 'Missing VS build revision record' >&2
    exit 1
fi
revision=$(< artifacts/vs/build-revision.txt)
current_revision=$(git rev-parse HEAD)
if [[ "$current_revision" != "$revision" ]]; then
    echo 'Source revision changed since the VS build started' >&2
    exit 1
fi
if "$runtime_only"; then
    # Only the runtime OCI layouts and archives enter the installed-image checks.
    ./scripts/run_with_retry make -f Makefile.work BLDENV=trixie sonic-slave-run "${options[@]}" \
        'SONIC_RUN_CMDS=bash tools/bazel/ci/verify_vs_oci.sh --runtime-only $(SONIC_BUILD_INSTRUCTION)'
    exit
fi
clock() { python3 -c 'import time; print(time.monotonic())'; }
capture_start=$(clock)
# Check both debug side outputs and installer selection before any further
# builder can repair missing archives. Validation must consume the VS outputs.
python3 -B tools/bazel/ci/vs_archive_reuse.py capture-pairs --workspace "$PWD" \
    --receipt "$pairs" --orchagent-receipt "$receipt"
capture_end=$(clock)

# Resolve legacy P4 debug inputs and run their checks without requesting any
# standalone Orchagent or Syncd container archive targets again.
p4_start=$(clock)
if [[ ${NOBOOKWORM:-0} == 0 ]]; then
    ./scripts/run_with_retry make -f Makefile.work BLDENV=bookworm bookworm "${options[@]}"
fi
# Make expands its normal slave command here, including configured overrides.
# Pass it as arguments so the verifier can reuse it without shell evaluation.
./scripts/run_with_retry make -f Makefile.work BLDENV=trixie sonic-slave-run "${options[@]}" \
    'SONIC_RUN_CMDS=bash tools/bazel/ci/verify_vs_oci.sh $(SONIC_BUILD_INSTRUCTION)'
make -f Makefile.work BLDENV=bookworm docker-cleanup "${options[@]}"
p4_end=$(clock)
verify_start=$(clock)
python3 -B tools/bazel/ci/vs_archive_reuse.py verify-pairs --workspace "$PWD" \
    --receipt "$pairs" --orchagent-receipt "$receipt"
verify_end=$(clock)
python3 - "$capture_start" "$capture_end" "$p4_start" "$p4_end" \
    "$verify_start" "$verify_end" "$revision" <<'PYTHON'
import json
from pathlib import Path
import subprocess
import sys

capture_start, capture_end, p4_start, p4_end, verify_start, verify_end = map(float, sys.argv[1:7])
revision = sys.argv[7]
if subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip() != revision:
    raise SystemExit("Source revision changed during VS validation")
pair_seconds = capture_end - capture_start + verify_end - verify_start
p4_seconds = p4_end - p4_start
report = {
    "schema": 2,
    "revision": revision,
    "profile": {"architecture": "amd64", "platform": "vs", "bazel": True,
                "fips": True, "dash": True, "asan": False, "syncd_rpc": False,
                "install_debug_tools": False},
    "p4_validation_seconds": p4_seconds,
    "pair_validation_seconds": pair_seconds,
    "validation_seconds": p4_seconds + pair_seconds,
    "scope": "P4 and archive-pair validation only; build and later CI steps are measured separately.",
}
Path("artifacts/vs/runtime-debug-pairs-timing.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, sort_keys=True))
PYTHON
