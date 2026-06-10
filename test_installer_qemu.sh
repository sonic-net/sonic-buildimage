#!/bin/bash
#
# test_installer_qemu.sh
#
set -e
set -o pipefail
# This script builds a unified Docker image and runs a QEMU-based test for the
# SONiC/SONIE installer. It prepares necessary artifacts (UKI, disk images, config bundles)
# and launches a container that emulates the PXE boot process.
#
# Usage:
#   ./test_installer_qemu.sh [-6|-4] [--install-disk] [--log-level <value>]
#
# Options:
#   -6              Use IPv6 (default)
#   -4              Use IPv4
#   --install-disk  Enable installation to disk
#   --secure-boot   Enable secure boot
#   --preserve-disk Preserve the disk image after the test
#   --log-level     Log verbosity: standard (default), verbose
#   --bootz         Enable bootz
#
# Environment Variables (Bazel):
#   RUNFILES_DIR    Path to Bazel runfiles directory
#   RUNFILES_MANIFEST_FILE Path to Bazel runfiles manifest

# --- BAZEL RUNFILES ---
# Wrapper for runfiles (Bazel dependency handling)
if [[ -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
  source "$(grep -m1 "^bazel_tools/tools/bash/runfiles/runfiles.bash " "$RUNFILES_MANIFEST_FILE" | cut -d ' ' -f 2-)"
elif [[ -f "${RUNFILES_DIR:-/dev/null}/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
  source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"
elif [[ -f "${0}.runfiles/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
  source "${0}.runfiles/bazel_tools/tools/bash/runfiles/runfiles.bash"
fi

#######################################
# Resolve a runfile path or use a local fallback.
# Arguments:
#   rpath: Path to look up in runfiles (e.g. "repo/file").
#   lpath: Fallback local path if runfiles lookups fail.
# Outputs:
#   Writes the resolved path to stdout.
#######################################
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

# Custom search paths for OVMF firmware
ovmf_search_paths=()
if [[ -n "${OVMF_PATH:-}" ]]; then
    ovmf_search_paths+=("${OVMF_PATH}")
fi

# Determine platform (default to 'vs' if .platform file is missing)
readonly PLATFORM="$([ -f .platform ] && cat .platform || echo vs)"
readonly ARCH="$([ -f .arch ] && cat .arch || echo amd64)"
if [[ "$ARCH" == "amd64" ]]; then
  readonly SONIE="sonie-${PLATFORM}"
  readonly SONIC="sonic-${PLATFORM}"
else
  readonly SONIE="sonie-${PLATFORM}-${ARCH}"
  readonly SONIC="sonic-${PLATFORM}-${ARCH}"
fi
# Resolve artifacts using Bazel runfiles or local fallback
readonly UKI_FILE="${UKI_FILE:-$(rlocation_or_local "${SONIE}.efi" "target/${SONIE}.efi")}"
readonly SONIC_BIN="$(rlocation_or_local "${SONIC}.bin" "target/${SONIC}.bin")"
readonly SONIE_BIN="$(rlocation_or_local "${SONIE}.bin" "target/${SONIE}.bin")"
readonly SHARCH_BODY="$(rlocation_or_local "sonie-installer/sharch_body.sh" "sonie-installer/sharch_body.sh")"
readonly WRAPPER_SCRIPT="$(rlocation_or_local "wrapper_install.sh" "wrapper_install.sh")"

echo "DEBUG: WRAPPER_SCRIPT resolved to: $WRAPPER_SCRIPT"
readonly FILTER_SCRIPT="$(rlocation_or_local "filter_ovmf.py" "filter_ovmf.py")"
# Default to local go install path if not in runfiles
readonly BOOTZ_SERVER_BIN="$(rlocation_or_local "sonie-test-trixie/bootz_server_/bootz_server" "${HOME}/go/bin/emulator")"

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
readonly IMAGESET_DIR="imageset"
readonly IMAGESET_FILE="imageset.tgz"

readonly CLIENT_OVMF_VARS="client_ovmf_vars.fd"
readonly CLIENT_MAC="52:54:00:65:43:21"

# Docker Context Directory
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
  docker run --rm --mount type=bind,source="$(pwd)",target=/data \
    debian:trixie sh -c '
    for log in http_server dnsmasq bootz_server ovmf_debug qemu_runner; do
      path="/data/sonie-test-logs/${log}.log"
      if [ -f "$path" ]; then
        echo "--- ${log}.log ---"
        cat "$path"
        echo "-----------------------"
      fi
    done
  ' 2>/dev/null || true
  docker rm -f "$TEST_CONTAINER_NAME" 2>/dev/null || true

  if [[ "$PRESERVE_DISK" -ne 1 ]]; then
      rm -f "$CLIENT_DISK_IMG"
  else
      echo "Preserving $CLIENT_DISK_IMG"
  fi
  rm -f "$CLIENT_OVMF_VARS" "Dockerfile.test"
}

#######################################
# Extract UKI from the SONIE installer bin payload
# Globals:
#   SONIE_BIN
#   UKI_FILE
#######################################
extract_uki_from_payload() {
  if [[ -s "$UKI_FILE" ]]; then
      return 0
  fi

  echo "Warning: UKI file missing. Attempting to extract from $SONIE_BIN..."
  if [[ -f "$SONIE_BIN" ]]; then
       local tmp_zip=$(mktemp --suffix=.zip)
       sed -e '1,/^exit_marker$/d' "$SONIE_BIN" | tar --wildcards -x -O 'installer/tmp.*.zip' > "$tmp_zip" 2>/dev/null || true
       if [[ -s "$tmp_zip" ]]; then
         local uki_basename=$(basename "$UKI_FILE")
         unzip -p "$tmp_zip" "$uki_basename" > "$UKI_FILE" 2>/dev/null || true
       fi
       rm -f "$tmp_zip"
  fi
}

#######################################
# Extract GRUB from the SONIE installer bin payload
# Globals:
#   SONIE_BIN
#######################################
extract_grub_from_payload() {
  echo "Extracting grubx64.efi from $SONIE_BIN..."
  if [[ -f "$SONIE_BIN" ]]; then
    # The payload is appended to the binary after 'exit_marker' as a tar archive
    # containing a zip file with the UKI and grubx64.efi.
    local tmp_zip=$(mktemp --suffix=.zip)
    sed -e '1,/^exit_marker$/d' "$SONIE_BIN" | tar --wildcards -x -O 'installer/tmp.*.zip' > "$tmp_zip" 2>/dev/null || true
    if [[ -s "$tmp_zip" ]]; then
      unzip -p "$tmp_zip" grubx64.efi > grubx64.efi 2>/dev/null || true
    fi
    rm -f "$tmp_zip"

    if [[ ! -s "grubx64.efi" ]]; then
      echo "Warning: Failed to extract grubx64.efi from $SONIE_BIN"
      rm -f grubx64.efi
    else
      echo "Successfully extracted grubx64.efi"
    fi
  else
    echo "Warning: $SONIE_BIN not found, cannot extract grubx64.efi"
  fi
}

#######################################
# Check if required files exist.
# Globals:
#   UKI_FILE
#######################################
check_requirements() {
  extract_uki_from_payload

  if [[ ! -f "$UKI_FILE" || ! -s "$UKI_FILE" ]]; then
    err "Error: UKI file not found or could not be extracted at $UKI_FILE"
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

  # Default search paths

  ovmf_search_paths+=(
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

  if [[ -f "sonie-test-trixie/Dockerfile" ]]; then
      # Local mode, or structure preserved
      docker build --network host -t "$TEST_IMAGE_NAME" -f sonie-test-trixie/Dockerfile .
  elif [[ -d "$DOCKER_CONTEXT_DIR" ]]; then
      local context_root
      # Try to find the repo root in runfiles
      if [[ "$DOCKER_CONTEXT_DIR" == *"sonie-test-trixie" ]]; then
          context_root="$(dirname "$DOCKER_CONTEXT_DIR")"
          echo "Guessed context root: $context_root"
          if [[ -f "$context_root/sonie-test-trixie/Dockerfile" ]]; then
             docker build --network host -t "$TEST_IMAGE_NAME" -f "$context_root/sonie-test-trixie/Dockerfile" "$context_root"
             return
          fi
      fi

      # Fallback: Create a temporary context that matches what Dockerfile expects
      echo "Adapting Docker context via structured copy..."
      local context_tmp="docker_context_tmp"
      rm -rf "$context_tmp"
      mkdir -p "$context_tmp/sonie-test-trixie"

      # Copy all files from the found directory to the structured temp dir
      cp -r "$DOCKER_CONTEXT_DIR"/* "$context_tmp/sonie-test-trixie/"

      docker build --network host -t "$TEST_IMAGE_NAME" -f "$context_tmp/sonie-test-trixie/Dockerfile" "$context_tmp"
      rm -rf "$context_tmp"
  else
      err "Could not find Dockerfile context"
      exit 1
  fi
}

#######################################
# Prepare the imageset archive.
# Globals:
#   IMAGESET_FILE
#   IMAGESET_DIR
#   SONIC_BIN
#   SONIE_BIN
#   PLATFORM
#######################################
prepare_imageset() {
  local needs_rebuild=0

  if [[ ! -f "$IMAGESET_FILE" ]]; then
    needs_rebuild=1
  else
    for bin in "${SONIC_BIN}" "${SONIE_BIN}"; do
      if [[ -f "$bin" && "$bin" -nt "$IMAGESET_FILE" ]]; then
        needs_rebuild=1
        break
      fi
    done
  fi

  if [[ "$needs_rebuild" -eq 0 ]]; then
    if [[ ! -f "$IMAGESET_FILE" || ! -x "$IMAGESET_FILE" ]]; then
        return
    fi
    echo "Imageset ($IMAGESET_FILE) is up to date."
    return
  fi

  echo "Building imageset..."
  rm -rf "$IMAGESET_DIR" "$IMAGESET_FILE"
  mkdir -p "$IMAGESET_DIR/installer"

  local binaries=("${SONIC_BIN}" "${SONIE_BIN}")
  local fallback_names=("${SONIC}.bin" "${SONIE}.bin")

  if [[ ! -f "$SHARCH_BODY" ]]; then
    err "Error: sharch_body.sh not found at $SHARCH_BODY"
    exit 1
  fi
  if [[ ! -f "$WRAPPER_SCRIPT" ]]; then
    err "Error: wrapper_install.sh not found at $WRAPPER_SCRIPT"
    exit 1
  fi

  for i in "${!binaries[@]}"; do
    local bin="${binaries[$i]}"
    local fallback="${fallback_names[$i]}"

    if [[ -f "$bin" ]]; then
      cp "$bin" "$IMAGESET_DIR/installer/$fallback"
      # Inject image_version if missing, to satisfy sonic-installer validation
      if ! grep -q "^image_version=" "$IMAGESET_DIR/installer/$fallback"; then
        echo "Injecting image_version into $fallback..."
        sed -i '2i image_version="deadbeef"' "$IMAGESET_DIR/installer/$fallback"
      fi
    else
      echo "Warning: $bin not found, creating dummy $fallback"
      touch "$IMAGESET_DIR/installer/$fallback"
      # Dummy must be a valid script with image_version
      {
        echo "#!/bin/bash"
        echo "image_version=\"deadbeef\""
        echo "echo 'Dummy Installer Running...'"
        echo "exit 0"
      } > "$IMAGESET_DIR/installer/$fallback"
      chmod +x "$IMAGESET_DIR/installer/$fallback"
    fi
  done

  # Add wrapper as install.sh
  cp "$WRAPPER_SCRIPT" "$IMAGESET_DIR/installer/install.sh"
  chmod +x "$IMAGESET_DIR/installer/install.sh"

  # Create payloads
  echo "Creating self-extracting installer..."
  local payload_tar="payload.tar"
  tar -cf "$payload_tar" -C "$IMAGESET_DIR" .

  # Construct final script
  cp "$SHARCH_BODY" "$IMAGESET_FILE"

  # Calculate SHA1 and size/replacements
  local sha1
  sha1=$(sha1sum "$payload_tar" | awk '{print $1}')
  local tar_size
  tar_size=$(stat -c%s "$payload_tar")

  sed -i -e "s/%%IMAGE_SHA1%%/$sha1/" "$IMAGESET_FILE"
  sed -i -e "s|%%PAYLOAD_IMAGE_SIZE%%|${tar_size}|" "$IMAGESET_FILE"

  cat "$payload_tar" >> "$IMAGESET_FILE"
  chmod +x "$IMAGESET_FILE"

  rm -f "$payload_tar"
  rm -rf "$IMAGESET_DIR"
}

#######################################
# Create dummy configuration and key bundles.
# Globals:
#   CONFIG_BUNDLE
#   KEY_BUNDLE
#######################################
create_bundles() {
  CONFIG_BUNDLE="config_bundle.tgz"
  KEY_BUNDLE="key_bundle.tar"
  echo "Creating robust dummy Config Bundle ($CONFIG_BUNDLE) and Key Bundle ($KEY_BUNDLE)..."

  # 1. Key Bundle
  rm -rf key_bundle_tmp
  mkdir -p key_bundle_tmp
  echo "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIDummyKeyForTesting test@example.com" > key_bundle_tmp/ssh_authorized_keys
  touch key_bundle_tmp/ssh_authorized_keys.rawsig.dummy
  touch key_bundle_tmp/signer_public_key.dummy
  tar -cf "$KEY_BUNDLE" -C key_bundle_tmp .
  rm -rf key_bundle_tmp

  # 2. Config Bundle
  rm -rf config_bundle_tmp
  mkdir -p config_bundle_tmp
  cat > config_bundle_tmp/config_bundle <<EOF
ConfigBundle_Key_Download_URL="/$KEY_BUNDLE"
ConfigBundle_NTP_VIPS=""
ConfigBundle_Syslog_VIPS=""
ConfigBundle_Enable_Syslog_Encryption="false"
EOF
  touch config_bundle_tmp/config_bundle.rawsig
  tar -czf "$CONFIG_BUNDLE" -C config_bundle_tmp .
  rm -rf config_bundle_tmp
}

#######################################
# ALL CAPS variables are global/exported usually, but here we use them for consistency
PRESERVE_DISK=0

#######################################
# Prepare Secure Boot Keys and sign artifacts if needed.
# Arguments:
#   secure_boot: 1 to enable, 0 to disable
# Globals:
#   PK_CERT, KEK_CERT, DB_CERT (exported)
#   UKI_FILE, PLATFORM
#   GRUB_FILE (exported)
#######################################
prepare_secure_boot_keys() {
  local secure_boot="$1"

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

        # Bootz Key and Certificate Generation
        if [[ ! -f "bootz.key" || ! -f "bootz.crt" ]]; then
            echo "Generating Bootz Attestation Key..."
            cat > bootz.cnf <<EOF
[ req ]
distinguished_name = bootz_dn
req_extensions = bootz_ext
prompt = no

[ bootz_dn ]
serialNumber = 123456789
O = SONiC
OU = Boot
CN = Bootz Test Key

[ bootz_ext ]
keyUsage = critical, digitalSignature, nonRepudiation
basicConstraints = critical, CA:FALSE
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always,issuer
EOF
            # Generate 3072-bit RSA key and certificate
            openssl req -new -x509 -newkey rsa:3072 \
                -keyout bootz.key -out bootz.crt -days 28000 \
                -nodes -sha256 -config bootz.cnf -extensions bootz_ext
            rm bootz.cnf
        fi

        rm openssl.cnf
      fi

      # Ensure UKI is signed
      if command -v sbsign >/dev/null; then
          if ! sbverify --cert db.crt "$UKI_FILE" &>/dev/null; then
              echo "Signing UKI..."
              sbsign --key db.key --cert db.crt --output "$UKI_FILE.signed" "$UKI_FILE"
              mv -f "$UKI_FILE.signed" "$UKI_FILE"
          else
              echo "UKI already signed."
          fi
      fi

      # Also sign Grub if it exists, as qemu_runner.sh uses it for BOOTX64.EFI
      local grub_path="grubx64.efi"
      if [[ ! -f "$grub_path" ]]; then
          grub_path="fsroot-${PLATFORM}-${ARCH}-recovery/usr/lib/grub/x86_64-efi/monolithic/grubx64.efi"
      fi
      if [[ -f "$grub_path" ]]; then
          if command -v sbsign >/dev/null; then
               local signed_grub="grubx64.efi.signed"
               if sbverify --cert db.crt "$signed_grub" &>/dev/null; then
                   echo "Grub already signed."
               else
                   echo "Signing Grub from $grub_path to local $signed_grub..."
                   sbsign --key db.key --cert db.crt --output "$signed_grub" "$grub_path"

                   if ! sbverify --cert db.crt "$signed_grub" &>/dev/null; then
                       echo "Error: Grub signature verification failed!"
                       exit 1
                   fi
               fi
               export GRUB_FILE="$signed_grub"
          fi
      fi
  else
      echo "Secure Boot disabled (default). Skipping key generation and UKI signing."
  fi
}

#######################################
# Main entry point.
# Arguments:
#   Command line arguments.
#######################################
main() {
  local secure_boot=0
  local log_level="standard"
  # preserve_disk is now global PRESERVE_DISK
  local ENABLE_RECOVERY_MENU=0

  while [[ "$#" -gt 0 ]]; do
    case $1 in
      -6) use_ipv6=1 ;;
      -4) use_ipv6=0 ;;
      --install-disk) install_disk=1 ;;
      --ovmf-path) OVMF_PATH="$2"; shift ;;
      --secure-boot) secure_boot=1 ;;
      --preserve-disk) PRESERVE_DISK=1 ;;
      --log-level) log_level="$2"; shift ;;
      --bootz) bootz_enabled=1 ;;
      --direct-boot) direct_boot=1 ;;
      --recovery)
          ENABLE_RECOVERY_MENU=1
          ;;
      --cmdline) KERNEL_CMDLINE="$2"; shift ;;
      *) err "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
  done

  if [[ "$ENABLE_RECOVERY_MENU" -eq 1 ]]; then
      KERNEL_CMDLINE="${KERNEL_CMDLINE:-} boot_reason=recovery"
  fi

  check_requirements
  resolve_ovmf_paths

  trap cleanup EXIT

  build_docker_image

  if [[ "${bootz_enabled:-0}" -eq 1 ]]; then
      echo "Checking for Bootz Server..."
      if [[ ! -f "${BOOTZ_SERVER_BIN}" ]]; then
          echo "Bootz server binary not found at ${BOOTZ_SERVER_BIN}. Attempting to install..."
          if ! command -v go >/dev/null; then
              err "Error: 'go' not found. Cannot install Bootz server."
              exit 1
          fi
          # Install specific version to ensure consistency (matching MODULE.bazel roughly)
          go install github.com/openconfig/bootz/server/emulator@v0.3.1

          if [[ ! -f "${BOOTZ_SERVER_BIN}" ]]; then
               err "Error: Failed to install Bootz server to ${BOOTZ_SERVER_BIN}"
               exit 1
          fi
      fi
      echo "Using Bootz Server at ${BOOTZ_SERVER_BIN}"
  fi


  cp "$OVMF_VARS_FILE" "$CLIENT_OVMF_VARS"

  prepare_imageset
  create_bundles

  extract_grub_from_payload

  prepare_secure_boot_keys "$secure_boot"

  local test_grub_file="grubx64.efi"
  local grub_candidates=(
    "grubx64.efi.signed"
    "grubx64.efi"
    "fsroot-${PLATFORM}-${ARCH}-recovery/usr/lib/grub/x86_64-efi/monolithic/grubx64.efi.signed"
    "fsroot-${PLATFORM}-${ARCH}-recovery/usr/lib/grub/x86_64-efi/monolithic/grubx64.efi"
  )

  for f in "${grub_candidates[@]}"; do
    if [[ -s "$f" ]]; then
      test_grub_file="$f"
      break
    fi
  done

  # --- 3. RUN TEST CONTAINER ---
  echo "Starting Test Container (IPv6=$use_ipv6, Log=$log_level)..."
  docker rm -f "$TEST_CONTAINER_NAME" 2>/dev/null || true

  local filter_cmd="cat"
  if [[ "$log_level" != "verbose" ]]; then
     # Filter logs using python script if available
     if [[ -f "$FILTER_SCRIPT" ]]; then
        filter_cmd="python3 $FILTER_SCRIPT"
     fi
  fi

  mkdir -p sonie-test-logs
  docker run -i --rm --name "$TEST_CONTAINER_NAME" \
      --privileged \
      --memory=32g \
      --device /dev/kvm \
      -w /data \
      --mount type=bind,source="$OVMF_CODE_FILE",target=/ovmf_code.fd,readonly \
      --mount type=bind,source="$(readlink -f "$UKI_FILE")",target=/data/sonie.efi,readonly \
      --mount type=bind,source="$(readlink -f "$SONIC_BIN")",target=/data/sonic-vs.bin,readonly \
      --mount type=bind,source="$(pwd)",target=/data \
      -e USE_IPV6="$use_ipv6" \
      -e SECURE_BOOT="$secure_boot" \
      -e INSTALL_DISK="$install_disk" \
      -e PRESERVE_DISK="$PRESERVE_DISK" \
      -e CLIENT_DISK_IMG="$CLIENT_DISK_IMG" \
      -e DISK_IMG_SIZE="128G" \
      -e UKI_FILE="/data/sonie.efi" \
      -e RECOVERY_ADDON="/data/target/${SONIE}-recovery.addon.efi" \
      -e BRIDGE_IP="$BRIDGE_IP" \
      -e CLIENT_IP_START="$CLIENT_IP_START" \
      -e CLIENT_IP_END="$CLIENT_IP_END" \
      -e BRIDGE_IP6="$BRIDGE_IP6" \
      -e CLIENT_IP6_START="$CLIENT_IP6_START" \
      -e CLIENT_IP6_END="$CLIENT_IP6_END" \
      -e HTTP_PORT="$HTTP_PORT" \
      -e IMAGESET_FILE="$IMAGESET_FILE" \
      -e CONFIG_BUNDLE="$CONFIG_BUNDLE" \
      -e CLIENT_OVMF_VARS="/data/${CLIENT_OVMF_VARS}" \
      -e CLIENT_MAC="$CLIENT_MAC" \
      -e PK_CERT="/data/PK.crt" \
      -e KEK_CERT="/data/KEK.crt" \
      -e DB_CERT="/data/db.crt" \
      -e BOOTZ_KEY="/data/bootz.key" \
      -e BOOTZ_CERT="/data/bootz.crt" \
      -e BOOTZ_ENABLED="${bootz_enabled:-0}" \
      -e DIRECT_BOOT="${direct_boot:-0}" \
      $([[ "${bootz_enabled:-0}" -eq 1 ]] && echo "--mount type=bind,source=$(readlink -f "${BOOTZ_SERVER_BIN}"),target=/usr/local/bin/bootz_server,readonly") \
      --mount type=bind,source="$(readlink -f "sonie-test-trixie/qemu_runner.sh")",target=/usr/local/bin/qemu_runner.sh,readonly \
      --mount type=bind,source="$(readlink -f "sonie-test-trixie/http_server_progress.py")",target=/usr/local/bin/http_server_progress.py,readonly \
      --mount type=bind,source="$(readlink -f "sonie-test-trixie/templates")",target=/etc/qemu-runner/templates,readonly \
      -e GRUB_FILE="${test_grub_file}" \
      -e QEMU_MEMORY="24G" \
      -e DIRECT_BOOT \
      -e KERNEL_CMDLINE="${KERNEL_CMDLINE:-}" \
      -e ENABLE_RECOVERY_MENU="${ENABLE_RECOVERY_MENU}" \
      "$TEST_IMAGE_NAME" \
      qemu_runner.sh 2>&1 | tee ./sonie-test-logs/qemu_runner.log | $filter_cmd
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
fi
