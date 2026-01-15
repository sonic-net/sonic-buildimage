#!/bin/bash
#
# Generates a self-extracting SONiE installer image.
#
# This script bundles the installer logic, platform config, and EFI payload
# into a self-extracting shell archive (sharch) to be executed on targets.

set -euo pipefail

# Global state for trap cleanup
readonly TMP_DIR="$(mktemp --directory)"

#######################################
# Cleanup temporary directories on exit
# Globals:
#   TMP_DIR
# Arguments:
#   None
#######################################
cleanup() {
  if [[ -n "${TMP_DIR:-}" && -d "${TMP_DIR}" ]]; then
    rm -rf "${TMP_DIR}"
  fi
}
trap cleanup EXIT

#######################################
# Validates input directories and files
#######################################
validate_inputs() {
  local installer_dir="$1"
  local image_version="$2"
  local xbootldr_part_size="$3"
  local nos_type="$4"

  if [[ ! -d "${installer_dir}" || ! -r "${installer_dir}/sharch_body.sh" ]]; then
    echo "Error: Invalid installer script directory: ${installer_dir}" >&2
    exit 1
  fi

  if [[ ! -r "${installer_dir}/install.sh" ]]; then
    echo "Error: Invalid arch installer directory: ${installer_dir}" >&2
    exit 1
  fi

  if [[ -z "${image_version}" ]]; then
    echo "Error: Invalid git revision version" >&2
    exit 1
  fi

  if [[ -z "${xbootldr_part_size}" ]]; then
    echo "Error: Invalid xbootldr_part_size" >&2
    exit 1
  fi

  case "${nos_type}" in
    OS|DIAG|sonie)
      ;;
    *)
      echo "Error: Unsupported nos type: ${nos_type}" >&2
      exit 1
      ;;
  esac
}

