#!/bin/bash
#
# Copyright (c) 2020-2022 NVIDIA CORPORATION & AFFILIATES.
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


#==============================================================================#
#=  Global variable                                                            #
#=
#=====
VERSION="1.6"
#=====
UTIL_TITLE="This is MLNX SSD firmware update utility to read and write SSD FW. Version ${VERSION}"
DEPENDECIES=("smartctl" "sha256sum" "tar" "/bin/bash" "gpg" "sed" "realpath" "dirname")
TRUE="0"
FALSE="1"
ERR_MSG="ERR_MSG"
INI_PREFIX="ini_section_"
PUBLIC_CERT_NAME="trusted.gpg"
CHECKSUM_NAME="checksum"
SCRIPT_MODE="RELESE"   # RELESE -or- DEBUG
DEBUG_MSG="DEBUG"   # remove all instance after script is ready.
#=====
PKG_EXTRACTED=$FALSE
LOGGER_UTIL=$FALSE
SSD_DEV_NAME=""
SSD_FW_VER=""
SSD_DEVICE_MODEL=""
SSD_SERIAL=""
SSD_SIZE=""
SECTIONS=()
#=====
ARG_IMAGE_FLAG=$FALSE
ARG_IMAGE_VAL=""
ARG_QUERY_FLAG=$FALSE
ARG_YES_FLAG=$FALSE
ARG_POWER_CYCLE_FLAG=$FALSE
ARG_FORCE_POWER_CYCLE_FLAG=$FALSE
ARG_HELP_FLAG=$FALSE
ARG_VERSION_FLAG=$FALSE
ARG_PACKAGE_INFO_FLAG=$FALSE
ARG_UPDATE_FLAG=$FALSE



#==============================================================================#
#=  usage function.                                                            #
#=
function init_script() {
# check if logger utility supported
    if [ -x "$(command -v logger)" ]; then
        LOGGER_UTIL=$TRUE
    else
        LOGGER_UTIL=$FALSE
    fi
    export LC_ALL=
    export LANG="en_US.UTF-8"
}

#==============================================================================#
#=  usage function.                                                            #
#=
function usage() {
    echo
    echo -e "$UTIL_TITLE"
    echo 
    echo -e "Usage:"
    echo -e "\tmlnx_ssd_fw_update.sh [OPTION]"
    echo -e "Commands:"
    echo -e "\t-i, --image\t\t Path to SSD FW package"
    echo -e "\t-q, --query\t\t Print SSD information  (SSD model, serial number, version and size)"
    echo -e "\t\t\t\t Combined with image, comparison is made if update is required"
    echo -e "\t-p, --package-info\t Get package info"
    echo -e "\t-u, --update\t\t Upgrade firmware"
    echo -e "\t-y  --yes\t\t Assume \"yes\" to all questions"
    echo -e "\t-V, --version\t\t Print utility version"
    echo -e "\t-h, --help\t\t Show this usage"
    echo -e "\t    --power-cycle\t Execute power cycle at completion, even if not required"
    echo
    echo -e "Example:"
    echo -e "\tmlnx_ssd_fw_update.sh -q"
    echo -e "\tmlnx_ssd_fw_update.sh -q -i mlnx_ssd_fw_package.pkg"
    echo -e "\tmlnx_ssd_fw_update.sh -p -i mlnx_ssd_fw_package.pkg"
    echo -e "\tmlnx_ssd_fw_update.sh -u -i mlnx_ssd_fw_package.pkg"
    echo
}

