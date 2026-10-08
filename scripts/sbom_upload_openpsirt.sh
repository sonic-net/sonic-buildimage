#!/bin/bash
#
# sbom_upload_openpsirt.sh — send each installer image's CycloneDX SBOM to
# an OpenPSIRT instance, which scans it and tracks what changed since the
# previous build of the same branch and platform.
#
# Usage: sbom_upload_openpsirt.sh <directory holding the images and SBOMs>
#
# Environment:
#   OPENPSIRT_URL      base URL of the instance, e.g. https://sonic-psirt.nexthop.ai
#   OPENPSIRT_API_KEY  pipeline key (opk_...) scoped to the product
#   OPENPSIRT_STREAM   branch the image was built from, e.g. master or 202611
#   OPENPSIRT_PRODUCT  product name on the instance (default: sonic)
#
# One upload per machine: sonic-<machine>.bin, .img.gz or .tar is sent to
# variant <machine>. The aboot .swi and the BlueField .bfb repackage the
# rootfs their machine's .bin already carries, so they are not sent again.
# An SBOM with no image beside it is skipped: CI renames debug and RPC
# images after building them, and the sidecar left behind describes
# whichever build ran last.
#
# OpenPSIRT refuses an upload to a branch or platform nobody declared there,
# and one whose build time is not newer than what it holds; the build time is
# the HEAD commit (SOURCE_DATE_EPOCH), so a nightly of an unchanged branch is
# refused as not newer, which is logged and is not a warning. An undeclared
# name is a warning. A credential or server failure exits non-zero.

set -euo pipefail

dir="${1:?usage: $0 <directory>}"
product="${OPENPSIRT_PRODUCT:-sonic}"
stream="${OPENPSIRT_STREAM:-}"
url="${OPENPSIRT_URL:-}"
url="${url%/}"

# Azure Pipelines turns a logging command into an annotation on the run.
warn() {
    if [ "${TF_BUILD:-}" = "True" ]; then
        echo "##vso[task.logissue type=warning]$*"
    else
        echo "warning: $*" >&2
    fi
}
error() {
    if [ "${TF_BUILD:-}" = "True" ]; then
        echo "##vso[task.logissue type=error]$*"
    else
        echo "error: $*" >&2
    fi
}

# An unset pipeline variable reaches the step as its own macro text.
# shellcheck disable=SC2016
case "${OPENPSIRT_API_KEY:-}" in
    ""|'$('*)
        warn "OPENPSIRT_API_KEY is not set; no SBOM uploaded"
        exit 0
        ;;
esac
case "$url" in
    https://*|http://*) ;;
    *)
        warn "OPENPSIRT_URL is not set; no SBOM uploaded"
        exit 0
        ;;
esac

# Names go into the request path unencoded.
name_ok() { [[ "$1" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; }
if ! name_ok "$stream"; then
    error "OPENPSIRT_STREAM '$stream' is not a branch name"
    exit 1
fi
if ! name_ok "$product"; then
    error "OPENPSIRT_PRODUCT '$product' is not a product name"
    exit 1
fi

response="$(mktemp)"
trap 'rm -f "$response"' EXIT

declare -A sent=()
failed=0
found=0

shopt -s nullglob
for sbom in "$dir"/sonic-*.cdx.json; do
    image="${sbom%.cdx.json}"
    base="$(basename "$image")"
    case "$base" in
        sonic-aboot-*.swi|*.bfb)
            continue
            ;;
        *.bin)    variant="${base%.bin}" ;;
        *.img.gz) variant="${base%.img.gz}" ;;
        *.tar)    variant="${base%.tar}" ;;
        *)
            continue
            ;;
    esac
    variant="${variant#sonic-}"

    if [ ! -f "$image" ]; then
        echo "$base: no image beside $(basename "$sbom"), skipped"
        continue
    fi
    if [ -n "${sent[$variant]:-}" ]; then
        echo "$base: variant $variant already sent from ${sent[$variant]}, skipped"
        continue
    fi
    if ! name_ok "$variant"; then
        warn "$base: '$variant' is not a variant name, skipped"
        continue
    fi
    sent[$variant]="$base"
    found=$((found + 1))

    endpoint="$url/v1/products/$product/streams/$stream/variants/$variant/scans"
    echo "$base -> $product/$stream/$variant"

    # The key is passed on a file descriptor so it is not on the command line.
    # curl retries a timeout and a 503 from a full queue; a retried upload the
    # server already took is answered as already held.
    status="$(curl --silent --show-error \
        --output "$response" --write-out '%{http_code}' \
        --retry 5 --retry-delay 60 --retry-max-time 1200 \
        --connect-timeout 30 --max-time 1800 \
        --header @<(printf 'Authorization: Bearer %s\n' "$OPENPSIRT_API_KEY") \
        --form "inventory=@$sbom;type=application/json" \
        "$endpoint")" || status="000"
    body="$(head -c 2000 "$response" 2>/dev/null || true)"

    case "$status" in
        200|202)
            echo "  accepted: $body"
            ;;
        404)
            warn "$product/$stream/$variant is not declared on $url; an administrator declares the branch and the variant. $body"
            ;;
        409|400)
            if [[ "$body" == *"(not newer)"* ]]; then
                echo "  not newer than the build OpenPSIRT holds, nothing to do"
            else
                warn "$base was refused ($status): $body"
            fi
            ;;
        *)
            error "$base: upload failed ($status): $body"
            failed=1
            ;;
    esac
done

if [ "$found" -eq 0 ]; then
    warn "no installer image SBOM found in $dir"
fi
exit "$failed"