#######################################
# Main execution function
#######################################
main() {
  if [[ $# -lt 10 ]]; then
    echo "Error: Invalid arguments." >&2
    echo "Usage: $0 <arch> <machine> <platform> <installer_dir> <platform_conf> <out_file> <type> <version> <part_size> <payload> [acpi_conf]" >&2
    exit 1
  fi

  local arch="$1"
  local machine="$2"
  local platform="$3"
  local installer_dir="$4"
  local platform_conf="$5"
  local output_file="$6"
  local nos_type="$7"
  local image_version="$8"
  local xbootldr_part_size="$9"
  local onie_installer_payload="${10}"
  local acpi_conf="${11:-}"

  echo "DEBUG args: $*"

  validate_inputs "${installer_dir}" "${image_version}" "${xbootldr_part_size}" "${nos_type}"

  if [[ ! -r "${platform_conf}" ]]; then
    echo "Warning: Unable to read installer platform configuration file: ${platform_conf}" >&2
  fi

  echo -n "Building self-extracting install image ."

  local tmp_installdir="${TMP_DIR}/installer"
  mkdir -p "${tmp_installdir}"

  cp -r "${installer_dir}"/* "${tmp_installdir}/"
  cp onie-image.conf "${tmp_installdir}/"
  if [[ -r "onie-image-${arch}.conf" ]]; then
    cp "onie-image-${arch}.conf" "${tmp_installdir}/"
  fi

  local extra_cmdline="${EXTRA_CMDLINE_LINUX:-}"

  # Set sonic fips config for the installer script
  if [[ "${ENABLE_FIPS:-}" == "y" ]]; then
    extra_cmdline="${extra_cmdline} sonic_fips=1"
  fi

  # Ensure onie_platform is passed to the kernel command line
  extra_cmdline="${extra_cmdline} onie_platform=${machine}"

  # Escape special chars (\ / &) for sed
  extra_cmdline=$(echo "${extra_cmdline}" | sed -e 's/[\/&]/\\\&/g')

  local output_raw_image
  output_raw_image=$(grep OUTPUT_RAW_IMAGE onie-image.conf | cut -f2 -d"=")
  if [[ -z "${TARGET_MACHINE:-}" ]]; then
    output_raw_image=$(echo "${output_raw_image}" | sed -e "s/\$TARGET_MACHINE/${machine}/g")
  fi
  # Re-evaluate variable to expand any inner shell vars inside the string
  output_raw_image=$(eval echo "${output_raw_image}")

  # Tailor the installer for OS mode or DIAG mode
  sed -i -e "s/%%NOS_TYPE%%/${nos_type}/g" \
         -e "s/%%IMAGE_VERSION%%/${image_version}/g" \
         -e "s/%%XBOOTLDR_PART_SIZE%%/${xbootldr_part_size}/" \
         -e "s/%%EXTRA_CMDLINE_LINUX%%/${extra_cmdline}/" \
         -e "s@%%OUTPUT_RAW_IMAGE%%@${output_raw_image}@" \
    "${tmp_installdir}/install.sh"

  echo -n "."
  cp -r "${onie_installer_payload}" "${tmp_installdir}/"
  echo -n "."

  if [[ -r "${platform_conf}" ]]; then
    cp "${platform_conf}" "${tmp_installdir}/"
  fi

  if [[ -n "${acpi_conf}" ]]; then
    cp "${acpi_conf}" "${tmp_installdir}/"
  fi

  echo "machine=${machine}" > "${tmp_installdir}/machine.conf"
  echo "platform=${platform}" >> "${tmp_installdir}/machine.conf"
  echo -n "."

  # Reduce the size of the usr directory
  local payload_basename
  payload_basename=$(basename "${onie_installer_payload}")
  echo "Reducing size of ${payload_basename}/usr"
  local usr_dir="${tmp_installdir}/${payload_basename}/usr"
  if [[ -d "${usr_dir}" ]]; then
    find "${usr_dir}" -name "*.pyc" -type f -delete
    find "${usr_dir}" -name "*.o" -type f -delete
    rm -rf "${usr_dir}/share/doc" "${usr_dir}/share/man" "${usr_dir}/share/locale"
    find "${usr_dir}/lib" -type f -exec strip --strip-unneeded {} + 2>/dev/null || true
    find "${usr_dir}/bin" -type f -exec strip --strip-unneeded {} + 2>/dev/null || true
    find "${usr_dir}/sbin" -type f -exec strip --strip-unneeded {} + 2>/dev/null || true
  fi

  echo -n "."

  local sharch="${TMP_DIR}/sharch.tar"
  if ! tar -C "${TMP_DIR}" -cf "${sharch}" installer; then
    echo "Error: Problems creating ${sharch} archive" >&2
    exit 1
  fi
  echo -n "."

  local sha1
  sha1=$(sha1sum "${sharch}" | awk '{print $1}')
  echo -n "."
  if ! cp "${installer_dir}/sharch_body.sh" "${output_file}"; then
    echo "Error: Problems copying sharch_body.sh" >&2
    exit 1
  fi

  # Replace variables in the sharch template
  sed -i -e "s/%%IMAGE_SHA1%%/${sha1}/" "${output_file}"
  echo -n "."
  local tar_size
  tar_size="$(wc -c < "${sharch}")"
  sed -i -e "s|%%PAYLOAD_IMAGE_SIZE%%|${tar_size}|" "${output_file}"
  cat "${sharch}" >> "${output_file}"

  echo "secure upgrade flags: SECURE_UPGRADE_MODE = ${SECURE_UPGRADE_MODE:-}, \
SECURE_UPGRADE_DEV_SIGNING_KEY = ${SECURE_UPGRADE_DEV_SIGNING_KEY:-}, SECURE_UPGRADE_SIGNING_CERT = ${SECURE_UPGRADE_SIGNING_CERT:-}"

  if [[ "${SECURE_UPGRADE_MODE:-}" == "dev" || "${SECURE_UPGRADE_MODE:-}" == "prod" ]]; then
    local cms_sig="${TMP_DIR}/signature.sig"
    local scripts_dir
    scripts_dir="$(dirname "$0")/scripts"
    echo "$0 ${SECURE_UPGRADE_MODE} signing - creating CMS signature for ${output_file}. Output file ${cms_sig}"

    if [[ "${SECURE_UPGRADE_MODE}" == "dev" ]]; then
      echo "$0 dev keyfile location: ${SECURE_UPGRADE_DEV_SIGNING_KEY:-}."
      if [[ ! -f "${scripts_dir}/sign_image_dev.sh" ]]; then
        echo "dev sign script ${scripts_dir}/sign_image_dev.sh not found" >&2
        rm -rf "${output_file}"
        exit 1
      fi
      if ! "${scripts_dir}/sign_image_dev.sh" "${SECURE_UPGRADE_SIGNING_CERT:-}" "${SECURE_UPGRADE_DEV_SIGNING_KEY:-}" "${output_file}" "${cms_sig}"; then
        echo "CMS sign error $?" >&2
        rm -rf "${cms_sig}" "${output_file}"
        exit 1
      fi
    else
      if [[ ! -f "${scripts_dir}/sign_image_${machine}.sh" ]]; then
        echo "prod sign script ${scripts_dir}/sign_image_${machine}.sh not found" >&2
        rm -rf "${output_file}"
        exit 1
      fi
      # shellcheck disable=SC2086
      if ! "${scripts_dir}/sign_image_${machine}.sh" -c "${SECURE_UPGRADE_SIGNING_CERT:-}" -f "${SECURE_UPGRADE_PROD_TOOL_CONFIG:-}" \
                                                       -i "${output_file}" -o "${cms_sig}" ${SECURE_UPGRADE_PROD_TOOL_ARGS:-}; then
        echo "CMS sign error $?" >&2
        rm -rf "${cms_sig}" "${output_file}"
        exit 1
      fi
    fi

    if [[ ! -f "${cms_sig}" ]]; then
       echo "Error: CMS signature not created - exiting without signing" >&2
       exit 1
    fi
    # Append signature to binary
    cat "${cms_sig}" >> "${output_file}"
    rm -rf "${cms_sig}"
  elif [[ "${SECURE_UPGRADE_MODE:-}" != "no_sign" ]]; then
    echo "SECURE_UPGRADE_MODE not defined or defined as ${SECURE_UPGRADE_MODE:-} - build without signing"
  fi

  echo " Done."
  echo "Success: SONIE install image is ready in ${output_file}:"
  ls -l "${output_file}"
}

main "$@"
