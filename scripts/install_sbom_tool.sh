#!/bin/bash
#
# install_sbom_tool.sh — fetch the binary scanner used by SBOM generation.
#
# Runs *inside the slave container* (or another build context where the
# src/sonic-build-hooks/hooks/wget shim is active) so the fetched URL +
# MD5 is recorded into target/versions/.../versions-web automatically.
#
# Resolution order (per tool):
#   1. If $TOOL is already on PATH, print its path and exit.
#   2. If the cached binary is present under $SBOM_TOOLS_DIR, use it.
#   3. Else wget the upstream release, verify SHA-256 against the pin in
#      this script, extract into the cache, and print the path.
#
# Usage:
#   scripts/install_sbom_tool.sh syft   # prints absolute path to the binary
#   scripts/install_sbom_tool.sh trivy
#   scripts/install_sbom_tool.sh grype
#
# Pins are intentionally hard-coded here so a single edit moves the
# tool version and the existing build-hook infrastructure picks up the
# new URL on next build.

set -euo pipefail

TOOL="${1:-}"
if [ -z "$TOOL" ]; then
    echo "usage: $0 <syft|trivy|grype|cyclonedx-cli>" >&2
    exit 2
fi

# Honour TARGET_PATH from slave.mk if set; otherwise default to ./target.
TARGET_PATH="${TARGET_PATH:-target}"
SBOM_TOOLS_DIR="${SBOM_TOOLS_DIR:-${TARGET_PATH}/sbom-tools}"

# Map our $(dpkg --print-architecture) names to the names releases use.
host_arch="$(dpkg --print-architecture 2>/dev/null || uname -m)"
case "$host_arch" in
    amd64|x86_64)  rel_arch="amd64" ;;
    arm64|aarch64) rel_arch="arm64" ;;
    *)
        echo "[install_sbom_tool.sh] WARNING: unsupported host arch '$host_arch'." >&2
        echo "[install_sbom_tool.sh] The binary scanner is not available; SBOM" >&2
        echo "[install_sbom_tool.sh] generation will fall back to recipe-driven" >&2
        echo "[install_sbom_tool.sh] data only on this build." >&2
        exit 3
        ;;
esac

##############################################################################
# Pinned versions and SHA-256s.
#
# To bump a tool version: update the version, paste the SHA-256 lines from
# the upstream <tool>_<ver>_checksums.txt. The wget call below is shimmed,
# so the URL+MD5 gets captured into versions-web on next build and the
# pin propagates through scripts/versions_manager.py.
##############################################################################

SYFT_VERSION="1.44.0"
SYFT_URL="https://github.com/anchore/syft/releases/download/v${SYFT_VERSION}/syft_${SYFT_VERSION}_linux_${rel_arch}.tar.gz"
SYFT_SHA256_amd64="0e91737aee2b5baf1d255b959630194a302335d848ff97bb07921eb6205b5f5a"
SYFT_SHA256_arm64="6f6cdcdc695721d91ce756e3b5bc3e3416599c464101f5e32e9c3f33054ee6d9"

# CycloneDX-CLI (CycloneDX/cyclonedx-cli on GitHub). Single static
# binary (.NET self-contained) — no tarball, fetched directly.
# Used by build_sbom.py for CycloneDX -> SPDX conversion when
# SBOM_FORMAT includes 'spdx' or 'both'.
CYCLONEDX_CLI_VERSION="0.32.0"
case "$rel_arch" in
    amd64) CYCLONEDX_CLI_URL="https://github.com/CycloneDX/cyclonedx-cli/releases/download/v${CYCLONEDX_CLI_VERSION}/cyclonedx-linux-x64" ;;
    arm64) CYCLONEDX_CLI_URL="https://github.com/CycloneDX/cyclonedx-cli/releases/download/v${CYCLONEDX_CLI_VERSION}/cyclonedx-linux-arm64" ;;
esac
CYCLONEDX_CLI_SHA256_amd64="454879e6a4a405c8a13bff49b8982adcb0596f3019b26b0811c66e4d7f0783e1"
CYCLONEDX_CLI_SHA256_arm64="abf0b7c5648a5b127791d691cad41f004aceea27c75bb42c9572fdc9694770cf"

GRYPE_VERSION="0.112.0"
GRYPE_URL="https://github.com/anchore/grype/releases/download/v${GRYPE_VERSION}/grype_${GRYPE_VERSION}_linux_${rel_arch}.tar.gz"
GRYPE_SHA256_amd64="acb14a030010fe9bdb9594b4ae108d9d14ef2f926d936aa0916dc62c89c058ea"
GRYPE_SHA256_arm64="7fdeccf065965cc59386c656e5fcc1eb1bdf820e2433000bca7f010b8e6da155"

# Trivy is a placeholder — fill in when SBOM_SCAN_TOOL=trivy or
# vulnerability scanning via trivy is exercised. For now the script
# errors out cleanly if it's requested.
TRIVY_VERSION=""

##############################################################################
# Common helpers.
##############################################################################

