#!/bin/bash
#
# Copyright (c) 2018-2023 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

FFB_SUCCESS=0
FFB_FAILURE=1

# Check if ISSU is enabled on this device
check_issu_enabled()
{
    CHECK_RESULT="${FFB_FAILURE}"
    ISSU_CHECK_CMD="show platform mlnx issu"

    # Check whether show ISSU status outputs ENABLED
    if [[ `$ISSU_CHECK_CMD` =~ "enabled" ]]; then
        # ISSU enabled, return success
        CHECK_RESULT="${FFB_SUCCESS}"
    fi

    return "${CHECK_RESULT}"
}

# Check if ISSU upgrade from current SDK to next image SDK is supported
check_sdk_upgrade()
{
    CHECK_RESULT="${FFB_FAILURE}"

    NEXT_SONIC_IMAGE="$(sonic-installer list | grep "Next: " | cut -f2 -d' ')"
    CURRENT_SONIC_IMAGE="$(sonic-installer list | grep "Current: " | cut -f2 -d' ')"

    FS_PATH="/host/image-${NEXT_SONIC_IMAGE#SONiC-OS-}/fs.squashfs"
    FS_MOUNTPOINT="/tmp/image-${NEXT_SONIC_IMAGE#SONiC-OS-}-fs"

    if [[ "${CURRENT_SONIC_IMAGE}" == "${NEXT_SONIC_IMAGE}" ]]; then
        return "${FFB_SUCCESS}"
    fi

    ISSU_VERSION_FILE_PATH="/etc/mlnx/issu-version"
    CURRENT_ISSU_VERSION="$(cat ${ISSU_VERSION_FILE_PATH})"
    NEXT_ISSU_VERSION="Unknown"

    # /host/image-<version>/platform/fw/asic/issu-version is now the new location for ISSU version.
    NEXT_IMAGE_ISSU_VERSION_FILE_PATH="/host/image-${NEXT_SONIC_IMAGE#SONiC-OS-}/platform/fw/asic/issu-version"

    if [ -f "${NEXT_IMAGE_ISSU_VERSION_FILE_PATH}" ]; then
        NEXT_ISSU_VERSION="$(cat ${NEXT_IMAGE_ISSU_VERSION_FILE_PATH})"
    else
        while :; do
            mkdir -p "${FS_MOUNTPOINT}"
            mount -t squashfs "${FS_PATH}" "${FS_MOUNTPOINT}" || {
                >&2 echo "Failed to mount next SONiC image"
                break
            }

            [ -f "${ISSU_VERSION_FILE_PATH}" ] || {
                >&2 echo "No ISSU version file found ${ISSU_VERSION_FILE_PATH}"
                break
            }

            [ -f "${FS_MOUNTPOINT}/${ISSU_VERSION_FILE_PATH}" ] || {
                >&2 echo "No ISSU version file found ${ISSU_VERSION_FILE_PATH} in ${NEXT_SONIC_IMAGE}"
                break
            }
            NEXT_ISSU_VERSION="$(cat ${FS_MOUNTPOINT}/${ISSU_VERSION_FILE_PATH})"
            break
        done

        umount -rf "${FS_MOUNTPOINT}" 2> /dev/null || true
        rm -rf "${FS_MOUNTPOINT}" 2> /dev/null || true
    fi

    if [[ "${CURRENT_ISSU_VERSION}" == "${NEXT_ISSU_VERSION}" ]]; then
        CHECK_RESULT="${FFB_SUCCESS}"
    else
        >&2 echo "Current and next ISSU version do not match:"
        >&2 echo "Current ISSU version: ${CURRENT_ISSU_VERSION}"
        >&2 echo "Next ISSU version: ${NEXT_ISSU_VERSION}"
    fi

    return "${CHECK_RESULT}"
}

check_ffb()
{
    check_issu_enabled || {
        >&2 echo "ISSU is not enabled on this HWSKU"
        return "${FFB_FAILURE}"
    }

    check_sdk_upgrade || {
        >&2 echo "SDK upgrade check failued"
        return "${FFB_FAILURE}"
    }

    return "${FFB_SUCCESS}"
}