#==============================================================================#
#=  Log function.                                                              #
#=
function LOG_MSG() {
    if [ $# -gt 0 ]; then
        LOG_STR=$1
        if [[ $# -eq 1 ]]; then
            [[ "$LOGGER_UTIL" == "$TRUE" && "$LOG_STR" != "" ]] && logger -t  mlnx_ssd_fw_update.sh -p user.notice $(echo "$LOG_STR" | sed 's/\\t//g')
            echo -e "$LOG_STR"
        elif [[ $# -eq 2 &&  "$2" == "$ERR_MSG" ]]; then
            [[ "$LOGGER_UTIL" == "$TRUE" && "$LOG_STR" != "" ]] && logger -t  mlnx_ssd_fw_update.sh -p user.err $(echo "$LOG_STR" | sed 's/\\t//g')
            echo -e "$LOG_STR"
        elif [[ $# -eq 2 &&  "$2" == "$SCRIPT_MODE" ]]; then
            echo -e "DBG: $LOG_STR"
        fi
    fi
}

#==============================================================================#
#=  Log function.                                                              #
#=
function LOG_MSG_AND_EXIT() {
    LOG_MSG "$@" "$ERR_MSG"
    erase_extract_package "$extraction_path"
    LOG_MSG "Exiting..."
    exit 1
}

#==============================================================================#
#=  This function check if given argument is valid and return boolean result.  #
#=
function check_usage() {
    local argument_count=$#

    LOG_MSG "Number of argument:$argument_count" ${DEBUG_MSG}

    if [ $# -eq 0 ]; then
        LOG_MSG "Error: false usage given."
        usage
        exit 1
    fi

    while [[ $# -gt 0 ]]
    do
    key="$1"

    case $key in
        -i|--image)
            ARG_IMAGE_FLAG=$TRUE
            ARG_IMAGE_VAL="$2"
            shift # past argument
            shift # past value
            ;;
        -q|--query)
            ARG_QUERY_FLAG=$TRUE
            shift # past argument
            ;;
        -y|--yes)
            ARG_YES_FLAG=$TRUE
            shift # past argument
            ;;
        -h|--help)
            ARG_HELP_FLAG=$TRUE
            shift # past argument
            ;;
        -V|--version)
            ARG_VERSION_FLAG=$TRUE
            shift # past argument
            ;;
        -u|--update)
            ARG_UPDATE_FLAG=$TRUE
            shift # past argument
            ;;
        -p|--package-info)
            ARG_PACKAGE_INFO_FLAG=$TRUE
            shift # past argument
            ;;
        --power-cycle)
            ARG_POWER_CYCLE_FLAG=$TRUE
            shift # past argument
            ;;
        --no-power-cycle)
            ARG_FORCE_NO_POWER_CYCLE_FLAG=$TRUE
            shift # past argument
            ;;
        *)
            LOG_MSG "Error: false usage given."
            usage
            exit 1
            ;;
    esac
    done
    
    if [[ ("$ARG_IMAGE_FLAG" == "$TRUE"  && ( $argument_count -lt 3 )) ||
          ("$ARG_IMAGE_FLAG" == "$TRUE"  && ( $argument_count -gt 5 )) ||
          ("$ARG_PACKAGE_INFO_FLAG" == "$TRUE"  && ( $argument_count -ne 3 )) ||
          ("$ARG_QUERY_FLAG" == "$TRUE"  && ( $argument_count -lt  1 )) ||
          ("$ARG_QUERY_FLAG" == "$TRUE"  && ( $argument_count -gt 3 )) ||
          ("$ARG_HELP_FLAG" == "$TRUE"  && ( $argument_count -gt 1 ))  ||
          ("$ARG_VERSION_FLAG" == "$TRUE"  && ( $argument_count -gt 1 )) ||
          ("$ARG_IMAGE_FLAG" == "$TRUE" && "$ARG_IMAGE_VAL" == "") ||
          ("$ARG_UPDATE_FLAG" == "$TRUE" && "$ARG_IMAGE_FLAG" == "$FALSE") ||
          ("$ARG_PACKAGE_INFO_FLAG" == "$TRUE" && "$ARG_IMAGE_FLAG" == "$FALSE") ||
          ("$ARG_POWER_CYCLE_FLAG" == "$TRUE" && "$ARG_UPDATE_FLAG" == "$FALSE") ||
          ("$ARG_FORCE_NO_POWER_CYCLE_FLAG" == "$TRUE" && "$ARG_POWER_CYCLE_FLAG" == "$TRUE") ||
          ("$ARG_UPDATE_FLAG" == "$TRUE" && "$ARG_PACKAGE_INFO_FLAG" == "$TRUE") ]]; then

        LOG_MSG "Error: false usage given."
        usage
        exit 1
    fi

### Debug message remove when script is done.
    LOG_MSG "ARG_IMAGE_FLAG            = ${ARG_IMAGE_FLAG}"             ${DEBUG_MSG}
    LOG_MSG "ARG_IMAGE_VAL             = ${ARG_IMAGE_VAL}"              ${DEBUG_MSG}
    LOG_MSG "ARG_QUERY_FLAG            = ${ARG_QUERY_FLAG}"             ${DEBUG_MSG}
    LOG_MSG "ARG_YES_FLAG              = ${ARG_YES_FLAG}"               ${DEBUG_MSG}
    LOG_MSG "ARG_HELP_FLAG             = ${ARG_HELP_FLAG}"              ${DEBUG_MSG}
    LOG_MSG "ARG_VERSION_FLAG          = ${ARG_VERSION_FLAG}"           ${DEBUG_MSG}
    LOG_MSG "ARG_PACKAGE_INFO_FLAG     = ${ARG_PACKAGE_INFO_FLAG}"      ${DEBUG_MSG}
    LOG_MSG "ARG_POWER_CYCLE_FLAG      = ${ARG_POWER_CYCLE_FLAG}"       ${DEBUG_MSG}
    LOG_MSG "ARG_FORCE_NO_POWER_CYCLE_FLAG      = ${ARG_FORCE_NO_POWER_CYCLE_FLAG}"       ${DEBUG_MSG}

}

