#!/bin/bash
#
# test_installer_qemu.sh
#
# This script builds a unified Docker image and runs a QEMU-based test for the
# SONiC installer. It prepares necessary artifacts (UKI, disk images, config bundles)
# and launches a container that emulates the PXE boot process.
#
# Usage:
#   ./test_installer_qemu.sh [-6|-4] [--install-disk]
#
# Options:
#   -6              Use IPv6 (default)
#   -4              Use IPv4
#   --install-disk  Enable installation to disk
#   --secure-boot   Enable secure boot
#
# Environment Variables (Bazel):
#   RUNFILES_DIR    Path to Bazel runfiles directory
#   RUNFILES_MANIFEST_FILE Path to Bazel runfiles manifest

set -e

# --- BAZEL RUNFILES ---
# Wrapper for runfiles (Bazel dependency handling)
if [[ -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
  source "$(grep -m1 "^bazel_tools/tools/bash/runfiles/runfiles.bash " "$RUNFILES_MANIFEST_FILE" | cut -d ' ' -f 2-)"
elif [[ -f "${RUNFILES_DIR:-/dev/null}/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
  source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"
elif [[ -f "${0}.runfiles/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
  source "${0}.runfiles/bazel_tools/tools/bash/runfiles/runfiles.bash"
fi

# Runfiles lookup function with fallback
rlocation_or_local() {
  local rpath="$1"
  local lpath="$2"
  if command -v rlocation >/dev/null 2>&1; then
    # Try looking up in the main workspace (usually __main__ or the repo name)
    local path
    path="$(rlocation "__main__/$rpath" 2>/dev/null || true)"
    if [[ -z "$path" ]]; then
        path="$(rlocation "$rpath" 2>/dev/null || true)"
    fi
    if [[ -n "$path" && -e "$path" ]]; then
      echo "$path"
      return
    fi
  fi
  echo "$lpath"
}

# --- CONFIGURATION ---

# Determine platform (default to 'vs' if .platform file is missing)
readonly PLATFORM="$([ -f .platform ] && cat .platform || echo vs)"

# Resolve artifacts using Bazel runfiles or local fallback
readonly UKI_FILE="$(rlocation_or_local "sonie-${PLATFORM}.efi" "target/sonie-${PLATFORM}.efi")"
readonly SONIC_BIN="$(rlocation_or_local "sonic-${PLATFORM}.bin" "target/sonic-${PLATFORM}.bin")"
readonly SONIE_BIN="$(rlocation_or_local "sonie-${PLATFORM}.bin" "target/sonie-${PLATFORM}.efi.bin")"

readonly PK_AUTH="PK.auth"
readonly KEK_AUTH="KEK.auth"
readonly DB_AUTH="db.auth"

readonly CLIENT_DISK_IMG="client_disk.img"
# Internal IP for the bridge inside the container
readonly BRIDGE_IP="192.168.0.1"
readonly CLIENT_IP_START="192.168.0.100"
readonly CLIENT_IP_END="192.168.0.200"

readonly BRIDGE_IP6="fd00:abcd::1"
readonly CLIENT_IP6_START="fd00:abcd::100"
readonly CLIENT_IP6_END="fd00:abcd::200"

readonly HTTP_PORT=8000
readonly TEST_CONTAINER_NAME="sonic-pxe-test"
readonly TEST_IMAGE_NAME="sonic-pxe-test-img"


readonly CLIENT_OVMF_VARS="client_ovmf_vars.fd"
readonly CLIENT_MAC="52:54:00:65:43:21"


readonly DOCKER_CONTEXT_DIR="$(dirname "$(rlocation_or_local "sonie-test-trixie/Dockerfile" "sonie-test-trixie/Dockerfile")")"

#######################################
# Print error message to stderr.
# Arguments:
#   Message to print.
#######################################
err() {
  echo "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $*" >&2
}

#######################################
# Cleanup function to run on exit.
# Removes temporary files and stops the test container.
#######################################
cleanup() {
  echo "Cleaning up..."
  # Use docker to cat logs because they are root-owned
  docker run --rm -v "$(pwd):/data" debian:trixie sh -c \
    "echo '--- http_server.log ---'; cat /data/http_server.log; echo '-----------------------'; echo '--- dnsmasq.log ---'; cat /data/dnsmasq.log; echo '---------------------'" \
    2>/dev/null || true
  docker rm -f "$TEST_CONTAINER_NAME" 2>/dev/null || true
  rm -f "$CLIENT_DISK_IMG" "$CLIENT_OVMF_VARS" "Dockerfile.test"
}

#######################################
# Check if required files exist.
# Globals:
#   UKI_FILE
#######################################
check_requirements() {
  if [[ ! -f "$UKI_FILE" ]]; then
    err "Error: UKI file not found at $UKI_FILE"
    exit 1
  fi
}

#######################################
# Resolve OVMF firmware paths.
# Globals:
#   OVMF_CODE_FILE
#   OVMF_VARS_FILE
#######################################
resolve_ovmf_paths() {
  echo "Resolving OVMF paths..."

  local ovmf_search_paths=(
    "/usr/share/OVMF"
    "/usr/share/edk2/ovmf"
  )

  local code_names=("OVMF_CODE.fd" "OVMF_CODE_4M.fd")
  local vars_names=("OVMF_VARS.fd" "OVMF_VARS_4M.fd")

  for path in "${ovmf_search_paths[@]}"; do
    for i in "${!code_names[@]}"; do
      local code="${path}/${code_names[$i]}"
      local vars="${path}/${vars_names[$i]}"
      
      if [[ -f "$code" && -f "$vars" ]]; then
         OVMF_CODE_FILE="$code"
         OVMF_VARS_FILE="$vars"
         echo "Found OVMF at $OVMF_CODE_FILE and $OVMF_VARS_FILE"
         return
      fi
    done
  done

  err "Error: OVMF not found."
  exit 1
}

#######################################
# Build the unified Docker image for testing.
# Globals:
#   TEST_IMAGE_NAME
#   DOCKER_CONTEXT_DIR
#######################################
build_docker_image() {
  echo "Building Unified Docker Image..."
  echo "Using build context: $DOCKER_CONTEXT_DIR"

  if [[ ! -d "$DOCKER_CONTEXT_DIR" ]]; then
    err "Error: Docker context directory $DOCKER_CONTEXT_DIR not found."
    exit 1
  fi

  # Build the image using the context directory
  docker build --no-cache --network host -t "$TEST_IMAGE_NAME" "$DOCKER_CONTEXT_DIR"
}

#######################################
# Main entry point.
# Arguments:
#   Command line arguments.
#######################################
main() {
  local secure_boot=0

  while [[ "$#" -gt 0 ]]; do
    case $1 in
      -6) use_ipv6=1 ;;
      -4) use_ipv6=0 ;;
      --install-disk) install_disk=1 ;;
      --secure-boot) secure_boot=1 ;;
      *) err "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
  done

  check_requirements
  resolve_ovmf_paths

  # trap cleanup EXIT

  build_docker_image

  cp "$OVMF_VARS_FILE" "$CLIENT_OVMF_VARS"
  




  # --- PREPARE SECURE BOOT KEYS ---
  # We use environment variables for key paths
  # Only export them if Secure Boot is requested
  if [[ "$secure_boot" -eq 1 ]]; then
      export PK_CERT="PK.crt"
      export KEK_CERT="KEK.crt"
      export DB_CERT="db.crt"
      
      if [[ ! -f "$PK_CERT" || ! -f "$KEK_CERT" || ! -f "$DB_CERT" ]]; then
        echo "Generating Test Secure Boot Keys..."
        
        # Create config file for openssl to disable prompts
        cat > openssl.cnf <<EOF
[ req ]
distinguished_name = req_distinguished_name
prompt = no

[ req_distinguished_name ]
CN = Test Key
EOF

        for key in PK KEK db; do
          if [[ ! -f "${key}.key" || ! -f "${key}.crt" ]]; then
            openssl req -new -x509 -newkey rsa:2048 -subj "/CN=Test ${key}/" -keyout "${key}.key" -out "${key}.crt" -days 365 -nodes -sha256 -config openssl.cnf
          fi
        done
        rm openssl.cnf
      fi
      
      # Ensure UKI is signed
      if command -v sbsign >/dev/null; then
          if ! sbverify --cert db.crt "$UKI_FILE" &>/dev/null; then
              echo "Signing UKI..."
              sbsign --key db.key --cert db.crt --output "$UKI_FILE.signed" "$UKI_FILE"
              mv "$UKI_FILE.signed" "$UKI_FILE"
          else
              echo "UKI already signed."
          fi
      fi
  else
      echo "Secure Boot disabled (default). Skipping key generation and UKI signing."
  fi

  # --- 3. RUN TEST CONTAINER ---
  echo "Starting Test Container (IPv6=$use_ipv6)..."
  docker rm -f "$TEST_CONTAINER_NAME" 2>/dev/null || true
  docker run -i --name "$TEST_CONTAINER_NAME" \
      --cap-add=NET_ADMIN \
      --device /dev/kvm \
      -v "$(pwd):/data" \
      -v "$OVMF_CODE_FILE:/ovmf_code.fd:ro" \
      -w /data \
      -e USE_IPV6="$use_ipv6" \
      -e INSTALL_DISK="$install_disk" \
      -e CLIENT_DISK_IMG="$CLIENT_DISK_IMG" \
      -e UKI_FILE="$UKI_FILE" \
      -e BRIDGE_IP="$BRIDGE_IP" \
      -e CLIENT_IP_START="$CLIENT_IP_START" \
      -e CLIENT_IP_END="$CLIENT_IP_END" \
      -e BRIDGE_IP6="$BRIDGE_IP6" \
      -e CLIENT_IP6_START="$CLIENT_IP6_START" \
      -e CLIENT_IP6_END="$CLIENT_IP6_END" \
      -e HTTP_PORT="$HTTP_PORT" \
      -e CLIENT_OVMF_VARS="$CLIENT_OVMF_VARS" \
      -e CLIENT_MAC="$CLIENT_MAC" \
      -e PK_CERT="$PK_CERT" \
      -e KEK_CERT="$KEK_CERT" \
      -e DB_CERT="$DB_CERT" \
      "$TEST_IMAGE_NAME" \
      qemu_runner.sh
}

main "$@"
