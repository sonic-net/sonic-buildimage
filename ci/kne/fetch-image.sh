#!/usr/bin/env bash
# Select a SONiC VS disk image for this pull request.
# image-source is "s3" or "build". "build" skips. "s3" copies each key in
# images.txt from the MinIO bucket on this host.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SOURCE="$(sed -e 's/#.*//' -e 's/[[:space:]]//g' -e '/^$/d' "${ROOT}/ci/kne/image-source" | head -1)"
SOURCE="${SOURCE:-s3}"

if [[ "$SOURCE" == "build" ]]; then
  echo "::notice::local sonic-vs build is a placeholder and is skipped"
  echo "KNE_IMAGE_SOURCE=build" >> "${GITHUB_ENV:-/dev/null}"
  exit 0
fi
if [[ "$SOURCE" != "s3" ]]; then
  echo "::error::ci/kne/image-source must be s3 or build, got '${SOURCE}'"
  exit 1
fi

MC="${MINIO_MC:-/home/upscaleai/minio/bin/mc}"
[[ -x "$MC" ]] || { echo "::error::MinIO client not found at ${MC}"; exit 1; }
DEST="${KNE_IMAGE_DIR:-/home/upscaleai/kne-images}"
mkdir -p "$DEST"

mapfile -t KEYS < <(sed -e 's/#.*//' -e 's/[[:space:]]//g' -e '/^$/d' "${ROOT}/ci/kne/images.txt")
[[ ${#KEYS[@]} -gt 0 ]] || { echo "::error::no keys in ci/kne/images.txt"; exit 1; }

for key in "${KEYS[@]}"; do
  echo "pulling s3://sonic-vs/${key} -> ${DEST}/${key}"
  "$MC" cp "local/sonic-vs/${key}" "${DEST}/${key}"
  ls -lh "${DEST}/${key}"
done

echo "KNE_IMAGE_SOURCE=s3" >> "${GITHUB_ENV:-/dev/null}"
echo "KNE_IMAGE_DIR=${DEST}" >> "${GITHUB_ENV:-/dev/null}"
echo "::notice::pulled ${#KEYS[@]} image(s) from MinIO into ${DEST}"
