#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
pipeline="${repo_root}/.azure-pipelines/azure-pipelines-build.yml"

asan_block="$({
    awk '
        index($0, "if [ $(asan_image) == yes ]; then") { in_block = 1 }
        in_block { print }
        in_block && /^[[:space:]]*fi[[:space:]]*$/ { exit }
    ' "${pipeline}"
})"

for artifact in \
    'target/debs/bookworm/libsairedis_*.deb' \
    'target/debs/bookworm/syncd-vs_*.deb' \
    'target/debs/bookworm/syncd-vs-dbgsym_*.deb'
do
    if ! grep -Fq "${artifact}" <<< "${asan_block}"; then
        echo "ASAN cleanup is missing ${artifact}" >&2
        exit 1
    fi
done