#==============================================================================#
# This function return SSD fw version using hdparm utility                      #
#
function get_ssd_fw_version() {
    [ $1 ] || { LOG_MSG_AND_EXIT "Wrong usage - ${FUNCNAME[0]}()"; }

    local device_fw_version
    device_fw_version=$(smartctl -i $SSD_DEV_NAME | grep -Po "Firmware Version: +\K[^,]+")
    LOG_MSG "device_fw_version: $device_fw_version" ${DEBUG_MSG}
    eval $1='$device_fw_version'
}

#==============================================================================#
# This function return SSD device model using hdparm utility                    #
#
function get_ssd_device_model() {
    [ $1 ] || { LOG_MSG_AND_EXIT "Wrong usage - ${FUNCNAME[0]}()"; }

    local device_model_name
    device_model_name=$(smartctl -i $SSD_DEV_NAME | grep -E "Device Model:|Model Number:" | awk '{$1=$2="";print $0}'| sed 's/^ *//g')
    LOG_MSG "device_model_name: $device_model_name" ${DEBUG_MSG}
    eval $1='$device_model_name'
}

#==============================================================================#
# This function return SSD size using hdparm utility                            #
#
function get_ssd_size() {
    [ $1 ] || { LOG_MSG_AND_EXIT "Wrong usage - ${FUNCNAME[0]}()"; }

    local device_size
    device_size=$(smartctl -i $SSD_DEV_NAME | grep -E "User Capacity:|Size/Capacity" | awk -F '\[|\]' '{print $2}' | awk '{print $1}')
    LOG_MSG "device_size: $device_size" ${DEBUG_MSG}
    eval $1='$device_size'
}

#==============================================================================#
# This function return SSD serial using hdparm utility                          #
#
function get_ssd_serial() {
    [ $1 ] || { LOG_MSG_AND_EXIT "Wrong usage - ${FUNCNAME[0]}()"; }

    local device_serial
    device_serial=$(smartctl -i $SSD_DEV_NAME | grep -Po "Serial Number: +\K[^,]+")
    LOG_MSG "device_serial: $device_serial" ${DEBUG_MSG}
    eval $1='$device_serial'
}

#==============================================================================#
# This function check SSD device name                                                 #
#
function get_ssd_device_name() {
    [ $1 ] || { LOG_MSG_AND_EXIT "Wrong usage - ${FUNCNAME[0]}()"; }

    non_rem_mount_disks=""
    non_rem_mount_disks_count=0
    mount_parts=$(cat /proc/partitions | grep -v "^major" | grep -v ram | awk '{{print $4}}')
    for blk_dev_name in ${mount_parts}
    do
        blk_dev_link=$(find /sys/bus /sys/class /sys/block/ -name ${blk_dev_name})
        for first_blk_dev_link in ${blk_dev_link}
        do
            if ls -l ${first_blk_dev_link} | grep -q virtual; then
                continue
            fi
            if [ -e ${first_blk_dev_link}/removable ] ; then
                if [ "0" = $(cat ${first_blk_dev_link}/removable) ] ; then
                    let non_rem_mount_disks_count=${non_rem_mount_disks_count}+1
                    if [ "1" == "${non_rem_mount_disks_count}" ] ; then
                        non_rem_mount_disks="${blk_dev_name}"
                    else
                        non_rem_mount_disks="${non_rem_mount_disks} ${blk_dev_name}"
                    fi
                fi
            fi
            break
        done
    done
    if [ "1" == "${non_rem_mount_disks_count}" ] ; then
	device_name="/dev/${non_rem_mount_disks}"
    else
        $1="/dev/sda"
    fi
    LOG_MSG "device_name: $device_name" ${DEBUG_MSG}
    eval $1='$device_name'
}