cache_path() {
    local tool="$1" version="$2"
    echo "${SBOM_TOOLS_DIR}/${tool}-${version}/${tool}"
}

verify_sha256() {
    local file="$1" expected="$2"
    local actual
    actual="$(sha256sum "$file" | awk '{print $1}')"
    if [ "$actual" != "$expected" ]; then
        echo "[install_sbom_tool.sh] SHA-256 mismatch for $file:" >&2
        echo "  expected: $expected" >&2
        echo "  actual:   $actual" >&2
        return 1
    fi
}

fetch_and_extract() {
    local tool="$1" version="$2" url="$3" sha256="$4"
    local dest_dir="${SBOM_TOOLS_DIR}/${tool}-${version}"
    local tarball="${dest_dir}.tar.gz"

    mkdir -p "$dest_dir"

    # Use wget so the build-hook shim records URL+MD5 into versions-web.
    # --no-verbose keeps build logs clean; --tries handles transient network.
    wget --no-verbose --tries=3 -O "$tarball" "$url"
    verify_sha256 "$tarball" "$sha256"

    tar -C "$dest_dir" -xzf "$tarball" "$tool"
    rm -f "$tarball"

    chmod +x "${dest_dir}/${tool}"
}

# Fetch a single-binary tool (no tarball wrapper). cyclonedx-cli ships
# as a raw .NET self-contained executable, not a .tar.gz.
fetch_single_binary() {
    local tool="$1" version="$2" url="$3" sha256="$4"
    local dest_dir="${SBOM_TOOLS_DIR}/${tool}-${version}"
    local dest="${dest_dir}/${tool}"

    mkdir -p "$dest_dir"
    wget --no-verbose --tries=3 -O "$dest" "$url"
    verify_sha256 "$dest" "$sha256"
    chmod +x "$dest"
}

##############################################################################
# Per-tool resolution.
##############################################################################

resolve_syft() {
    if command -v syft >/dev/null 2>&1; then
        command -v syft
        return 0
    fi

    local cached
    cached="$(cache_path syft "$SYFT_VERSION")"
    if [ -x "$cached" ]; then
        echo "$cached"
        return 0
    fi

    local sha256_var="SYFT_SHA256_${rel_arch}"
    local sha256="${!sha256_var:-}"
    if [ -z "$sha256" ]; then
        echo "[install_sbom_tool.sh] no SHA-256 pin for syft on $rel_arch" >&2
        return 4
    fi

    fetch_and_extract syft "$SYFT_VERSION" "$SYFT_URL" "$sha256" >&2
    echo "$cached"
}

resolve_trivy() {
    echo "[install_sbom_tool.sh] trivy installer not yet implemented." >&2
    echo "[install_sbom_tool.sh] Use SBOM_SCAN_TOOL=syft (the default)." >&2
    return 5
}

resolve_grype() {
    if command -v grype >/dev/null 2>&1; then
        command -v grype
        return 0
    fi

    local cached
    cached="$(cache_path grype "$GRYPE_VERSION")"
    if [ -x "$cached" ]; then
        echo "$cached"
        return 0
    fi

    local sha256_var="GRYPE_SHA256_${rel_arch}"
    local sha256="${!sha256_var:-}"
    if [ -z "$sha256" ]; then
        echo "[install_sbom_tool.sh] no SHA-256 pin for grype on $rel_arch" >&2
        return 4
    fi

    fetch_and_extract grype "$GRYPE_VERSION" "$GRYPE_URL" "$sha256" >&2
    echo "$cached"
}

resolve_cyclonedx_cli() {
    # cyclonedx-cli ships under the name "cyclonedx" on PATH.
    if command -v cyclonedx >/dev/null 2>&1; then
        command -v cyclonedx
        return 0
    fi

    local cached
    cached="$(cache_path cyclonedx "$CYCLONEDX_CLI_VERSION")"
    if [ -x "$cached" ]; then
        echo "$cached"
        return 0
    fi

    if [ -z "${CYCLONEDX_CLI_URL:-}" ]; then
        echo "[install_sbom_tool.sh] no cyclonedx-cli release for arch $rel_arch" >&2
        return 4
    fi

    local sha256_var="CYCLONEDX_CLI_SHA256_${rel_arch}"
    local sha256="${!sha256_var:-}"
    if [ -z "$sha256" ]; then
        echo "[install_sbom_tool.sh] no SHA-256 pin for cyclonedx-cli on $rel_arch" >&2
        return 4
    fi

    fetch_single_binary cyclonedx "$CYCLONEDX_CLI_VERSION" \
        "$CYCLONEDX_CLI_URL" "$sha256" >&2
    echo "$cached"
}

case "$TOOL" in
    syft)           resolve_syft ;;
    trivy)          resolve_trivy ;;
    grype)          resolve_grype ;;
    cyclonedx-cli|cyclonedx)
                    resolve_cyclonedx_cli ;;
    *)
        echo "[install_sbom_tool.sh] unknown tool: $TOOL" >&2
        exit 2
        ;;
esac
