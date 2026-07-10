#!/bin/bash

# Lock macro for shared file access
# Parameters:
#  $(1) - Lock file name
#  $(2) - Timeout value
function FLOCK()
{
	local filename=$(dirname $1)
    local timeout=${2:-360}
    if [[ ! -f ${filename}.flock ]]; then
        touch ${filename}.flock
        chmod -f 777 ${filename}.flock;
    fi
	local lockname=$(basename ${filename})
    local lock_fd=lock_${lockname//[%.\/\-+~]/_}_fd
    eval $(echo exec {${lock_fd}}\<\>"${filename}.flock")
    #echo ${!lock_fd}
    if ! flock -x -w ${timeout} "${!lock_fd}" ; then
        echo "ERROR: Lock timeout trying to access ${filename}.flock" 1>&2;
        exit 1;
    fi
    #echo "Lock acquired .."
}

# UnLock macro for shared file access
# Parameters:
#  $(1) - Lock file name
function FUNLOCK()
{
	local filename=$(dirname $1)
	local lockname=$(basename ${filename})
    local lock_fd=lock_${lockname//[%.\/\-+~]/_}_fd
    eval $(echo exec "${!lock_fd}<&-")
	#rm -f ${filename}.flock
}