#==============================================================================#
#=  This function check if given argument.                                     #
#=
function get_ssd_info() {
    LOG_MSG "func: ${FUNCNAME[0]}()" ${DEBUG_MSG}
    get_ssd_device_name  SSD_DEV_NAME
    get_ssd_fw_version   SSD_FW_VER
    get_ssd_device_model SSD_DEVICE_MODEL
    get_ssd_serial       SSD_SERIAL
    get_ssd_size         SSD_SIZE
}

#==============================================================================#
#=  This function check if given argument is valid and return boolean result.  #
#=
function check_tool_dependencies() {
    LOG_MSG "func: ${FUNCNAME[0]}()" ${DEBUG_MSG}
    for i in "${!DEPENDECIES[@]}"
    do
        if [ ! -x "$(command -v ${DEPENDECIES[$i]})" ]; then
            LOG_MSG_AND_EXIT "Error: This tool require the following utils to be installed ${DEPENDECIES[$i]}"
        fi
    done
}

#==============================================================================#
#= This function parse package ini file and declare it attributes              #
#=
function ini_parser {
   LOG_MSG "func: ${FUNCNAME[0]}()" ${DEBUG_MSG}

   local filename="$1"
   LOG_MSG "filename:$filename" ${DEBUG_MSG}

   shopt -p extglob &> /dev/null
   CHANGE_EXTGLOB=$?
   if [ $CHANGE_EXTGLOB = 1 ]
   then
      shopt -s extglob
   fi
   ini="$(<$filename)"                 # read the file
   ini=${ini//$'\r'/}           # remove linefeed i.e dos2unix
   ini="${ini//[/\\[}"
   ini="${ini//]/\\]}"
   IFS=$'\n' && ini=( ${ini} )  # convert to line-array
   ini=( ${ini[*]//\)/\\\)} )   # append / before any parenthesis
   ini=( ${ini[*]//\(/\\\(} )   # append / before any parenthesis
   ini=( ${ini[*]/#*([[:space:]]);*/} )
   ini=( ${ini[*]/#*([[:space:]])\#*/} )
   ini=( ${ini[*]/#+([[:space:]])/} ) # remove init whitespace
   ini=( ${ini[*]/%+([[:space:]])/} ) # remove ending whitespace
   ini=( ${ini[*]/*([[:space:]])=*([[:space:]])/=} ) # remove whitespace around =
   ini=( ${ini[*]/#\\[/\}$'\n'"$INI_PREFIX"} ) # set section prefix
   ini=( ${ini[*]/%\\]/ \(} )   # convert text2function (1)
   ini=( ${ini[*]/=/=\( } )     # convert item to array
   ini=( ${ini[*]/%/ \)} )      # close array parenthesis
   ini=( ${ini[*]/%\\ \)/ \\} ) # the multiline trick
   ini=( ${ini[*]/%\( \)/\(\) \{} ) # convert text2function (2)
   ini=( ${ini[*]/%\} \)/\}} )  # remove extra parenthesis
   ini=( ${ini[*]/%\{/\{$'\n''ini_unset ${FUNCNAME/#'$INI_PREFIX'}'$'\n'} )  # clean previous definition of section 
   ini[0]=""                    # remove first element
   ini[${#ini[*]} + 1]='}'      # add the last brace
   eval "$(echo "${ini[*]}")"   # eval the result
   [ $? -ne 0 ] && LOG_MSG_AND_EXIT "Error: failed to parse package content."
   SECTIONS="$(echo ${ini[*]} | grep -Po "$INI_PREFIX+\K[\w]+")"
   if [ $CHANGE_EXTGLOB = 1 ]
   then
      shopt -u extglob
   fi
}

#==============================================================================#
#= This function unset parse ini section and variables                         #
#=
function ini_unset {
   LOG_MSG "func: ${FUNCNAME[0]}()" ${DEBUG_MSG}
   SECTION=$1
   OLDIFS="$IFS"
   IFS=' '$'\n'
   if [ -z "$SECTION" ] 
   then
      fun="$(declare -F)"
   else
      fun="$(declare -F $INI_PREFIX$SECTION)"
      if [ -z "$fun" ]
      then
         echo "section $SECTION not found" 1>&2
         return
      fi
   fi
   fun="${fun//declare -f/}"
   for f in $fun; do
      [ "${f#$INI_PREFIX}" == "${f}" ] && continue
      item="$(declare -f ${f})"
      item="${item##*\{}" # remove function definition
      item="${item##*FUNCNAME*$INI_PREFIX\};}" # remove clear section
      item="${item/\}}"  # remove function close
      item="${item%)*}" # remove everything after parenthesis
      item="${item});" # add close parenthesis
      vars=""
      while [ "$item" != "" ]
      do
         newvar="${item%%=*}" # get item name
         vars="$vars $newvar" # add name to collection
         item="${item#*;}" # remove readed line
      done
      for var in $vars; do
         unset $var
      done
   done
   IFS="$OLDIFS"
}

#==============================================================================#
#=  This function check package signing and returns back true or false         #
#=
function check_package_signing() {
    LOG_MSG "func: ${FUNCNAME[0]}()" ${DEBUG_MSG}
    
    [ $1 ] || { LOG_MSG_AND_EXIT "Wrong usage - ${FUNCNAME[0]}()"; }

    local package_path=$1
    local checksum_unsigned_file="$package_path/$CHECKSUM_NAME"
    local checksum_signed_file="$package_path/$CHECKSUM_NAME.sig"
    local public_cert_file="$package_path/$PUBLIC_CERT_NAME"

### Check if unsigned checksum file exists
    [ ! -f "$checksum_unsigned_file" ] && LOG_MSG_AND_EXIT "Error: fail to find unsigned checksum file to verify package signing."

### Check if signed checksum file exists
    [ ! -f "$checksum_signed_file" ] && LOG_MSG_AND_EXIT "Error: fail to find sign checksum file to verify package signing."

### Check if public key exists
    [ ! -f "$public_cert_file" ] && LOG_MSG_AND_EXIT "Error: fail to find public certificate to verify package signing."


    LOG_MSG "public_cert_file: ${public_cert_file}" ${DEBUG_MSG}
    LOG_MSG "checksum_signed_file: ${checksum_signed_file}" ${DEBUG_MSG}
    LOG_MSG "checksum_unsigned_file: ${checksum_unsigned_file}" ${DEBUG_MSG}

    gpg --ignore-time-conflict --keyring  "$public_cert_file" --verify "$checksum_signed_file" "$checksum_unsigned_file" > /dev/null 2>&1
    [ $? -ne 0 ] && LOG_MSG_AND_EXIT "Error: fault package signing."

    LOG_MSG "cd into: ${package_path}" ${DEBUG_MSG}
    cd $package_path > /dev/null 2>&1
    sha256sum -c $CHECKSUM_NAME > /dev/null 2>&1
    [ $? -ne 0 ] && LOG_MSG_AND_EXIT "Error: fault package SHA signing, file has been compromised" 
    LOG_MSG "backing back:" ${DEBUG_MSG}
    cd - > /dev/null 2>&1
    LOG_MSG "exiting:" ${DEBUG_MSG}

}

#==============================================================================#
#=  This function prints supported SSD from package ini                        #
#=
function string_supported_model() {

    local section=$1

    if [[ ! -z "${Vendor[*]}" ]] && [[ ! -z "${SSD_FW_Model[*]}" ]] && [[ ! -z "${SSD_FW_Version[*]}" ]] \
    && [[ ! -z "${SSD_Size[*]}" ]] && [[ ! -z ${Shutdown_Policy[*]} ]]; then
        printf 'o %-10s | %-30s | %-12s | %-6sGB |     %-7s |\n' \
                "$( IFS=$'\n'; echo "${Vendor[@]}" )" "$( IFS=$'\n'; echo "${SSD_FW_Model[@]}" )" \
                "${SSD_FW_Version[@]}" "${SSD_Size[@]}" "${Shutdown_Policy[@],,}" 
    fi

}

#==============================================================================#
#=  This function extract SSD FW package into /tmp                             #
#=
function extract_package() {
    LOG_MSG "func: ${FUNCNAME[0]}()" ${DEBUG_MSG}

    local filename=$1
    LOG_MSG "filename:$filename" ${DEBUG_MSG}
### Check if file exists
    [ ! -f $filename ] && LOG_MSG_AND_EXIT "Error: given file ($filename) not found."
### Check if tmp available
    [ ! -d "/tmp" ] && LOG_MSG_AND_EXIT "Error: directory /tmp DOES NOT exists."

    local base_filename="${filename##*/}"
    local folder_name="/tmp/""${base_filename%%.*}"

### Check if full path available
    if [ -d $folder_name ]; then
        LOG_MSG "Path:$folder_name already exists, removing folder." ${DEBUG_MSG}
        rm -rf ${folder_name} 
        [ $? -ne 0 ] && LOG_MSG_AND_EXIT "Error: folder:$folder_name is already in use and can't be overwrite, please remove it and retry."
    fi
    
    mkdir ${folder_name} && tar xf ${filename} -C ${folder_name} --strip-components 1 --warning=no-timestamp > /dev/null 2>&1
    #tar -xf $filename --directory /tmp/  --warning=no-timestamp > /dev/null 2>&1
### Check if untar succeed.
    [ $? -ne 0 ] && LOG_MSG_AND_EXIT "Error: fail to extract given package ($filename)."

### return the path file extraction is
    # local base_filename="${filename##*/}"
    # local folder_name="/tmp/""${base_filename%%.*}"
    eval $2="$folder_name"

    PKG_EXTRACTED=$TRUE

    check_package_signing $folder_name
    
    LOG_MSG "successfully untar file." ${DEBUG_MSG}
}

#==============================================================================#
#=  This function extract SSD FW package into /tmp                             #
#=
function erase_extract_package() {
    LOG_MSG "func: ${FUNCNAME[0]}()" ${DEBUG_MSG}

    [[ "$PKG_EXTRACTED" == "$FALSE" ]] && return

    local folder_name=$1

    LOG_MSG "folder_name: $folder_name" ${DEBUG_MSG}

### Check if folder exists
    if [ ! -d "$folder_name" ]; then
        LOG_MSG "Error: directory $folder_name DOES NOT exists." "$ERR_MSG"
        LOG_MSG "Exiting..."
        exit 1
    fi
    rm -rf $folder_name
### Check if untar succeed.
    if [ $? -ne 0 ]; then 
        LOG_MSG "Error: fail to delete $folder_name folder." "$ERR_MSG"
        LOG_MSG "Exiting..."
        exit 1    
    fi

    PKG_EXTRACTED=$FALSE
    LOG_MSG "successfully removed folder:$folder_name" ${DEBUG_MSG}
}


#==============================================================================#
#=  This function returns back ini section array.
#=
function call_ini_section() {
    LOG_MSG "func: ${FUNCNAME[0]}()" ${DEBUG_MSG}

    local ini_section=$1
    LOG_MSG "ini_section:$ini_section" ${DEBUG_MSG}

    [[ -z "$ini_section" ]] && LOG_MSG_AND_EXIT "Error: given INI section is null."
    [[ -z "$(declare -F "$INI_PREFIX$ini_section")" ]]  && LOG_MSG_AND_EXIT "Error: $ini_section section is missing in INI file."

   eval "$(echo "$INI_PREFIX$ini_section")"             # call given section function.
}

#==============================================================================#
#=  This function prints ssd info
#=
function print_ssd_info() {
    LOG_MSG "func: ${FUNCNAME[0]}()" ${DEBUG_MSG}

    local argument_count=$#

    if [ $argument_count -eq 2 ]; then
        local newer_fw_version=$1
        local power_policy=$2
        LOG_MSG "Device Model\t\t   : $SSD_DEVICE_MODEL"
        LOG_MSG "Serial Number\t\t   : $SSD_SERIAL"
        LOG_MSG "User Capacity\t\t   : $SSD_SIZE GB"
        LOG_MSG "Current Firmware Version   : $SSD_FW_VER"
        LOG_MSG "Available Firmware Version : $Newer_FW_Version"
        LOG_MSG "Power Cycle Required\t   : $power_policy"
        LOG_MSG "Upgrade Required\t   : yes"
    elif [ $argument_count -eq 1 ]; then
        local _upgrade_require=$1
        LOG_MSG "Device Model\t : $SSD_DEVICE_MODEL"
        LOG_MSG "Serial Number\t : $SSD_SERIAL"
        LOG_MSG "User Capacity\t : $SSD_SIZE GB"
        LOG_MSG "Firmware Version : $SSD_FW_VER"
        LOG_MSG "Upgrade Required : $_upgrade_require"
    else
        LOG_MSG "Device Model\t : $SSD_DEVICE_MODEL"
        LOG_MSG "Serial Number\t : $SSD_SERIAL"
        LOG_MSG "User Capacity\t : $SSD_SIZE GB"
        LOG_MSG "Firmware Version : $SSD_FW_VER"
    fi
}

# Main
# ------------------------------------------------------------------------------
init_script
check_usage "$@"

# show help
if [ $ARG_HELP_FLAG == $TRUE ]; then
    usage
    exit 0
# show version
elif [ $ARG_VERSION_FLAG == $TRUE ]; then
    echo $UTIL_TITLE
    exit 0
# show SSD info
elif [ $ARG_QUERY_FLAG == $TRUE ]; then
    match_found=$FALSE
    check_tool_dependencies
    get_ssd_info

    if [ $ARG_IMAGE_FLAG == $TRUE ]; then
        extract_package $ARG_IMAGE_VAL extraction_path
        ini_parser "$extraction_path/list.ini"

        for section in $SECTIONS; do
        if [[ $section != "main" ]]; then
            call_ini_section $section
            if [[ "$SSD_DEVICE_MODEL" == "$( IFS=$'\n'; echo "${SSD_FW_Model[@]}" )" ]] && \
               [[ "$SSD_FW_VER" == "${SSD_FW_Version[@]}" ]] && \
               [[ "$SSD_SIZE" == "${SSD_Size[@]}" ]]; then
  
                match_found=$TRUE
                break
            fi
            ini_unset $section
        fi
        done

        erase_extract_package "$extraction_path"
        if [[ "$match_found" == "$FALSE" ]]; then
            #LOG_MSG "SSD FW upgrade not require, based on given package latest version is in used."
            print_ssd_info "no"
            echo -e ""
            exit 0
        fi
    fi

    if [[ "$match_found" == "$TRUE" ]]; then
        print_ssd_info $Newer_FW_Version ${Shutdown_Policy[0],,}
        erase_extract_package "$extraction_path"
    else
        print_ssd_info
    fi

    echo -e ""

    exit 0
# show package version
elif [ $ARG_PACKAGE_INFO_FLAG == $TRUE ]; then
    check_tool_dependencies
    extract_package $ARG_IMAGE_VAL extraction_path
    # 2. check signing 
    ini_parser "$extraction_path/list.ini"
    
    call_ini_section "main"
    LOG_MSG "Package Name: $ARG_IMAGE_VAL"
    [[ ! -z ${description[@]} ]]    && LOG_MSG "Description: ${description[@]}"
    [[ ! -z ${version[@]} ]]        && LOG_MSG "Version: ${version[@]}"
    [[ ! -z ${release_date[@]} ]]   && LOG_MSG "Release Date: ${release_date[@]}"
    LOG_MSG "Supported SSDs:"
    LOG_MSG "    Vendor   |             Model              |    FW ver    |   Size   | Pwr Cyc Req |"
    LOG_MSG "=============|================================|==============|==========|=============|"
    for section in $SECTIONS; do
        if [[ "$section" != "main" ]]; then
            call_ini_section $section
            supported_model=$(string_supported_model  $section)
            LOG_MSG "$supported_model"
            ini_unset $section
        fi
    done
    echo -e ""
    erase_extract_package "$extraction_path"
    exit 0 
# operate SSD fw update
elif [ $ARG_UPDATE_FLAG == $TRUE ]; then
    check_tool_dependencies
    get_ssd_info
    extract_package $ARG_IMAGE_VAL extraction_path
    # 2. check signing 
    UPDATE_DONE=$FALSE
    ini_parser "$extraction_path/list.ini"
    for section in $SECTIONS; do
        if [[ $section != "main" ]]; then
            call_ini_section $section
            if [[ "$SSD_DEVICE_MODEL" == "$( IFS=$'\n'; echo "${SSD_FW_Model[@]}" )" ]] && \
               [[ "$SSD_FW_VER" == "${SSD_FW_Version[@]}" ]] && \
               [[ "$SSD_SIZE" == "${SSD_Size[@]}" ]]; then
                UPDATE_DONE=$TRUE

                power_policy=${Shutdown_Policy[0],,}
                LOG_MSG "Power policy:$power_policy" ${DEBUG_MSG}
                print_ssd_info $Newer_FW_Version ${Shutdown_Policy[0],,}
                echo -e ""
                #[[ "yes" == "$power_policy" ]] && LOG_MSG "PLEASE NOTE: System will power-cycle automatically once SSD FW Update complete!"
                [[ "yes" == "$power_policy" || "$ARG_POWER_CYCLE_FLAG" == "$TRUE" ]] && LOG_MSG "Please note: Once SSD FW Update process ends, system will power-cycle automaticly and it will take up to 1 minute to access it back."

                # Prompt approval for FW update if ignore in case "yes" flag is on.
                if [[ "$ARG_YES_FLAG" == "$FALSE" ]]; then
                    read -p "Do you want to continue? [Y/N]" -n 1 -r
                    echo    # (optional) move to a new line
                    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                        LOG_MSG_AND_EXIT "Aborting..."
                        exit 0
                    fi
                fi

                # Check FWUpgrade scripts exists & if so call it.
                ssd_script_name=$Update_Script
                ssd_script_path="${extraction_path}/${section}/${ssd_script_name}"
                LOG_MSG "ssd_script_path: $ssd_script_path"     ${DEBUG_MSG}
                if [ ! -f $ssd_script_path ]; then
                    LOG_MSG_AND_EXIT "Error: fail to call upgrade script ($ssd_script_path)!"
                fi
                (
                    if [[ "yes" == "$power_policy" && $ARG_FORCE_NO_POWER_CYCLE_FLAG == $TRUE ]]; then
                        # If a power cycle is required and we are not power cycling automatically lock the file system for safety
                        LOG_MSG "Immediate power cycle is required but override flag has been given. Locking file system as read only to protect system integrity."
                        echo u > /proc/sysrq-trigger
                    fi
                    cd "${extraction_path}/${section}" > /dev/null 2>&1 || exit
                    /bin/bash "$ssd_script_path" "${extraction_path}/${section}"
                    #cd - > /dev/null 2>&1 || exit
                )
                if [ $? -ne 0 ]; then
                    LOG_MSG_AND_EXIT "Error: SSD FW update failed."
                else
                    LOG_MSG "SSD FW update completed successfully."

                    if [[ "yes" == "$power_policy" || $ARG_POWER_CYCLE_FLAG == $TRUE ]]; then

                        if [[ $ARG_FORCE_NO_POWER_CYCLE_FLAG == $TRUE ]]; then
                            LOG_MSG_AND_EXIT "An IMMEDIATE power cycle is REQUIRED to upgrade the SSD. Please perform a cold reboot as soon as possible."
                        fi
                        
                        LOG_MSG "Execute power cycle..."
                        sleep 1
                        sync
                        power_cycle_script="${extraction_path}/common/mlnx_shutdown.sh"
                        [ ! -f $power_cycle_script ]&& LOG_MSG_AND_EXIT "Error: failed to initiate power cycle."
                        ($power_cycle_script "-s")
                        [ $? -ne 0 ] && LOG_MSG_AND_EXIT "Error: failed to power cycle the system automatically."
                        erase_extract_package "$extraction_path"

                    fi
                        
                fi

                break       # Exit the for loop
            fi  
            ini_unset $section
        fi
    done  
    if [ $UPDATE_DONE == $FALSE ]; then
        LOG_MSG "SSD FW upgrade is not required, latest version based on given package is in use."
        print_ssd_info "no"
    fi

    echo -e ""
    erase_extract_package "$extraction_path"
    exit 0 
fi

exit 0
