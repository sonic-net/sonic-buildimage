#!/bin/bash
# This script is intended to be called during platform module building in order
# to sign any kernel modules with the necessary secureboot keys.  Platform
# modules are packed into .deb files so aren't caught by the normal signing
# procedure.  This script simply calls into the pre-existing scripts used for
# signing.
#
# The exception is when "${SECURE_UPGRADE_MODE}" is not specified, it will look
# for dev-keys/non-production.crt and dev-keys/non-production.key and call the
# kernel's sign-file directly.  Please note this logic is for Nexthop-internal
# development purposes only and these keys do not normally exist and this path
# is not expected to be used outside of Nexthop.
#
# When the environment variable SECURE_UPGRADE_MODE is "prod" or "dev", this
# depends on these environment variables:
#   From rules/config:
#     SECURE_UPGRADE_SIGNING_CERT
#     dev-only:
#       SECURE_UPGRADE_DEV_SIGNING_KEY
#     prod-only:
#       SECURE_UPGRADE_PROD_SIGNING_TOOL
#       SECURE_UPGRADE_PROD_TOOL_ARGS
#   Passed in from configuration/build:
#     CONFIGURED_ARCH
#     KVERSION
#   From slave.mk
#     PROJECT_ROOT - base path for source
#     BUILD_WORKDIR - base build path (typically same as PROJECT_ROOT)
#
# This script takes a single command line argument, the path to recursively
# search for kernel modules.

die() {
  echo "$*"
  exit 1
}

if [ "${SECURE_UPGRADE_MODE}" = "dev" ]; then
  ${PROJECT_ROOT}/scripts/signing_kernel_modules.sh -l ${KVERSION} -c ${SECURE_UPGRADE_SIGNING_CERT} -p ${SECURE_UPGRADE_DEV_SIGNING_KEY} -k ${1} || die "Kernel module signing failed (dev)"
elif [ "${SECURE_UPGRADE_MODE}" = "prod" ]; then
  ${PROJECT_ROOT}/${SECURE_UPGRADE_PROD_SIGNING_TOOL} ${SECURE_UPGRADE_PROD_TOOL_ARGS} -a ${CONFIGURED_ARCH} -l ${KVERSION} -r ${1} || die "Kernel module signing failed (prod)"
elif [ "${SECURE_UPGRADE_MODE}" = "no_sign" ]; then
  # Do nothing
  echo "No kernel module signing requested"
else
  # Signing for Nexthop kernel module development.
  #
  # This is only used during development of the kernel modules themselves for
  # quick iteration as the kernel may be built with Secureboot enabled to
  # ensure Linux "lockdown" mode is properly honored.
  #
  # Sign using a local key and certificate trusted by the development
  # environment.  This method is never used for full SONiC image builds,
  # regardless if this is for Development or Production.
  #
  # Requires:
  #   * dev-keys/non-production.key and dev-keys/non-production.crt
  #   * /usr/lib/linux-kbuild-${KVERSION}/scripts/sign-file
  FILESYSTEM_ROOT=fsroot

  echo " ** ATTEMPTING DEVELOPMENT-ONLY KERNEL MODULE SIGNING **"

  script_path=`dirname $0`
  if [ "${script_path}" != "" ] ; then
    script_path="${script_path}/"
  fi

  keypath="${script_path}./dev-keys/non-production.key"
  certpath="${script_path}./dev-keys/non-production.crt"
  if [ ! -f "${keypath}" -o ! -f "${certpath}" ] ; then
    die "Could not find ${keypath} or ${certpath}"
  fi

  KVER="$(echo ${KVERSION} | cut -d '.' -f 1)"."$(echo ${KVERSION} | cut -d '.' -f 2)"
  sign_file="${BUILD_WORKDIR}/${FILESYSTEM_ROOT}/usr/lib/linux-kbuild-${KVER}/scripts/sign-file"
  if [ ! -x "${sign_file}" ] ; then
    # Fallback for building on-DUT
    sign_file="/usr/lib/linux-kbuild-${KVER}/scripts/sign-file"
    if [ ! -x "${sign_file}" ] ; then
      die "Could not find ${sign_file}"
    fi
  fi

  # Sign the modules
  modules_list=$(find ${1} -name "*.ko")
  # Do sign for each found module
  cnt=0
  for mod in $modules_list ; do
      echo "signing module ${mod} ..."
      ${sign_file} sha512 ${keypath} ${certpath} ${mod} || die "failed to sign ${mod}"
      cnt=$((cnt+1))
  done

  if [ "$cnt" = "0" ] ; then
    die "No kernel modules found"
  fi

  echo "${cnt} modules signed"
fi
