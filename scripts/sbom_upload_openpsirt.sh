#!/bin/bash
#
# sbom_upload_openpsirt.sh — send each installer image's CycloneDX SBOM to
# an OpenPSIRT instance, which scans it and tracks what changed since the
# previous build of the same branch and image.
#
# Usage: sbom_upload_openpsirt.sh <directory holding the images and SBOMs>
#
# Environment:
#   OPENPSIRT_URL      base URL of the instance, https only
#   OPENPSIRT_API_KEY  pipeline key (opk_...) scoped to the product
#   OPENPSIRT_BRANCH   branch the image was built from: refs/heads/master,
#                      or master
#   OPENPSIRT_PRODUCT  product name on the instance (default: sonic)
#   OPENPSIRT_BUDGET   seconds the whole run may take (default: 900)
#
# The script always exits 0. Every problem is reported as a warning, so a
# nightly build is never failed or marked partially succeeded by an upload.
#
# The variant is the image name: sonic-broadcom-dnx.bin is broadcom-dnx,
# sonic-vs.img.gz is vs. The aboot .swi and the BlueField .bfb repackage
# the rootfs the same machine's .bin carries, so they are not sent. An SBOM
# is sent only with its image beside it.
#
# A branch whose name contains '/' is not sent: OpenPSIRT names a stream
# with one segment, and cutting the name to its last segment would file a
# build of users/x/master as master.
#
# OpenPSIRT refuses an upload whose build time is not newer than the one it
# holds. The build time is the HEAD commit (SOURCE_DATE_EPOCH), so a nightly
# of an unchanged branch is refused as not newer, which is logged rather
# than warned about.

set -uo pipefail

dir="${1:?usage: $0 <directory>}"
product="${OPENPSIRT_PRODUCT:-sonic}"
branch="${OPENPSIRT_BRANCH:-}"
url="${OPENPSIRT_URL:-}"
url="${url%/}"
budget="${OPENPSIRT_BUDGET:-900}"

# Azure Pipelines turns a logging command into an annotation on the run.
warn() {
    if [ "${TF_BUILD:-}" = "True" ]; then
        echo "##vso[task.logissue type=warning]$*"
    else
        echo "warning: $*" >&2
    fi
}

# Server text goes into the log on one line, with nothing the agent reads as
# a logging command.
clean() {
    local text="$1"
    text="${text//$'\r'/ }"
    text="${text//$'\n'/ }"
    text="${text//##/#}"
    printf '%s' "$text"
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
    https://*) ;;
    *)
        warn "OPENPSIRT_URL is not an https URL; no SBOM uploaded"
        exit 0
        ;;
esac
if ! [[ "$budget" =~ ^[0-9]+$ ]]; then
    budget=900
fi

# Names go into the request path unencoded.
name_ok() { [[ "$1" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; }
stream="${branch#refs/heads/}"
if [[ "$stream" == */* ]] || ! name_ok "$stream"; then
    warn "'$branch' is not a branch OpenPSIRT tracks; no SBOM uploaded"
    exit 0
fi
if ! name_ok "$product"; then
    warn "'$product' is not a product name; no SBOM uploaded"
    exit 0
fi

response="$(mktemp)" || { warn "no temporary file; no SBOM uploaded"; exit 0; }
trap 'rm -f "$response"' EXIT

declare -A sent=()
found=0
deadline=$((SECONDS + budget))

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

    remaining=$((deadline - SECONDS))
    if [ "$remaining" -lt 60 ]; then
        warn "out of time; $base and any image after it not uploaded"
        break
    fi

    endpoint="$url/v1/products/$product/streams/$stream/variants/$variant/scans"
    echo "$base -> $product/$stream/$variant"

    # The key is passed on a file descriptor so it is not on the command line.
    # curl retries a timeout and a 503 from a full queue, and a retried upload
    # the server already took is answered as already held. timeout bounds the
    # attempts together.
    : > "$response"
    status="$(timeout "$remaining" curl --silent --show-error \
        --output "$response" --write-out '%{http_code}' \
        --retry 3 --retry-delay 30 \
        --connect-timeout 30 --max-time 300 \
        --header @<(printf 'Authorization: Bearer %s\n' "$OPENPSIRT_API_KEY") \
        --form "inventory=@$sbom;type=application/json" \
        "$endpoint" 2>&1)"
    code=$?
    body="$(clean "$(head -c 2000 "$response" 2>/dev/null)")"

    if [ "$code" -ne 0 ] || ! [[ "$status" =~ ^[0-9]{3}$ ]]; then
        # Unreachable, or out of time: the images after this one fare no
        # better, and each would spend the same wait finding that out.
        reason="$(clean "$status")"
        [ "$code" -eq 124 ] && reason="timed out"
        warn "$base: no answer from $url (${reason:-curl exit $code}); remaining images not uploaded"
        break
    fi

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
            warn "$base: upload failed ($status): $body"
            ;;
    esac
done

if [ "$found" -eq 0 ]; then
    warn "no installer image SBOM found in $dir"
fi
exit 0
